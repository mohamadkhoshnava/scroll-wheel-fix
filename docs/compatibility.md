# Mouse compatibility and verification

Scroll Wheel Fix has no brand/model allowlist. A Logitech M220 inspired the project, but a standard wheel from another vendor can use the same backend. Compatibility depends on event format, privileges, desktop behavior, and competing remapping tools.

| Input / environment | macOS | Windows | Linux |
| --- | --- | --- | --- |
| Standard relative vertical wheel | All noncontinuous wheel input | Standard 120-unit deltas in the session | One selected relative mouse |
| Multiple mice | Shared global direction state | Shared global direction state | One explicit device per process |
| Trackpad / continuous input | Passed through when marked continuous | Fractional deltas bypass; not a guaranteed device distinction | Absolute devices rejected |
| High-resolution wheel | Continuous-formatted input bypasses | Fractional deltas bypass | Low/high-resolution fields filtered together |
| Horizontal wheel | Bypassed | Unchanged | Horizontal/diagonal frames bypass |
| Modified scrolling / zoom | Bypassed | Bypassed by modifier-state check | Still filtered; keyboard is not monitored |
| Elevated/protected apps | Depends on macOS event routing | UIPI can prevent replay | Depends on access/session handling |
| Games / raw input | Not broadly verified | Not broadly verified | Virtual mouse acceptance depends on software |
| X11 / Wayland | N/A | N/A | Kernel backend designed for either; compositor testing pending |

## What has actually been verified

- Original Swift utility: used on one Mac with a Logitech M220; the user reported improvement. No numeric live success rate was saved.
- Current macOS app: builds for Intel and Apple Silicon; synthetic Swift/C++ checks cover its filtering rules.
- Windows: native C++ build and shared-filter tests in CI. The live hook/tray/input replay needs Windows hardware testing.
- Linux: native build, CLI checks, shared-filter and paired-wheel-frame regression tests in CI. Device grabbing, virtual-device forwarding, and compositor interaction need physical Linux testing.

No claim is made that all Logitech, Razer, Microsoft, Corsair, SteelSeries, or other mice have been tested. Brand names describe possible hardware, not certification. Faulty sensors, drivers, or wireless links may produce symptoms the direction filter cannot resolve.

## Report a result

Open a compatibility issue with the mouse model, connection type, OS/session, mode, other input utilities, and a comparison with the filter stopped. Do not include serial numbers, private logs, or credentials. Reports can establish support for specific combinations; compilation alone cannot.
