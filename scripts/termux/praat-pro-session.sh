#!/usr/bin/env bash
set -euo pipefail
package_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export DISPLAY="${DISPLAY:-:1}" GDK_BACKEND=x11 PRAAT_TABLET=1
export GDK_SCALE="${PRAAT_GDK_SCALE:-1}" GDK_DPI_SCALE="${PRAAT_GDK_DPI_SCALE:-1}"
export PULSE_SERVER="${PULSE_SERVER:-unix:/tmp/praat-pro-pulse/native}"
unset WAYLAND_DISPLAY
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/praat-pro-runtime-$(id -u)}"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
if [[ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]]; then
    exec dbus-run-session -- "$0" "$@"
fi
wm_pid=
cleanup() { [[ -z "$wm_pid" ]] || kill "$wm_pid" 2>/dev/null || true; }
trap cleanup EXIT
if ! wmctrl -m >/dev/null 2>&1; then
    openbox >"$XDG_RUNTIME_DIR/openbox.log" 2>&1 &
    wm_pid=$!
    for attempt in {1..30}; do
        wmctrl -m >/dev/null 2>&1 && break
        sleep 0.1
    done
fi
"$package_dir/praat-pro" "$@"
