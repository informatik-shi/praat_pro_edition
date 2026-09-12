#!/usr/bin/env bash
# Run in the isolated x86 build tree, after building the application.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
[[ "${MSYSTEM:-}" == MINGW32 ]] || exit 2
mkdir -p .local-build
g++ -std=c++17 -march=i686 -O2 -static tests/folding/model.cpp -o .local-build/fold-model-tests.exe
./.local-build/fold-model-tests.exe | tee .local-build/fold-model-tests.log
for suite in debugger lua-debugger lua-plot-host; do
    source="tests/$suite/runner.cpp"
    [[ "$suite" != lua-plot-host ]] || source=tests/lua-plot/host-runner.cpp
    g++ -std=gnu++17 -municode -D_FILE_OFFSET_BITS=64 -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 \
        -march=i686 -m32 -O2 -I sys -I melder -I kar -I fon -I main \
        -c "$source" -o ".local-build/$suite-runner.o"
    sed "s@main/main_Praat.o@.local-build/$suite-runner.o@g" Makefile > ".local-build/Makefile.$suite"
    make -f ".local-build/Makefile.$suite" PRAAT_OS=windows PRAAT_COMPILER=gcc \
        PRAAT_ARCH=i686 PRAAT_WIN7_X86=1 EXECUTABLE_FILE=".local-build/$suite-tests.exe" \
        -j"${JOBS:-2}" > ".local-build/$suite-build.log" 2>&1
    "./.local-build/$suite-tests.exe" > ".local-build/$suite-tests.log" 2>&1
    cat ".local-build/$suite-tests.log"
    grep -q 'TESTS: PASS' ".local-build/$suite-tests.log"
done
