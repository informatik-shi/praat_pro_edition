#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./dist/praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/seg/test.praat > .local-build/seg-tests.log 2>&1
cat .local-build/seg-tests.log
grep -q 'SEG TESTS: PASS' .local-build/seg-tests.log