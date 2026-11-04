#!/usr/bin/env bash
set -euo pipefail
root="${1:?Linux source directory required}"
output="${2:?Output directory required}"
cd "$root"
mkdir -p .local-build/linux-gui-fb "$output"
if [[ "${3:-}" != inside ]]; then
    rm -f .local-build/spectrogram-s-gui-final.ok
    exec xvfb-run -a -s "-screen 0 1280x900x24 -fbdir $root/.local-build/linux-gui-fb" \
        bash "$0" "$root" "$output" inside
fi
unset DBUS_SESSION_BUS_ADDRESS
./praat-pro --utf8 --new-send --no-pref-files --no-plugins --FULL-TRUST tests/spectrogram-s/gui.praat > .local-build/linux-gui.log 2>&1 &
app=$!
trap 'kill "$app" 2>/dev/null || true' EXIT
for attempt in $(seq 1 40); do
    [[ -f .local-build/spectrogram-s-gui-final.ok ]] && break
    kill -0 "$app" 2>/dev/null || break
    sleep 0.25
done
sleep 1 # let GTK map and paint windows after the startup script returns
cat .local-build/linux-gui.log
cat .local-build/spectrogram-s-gui-final.ok
cp .local-build/linux-gui-fb/Xvfb_screen0 "$output/linux-gui.xwd"
cp .local-build/linux-gui.log .local-build/spectrogram-s-gui-final.ok "$output/"
