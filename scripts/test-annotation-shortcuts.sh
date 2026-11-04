#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang++ -std=c++17 -O2 tests/annotation-shortcuts/model.cpp -o .local-build/annotation-shortcuts-tests.exe
./.local-build/annotation-shortcuts-tests.exe | tee .local-build/annotation-shortcuts-tests.log
