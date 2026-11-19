#include <windows.h>
#include <shellapi.h>
#include "filter.hpp"
#include <chrono>
#include <cwchar>
#include <string>
#include <vector>

namespace {
constexpr UINT trayMessage = WM_APP + 1;
constexpr UINT injectionError = WM_APP + 2;
constexpr ULONG_PTR replayMarker = 0x53574658;
constexpr UINT toggleId = 100, balancedId = 101, strongId = 102, helpId = 103, quitId = 104;
HHOOK hook = nullptr;
HWND window = nullptr;
NOTIFYICONDATAW icon{};
bool enabled = true;
swf::BurstFilter<int> filter;

double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
bool modified() {
    for (int key : {VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN})
        if (GetAsyncKeyState(key) & 0x8000) return true;
    return false;
}
void updateIcon() {
    const wchar_t* text = enabled ? L"Scroll Wheel Fix - active" : L"Scroll Wheel Fix - paused";
    wcsncpy_s(icon.szTip, text, _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &icon);
}

LRESULT CALLBACK mouseHook(int code, WPARAM message, LPARAM data) {
    if (code >= 0 && message == WM_MOUSEHWHEEL) filter.reset();
    if (code < 0 || message != WM_MOUSEWHEEL)
        return CallNextHookEx(hook, code, message, data);
    const auto* event = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
    // Our replay must not feed the filter again. Other injected input bypasses it.
    if (event->dwExtraInfo == replayMarker)
        return CallNextHookEx(hook, code, message, data);
    if (!enabled || (event->flags & LLMHF_INJECTED) || modified()) {
        filter.reset();
        return CallNextHookEx(hook, code, message, data);
    }
    const int delta = static_cast<short>(HIWORD(event->mouseData));
    // Preserve precision/fractional scrolling; this does not identify a device.
    if (delta == 0 || delta % WHEEL_DELTA != 0) {
        filter.reset();
        return CallNextHookEx(hook, code, message, data);
    }
    auto output = filter.process(delta > 0 ? 1 : -1, now(), delta);
    if (output.empty()) return 1;
    if (output.size() == 1) return CallNextHookEx(hook, code, message, data);

    // Suppress the current original event and replay the whole confirmed burst
    // in order, tagging it so the hook passes it through exactly once.
    std::vector<INPUT> inputs(output.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        inputs[i].type = INPUT_MOUSE;
        inputs[i].mi.dwFlags = MOUSEEVENTF_WHEEL;
        inputs[i].mi.mouseData = static_cast<DWORD>(output[i]);
        inputs[i].mi.dwExtraInfo = replayMarker;
    }
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent == inputs.size()) return 1;
    // UIPI or another injection failure: pause, preserve the current original
    // tick, and notify outside the hook callback rather than silently blocking.
    enabled = false;
    filter.reset();
    PostMessageW(window, injectionError, 0, 0);
    return CallNextHookEx(hook, code, message, data);
}

void showHelp() {
    MessageBoxW(window,
        L"Filters short wrong-direction bursts from standard mouse wheels.\n\n"
        L"Right-click the tray icon to pause, select Balanced (80 ms) or Strong (140 ms), or quit.\n\n"
        L"A deliberate reverse tick may be suppressed or delayed until another tick confirms it. "
        L"This masks noisy input; it does not repair hardware.\n\n"
        L"All mice in this desktop session are affected. Fractional wheel input, horizontal scroll, "
        L"and modified scrolling pass through. No keyboard hook is installed.\n\n"
        L"Elevated apps and raw-input games may not work with the filter. Run normally; "
        L"do not disable Windows security protections. No startup entry is installed.",
        L"Scroll Wheel Fix 0.2.0", MB_OK | MB_ICONINFORMATION);
}
void showMenu() {
    HMENU menu = CreatePopupMenu();
    const auto counters = L"Wheel: " + std::to_wstring(filter.seen) +
        L" | suppressed: " + std::to_wstring(filter.suppressed);
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, counters.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, toggleId, enabled ? L"Pause filter" : L"Enable filter");
    AppendMenuW(menu, MF_STRING | (filter.window() == .080 ? MF_CHECKED : 0), balancedId, L"Balanced - 80 ms");
    AppendMenuW(menu, MF_STRING | (filter.window() == .140 ? MF_CHECKED : 0), strongId, L"Strong - 140 ms");
    AppendMenuW(menu, MF_STRING, helpId, L"Help and limitations");
    AppendMenuW(menu, MF_STRING, quitId, L"Quit Scroll Wheel Fix");
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, window, nullptr);
    PostMessageW(window, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    static const UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    if (message == taskbarCreated) { Shell_NotifyIconW(NIM_ADD, &icon); return 0; }
    if (message == trayMessage) {
        if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU) showMenu();
        if (lparam == WM_LBUTTONDBLCLK) showHelp();
        return 0;
    }
    if (message == injectionError) {
        updateIcon();
        MessageBoxW(hwnd, L"Windows did not accept a replayed scroll burst. The filter has paused. "
            L"Original input now passes through. This can happen with elevated applications (UIPI).",
            L"Scroll Wheel Fix paused", MB_OK | MB_ICONWARNING);
        return 0;
    }
    if (message == WM_COMMAND) {
        switch (LOWORD(wparam)) {
        case toggleId: enabled = !enabled; filter.reset(); updateIcon(); break;
        case balancedId: filter.setWindow(.080); break;
        case strongId: filter.setWindow(.140); break;
        case helpId: showHelp(); break;
        case quitId: DestroyWindow(hwnd); break;
        }
        return 0;
    }
    if (message == WM_DESTROY) {
        if (hook) { UnhookWindowsHookEx(hook); hook = nullptr; }
        Shell_NotifyIconW(NIM_DELETE, &icon);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\ScrollWheelFix");
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Scroll Wheel Fix is already running, or the session lock is unavailable. "
            L"Look for its icon in the notification area.", L"Scroll Wheel Fix", MB_OK);
        if (mutex) CloseHandle(mutex);
        return 1;
    }
    WNDCLASSW cls{};
    cls.lpfnWndProc = windowProc;
    cls.hInstance = instance;
    cls.lpszClassName = L"ScrollWheelFixWindow";
    if (!RegisterClassW(&cls)) { CloseHandle(mutex); return 1; }
    window = CreateWindowW(cls.lpszClassName, L"Scroll Wheel Fix", WS_OVERLAPPED,
        0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!window) { CloseHandle(mutex); return 1; }
    icon.cbSize = sizeof(icon);
    icon.hWnd = window;
    icon.uID = 1;
    icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    icon.uCallbackMessage = trayMessage;
    icon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcsncpy_s(icon.szTip, L"Scroll Wheel Fix - active", _TRUNCATE);
    if (!Shell_NotifyIconW(NIM_ADD, &icon)) { DestroyWindow(window); CloseHandle(mutex); return 1; }
    hook = SetWindowsHookExW(WH_MOUSE_LL, mouseHook, instance, 0);
    if (!hook) {
        MessageBoxW(window, L"Could not install the mouse hook. No filtering is active.",
            L"Scroll Wheel Fix", MB_OK | MB_ICONERROR);
        DestroyWindow(window); CloseHandle(mutex); return 1;
    }
    MSG message{};
    int status;
    while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (hook) UnhookWindowsHookEx(hook);
    CloseHandle(mutex);
    return status == -1 ? 1 : 0;
}
