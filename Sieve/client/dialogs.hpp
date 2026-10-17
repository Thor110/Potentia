// Sieve — the system's file and folder dialogs, with the window brought back to the front when
// one closes. On Windows the window a dialog belonged to can stay behind other windows after it
// closes; each of these brings it forward from the dialog's own callback (window_front.cpp, which
// Windows needs), and raises it again on the main thread once that callback has run. They take
// the same arguments as SDL's.
#pragma once

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

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
};

inline void SDLCALL raise_window(void* window)
{
    SDL_RaiseWindow(static_cast<SDL_Window*>(window));
}

inline void SDLCALL dialog_closed(void* call, const char* const* files, int filter)
{
    const DialogCall c = *static_cast<DialogCall*>(call);
    delete static_cast<DialogCall*>(call);
    c.callback(c.userdata, files, filter);
    if (c.window)
    {
        bring_to_front(c.window);
        SDL_RunOnMainThread(raise_window, c.window, false);
    }
}

} // namespace detail

inline void open_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location, bool allow_many)
{
    SDL_ShowOpenFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window}, window, filters, nfilters,
                           default_location, allow_many);
}

inline void save_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location)
{
    SDL_ShowSaveFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window}, window, filters, nfilters,
                           default_location);
}

inline void open_folder_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const char* default_location,
                               bool allow_many)
{
    SDL_ShowOpenFolderDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window}, window, default_location,
                             allow_many);
}

} // namespace hallway
