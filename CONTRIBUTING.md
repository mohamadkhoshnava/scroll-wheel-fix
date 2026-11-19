# Contributing

Please include reproducible input sequences or synthetic fixtures for filtering changes. Do not commit private event recordings, device serial numbers, credentials, or machine-specific paths.

## Layout and tests

- `Sources/DirectionGate.swift`: macOS direction-confirmation rules.
- `Sources/main.swift`: Swift/AppKit event tap and menu.
- `native/common/filter.hpp`: equivalent C++ gate and bounded wheel buffer.
- `native/windows/main.cpp`: Win32 tray, mouse hook, and replay.
- `native/linux/`: libevdev/uinput proxy and wheel-frame adapter.
- `Tests/` and `native/tests/`: synthetic regression checks.

Keep Swift and C++ gate semantics aligned. Test same-direction input, isolated bounce, paired bursts, deliberate reversals, mode/reset transitions, idle gaps, and bounded buffering. On Linux, keep low/high-resolution wheel representations in one frame-level decision and never buffer movement or buttons.

Run the [platform build instructions](README.md#build-from-source). CMake/CTest covers native logic; `./scripts/test.sh` covers Swift. Tests do not require Accessibility, root, or device access. New backend changes also need manual OS/hardware validation; document what you did and what remains untested.

## CI and publishing

Pushes to main, pull requests, and manual dispatch build macOS universal, Windows x64, and Linux x64 artifacts. A `v*` tag publishes a downloadable release marked Latest only after all platform jobs pass. Backend maturity and preview limitations must remain explicit in its notes. Actions are pinned to upstream commit SHAs. Only the release job has repository-content write permission.

Before tagging, update the version in `CMakeLists.txt`, `native/common/filter.hpp`, `Resources/Info.plist`, and the CLI version check in `.github/workflows/ci.yml`. Add `docs/releases/vX.Y.Z.md` with accurate notes, download requirements, and untested behavior. Tag the reviewed commit and push the tag; the workflow adds the archives and `SHA256SUMS.txt` automatically. Releases are published as regular GitHub releases so they appear in the repository sidebar and `/releases/latest`. The Latest label identifies the current download; it does not replace backend validation. Keep unverified backends explicitly labeled as previews in the release notes and compatibility table.

The local build scripts do not replace a running installation. Do not change an installed copy or its permissions as part of a build/test.
