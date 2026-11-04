#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang++ -std=c++17 -O2 tests/smart-spectrogram/display.cpp -o .local-build/smart-display-tests.exe
./.local-build/smart-display-tests.exe | tee .local-build/smart-display-tests.log
