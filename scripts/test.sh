#!/bin/bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/.build"
xcrun swiftc -swift-version 5 -O \
    "$ROOT/Sources/DirectionGate.swift" "$ROOT/Tests/main.swift" \
    -o "$ROOT/.build/DirectionGateTests"
"$ROOT/.build/DirectionGateTests"
