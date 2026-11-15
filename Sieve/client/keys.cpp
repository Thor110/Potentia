// Sieve hallway — every key in one registry (see keys.hpp; the rest of it is the core's key_registry).
#include "keys.hpp"

namespace hallway {

// (a chord holds SDL's own modifier bits, so an event's are given to the core's registry as they come)
static_assert(sieve::kKeyShift == SDL_KMOD_SHIFT && sieve::kKeyCtrl == SDL_KMOD_CTRL && sieve::kKeyAlt == SDL_KMOD_ALT);

Keys& Keys::get()
{
    // SDL's key names, before the first action is declared (the systems declare theirs as the program
    // starts, through here). SDL names keys without a window, from its own table.
    static Keys keys = [] {
        sieve::set_key_naming({[](const std::string& n) -> uint32_t { return SDL_GetKeyFromName(n.c_str()); },
                               [](uint32_t code) -> std::string {
                                   const char* k = SDL_GetKeyName(SDL_Keycode(code));
                                   return k ? k : "";
                               }});
        return Keys{};
    }();
    return keys;
}

bool Keys::is(const std::string& full_id, const SDL_Event& e) const
{
    return e.type == SDL_EVENT_KEY_DOWN && is(full_id, e.key.key, e.key.mod);
}

bool Keys::within(const std::string& full_id, const SDL_Event& e) const
{
    return e.type == SDL_EVENT_KEY_DOWN && within(full_id, e.key.key, e.key.mod);
}

std::string Keys::which(const std::vector<std::string>& full_ids, const SDL_Event& e) const
{
    return e.type == SDL_EVENT_KEY_DOWN ? which(full_ids, e.key.key, e.key.mod) : std::string();
}

bool Keys::changed_in(const std::string& section) const
{
    for (const Row& row : listing())
        if (row.changed && (row.section == section || row.section.rfind(section + ".", 0) == 0)) return true;
    return false;
}

bool Keys::held(const std::string& full_id, const bool* keyboard) const
{
    if (!keyboard) return false;
    for (const KeyChord& c : chords(full_id))
    {
        const SDL_Scancode sc = SDL_GetScancodeFromKey(SDL_Keycode(c.key), nullptr);
        if (sc != SDL_SCANCODE_UNKNOWN && keyboard[sc]) return true;
    }
    return false;
}

} // namespace hallway
