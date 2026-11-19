# Linux setup (evdev + uinput)

This preview is a foreground CLI, not a tray app. It reads one explicitly selected relative mouse, temporarily grabs that device, and forwards its events through a virtual mouse. The backend works below X11/Wayland, so it does not depend on compositor-specific scroll interception. Actual hardware/compositor operation still needs manual validation.

The x64 release is built on Ubuntu 24.04, dynamically linked to libevdev and the system C++/glibc runtime. Build from source on other distributions or architectures if the binary cannot run there.

## Permissions and device selection

The process needs read access to the selected `/dev/input/event*` node and write access to `/dev/uinput`. Running the foreground command with `sudo` is a simple explicit option; both device access and root privileges are powerful. The project installs no permission rules, setuid binary, daemon, or service. Do not make all input devices world-readable/writable.

It accepts mouse-only relative event nodes with X/Y movement, a left button, and a vertical wheel. It rejects absolute devices, nodes advertising keyboard keys, and its own virtual devices. For a keyboard/mouse receiver, select its separate `event-mouse` node. A gaming mouse that exposes keyboard keys on the same node is deliberately refused.

```bash
sudo apt install libevdev2
./scroll-wheel-fix --help
sudo ./scroll-wheel-fix --list
```

`--list` inspects device capabilities/names, not their event streams. Choose the correct mouse by path and name. Prefer a stable `/dev/input/by-id/...-event-mouse` or by-path link if available; event numbers can change after reconnecting.

If `/dev/uinput` is missing, check that your kernel provides uinput. On systems with the module available, `sudo modprobe uinput` loads it for the current system. The app does not load modules itself.

## Filter a selected mouse

Replace `eventN` below with the actual device shown by `--list`:

```bash
sudo ./scroll-wheel-fix --device /dev/input/eventN --mode balanced
# Or use a stable symlink, and the stronger threshold:
sudo ./scroll-wheel-fix --device /dev/input/by-id/YOUR_MOUSE-event-mouse --mode strong
```

Keep the terminal open. Release mouse buttons before starting. **Ctrl+C** stops filtering, releases the grab, and removes the virtual device. Stop/restart with a different mode to compare behavior; there is no interactive pause menu.

Only the selected mouse is proxied. Movement and mouse buttons are forwarded without entering the reverse-wheel buffer. Legacy `REL_WHEEL` and `REL_WHEEL_HI_RES` values share one decision per `SYN_REPORT`, preserving both representations without treating them as two ticks. Horizontal/diagonal wheel frames bypass filtering. Kernel synchronization loss resets the filter and forwards resynchronized button state.

The keyboard is never opened for modifier monitoring. As a result, Ctrl/Shift/Alt + wheel gestures are still filtered on Linux. High-resolution devices are supported at the frame level but are not broadly hardware-tested. The application ignores the original wheel's vendor/model and does not preserve device-specific vendor control channels in its virtual mouse.

## Stopping and recovery

- Press **Ctrl+C**, or send **SIGTERM** from another terminal. The code releases the selected physical device during cleanup.
- Closing the process also closes the evdev/uinput descriptors; the kernel drops the exclusive grab and removes the virtual mouse.
- A device disconnect or read/write failure stops the program. It does not automatically reconnect or keep filtering another mouse.
- If you cannot use the pointer, the keyboard remains outside the grab: use it to reach the terminal or another TTY and terminate the foreground process. Stop before unplugging devices or suspending while testing this preview.

There is no system-wide input permission change to undo. No raw event values, typed keys, or device serials are logged; the CLI prints status and aggregate counters.

## Build from source

Ubuntu/Debian build dependencies:

```bash
sudo apt install build-essential cmake pkg-config libevdev-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/scroll-wheel-fix --help
```

Fedora's corresponding development package is `libevdev-devel`; other distributions need an equivalent C++17 toolchain, CMake, pkg-config, and libevdev headers/library. Tests do not open input devices or require root.

Primary references: [libevdev uinput devices](https://www.freedesktop.org/software/libevdev/doc/latest/group__uinput.html), [SYN_DROPPED handling](https://www.freedesktop.org/software/libevdev/doc/latest/syn_dropped.html), [Linux wheel event codes](https://docs.kernel.org/input/event-codes.html).
