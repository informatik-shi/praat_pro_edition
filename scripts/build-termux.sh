#!/usr/bin/env bash
# Ubuntu/glibc build for proot-distro + Termux:X11, not an Android/Bionic binary.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
[[ "$(uname -s)" == Linux ]] || { echo 'Build inside Ubuntu Linux.' >&2; exit 2; }
case "$(uname -m)" in
    aarch64) architecture=arm64 ;;
    *) echo 'The tablet package must be built on ARM64.' >&2; exit 2 ;;
esac
jobs="${JOBS:-4}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
mkdir -p .local-build
compiler=()
if command -v ccache >/dev/null; then compiler=(CC='ccache gcc' CXX='ccache g++'); fi
make PRAAT_OS=linux PRAAT_ARCH="$architecture" EXECUTABLE_FILE=praat-pro "${compiler[@]}" -j"$jobs" \
    2>&1 | tee .local-build/build-termux.log
./praat-pro --utf8 --version | tee .local-build/termux-version.txt
export PRAAT_EXECUTABLE=./praat-pro
for suite in spectrogram-s smart-spectrogram trajectories seg lua lua-plot; do
    bash "scripts/test-$suite.sh"
done
# Real GTK windows, independent X server, window manager, screenshots and bounds.
for size in 1280x800 800x1280; do
    bash scripts/platforms/test-tablet-gui.sh "$PWD" "$PWD/.local-build/tablet-gui-$size" "$size"
done
package=dist/termux/praat-pro
mkdir -p "$package/bin"
cp praat-pro "$package/"
bash scripts/platforms/package-common.sh "$package"
cp docs/termux-tablet.md "$package/docs/"
cp scripts/termux/praat-pro-session.sh "$package/bin/praat-pro-session"
cp scripts/termux/start.sh "$package/bin/start-termux.sh"
chmod 755 "$package/bin/praat-pro-session" "$package/praat-pro"
ldd praat-pro > "$package/DEPENDENCIES.txt"
if grep -q 'not found' "$package/DEPENDENCIES.txt"; then
    echo 'Missing runtime dependencies' >&2; exit 1
fi
cp scripts/termux/install.sh dist/install-praat-pro-termux.sh
tar -C dist/termux -czf dist/praat-pro-termux-aarch64.tar.gz praat-pro
(cd dist && sha256sum praat-pro-termux-aarch64.tar.gz) | tee dist/praat-pro-termux-aarch64.sha256
