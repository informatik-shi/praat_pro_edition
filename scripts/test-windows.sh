#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
if [[ ! -f Praat.exe ]]; then
    echo 'Build Praat.exe first with scripts/build-windows.sh.' >&2
    exit 2
fi
mkdir -p .local-build
result=0
: > .local-build/test-results.txt
for suite in test dwtest; do
    echo "Running $suite/runAllTests_batch.praat"
    # Upstream tests create and remove temporary fixtures; trust only these suites.
    if ./Praat.exe --utf8 --no-pref-files --no-plugins --FULL-TRUST --run "$suite/runAllTests_batch.praat" > ".local-build/$suite.log" 2>&1; then
        echo "$suite: PASS (exit 0)" | tee -a .local-build/test-results.txt
    else
        code=$?
        echo "$suite: FAIL (exit $code)" | tee -a .local-build/test-results.txt
        tail -n 30 ".local-build/$suite.log"
        result=1
    fi
done
exit "$result"
