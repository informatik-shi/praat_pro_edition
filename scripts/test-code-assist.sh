#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang++ -std=c++17 -O2 tests/code-assist/model.cpp -o .local-build/code-assist-tests.exe
./.local-build/code-assist-tests.exe | tee .local-build/code-assist-tests.log
