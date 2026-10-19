// Sieve — the system's file and folder dialogs, with the window brought back to the front when
// one closes. On Windows the window a dialog belonged to can stay behind other windows after it
// closes; each of these brings it forward from the dialog's own callback (window_front.cpp, which
// Windows needs), and raises it again on the main thread once that callback has run. They take
// the same arguments as SDL's, and one more: `then_another`, for a dialog that leads straight into
// another (install's "which instructions", then "where"), which leaves the window where it is, so
// that raising it cannot put it over the second dialog.
//
// A window in exclusive fullscreen is set to the desktop's fullscreen while a dialog is open (an
// exclusive window can sit over the dialog, which then looks like the hallway locking up), and
// given its mode back once the last dialog closes. (The window not minimising when a dialog takes
// the focus is a hint set where it is made: app_main.cpp.)
#pragma once

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <optional>

namespace hallway {

namespace detail {

// On Windows, gives the window the foreground from the thread the dialog answered on, while that
// still counts as the user's doing (window_front.cpp); elsewhere it does nothing.
void bring_to_front(SDL_Window* window);

struct DialogCall
{
    SDL_DialogFileCallback callback;
    void* userdata;
    SDL_Window* window;
    bool then_another;
};

// The exclusive fullscreen mode a window had before a dialog opened (one window has dialogs).
inline std::optional<SDL_DisplayMode>& exclusive_mode()
{
    static std::optional<SDL_DisplayMode> m;
    return m;
}

// On the main thread, before a dialog opens: an exclusive fullscreen window goes to the desktop's.
inline void before_dialog(SDL_Window* window)
{
    if (!window || exclusive_mode() || !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)) return;
    if (const SDL_DisplayMode* m = SDL_GetWindowFullscreenMode(window))
    {
        exclusive_mode() = *m;
        SDL_SetWindowFullscreenMode(window, nullptr);
        SDL_SyncWindow(window);
    }
}

inline void SDLCALL raise_window(void* window)
{
    SDL_Window* w = static_cast<SDL_Window*>(window);
    if (exclusive_mode())
    {
        SDL_SetWindowFullscreenMode(w, &*exclusive_mode());
        exclusive_mode().reset();
    }
    SDL_RaiseWindow(w);
}

inline void SDLCALL dialog_closed(void* call, const char* const* files, int filter)
{
    const DialogCall c = *static_cast<DialogCall*>(call);
    delete static_cast<DialogCall*>(call);
    c.callback(c.userdata, files, filter);
    // A dialog that leads into another leaves the window as it is, unless nothing was chosen (no
    // second dialog then follows).
    const bool chosen = files && files[0];
    if (c.window && (!c.then_another || !chosen))
    {
        bring_to_front(c.window);
        SDL_RunOnMainThread(raise_window, c.window, false);
    }
}

} // namespace detail

inline void open_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location, bool allow_many, bool then_another = false)
{
    detail::before_dialog(window);
    SDL_ShowOpenFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window, then_another}, window, filters, nfilters,
                           default_location, allow_many);
}

inline void save_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location)
{
    detail::before_dialog(window);
    SDL_ShowSaveFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window, false}, window, filters, nfilters,
                           default_location);
}

inline void open_folder_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const char* default_location,
                               bool allow_many)
{
    detail::before_dialog(window);
    SDL_ShowOpenFolderDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window, false}, window, default_location,
                             allow_many);
}

} // namespace hallway
