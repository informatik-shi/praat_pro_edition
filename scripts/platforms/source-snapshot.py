"""Archive the current working sources, including uncommitted custom code.

Never copy platform object files or application/user caches into another build.
"""
import argparse
import pathlib
import subprocess
import tarfile

root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("output", type=pathlib.Path)
args = parser.parse_args()
output = args.output.resolve()
output.parent.mkdir(parents=True, exist_ok=True)
paths = subprocess.check_output(
    ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=root
).decode("utf-8").split("\0")
count = 0
with tarfile.open(output, "w:gz", compresslevel=3) as archive:
    for name in sorted(set(paths)):
        if not name:
            continue
        first = pathlib.PurePosixPath(name).parts[0]
        if first == "dist" or (first.startswith(".") and first != ".github"):
            continue
        path = root / name
        if not path.is_file() or path.resolve() == output or path.suffix.lower() in {".o", ".a", ".exe", ".dmp"}:
            continue
        archive.add(path, arcname=name, recursive=False)
        count += 1
print(f"Source snapshot: {output} ({count} files, {output.stat().st_size} bytes)")
