#!/usr/bin/env bash
set -euo pipefail
root="${1:?Source directory required}"
output="${2:?Output directory required}"
size="${3:-1280x800}"
[[ "$size" =~ ^[0-9]+x[0-9]+$ ]] || exit 2
cd "$root"
mkdir -p "$output"
if [[ "${4:-}" != inside ]]; then
    # The runner uses the same compiled libraries and GUI factories as praat-pro.
    g++ -std=c++17 -O1 -no-pie -DUNIX -Dlinux -DHAVE_PULSEAUDIO \
        -Ikar -Imelder -Isys -Idwsys -Ifon -Ifoned $(pkg-config --cflags gtk+-3.0) \
        tests/termux/editors.cpp -o .local-build/tablet-editors \
        -Wl,--start-group fon/libfon.a artsynth/libartsynth.a FFNet/libFFNet.a \
        gram/libgram.a EEG/libEEG.a LPC/libLPC.a dwtools/libdwtools.a sensors/libsensors.a \
        foned/libfoned.a stat/libstat.a dwsys/libdwsys.a sys/libsys.a melder/libmelder.a kar/libkar.a \
        external/{espeak,portaudio,flac,lame,mp3,glpk,clapack,gsl,num,vorbis,opusfile,whispercpp,blake3,zlib}/*.a \
        external/lua-5.5.1/liblua.a -Wl,--end-group \
        $(pkg-config --libs gtk+-3.0 jack) -lpulse -lasound -lm -lpthread
    exec xvfb-run -a -s "-screen 0 ${size}x24" dbus-run-session -- \
        bash "$0" "$root" "$output" "$size" inside
fi
unset WAYLAND_DISPLAY
export GDK_BACKEND=x11 PRAAT_TABLET=1 GDK_SCALE=1 GDK_DPI_SCALE=1
export XDG_RUNTIME_DIR="$output/runtime"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
openbox >"$output/openbox.log" 2>&1 &
wm=$!
app=
cleanup() { [[ -z "$app" ]] || kill "$app" 2>/dev/null || true; kill "$wm" 2>/dev/null || true; }
trap cleanup EXIT
for attempt in {1..40}; do wmctrl -m >/dev/null 2>&1 && break; sleep 0.1; done
rm -f .local-build/tablet-windows.ok
.local-build/tablet-editors --utf8 --new-send --no-pref-files --no-plugins --FULL-TRUST \
    tests/termux/windows.praat >"$output/praat.log" 2>&1 &
app=$!
for attempt in {1..120}; do
    [[ -f .local-build/tablet-windows.ok ]] && break
    kill -0 "$app" 2>/dev/null || { cat "$output/praat.log"; exit 1; }
    sleep 0.25
done
if [[ ! -f .local-build/tablet-windows.ok ]]; then cat "$output/praat.log"; exit 1; fi
python3 scripts/platforms/check-tablet-windows.py "$app" "$output" "$size"
cp .local-build/tablet-windows.ok "$output/"
