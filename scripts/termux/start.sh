#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail
: "${PREFIX:?Run this launcher in Termux, not inside Ubuntu}"
container=praat-pro
export TMPDIR="${TMPDIR:-$PREFIX/tmp}"
if [[ ! -S "$TMPDIR/.X11-unix/X1" ]]; then
    termux-x11 :1 >"$TMPDIR/praat-pro-x11.log" 2>&1 &
    for attempt in {1..50}; do
        [[ -S "$TMPDIR/.X11-unix/X1" ]] && break
        sleep 0.1
    done
fi
[[ -S "$TMPDIR/.X11-unix/X1" ]] || {
    echo 'Termux:X11 did not start. Install/open its Android application; see the setup guide.' >&2
    exit 1
}
pulseaudio --start --exit-idle-time=-1
mkdir -p "$TMPDIR/praat-pro-pulse"
chmod 700 "$TMPDIR/praat-pro-pulse"
if [[ ! -S "$TMPDIR/praat-pro-pulse/native" ]]; then
    pactl load-module module-native-protocol-unix \
        "socket=$TMPDIR/praat-pro-pulse/native" auth-anonymous=1 >/dev/null
fi
# The private UNIX socket is shared with Ubuntu; no unauthenticated TCP listener.
am start --user 0 -n com.termux.x11/com.termux.x11.MainActivity >/dev/null
exec proot-distro login "$container" --shared-tmp -- \
    /usr/bin/env DISPLAY=:1 PULSE_SERVER=unix:/tmp/praat-pro-pulse/native \
    /opt/praat-pro/bin/praat-pro-session "$@"
