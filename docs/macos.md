# macOS setup

Requires macOS 13+, on Intel or Apple Silicon. The release ZIP contains a universal app. Builds are ad hoc signed, not Developer ID signed or notarized; building from source is also available.

## Install and enable

1. Extract the archive and move **Scroll Wheel Fix.app** into Applications.
2. Quit any other copy of Scroll Wheel Fix or the original M220 Scroll Fix.
3. Open the app. Use **Open Accessibility settings** in the setup window.
4. In **Privacy & Security → Accessibility**, add the installed app with **+** if absent and enable it yourself. Authenticate if macOS requests it.
5. Within a few seconds, **SWF ✓** appears in the menu bar. **SWF ○** means paused or waiting for permission.

Downloaded builds may be flagged by macOS because they are not notarized. Review the source and your system's security notice; building locally is an alternative. Do not disable Gatekeeper or grant unrelated permissions.

Use **Setup and help** to reopen the window. **Pause filter** removes the active event tap until resumed. **Balanced** uses an 80 ms confirmation threshold; **Strong** uses 140 ms. These are not automatic release timers. Mode, pause state, and counters reset at restart.

## Scope

The event tap requests scroll-wheel events only. It filters all noncontinuous vertical wheel input, regardless of mouse brand. Continuous/pixel input, horizontal scrolling, and modified scroll gestures bypass it. The continuous flag is an event format, not a reliable mouse/trackpad identity.

Other tools that convert wheel input to continuous events may prevent this filter from acting. Run one wheel-remapping utility at a time while troubleshooting. The renamed app has a new bundle identifier; permission granted to the original M220 utility does not transfer.

## Build from source

Install Xcode Command Line Tools or Xcode, then run from the checkout:

```bash
./scripts/test.sh
ARCHS=universal ./scripts/build.sh
```

Output: `dist/Scroll Wheel Fix.app`. Omitting `ARCHS` builds only the current architecture; `ARCHS=arm64` or `ARCHS=x86_64` selects one. The build script never replaces an installed app.

Quit before replacing an older app. Moving or rebuilding an ad hoc signed app may require removing and re-adding its Accessibility entry. If permission is on but the filter is unavailable, quit and reopen the app.

## Stop and remove

Choose **Quit Scroll Wheel Fix** in the SWF menu. Remove its Accessibility entry and move the app to Trash. No launch agent or login item is installed. Add it yourself in macOS Login Items only if you want startup behavior.

API reference: [Apple Core Graphics event taps](https://developer.apple.com/documentation/coregraphics/cgevent/tapcreate(tap:place:options:eventsofinterest:callback:userinfo:)).
