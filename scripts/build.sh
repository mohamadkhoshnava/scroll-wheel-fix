#!/bin/bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT/dist/Scroll Wheel Fix.app"
ARCHS="${ARCHS:-$(uname -m)}"
if [[ "$ARCHS" == "universal" ]]; then ARCHS="arm64 x86_64"; fi

mkdir -p "$APP/Contents/MacOS"
cp "$ROOT/Resources/Info.plist" "$APP/Contents/Info.plist"

BINARIES=()
for ARCH in $ARCHS; do
    case "$ARCH" in
        arm64|x86_64) ;;
        *) echo "Unsupported architecture: $ARCH" >&2; exit 1 ;;
    esac
    mkdir -p "$ROOT/.build/macos/$ARCH"
    BINARY="$ROOT/.build/macos/$ARCH/ScrollWheelFix"
    xcrun swiftc -swift-version 5 -O \
        -target "${ARCH}-apple-macosx13.0" \
        -framework AppKit -framework ApplicationServices -framework CoreGraphics \
        "$ROOT/Sources/DirectionGate.swift" "$ROOT/Sources/main.swift" \
        -o "$BINARY"
    BINARIES+=("$BINARY")
done
if [[ ${#BINARIES[@]} -eq 1 ]]; then
    cp "${BINARIES[0]}" "$APP/Contents/MacOS/ScrollWheelFix"
else
    xcrun lipo -create "${BINARIES[@]}" -output "$APP/Contents/MacOS/ScrollWheelFix"
fi

codesign --force --sign - \
    --identifier io.github.mohamadkhoshnava.scrollwheelfix "$APP"
codesign --verify --strict "$APP"
plutil -lint "$APP/Contents/Info.plist"
echo "Built for $ARCHS: $APP"
