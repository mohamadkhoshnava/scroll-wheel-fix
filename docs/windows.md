# Windows setup

The preview targets Windows 10/11 x64 desktop sessions. It is a native C++/Win32 tray utility with a statically linked C++ runtime, no installer, and no driver. The executable is unsigned; Windows may show a reputation warning. Physical-mouse behavior has not yet been manually verified on Windows.

## Run

1. Extract `Scroll-Wheel-Fix-Windows-x64.zip`.
2. Run **ScrollWheelFix.exe** normally in your interactive user session.
3. Look in the notification area, including its hidden-icons arrow. Right-click the app icon for controls; double-click for help.
4. Start with **Balanced — 80 ms**. Compare with **Pause filter**. Use **Quit Scroll Wheel Fix** to remove the hook and exit.

The app allows one instance per desktop session. Mode, counters, and pause state are not saved. No startup task or registry entry is created.

## Scope and limits

The `WH_MOUSE_LL` hook is session-wide and cannot select a mouse model. It immediately passes movement and click messages; it filters standard `WM_MOUSEWHEEL` input only. It does not install a keyboard hook. Shift, Control, Alt, and Windows modifier state is checked to bypass modified scrolling.

Injected events and deltas that are not multiples of `WHEEL_DELTA` (120) pass through. This protects many fractional/precision inputs but does not reliably identify touchpads. Horizontal scrolling is unchanged. The hook can affect every standard wheel mouse in the session, regardless of brand.

Confirmed reverse bursts are replayed with `SendInput` and a marker that prevents recursion. A replay failure pauses filtering and displays a notice while allowing the current original tick through. Replayed ticks use the current pointer/window context; no pointer movement is injected.

Windows integrity isolation (UIPI) can block replay to elevated applications. Some games and raw-input applications may bypass ordinary message behavior. Use the normal user session; do not disable security features to force compatibility. A stalled process can cause Windows to remove a low-level hook; quit and restart if the filter stops responding. There is no guarantee of operation in protected desktops, games, or remote sessions.

## Build from source

Install CMake and Visual Studio with the **Desktop development with C++** workload. In a developer PowerShell, from the checkout:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/Release/ScrollWheelFix.exe`. Tests exercise the direction gate and buffer without installing an input hook. CI uses the same build/test flow on `windows-latest`.

## Stop and remove

Quit from the tray menu and delete the extracted folder. If the tray is unavailable, end **ScrollWheelFix.exe** in Task Manager; Windows removes its process-owned hook. No system scroll setting is changed.

Primary references: [LowLevelMouseProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelmouseproc), [MSLLHOOKSTRUCT](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-msllhookstruct), [SendInput and UIPI](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
