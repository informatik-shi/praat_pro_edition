#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./dist/praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/pro/test.praat > .local-build/pro-tests.log 2>&1
cat .local-build/pro-tests.log
grep -q 'PRO TESTS: PASS' .local-build/pro-tests.log
