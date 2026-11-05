#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
[[ "$(uname -s)" == Darwin ]] || { echo 'macOS and Xcode are required.' >&2; exit 2; }
mkdir -p .local-build
python3 scripts/platforms/prepare-macos.py
xcodebuild -project praat.xcodeproj -target praat_mac -configuration Configuration64 \
    ARCHS='arm64 x86_64' ONLY_ACTIVE_ARCH=NO CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO \
    DEVELOPMENT_TEAM='' MACOSX_DEPLOYMENT_TARGET=10.15 \
    BUILD_DIR="$PWD/.local-build/macos/products" OBJROOT="$PWD/.local-build/macos/objects" \
    SYMROOT="$PWD/.local-build/macos/products" -jobs "${JOBS:-4}" \
    2>&1 | tee .local-build/build-macos.log
app=.local-build/macos/products/Configuration64/Praat.app
binary="$app/Contents/MacOS/Praat"
[[ -x "$binary" ]]
lipo "$binary" -verify_arch arm64 x86_64
export PRAAT_EXECUTABLE="$PWD/$binary"
bash scripts/test-spectrogram-s.sh
bash scripts/test-smart-spectrogram.sh
bash scripts/test-trajectories.sh
bash scripts/test-seg.sh
bash scripts/test-lua.sh
bash scripts/test-lua-plot.sh
mkdir -p dist/macos
ditto "$app" 'dist/macos/Praat Pro.app'
bash scripts/platforms/package-common.sh dist/macos
# Local ad-hoc signing, not Developer ID signing or Apple notarization.
codesign --force --deep --sign - 'dist/macos/Praat Pro.app'
codesign --verify --deep --strict 'dist/macos/Praat Pro.app'
ditto -c -k --sequesterRsrc --keepParent dist/macos dist/praat-pro-macos-universal.zip
hdiutil create -volname 'Praat Pro Edition' -srcfolder dist/macos -ov -format UDZO dist/praat-pro-macos-universal.dmg
shasum -a 256 dist/praat-pro-macos-universal.{zip,dmg} | tee .local-build/macos-package.sha256
