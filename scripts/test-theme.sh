#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang++ -std=c++17 -O2 tests/theme/palette.cpp -o .local-build/theme-palette-tests.exe
./.local-build/theme-palette-tests.exe | tee .local-build/theme-palette-tests.log
