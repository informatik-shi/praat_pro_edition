#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang++ -std=c++17 -O2 tests/folding/model.cpp -o .local-build/fold-model-tests.exe
./.local-build/fold-model-tests.exe | tee .local-build/fold-model-tests.log
