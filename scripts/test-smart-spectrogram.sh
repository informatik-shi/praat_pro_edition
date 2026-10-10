#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/smart-spectrogram/test.praat > .local-build/smart-spectrogram-tests.log 2>&1
cat .local-build/smart-spectrogram-tests.log
grep -q 'SMART SPECTROGRAM TESTS: PASS' .local-build/smart-spectrogram-tests.log
