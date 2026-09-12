#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
clang -std=gnu++17 -municode -D_FILE_OFFSET_BITS=64 -O2 -I sys -I melder -I kar -I fon -I main -c tests/lua-plot/host-runner.cpp -o .local-build/lua-plot-host-runner.o
sed 's@main/main_Praat.o@.local-build/lua-plot-host-runner.o@g' Makefile > .local-build/Makefile.lua-plot-host
make -f .local-build/Makefile.lua-plot-host PRAAT_ARCH=x64v1 EXECUTABLE_FILE=.local-build/lua-plot-host-tests.exe -j4 > .local-build/lua-plot-host-build.log 2>&1
./.local-build/lua-plot-host-tests.exe > .local-build/lua-plot-host-tests.log 2>&1
cat .local-build/lua-plot-host-tests.log
grep -q 'LUA PLOT HOST TESTS: PASS' .local-build/lua-plot-host-tests.log
