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
#include <shobjidl.h>
#include <string>
#include <thread>
#if defined(_MSC_VER)
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#endif
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

bool native_folder_dialog(SDL_DialogFileCallback done, void* call, SDL_Window* window, const char* default_location)
{
#ifdef _WIN32
    const HWND owner = window ? static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr))
                              : nullptr;
    std::wstring start;
    if (default_location && *default_location)
    {
        const int n = MultiByteToWideChar(CP_UTF8, 0, default_location, -1, nullptr, 0);
        if (n > 1)
        {
            start.resize(size_t(n));
            MultiByteToWideChar(CP_UTF8, 0, default_location, -1, start.data(), n);
            start.resize(size_t(n - 1));
        }
    }
    // Tried on this thread first, so that a machine where it cannot be made falls back to SDL's
    // at once; then shown on a thread of its own, as SDL shows its dialogs.
    {
        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
            return false;
        IFileOpenDialog* probe = nullptr;
        const bool can = SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&probe)));
        if (probe) probe->Release();
        CoUninitialize();
        if (!can) return false;
    }
    std::thread([done, call, owner, start] {
        std::string chosen;
        bool failed = true;
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        {
            IFileOpenDialog* d = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&d))))
            {
                DWORD opts = 0;
                d->GetOptions(&opts);
                d->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
                if (!start.empty())
                {
                    IShellItem* at = nullptr;
                    if (SUCCEEDED(SHCreateItemFromParsingName(start.c_str(), nullptr, IID_PPV_ARGS(&at))))
                    {
                        d->SetFolder(at);
                        at->Release();
                    }
                }
                const HRESULT shown = d->Show(owner);
                failed = false; // (cancelled, or chosen: either is an answer)
                IShellItem* item = nullptr;
                if (SUCCEEDED(shown) && SUCCEEDED(d->GetResult(&item)))
                {
                    PWSTR path = nullptr;
                    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path)
                    {
                        const int n = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                        if (n > 1)
                        {
                            chosen.resize(size_t(n));
                            WideCharToMultiByte(CP_UTF8, 0, path, -1, chosen.data(), n, nullptr, nullptr);
                            chosen.resize(size_t(n - 1));
                        }
                        CoTaskMemFree(path);
                    }
                    item->Release();
                }
                else if (shown != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                    failed = true;
                d->Release();
            }
            CoUninitialize();
        }
        // As SDL answers: the folder then null; null first when cancelled; no list on a failure.
        const char* list[2] = {chosen.empty() ? nullptr : chosen.c_str(), nullptr};
        done(call, failed ? nullptr : list, -1);
    }).detach();
    return true;
#else
    (void)done;
    (void)call;
    (void)window;
    (void)default_location;
    return false;
#endif
}

} // namespace hallway::detail
