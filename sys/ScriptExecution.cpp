// Praat Pro Edition. GPL-3.0-or-later.
#include "ScriptExecution.h"
#include "melder.h"

#if defined (_WIN32)
#include <windows.h>
#include <vector>

namespace {
constexpr wchar_t stopClass [] = L"PraatScriptStopWindow";
HWND stopWindow = nullptr;
HWND stopButton = nullptr;
std::vector<HWND> disabledWindows;
int depth = 0;
bool stopRequested = false;
ULONGLONG lastPoll = 0;

LRESULT CALLBACK stopWindowProc (HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_COMMAND && LOWORD (wParam) == 1) {
        stopRequested = true;
        if (stopButton) EnableWindow (stopButton, FALSE);
        SetWindowTextW (window, L"Stopping script...");
        return 0;
    }
    if (message == WM_CLOSE || (message == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        stopRequested = true;
        if (stopButton) EnableWindow (stopButton, FALSE);
        SetWindowTextW (window, L"Stopping script...");
        return 0;
    }
    return DefWindowProcW (window, message, wParam, lParam);
}
BOOL CALLBACK disableOtherWindow (HWND window, LPARAM) {
    if (window != stopWindow && IsWindowVisible (window) && IsWindowEnabled (window)) {
        disabledWindows.push_back (window);
        EnableWindow (window, FALSE);
    }
    return TRUE;
}
void showStopWindow () {
    static bool registered = false;
    if (! registered) {
        WNDCLASSW klass {};
        klass.lpfnWndProc = stopWindowProc;
        klass.hInstance = GetModuleHandleW (nullptr);
        klass.hCursor = LoadCursorW (nullptr, IDC_ARROW);
        klass.hbrBackground = reinterpret_cast<HBRUSH> (COLOR_BTNFACE + 1);
        klass.lpszClassName = stopClass;
        registered = RegisterClassW (& klass) != 0;
    }
    if (! registered) return;
    RECT work {};
    SystemParametersInfoW (SPI_GETWORKAREA, 0, & work, 0);
    const int x = work.left + (work.right - work.left - 260) / 2;
    const int y = work.top + 50;
    stopWindow = CreateWindowExW (WS_EX_TOPMOST | WS_EX_TOOLWINDOW, stopClass,
        L"Praat script is running", WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, 260, 100, nullptr, nullptr, GetModuleHandleW (nullptr), nullptr);
    if (! stopWindow) return;
    stopButton = CreateWindowExW (0, L"BUTTON", L"Stop script", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        20, 17, 210, 34, stopWindow, reinterpret_cast<HMENU> (1), GetModuleHandleW (nullptr), nullptr);
    ShowWindow (stopWindow, SW_SHOWNOACTIVATE);
    UpdateWindow (stopWindow);
    EnumThreadWindows (GetCurrentThreadId(), disableOtherWindow, 0);
}
void hideStopWindow () {
    if (stopWindow) DestroyWindow (stopWindow);
    stopWindow = stopButton = nullptr;
    for (HWND window : disabledWindows) if (IsWindow (window)) EnableWindow (window, TRUE);
    disabledWindows.clear ();
}
}
#endif

ScriptExecutionScope::ScriptExecutionScope (bool enable) {
#if defined (_WIN32)
    if (enable && ! Melder_batch) {
        enabled_ = true;
        if (depth ++ == 0) {
            stopRequested = false;
            lastPoll = 0;
            showStopWindow ();
        }
    }
#else
    (void) enable;
#endif
}
ScriptExecutionScope::~ScriptExecutionScope () {
#if defined (_WIN32)
    if (enabled_ && -- depth == 0) hideStopWindow ();
#endif
}
bool ScriptExecutionScope::poll () {
#if defined (_WIN32)
    if (! enabled_) return true;
    if (stopRequested) return false;
    const ULONGLONG now = GetTickCount64 ();
    if (now - lastPoll < 15) return true;
    lastPoll = now;
    MSG message;
    int count = 0;
    while (count ++ < 100 && PeekMessageW (& message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            stopRequested = true;
            PostQuitMessage (static_cast<int> (message.wParam));
            break;
        }
        const bool ours = stopWindow && (message.hwnd == stopWindow || IsChild (stopWindow, message.hwnd));
        if (ours) {
            TranslateMessage (& message);
            DispatchMessageW (& message);
        } else if (message.message == WM_PAINT || message.message == WM_NCPAINT || message.message == WM_ERASEBKGND) {
            DispatchMessageW (& message);
        }
    }
    return ! stopRequested;
#else
    return true;
#endif
}
