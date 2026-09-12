#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang -std=gnu++17 -municode -D_FILE_OFFSET_BITS=64 -O2 -I sys -I melder -I kar -I fon -I main -c tests/lua-debugger/runner.cpp -o .local-build/lua-debugger-runner.o
sed 's@main/main_Praat.o@.local-build/lua-debugger-runner.o@g' Makefile > .local-build/Makefile.lua-debugger
make -f .local-build/Makefile.lua-debugger PRAAT_ARCH=x64v1 EXECUTABLE_FILE=.local-build/lua-debugger-tests.exe -j4 > .local-build/lua-debugger-test-build.log 2>&1
./.local-build/lua-debugger-tests.exe > .local-build/lua-debugger-tests.log 2>&1
cat .local-build/lua-debugger-tests.log
grep -q 'LUA DEBUGGER TESTS: PASS' .local-build/lua-debugger-tests.log
