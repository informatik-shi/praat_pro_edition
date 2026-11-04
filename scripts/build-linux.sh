#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
[[ "$(uname -s)" == Linux ]] || { echo 'Run in a native Linux checkout or isolated Linux source snapshot.' >&2; exit 2; }
jobs="${JOBS:-4}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
mkdir -p .local-build
make PRAAT_OS=linux PRAAT_ARCH="${PRAAT_ARCH:-x64v1}" EXECUTABLE_FILE=praat-pro -j"$jobs" 2>&1 | tee .local-build/build-linux.log
./praat-pro --utf8 --version | tee .local-build/linux-version.txt
export PRAAT_EXECUTABLE=./praat-pro
bash scripts/test-spectrogram-s.sh
bash scripts/test-smart-spectrogram.sh
bash scripts/test-trajectories.sh
bash scripts/test-seg.sh
bash scripts/test-lua.sh
bash scripts/test-lua-plot.sh
mkdir -p dist/linux/praat-pro
cp praat-pro dist/linux/praat-pro/
bash scripts/platforms/package-common.sh dist/linux/praat-pro
ldd praat-pro > dist/linux/praat-pro/DEPENDENCIES.txt
tar -C dist/linux -czf "dist/praat-pro-linux-$(uname -m).tar.gz" praat-pro
sha256sum "dist/praat-pro-linux-$(uname -m).tar.gz" | tee .local-build/linux-package.sha256
