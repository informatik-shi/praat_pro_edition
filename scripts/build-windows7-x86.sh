#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
root="$PWD"
if [[ -n "${PRAAT_GIT:-}" ]]; then
    export PATH="$(dirname "$(cygpath -u "$PRAAT_GIT")"):$PATH"
fi
[[ "${MSYSTEM:-}" == MINGW32 ]] || { echo 'Use MSYS2 MINGW32.' >&2; exit 2; }
[[ "$(gcc -dumpmachine)" == i686-w64-mingw32 ]] || { echo 'An i686 GCC toolchain is required.' >&2; exit 2; }
jobs="${JOBS:-2}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
git diff --quiet HEAD -- || { echo 'Commit tracked changes before building a source-matched release.' >&2; exit 2; }
revision="$(git rev-parse HEAD)"
mkdir -p .local-build dist
# Fresh source tree: never mix x86 and x64 object files or reuse stale headers.
build="$(mktemp -d "$root/.local-build/win7-x86-XXXXXXXX")"
git archive HEAD | tar -x -C "$build"
exec > >(tee "$root/.local-build/win7-x86-build.log") 2>&1
echo "Source: $revision; build directory: $build"
cd "$build"
mkdir -p .local-build
{ echo "$revision"; gcc --version; gcc -dumpmachine; pacman -Q; } > .local-build/toolchain.txt
make PRAAT_OS=windows PRAAT_COMPILER=gcc PRAAT_ARCH=i686 PRAAT_WIN7_X86=1 EXECUTABLE_FILE=Praat-win7-x86.exe -j"$jobs"
./Praat-win7-x86.exe --utf8 --version
export PRAAT_EXECUTABLE=./Praat-win7-x86.exe
bash scripts/test-lua.sh
bash scripts/test-lua-plot.sh
bash scripts/test-trajectories.sh
bash scripts/test-windows7-x86.sh
objdump -p Praat-win7-x86.exe > .local-build/pe-imports.txt
objdump -f Praat-win7-x86.exe | grep 'file format pei-i386'
# Standalone distribution must depend only on inbox Windows libraries.
if grep -Ei 'DLL Name:.*(api-ms-|ucrtbase|libgcc|libstdc|libwinpthread|msys-)' .local-build/pe-imports.txt; then
    echo 'Unexpected runtime DLL dependency.' >&2; exit 1
fi
release="$root/dist/windows7-x86"
mkdir -p "$release/examples" "$release/tutorials/lua-plots" "$release/verification"
cp Praat-win7-x86.exe "$release/praat-win7-x86.exe"
cp BUILD_WINDOWS7_X86.md README.md CHANGES_CUSTOM.md "$release/"
cp docs/manual/General_Public_License__version_3.html "$release/LICENSE-GPL-3.html"
cp external/lua-5.5.1/doc/readme.html "$release/LICENSE-Lua.html"
cp docs/lua.md docs/lua-debugger.md docs/lua-plot.md docs/code-folding.md docs/frequency-trajectories.md "$release/"
cp -R docs/tutorials/lua-plots/. "$release/tutorials/lua-plots/"
cp tests/trajectories/tracks.csv tests/trajectories/demo.praat "$release/examples/"
cp tests/lua-plot/demo.lua "$release/examples/lua-plot-demo.lua"
cp tests/lua-plot/embedded-demo.lua "$release/examples/lua-embedded-demo.lua"
cp tests/lua-plot/spectrogram-demo.lua "$release/examples/lua-spectrogram-demo.lua"
cp tests/lua-debugger/demo.lua "$release/examples/lua-debugger-demo.lua"
cp .local-build/toolchain.txt .local-build/pe-imports.txt .local-build/*tests.log "$release/verification/"
echo "$revision" > "$release/SOURCE-COMMIT.txt"
(cd "$release"; sha256sum praat-win7-x86.exe > SHA256SUMS.txt)
git -C "$root" archive --format=zip --output="$root/dist/praat-win7-x86-source.zip" "$revision"
echo "Built and batch-tested: $release/praat-win7-x86.exe"
