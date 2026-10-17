// Sieve — bringing a window back to the front after a system dialog closes (dialogs.hpp).
//
// SDL_RaiseWindow is enough on most systems, but not on Windows: there a program may take the
// foreground only while it is the one the user is working in, and otherwise Windows flashes its
// taskbar button instead (the "foreground lock"). By the time SDL raises the window on the main
// thread, the file dialog has closed and the foreground has already passed elsewhere. So on
// Windows this runs from the dialog's own callback, while the dialog's thread still holds the
// input the user just gave it, and it joins that thread's input to the window's for the moment it
// takes, which is the documented way for one thread to hand the foreground to another's window.
#include "dialogs.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <SDL3/SDL_properties.h>

namespace hallway::detail {

void bring_to_front(SDL_Window* window)
{
#ifdef _WIN32
    if (!window) return;
    const HWND hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (!hwnd) return;
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    const DWORD here = GetCurrentThreadId();
    const DWORD owner = GetWindowThreadProcessId(hwnd, nullptr);
    const HWND front = GetForegroundWindow();
    const DWORD front_thread = front ? GetWindowThreadProcessId(front, nullptr) : 0;
    // Joined to the thread that has the foreground now, and to the window's own, so that the
    // window may be given it; separated again at once.
    const bool joined_front = front_thread && front_thread != here && AttachThreadInput(here, front_thread, TRUE);
    const bool joined_owner = owner != here && owner != front_thread && AttachThreadInput(here, owner, TRUE);
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    if (joined_owner) AttachThreadInput(here, owner, FALSE);
    if (joined_front) AttachThreadInput(here, front_thread, FALSE);
#else
    (void)window; // SDL_RaiseWindow, on the main thread, does it
#endif
}

} // namespace hallway::detail
