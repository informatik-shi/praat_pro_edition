#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang -std=gnu++17 -municode -D_FILE_OFFSET_BITS=64 -O2 -I sys -I melder -I kar -I fon -I main -c tests/code-assist/runner.cpp -o .local-build/code-assist-runner.o
sed 's@main/main_Praat.o@.local-build/code-assist-runner.o@g' Makefile > .local-build/Makefile.code-assist
make -f .local-build/Makefile.code-assist PRAAT_ARCH=x64v1 EXECUTABLE_FILE=.local-build/code-assist-editor-tests.exe -j4 > .local-build/code-assist-editor-build.log 2>&1
./.local-build/code-assist-editor-tests.exe > .local-build/code-assist-editor-tests.log 2>&1
cat .local-build/code-assist-editor-tests.log
grep -q 'CODE ASSIST EDITOR TESTS: PASS' .local-build/code-assist-editor-tests.log
