# M220 Scroll Fix

A small macOS menu bar app that filters brief wrong-direction mouse-wheel scroll bursts. It was built to make a Logitech M220 with jumpy scrolling more usable.

[راهنمای فارسی](README.fa.md) · [MIT license](LICENSE)

This is a software workaround, not a hardware repair. It can suppress a quick intentional reverse tick and delay deliberate direction changes. Try **Pause filter** when comparing behavior.

## Features

- Passes same-direction wheel scrolling without an intentional delay.
- Holds brief reverse bursts; discards them if the original direction resumes.
- Replays buffered ticks when a sustained direction change is confirmed.
- Offers **Balanced (80 ms)** and **Strong (140 ms)** modes.
- Passes continuous/pixel input, horizontal scrolling, and modifier-key scrolling unchanged.
- Provides pause, resume, counters, and quit controls in the menu bar.
- Uses native Swift/AppKit and Core Graphics, with no third-party dependencies.

## Requirements

- macOS 13 or later.
- Xcode Command Line Tools or Xcode with `swiftc` available through `xcrun`.
- Accessibility permission for the installed app.

The build targets the current Mac's architecture: Apple Silicon or Intel. A build made on one architecture is not a universal binary. The minimum deployment target is macOS 13; runtime behavior has only been manually checked on one Mac with an M220.

## Build and install

```bash
git clone https://github.com/mohamadkhoshnava/m220-scroll-fix.git
cd m220-scroll-fix
./scripts/test.sh
./scripts/build.sh
```

The app is created at `dist/M220 Scroll Fix.app`. Move it to your Applications folder using Finder, then open it. Quit any older copy before replacing it. The build script only writes inside this checkout; it does not replace an installed app or grant permissions.

The app is ad hoc signed for local use. It is not Apple Developer ID signed or notarized. Building from source is the intended installation method.

## Enable the filter

1. Open **M220 Scroll Fix**.
2. Open **System Settings → Privacy & Security → Accessibility** using the button in its setup window.
3. Add the installed app with **+** if it is missing, then enable its switch. macOS may ask you to authenticate.
4. Wait a few seconds. **M220 ✓** in the menu bar means the filter is active. **M220 ○** means it is paused or waiting for permission.

If permission is enabled but the app cannot attach its filter, quit and reopen it. Moving or rebuilding an ad hoc signed app may require adding it again in Accessibility.

The app does not install a login item. Open it again after logging in, or add it yourself through macOS Login Items if you want automatic startup.

## Controls

| Menu item | Behavior |
| --- | --- |
| Pause filter / Enable filter | Toggle the workaround without changing macOS mouse settings. |
| Balanced — 80 ms | Default direction-confirmation window. |
| Strong — 140 ms | Longer confirmation window; may suppress more noise but feel less responsive. |
| Wheel / suppressed counters | In-memory counts of eligible wheel events and discarded buffered events. |
| Continuous input passed through | Count of continuous scroll events that bypass the filter. |
| Setup and help | Reopen the setup window; its explanatory text is currently Persian. |
| Quit M220 Scroll Fix | Remove the event tap and stop filtering completely. |

Mode and pause state reset when the app restarts. Counters reset too. Held events that never receive a follow-up remain buffered until a later event or reset, so the suppressed counter can lag behind actual input.

## How it works

The filter remembers the last accepted vertical wheel direction. An opposite event is buffered. If the original direction returns, the reverse burst is discarded. If at least two opposite events span the confirmation window, the app accepts the new direction and replays its buffered ticks. After more than 600 ms without an accepted tick, it resets direction and lets the next tick through immediately.

The confirmation window is a threshold, not a timer that releases events automatically. Deliberate reversals wait for a subsequent tick; a single quick reverse tick can be discarded. A bad tick after an idle gap, or a sustained faulty burst, may still get through.

## Scope and privacy

The app requests a Core Graphics event tap for **scroll-wheel events only**. Its code does not subscribe to keyboard input, clicks, pointer movement, or screen contents. It has no network client, telemetry, account, event log, or stored scroll history; counters remain in memory. Accessibility is still a broad macOS permission, which you grant yourself in System Settings.

Quartz scroll events do not identify the mouse model. The filter therefore affects **all noncontinuous vertical wheel input**, not just an M220. Continuous/pixel input—normally trackpads or smooth-scroll software—is passed through. The continuous flag describes the event format, so it is not a guaranteed device identity check. Other utilities that convert wheel input to continuous input may bypass this filter.

Initial hands-on use with one M220 indicated improvement. There is no measured live success rate, no broad compatibility claim, and no guarantee that every jump can be removed. Synthetic regression tests cover the direction gate; manual testing is still needed for real hardware and macOS permissions.

## Development

```text
Sources/DirectionGate.swift  Direction-confirmation state machine
Sources/main.swift           Menu bar app and scroll event tap
Tests/main.swift             Synthetic regression checks
Resources/Info.plist         App bundle metadata
scripts/test.sh             Run tests without Accessibility permission
scripts/build.sh            Build and ad hoc sign the app bundle
```

GitHub Actions runs the tests and build on macOS and uploads a ZIP of that runner's app bundle as a workflow artifact. Artifacts are not notarized releases.

When reporting an issue, include your macOS version, mouse model, selected mode, whether another scroll utility is running, and whether the problem remains with this filter paused. Please do not include credentials or unrelated personal data.

## Uninstall

Quit from the **M220** menu, remove the app's Accessibility permission, and move the app to Trash. There is no background service or login item installed by this project.

## License

[MIT](LICENSE), copyright © 2026 Mohammad Khoshnava. This is an independent utility, not an official Logitech product.
