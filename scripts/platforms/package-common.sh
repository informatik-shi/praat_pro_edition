#!/usr/bin/env bash
set -euo pipefail
target="${1:?package directory required}"
mkdir -p "$target/docs" "$target/examples"
cp CHANGES_CUSTOM.md README.md "$target/"
cp docs/manual/General_Public_License__version_3.html "$target/LICENSE-GPL-3.html"
cp external/lua-5.5.1/doc/readme.html "$target/LICENSE-Lua.html"
cp external/luafilesystem-1.9.0/LICENSE "$target/LICENSE-LuaFileSystem.txt"
cp docs/lua.md docs/lua-plot.md docs/smart-spectrogram.md docs/spectrogram-s.md \
   docs/frequency-trajectories.md docs/combined-editor.md docs/read-seg-files.md "$target/docs/"
if [[ -f docs/platform-builds.md ]]; then cp docs/platform-builds.md "$target/docs/"; fi
cp scripts/sample-count-spectrogram-demo.praat scripts/g1-period-smart-preview.praat \
   scripts/g1-period-smart-formants.praat "$target/examples/"
cp tests/trajectories/tracks.csv tests/trajectories/demo.praat "$target/examples/"
cp tests/lua/demo.lua "$target/examples/lua-demo.lua"
cp tests/lua-plot/demo.lua "$target/examples/lua-plot-demo.lua"
