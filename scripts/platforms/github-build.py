"""Dispatch and retrieve the explicitly authorized platform build.

Uses the existing Git credential helper in memory; never prints or stores tokens.
"""
import argparse
import json
import os
import pathlib
import re
import subprocess
import urllib.error
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("action", choices=("access", "dispatch", "status", "logs", "download", "cancel"))
parser.add_argument("--repo", default="informatik-shi/praat_pro_edition")
parser.add_argument("--ref", default="build/macos-linux")
parser.add_argument("--run", type=int)
parser.add_argument("--platform", choices=("all", "linux", "macos"), default="all")
parser.add_argument("--macos-runner", choices=("macos-26", "macos-15-intel", "macos-15"), default="macos-26")
args = parser.parse_args()
if not re.fullmatch(r"[\w.-]+/[\w.-]+", args.repo):
    raise SystemExit("Invalid repository name")
env = dict(os.environ, GIT_TERMINAL_PROMPT="0", GCM_INTERACTIVE="never")
credential = subprocess.run(["git", "credential", "fill"], cwd=ROOT, env=env,
    input="protocol=https\nhost=github.com\n\n", text=True, capture_output=True)
fields = dict(line.split("=", 1) for line in credential.stdout.splitlines() if "=" in line)
token = fields.get("password")
if credential.returncode or not token:
    raise SystemExit("GitHub authentication unavailable. Sign in with the existing Git credential helper, then retry.")

class Redirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        result = super().redirect_request(req, fp, code, msg, headers, newurl)
        if result and urllib.parse.urlparse(req.full_url).netloc != urllib.parse.urlparse(newurl).netloc:
            result.remove_header("Authorization")
        return result

opener = urllib.request.build_opener(Redirect())
def request(path, data=None, binary=False):
    req = urllib.request.Request("https://api.github.com/" + path,
        data=json.dumps(data).encode() if data is not None else None,
        headers={"Authorization": "Bearer " + token, "Accept": "application/vnd.github+json",
                 "User-Agent": "Praat-Pro-platform-build", "Content-Type": "application/json", "Cache-Control": "no-cache"})
    try:
        with opener.open(req, timeout=120) as response:
            raw = response.read()
            return raw if binary else json.loads(raw) if raw else None
    except urllib.error.HTTPError as error:
        try: message = json.loads(error.read()).get("message", error.reason)
        except (ValueError, AttributeError): message = error.reason
        raise SystemExit(f"GitHub API {error.code}: {message}")

base = f"repos/{args.repo}"
cache = ROOT / ".local-build/platforms"
cache.mkdir(parents=True, exist_ok=True)
if args.action == "access":
    repo = request(base)
    print(json.dumps({"repository": repo["full_name"], "permissions": repo.get("permissions", {})}))
elif args.action == "dispatch":
    request(base + "/actions/workflows/platform-builds.yml/dispatches", {"ref": args.ref, "inputs": {"platform": args.platform, "macos_runner": args.macos_runner}})
    print(f"Platform build requested for {args.repo} at {args.ref}")
elif args.action == "status":
    if args.run:
        runs = [request(base + f"/actions/runs/{args.run}")]
    else:
        query = urllib.parse.urlencode({"branch": args.ref, "per_page": 5})
        runs = request(base + "/actions/runs?" + query)["workflow_runs"]
        if not runs: print("No Actions runs found for this branch yet.")
    for run in runs:
        print(json.dumps({k: run[k] for k in ("id", "name", "path", "html_url", "status", "conclusion", "head_sha")}))
        if args.run:
            jobs = request(base + f"/actions/runs/{args.run}/jobs")["jobs"]
            for job in jobs:
                print(json.dumps({"job": job["name"], "status": job["status"], "conclusion": job["conclusion"], "labels": job.get("labels"), "runner": job.get("runner_name"), "started_at": job.get("started_at"),
                    "steps": [{k: step.get(k) for k in ("name", "status", "conclusion")} for step in job["steps"]]}))
elif args.action == "cancel":
    if not args.run: raise SystemExit("--run required")
    request(base + f"/actions/runs/{args.run}/cancel", {})
    print(f"Cancelled superseded platform build {args.run}")
elif args.action == "logs":
    if not args.run: raise SystemExit("--run required")
    target = cache / f"github-{args.run}-logs.zip"
    target.write_bytes(request(base + f"/actions/runs/{args.run}/logs", binary=True))
    print(target)
elif args.action == "download":
    if not args.run: raise SystemExit("--run required")
    artifacts = request(base + f"/actions/runs/{args.run}/artifacts")["artifacts"]
    for artifact in artifacts:
        name = artifact["name"]
        if not re.fullmatch(r"[\w.-]+", name): raise SystemExit("Unexpected artifact name")
        target = cache / f"{name}-github-{args.run}.zip"
        target.write_bytes(request(base + f"/actions/artifacts/{artifact['id']}/zip", binary=True))
        print(target)
