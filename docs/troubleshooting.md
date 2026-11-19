# Mouse wheel jumping, bouncing, or reverse scrolling

## The page scrolls up when I scroll down

If brief opposite-direction input is the cause, start with Balanced and compare with the filter stopped. Strong requires a longer reverse sequence and may block more bounce, but also makes intentional reversals less responsive.

If the page jumps without any wheel movement, or because new page content changes layout, this filter may not help. Test in more than one application. Check battery, receiver placement/USB connection, and another computer when possible; software cannot repair a damaged wheel or guarantee a good wireless signal.

## Changing direction feels sticky

This is the filter's tradeoff: it waits for at least two reverse events spanning its threshold. A single quick reverse tick can be discarded. Use Balanced instead of Strong, or pause/quit when precise rapid direction changes matter.

## Some jumps remain

A wrong tick after a 600 ms idle gap passes immediately. A long enough faulty burst can look like an intentional reversal. The filter cannot infer physical intent. There is no guaranteed success rate for every wheel.

## Nothing changes with the filter active

- **macOS:** verify SWF ✓, Accessibility for this installed copy, and that another utility is not converting wheel events to continuous input.
- **Windows:** check the tray, standard wheel deltas, and whether the target app runs elevated or uses raw input. Quit/restart if Windows removed a stalled hook.
- **Linux:** verify the selected event node is the actual mouse, uinput is available, and the foreground process is still running. Mouse-only relative devices are required.

See the platform setup pages for details. Do not run old and new copies together while comparing.

## Can I use it with any mouse brand?

There is no model restriction. Typical wired, wireless USB-receiver, and Bluetooth wheel mice can be candidates, but event formats and drivers differ. Read the [compatibility table](compatibility.md), then report your actual result. Touchpads and proprietary/raw-input paths are separate cases.

## Is this an encoder or sensor repair?

No. It masks some wrong-direction input in software. Persistent hardware defects still need physical diagnosis, repair, or replacement. The project's name describes the scrolling workaround, not a guarantee that the wheel itself is fixed.
