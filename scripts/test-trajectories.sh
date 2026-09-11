#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/trajectories/test.praat > .local-build/trajectories-tests.log 2>&1
cat .local-build/trajectories-tests.log
grep -q 'TRAJECTORIES TESTS: PASS' .local-build/trajectories-tests.log
