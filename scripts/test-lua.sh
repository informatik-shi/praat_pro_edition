#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/lua/test.praat > .local-build/lua-tests.log 2>&1
cat .local-build/lua-tests.log
grep -q 'LUA TESTS: PASS' .local-build/lua-tests.log
grep -q 'LUA WRAPPER: PASS' .local-build/lua-tests.log
