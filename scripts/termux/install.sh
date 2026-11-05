#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail
: "${PREFIX:?Run this installer in Termux}"
export TMPDIR="${TMPDIR:-$PREFIX/tmp}"
[[ "$(uname -m)" == aarch64 ]] || { echo 'This package requires 64-bit ARM Android/Termux.' >&2; exit 2; }
bundle="${1:?Usage: bash install-praat-pro-termux.sh /path/to/praat-pro-termux-aarch64.tar.gz}"
bundle="$(realpath -- "$bundle")"
[[ -f "$bundle" ]] || { echo "Package not found: $bundle" >&2; exit 2; }
container=praat-pro
pkg install -y x11-repo
pkg install -y proot-distro termux-x11-nightly pulseaudio
# Keep an existing Ubuntu environment intact. Create a separate named container.
if ! proot-distro login "$container" -- /usr/bin/true >/dev/null 2>&1; then
    proot-distro install ubuntu:24.04 --name "$container"
fi
stage="$(mktemp -d "$TMPDIR/praat-pro-install.XXXXXX")"
# Only this mktemp directory is cleaned; never remove an existing distribution.
trap 'rm -f -- "$stage/package.tar.gz"; rmdir -- "$stage"' EXIT
cp -- "$bundle" "$stage/package.tar.gz"
proot-distro login "$container" --bind "$stage:/mnt/praat-install" -- /bin/bash -s <<'GUEST'
set -euo pipefail
test "$(dpkg --print-architecture)" = arm64
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y libgtk-3-0t64 libasound2t64 \
    libpulse0 libjack-jackd2-0 librsvg2-common shared-mime-info fonts-dejavu-core \
    dbus-x11 openbox wmctrl x11-utils ca-certificates python3
# Reject archive paths outside the dedicated application directory, including links.
python3 - <<'PY'
import pathlib, tarfile
with tarfile.open('/mnt/praat-install/package.tar.gz') as archive:
    for member in archive.getmembers():
        name = pathlib.PurePosixPath(member.name)
        if name.is_absolute() or '..' in name.parts or not name.parts or name.parts[0] != 'praat-pro':
            raise SystemExit('Unsafe package path: ' + member.name)
        if not (member.isfile() or member.isdir()):
            raise SystemExit('Unsupported package entry: ' + member.name)
    archive.extractall('/opt', filter='data')
PY
chmod 755 /opt/praat-pro/praat-pro /opt/praat-pro/bin/praat-pro-session
/opt/praat-pro/praat-pro --utf8 --version
GUEST
# Extract the reviewed launcher from beside this installer, or from the bundle.
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$script_dir/start.sh" ]]; then
    cp "$script_dir/start.sh" "$PREFIX/bin/praat-pro"
else
    tar -xOf "$bundle" praat-pro/bin/start-termux.sh > "$PREFIX/bin/praat-pro"
fi
chmod 755 "$PREFIX/bin/praat-pro"
printf '\nInstalled. Open the Termux:X11 Android app, then run: praat-pro\n'
