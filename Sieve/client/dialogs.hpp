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

#include <mutex>
#include <optional>
#include <set>

namespace hallway {

namespace detail {

// On Windows, gives the window the foreground from the thread the dialog answered on, while that
// still counts as the user's doing (window_front.cpp); elsewhere it does nothing.
void bring_to_front(SDL_Window* window);

// On Windows, the system's own folder picker, Explorer's (IFileOpenDialog choosing folders, with
// its address bar, favourites and search), in place of SDL 3.2's, which is the old folder tree
// that has to be opened level by level (Edward, 29 October 2026: "the directory tree option isn't
// fun to have to navigate to deep directories with"). It answers on a thread of its own, as SDL's
// dialogs do, through `done` (files: the folder and then null; cancelled: null first). False where
// it cannot be had (elsewhere, or COM refused): SDL's is used then.
bool native_folder_dialog(SDL_DialogFileCallback done, void* call, SDL_Window* window, const char* default_location);

struct DialogCall
{
    SDL_DialogFileCallback callback;
    void* userdata;
    SDL_Window* window;
    bool then_another;
    const void* owner = nullptr; // (see DialogOwner: null, an answer always given)
};

// The owners of dialogs still open, registered while they live (DialogOwner). Where the system's
// dialog does not hold the window (Linux's portal), it can answer after the screen that opened it
// has closed: an answer for an owner that has gone is dropped, and an answer is given under this
// lock, so an owner cannot be destroyed while its answer is being given.
inline std::mutex& owners_mx()
{
    static std::mutex m;
    return m;
}
inline std::set<const void*>& owners()
{
    static std::set<const void*> s;
    return s;
}

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
    if (!c.owner) c.callback(c.userdata, files, filter);
    else
    {
        std::lock_guard<std::mutex> lock(owners_mx());
        if (!owners().count(c.owner)) return; // (its screen has gone: nothing to answer, nor a window to raise)
        c.callback(c.userdata, files, filter);
    }
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

// A screen that opens dialogs holds one of these, made with itself, as its last member (so it is
// the first let go): while it lives, its dialogs' answers reach it; after, they are dropped. Each
// dialog it opens names it as `owner`. Its callbacks are given under a lock: they only take what was
// chosen (under the screen's own lock at most), and open nothing.
class DialogOwner
{
public:
    explicit DialogOwner(const void* owner) : owner_(owner)
    {
        std::lock_guard<std::mutex> lock(detail::owners_mx());
        detail::owners().insert(owner_);
    }
    ~DialogOwner()
    {
        std::lock_guard<std::mutex> lock(detail::owners_mx());
        detail::owners().erase(owner_);
    }
    DialogOwner(const DialogOwner&) = delete;
    DialogOwner& operator=(const DialogOwner&) = delete;

private:
    const void* owner_;
};

inline void open_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location, bool allow_many, bool then_another = false, const void* owner = nullptr)
{
    detail::before_dialog(window);
    SDL_ShowOpenFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window, then_another, owner}, window, filters, nfilters,
                           default_location, allow_many);
}

inline void save_file_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter* filters,
                             int nfilters, const char* default_location, const void* owner = nullptr)
{
    detail::before_dialog(window);
    SDL_ShowSaveFileDialog(detail::dialog_closed, new detail::DialogCall{callback, userdata, window, false, owner}, window, filters, nfilters,
                           default_location);
}

inline void open_folder_dialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const char* default_location,
                               bool allow_many, const void* owner = nullptr)
{
    detail::before_dialog(window);
    auto* call = new detail::DialogCall{callback, userdata, window, false, owner};
    if (!allow_many && detail::native_folder_dialog(detail::dialog_closed, call, window, default_location)) return;
    SDL_ShowOpenFolderDialog(detail::dialog_closed, call, window, default_location, allow_many);
}

} // namespace hallway
