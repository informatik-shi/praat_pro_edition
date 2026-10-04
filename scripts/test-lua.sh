#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/lua/test.praat > .local-build/lua-tests.log 2>&1
cat .local-build/lua-tests.log
grep -q 'LUA TESTS: PASS' .local-build/lua-tests.log
grep -q 'LUA WRAPPER: PASS' .local-build/lua-tests.log
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --run tests/lua/cli.lua hello > .local-build/lua-cli-explicit.log 2>&1
cat .local-build/lua-cli-explicit.log
grep -q 'LUA CLI: PASS' .local-build/lua-cli-explicit.log
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins tests/lua/cli.lua hello > .local-build/lua-cli-direct.log 2>&1
cat .local-build/lua-cli-direct.log
grep -q 'LUA CLI: PASS' .local-build/lua-cli-direct.log
