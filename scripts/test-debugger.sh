#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang -std=gnu++17 -municode -D_FILE_OFFSET_BITS=64 -O2 -I sys -I melder -I kar -I fon -I main -c tests/debugger/runner.cpp -o .local-build/debugger-runner.o
# Exactly the app's libraries and order; use the root Makefile link recipe with a test main.
sed 's@main/main_Praat.o@.local-build/debugger-runner.o@g' Makefile > .local-build/Makefile.debugger
make -f .local-build/Makefile.debugger PRAAT_ARCH=x64v1 EXECUTABLE_FILE=.local-build/debugger-tests.exe -j4 > .local-build/debugger-test-build.log 2>&1
./.local-build/debugger-tests.exe > .local-build/debugger-tests.log 2>&1
cat .local-build/debugger-tests.log
grep -q 'DEBUGGER INTEGRATION TESTS: PASS' .local-build/debugger-tests.log
