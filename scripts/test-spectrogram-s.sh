#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .local-build
"${PRAAT_EXECUTABLE:-./Praat-custom.exe}" --utf8 --no-pref-files --no-plugins --FULL-TRUST --run tests/spectrogram-s/test.praat > .local-build/spectrogram-s-tests.log 2>&1
cat .local-build/spectrogram-s-tests.log
grep -q 'SPECTROGRAM S TESTS: PASS' .local-build/spectrogram-s-tests.log
