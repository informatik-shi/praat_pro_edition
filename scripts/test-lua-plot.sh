#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/lua-plot/test.praat > .local-build/lua-plot-tests.log 2>&1
cat .local-build/lua-plot-tests.log
grep -q 'LUA PLOT TESTS: PASS' .local-build/lua-plot-tests.log
