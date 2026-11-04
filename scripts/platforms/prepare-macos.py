"""Extend the pinned official Xcode project with the custom Praat sources.

The downloaded project is build metadata only, never an upstream executable.
"""
import argparse
import hashlib
import pathlib
import re
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
URL = "https://github.com/praat/praat/releases/download/v7.0.02/praat7002_xcodeproj.zip"
SHA256 = "db518760d2dd78c98da68e380d721bedb4825ee63eb8da37f0a3be813fabef5b"
parser = argparse.ArgumentParser()
parser.add_argument("--archive", type=pathlib.Path)
args = parser.parse_args()
archive = args.archive or ROOT / ".local-build/platforms/praat7002_xcodeproj.zip"
archive.parent.mkdir(parents=True, exist_ok=True)
if not archive.exists():
    urllib.request.urlretrieve(URL, archive)
if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
    raise SystemExit("Official Xcode archive checksum mismatch")
with zipfile.ZipFile(archive) as z:
    project = z.read("praat.xcodeproj/project.pbxproj").decode("utf-8")

new_sources = []
for directory in ("artsynth", "dwsys", "dwtools", "EEG", "FFNet", "fon", "foned", "gram", "kar", "LPC", "melder", "sensors", "stat", "sys"):
    makefile = (ROOT / directory / "Makefile").read_text(encoding="utf-8").replace("\\\n", " ")
    objects = re.search(r"^OBJECTS\s*=\s*([^\n]+)", makefile, re.M)
    if not objects:
        raise SystemExit(f"Cannot read source list for {directory}")
    for obj in objects.group(1).split():
        source = pathlib.Path(directory) / obj.replace(".o", ".cpp")
        if not (ROOT / source).exists():
            source = source.with_suffix(".c")
        if not (ROOT / source).exists():
            raise SystemExit(f"Missing source: {source}")
        if not re.search(r"/\* " + re.escape(source.name) + r" \*/ = \{isa = PBXFileReference;", project):
            new_sources.append(source.as_posix())
new_sources += sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / "external/lua-5.5.1/src").glob("*.c") if p.name not in {"lua.c", "luac.c"})
new_sources.append("external/luafilesystem-1.9.0/src/lfs.c")
new_sources = sorted(set(new_sources))

def identifier(kind, path):
    return hashlib.sha256(("PraatPro/" + kind + "/" + path).encode()).hexdigest()[:24].upper()

build_files, references, sources = [], [], []
for source in new_sources:
    ref, build = identifier("file", source), identifier("build", source)
    # Lua uses C++ exceptions to unwind Praat's RAII objects. Other new source
    # files are ObjC++ because Cocoa declarations occur in the GUI headers.
    language = "sourcecode.cpp.cpp" if source.endswith(".c") else "sourcecode.cpp.objcpp"
    references.append(f'\t\t{ref} /* {source} */ = {{isa = PBXFileReference; explicitFileType = {language}; path = "{source}"; sourceTree = SOURCE_ROOT; }};')
    flags = ' settings = {COMPILER_FLAGS = "-DLUA_USE_MACOSX"; };' if source.endswith(".c") and "/lua-" in source else ""
    build_files.append(f"\t\t{build} /* {source} in Sources */ = {{isa = PBXBuildFile; fileRef = {ref};{flags} }};")
    sources.append(f"\t\t\t\t{build} /* {source} in Sources */,")
project = project.replace("/* End PBXFileReference section */", "\n".join(references) + "\n/* End PBXFileReference section */", 1)
project = project.replace("/* End PBXBuildFile section */", "\n".join(build_files) + "\n/* End PBXBuildFile section */", 1)
phase = re.compile(r"(23E5C91909A228F9004E41D5 /\* Sources \*/ = \{.*?files = \()(.*?)(\n\s*\);)", re.S)
project, count = phase.subn(lambda m: m[1] + "\n" + "\n".join(sources) + m[2] + m[3], project, count=1)
if count != 1:
    raise SystemExit("Official application sources phase not found")
include_dirs = ("kar", "melder", "sys", "dwsys", "fon", "foned", "stat", "dwtools", "LPC", "external/lua-5.5.1/src", "external/portaudio", "external/flac", "external/mp3", "external/espeak", "external/whispercpp")
header_paths = '("$(inherited)", ' + ", ".join(f'"$(SRCROOT)/{d}"' for d in include_dirs) + ")"
config = re.compile(r"(23E5C92E09A228FB004E41D5 /\* Configuration64 \*/ = \{.*?)(\n\s*name = Configuration64;)", re.S)
def configure(match):
    body = re.sub(r'HEADER_SEARCH_PATHS = "";', "HEADER_SEARCH_PATHS = " + header_paths + ";", match[1])
    body = body.replace("PRODUCT_BUNDLE_IDENTIFIER = org.praat.Praat;", "PRODUCT_BUNDLE_IDENTIFIER = org.praatpro.Praat;")
    return body + match[2]
project, count = config.subn(configure, project, count=1)
if count != 1:
    raise SystemExit("Official app configuration not found")
output = ROOT / "praat.xcodeproj/project.pbxproj"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_bytes(project.encode("utf-8"))
print(f"Prepared {output}: {len(new_sources)} custom and Lua sources added")
