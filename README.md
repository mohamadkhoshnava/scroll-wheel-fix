# Scroll Wheel Fix

**Fix mouse scroll wheel jumping, bouncing, jitter, and random reverse scrolling** with a small, local input filter for **macOS, Windows, and Linux**.

[![Build and tests](https://github.com/mohamadkhoshnava/scroll-wheel-fix/actions/workflows/ci.yml/badge.svg)](https://github.com/mohamadkhoshnava/scroll-wheel-fix/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

[Download v0.2.0 preview](https://github.com/mohamadkhoshnava/scroll-wheel-fix/releases/tag/v0.2.0) · [راهنمای فارسی](README.fa.md) · [Report a mouse compatibility result](https://github.com/mohamadkhoshnava/scroll-wheel-fix/issues/new/choose)

## Does your mouse wheel scroll in the wrong direction?

You scroll down, but the page suddenly jumps up. Or you scroll up and it briefly goes down. Scroll Wheel Fix buffers short opposite-direction bursts and discards them when the intended direction resumes.

Originally called **M220 Scroll Fix**, this project began with a Logitech M220. It now has no mouse-brand or model allowlist: wired, USB-receiver, and Bluetooth mice can be candidates when they produce the supported wheel events. Support depends on the input format and OS backend, not the logo on the mouse. **This does not mean every mouse has been tested.**

This is a workaround for noisy wheel input, not a hardware repair. A quick intentional reverse tick can be suppressed; deliberate reversals can wait for a subsequent tick. It will not fix every cause of scrolling lag, page reflow, lost wireless packets, or a broken wheel.

## Downloads and platform support

Get the platform archive and `SHA256SUMS.txt` from the [release page](https://github.com/mohamadkhoshnava/scroll-wheel-fix/releases/tag/v0.2.0). The cross-platform version is a **preview**.

| Platform | Download | Interface and access | Verification |
| --- | --- | --- | --- |
| macOS 13+ | `Scroll-Wheel-Fix-macOS-universal.zip` | Native menu bar app, Intel + Apple Silicon; Accessibility permission | Swift/C++ tests and universal build in CI; earlier M220 version manually used on one Mac |
| Windows 10/11 x64 | `Scroll-Wheel-Fix-Windows-x64.zip` | Portable native tray app; ordinary interactive desktop session | Native build and C++ tests in CI; physical-mouse testing still needed |
| Linux x64 | `Scroll-Wheel-Fix-Linux-x64.tar.gz` | CLI; selected evdev mouse + uinput, usually `sudo`; designed for Wayland and X11 | Build, CLI checks, wheel-frame and C++ tests in CI; physical-mouse/compositor testing still needed |

The Linux binary is built on Ubuntu 24.04 and needs `libevdev.so.2` and a compatible C++/glibc runtime. Build from source for other distributions or architectures. Windows binaries are unsigned; macOS builds are ad hoc signed, not Developer ID signed or notarized. No account, subscription, or internet connection is required to run the tools.

### macOS quick start

1. Extract the ZIP, move **Scroll Wheel Fix.app** to Applications, and open it.
2. Enable it in **System Settings → Privacy & Security → Accessibility**. Add the installed app with **+** if needed.
3. **SWF ✓** in the menu bar means active. Use that menu to pause, select a mode, or quit.

Quit older copies before running the new app. The original M220 utility uses a different bundle identity; the renamed app needs its own permission. See [macOS setup and source build](docs/macos.md).

### Windows quick start

1. Extract the ZIP and run **ScrollWheelFix.exe** in your normal user session.
2. Right-click its icon in the notification area (possibly under the hidden-icons arrow).
3. Choose **Pause filter**, **Balanced**, **Strong**, or **Quit**. Double-click the icon for help.

No installer or driver is needed. Elevated applications and raw-input games can behave differently. See [Windows setup and source build](docs/windows.md).

### Linux quick start

```bash
# Ubuntu / Debian runtime dependency:
sudo apt install libevdev2
./scroll-wheel-fix --help
sudo ./scroll-wheel-fix --list
# Replace eventN with the mouse-only device identified above:
sudo ./scroll-wheel-fix --device /dev/input/eventN --mode balanced
```

The process temporarily grabs **only the selected mouse**, creates a virtual mouse, and forwards movement/buttons while filtering wheel frames. It releases the device on exit. **Ctrl+C** stops it. It does not grab the keyboard or install a privileged service. Read [Linux setup, uinput permissions, and recovery](docs/linux.md) before using this backend.

## How the scroll bounce filter works

- Same-direction wheel input passes without an intentional delay.
- Opposite input is held. Returning to the original direction discards that burst.
- At least two reverse events spanning **80 ms (Balanced)** or **140 ms (Strong)** confirm an intentional reversal; held events are replayed in order.
- After more than **600 ms** without an accepted tick, direction resets and the next tick passes immediately.

The window is a confirmation threshold, not a timer that releases a lone event. A single quick reverse tick may disappear. Noise after an idle gap or a long faulty burst may still pass. Start with Balanced and compare with the filter paused/stopped.

| Backend | Scope and bypasses |
| --- | --- |
| macOS | All noncontinuous vertical wheel input; continuous/pixel input, horizontal and modifier-key scroll bypass |
| Windows | All standard vertical wheel input in the session; injected, fractional, and modifier-key scroll bypass; device model is unavailable to this hook |
| Linux | One explicitly selected relative mouse; low/high-resolution vertical wheel fields share one decision per frame; horizontal frames bypass; keyboard modifiers are not monitored, so modified wheel gestures are still filtered |

Touchpads, absolute devices, composite keyboard nodes, unusual gaming drivers, and smooth-scroll converters are not universally supported. [Compatibility and known limits](docs/compatibility.md) explains the boundaries. Please report actual mouse/OS results rather than assuming support from a brand name.

## Automatic builds and releases

[GitHub Actions](https://github.com/mohamadkhoshnava/scroll-wheel-fix/actions/workflows/ci.yml) builds and tests all three platforms on pushes to `main`, pull requests, and manual runs. Each successful job uploads a downloadable platform archive. These workflow artifacts expire after 14 days and may require GitHub sign-in.

Pushing a `v*` tag runs the same checks and automatically creates a **preview release** with all three archives and SHA-256 checksums only after every platform succeeds. Release assets are the public download path. See [contribution and release instructions](CONTRIBUTING.md).

## Build from source

```bash
git clone https://github.com/mohamadkhoshnava/scroll-wheel-fix.git
cd scroll-wheel-fix
```

- **macOS:** `./scripts/test.sh`, then `ARCHS=universal ./scripts/build.sh` (requires Xcode Command Line Tools or Xcode).
- **Windows:** CMake + Visual Studio C++ tools; [build instructions](docs/windows.md#build-from-source).
- **Linux:** CMake + C++17 + libevdev development headers; [build instructions](docs/linux.md#build-from-source).

The native C++ filter and Linux frame tests run without input-device access. CI does not install an input hook or grab physical hardware. Passing CI proves compilation and regression checks, not end-to-end behavior on every desktop.

## Privacy, stopping, and removal

No backend has a network client, telemetry, stored input history, or automatic startup entry. Counters live in memory. The macOS tap requests only scroll events. The Windows mouse hook immediately passes non-wheel events and installs no keyboard hook; it checks modifier state. Linux reads and forwards events only from the selected mouse and rejects nodes advertising keyboard keys.

Accessibility and Linux device permissions are powerful; grant them yourself for the intended executable. Quit from the macOS/Windows menu or press Ctrl+C on Linux to stop. Remove the executable/app and any permission you granted to uninstall. No system mouse settings are rewritten.

## Troubleshooting and contributions

See [scroll wheel jumping / reverse scrolling FAQ](docs/troubleshooting.md), [compatibility](docs/compatibility.md), and [CONTRIBUTING.md](CONTRIBUTING.md). Reports should include OS, mouse model, connection, mode, and behavior with the filter stopped. Avoid credentials, serial numbers, and unrelated personal data.

[MIT license](LICENSE) · Copyright © 2026 Mohammad Khoshnava. Independent software; not an official Logitech or other mouse-vendor product.
