#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
if [[ "${MSYSTEM:-}" != CLANG64 ]]; then
    echo 'Run this script from the MSYS2 CLANG64 environment.' >&2
    exit 2
fi
jobs="${JOBS:-4}"
if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo 'JOBS must be a positive integer.' >&2
    exit 2
fi
mkdir -p .local-build
{
    date -Is
    clang --version
    make --version
    pkg-config --version
    pacman -Q
} > .local-build/toolchain.txt
make PRAAT_ARCH=x64v1 -j"$jobs" 2>&1 | tee .local-build/build-windows.log
./Praat.exe --utf8 --version | tee .local-build/version.txt
sha256sum Praat.exe | tee .local-build/Praat.exe.sha256
