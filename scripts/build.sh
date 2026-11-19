#!/bin/bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT/dist/M220 Scroll Fix.app"
ARCH="$(uname -m)"

case "$ARCH" in
    arm64|x86_64) ;;
    *) echo "Unsupported architecture: $ARCH" >&2; exit 1 ;;
esac

mkdir -p "$APP/Contents/MacOS"
cp "$ROOT/Resources/Info.plist" "$APP/Contents/Info.plist"

xcrun swiftc -swift-version 5 -O \
    -target "${ARCH}-apple-macosx13.0" \
    -framework AppKit -framework ApplicationServices -framework CoreGraphics \
    "$ROOT/Sources/DirectionGate.swift" "$ROOT/Sources/main.swift" \
    -o "$APP/Contents/MacOS/M220ScrollFix"

codesign --force --sign - \
    --identifier io.github.mohamadkhoshnava.m220scrollfix "$APP"
codesign --verify --strict "$APP"
plutil -lint "$APP/Contents/Info.plist"
echo "Built for $ARCH: $APP"
