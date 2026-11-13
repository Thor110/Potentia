// Sieve hallway — every key in one registry (see keys.hpp).
#include "keys.hpp"

#include <algorithm>

namespace hallway {

namespace {

SDL_Keymod plain(SDL_Keymod m)
{
    SDL_Keymod out = SDL_KMOD_NONE;
    if (m & SDL_KMOD_SHIFT) out = SDL_Keymod(out | SDL_KMOD_SHIFT);
    if (m & SDL_KMOD_CTRL) out = SDL_Keymod(out | SDL_KMOD_CTRL);
    if (m & SDL_KMOD_ALT) out = SDL_Keymod(out | SDL_KMOD_ALT);
    return out;
}

std::string trim(std::string s)
{
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

} // namespace

KeyChord parse_chord(const std::string& text)
{
    KeyChord c;
    std::string t = trim(text);
    for (;;)
    {
        if (t.rfind("Shift+", 0) == 0) c.mod = SDL_Keymod(c.mod | SDL_KMOD_SHIFT), t = t.substr(6);
        else if (t.rfind("Ctrl+", 0) == 0) c.mod = SDL_Keymod(c.mod | SDL_KMOD_CTRL), t = t.substr(5);
        else if (t.rfind("Alt+", 0) == 0) c.mod = SDL_Keymod(c.mod | SDL_KMOD_ALT), t = t.substr(4);
        else break;
    }
    // (the names a comma list cannot hold are written as words)
    if (t == "Comma") t = ",";
    else if (t == "Plus") t = "+";
    c.key = t.empty() ? SDLK_UNKNOWN : SDL_GetKeyFromName(t.c_str());
    return c;
}

std::vector<KeyChord> parse_chords(const std::string& text)
{
    std::vector<KeyChord> out;
    size_t start = 0;
    while (start <= text.size())
    {
        const size_t end = std::min(text.find(',', start), text.size());
        const KeyChord c = parse_chord(text.substr(start, end - start));
        if (c.key != SDLK_UNKNOWN) out.push_back(c);
        start = end + 1;
    }
    return out;
}

std::string chord_name(const KeyChord& c)
{
    std::string n;
    if (c.mod & SDL_KMOD_CTRL) n += "Ctrl+";
    if (c.mod & SDL_KMOD_ALT) n += "Alt+";
    if (c.mod & SDL_KMOD_SHIFT) n += "Shift+";
    const char* k = SDL_GetKeyName(c.key);
    std::string kn = k && *k ? k : "?";
    if (kn == ",") kn = "Comma";
    return n + kn;
}

Keys& Keys::get()
{
    static Keys keys;
    return keys;
}

const std::string& Keys::declare(const KeyAction& a)
{
    const std::string full = a.section + "/" + a.id;
    if (const auto it = index_.find(full); it != index_.end()) return it->first;
    Entry e{a, a.defaults, parse_chords(a.defaults)};
    if (const auto p = pending_.find(full); p != pending_.end())
    {
        if (auto chords = parse_chords(p->second); !chords.empty()) e.keys = p->second, e.chords = std::move(chords);
        pending_.erase(p);
    }
    entries_.push_back(std::move(e));
    return index_.emplace(full, entries_.size() - 1).first->first;
}

const Keys::Entry* Keys::find(const std::string& full_id) const
{
    const auto it = index_.find(full_id);
    return it == index_.end() ? nullptr : &entries_[it->second];
}

bool Keys::is(const std::string& full_id, SDL_Keycode key, SDL_Keymod mod) const
{
    const Entry* e = find(full_id);
    if (!e) return false;
    const SDL_Keymod m = plain(mod);
    for (const KeyChord& c : e->chords)
        if (c.key == key && c.mod == m) return true;
    return false;
}

bool Keys::is(const std::string& full_id, const SDL_Event& e) const
{
    return e.type == SDL_EVENT_KEY_DOWN && is(full_id, e.key.key, e.key.mod);
}

bool Keys::held(const std::string& full_id, const bool* keyboard) const
{
    const Entry* e = find(full_id);
    if (!e || !keyboard) return false;
    for (const KeyChord& c : e->chords)
    {
        const SDL_Scancode sc = SDL_GetScancodeFromKey(c.key, nullptr);
        if (sc != SDL_SCANCODE_UNKNOWN && keyboard[sc]) return true;
    }
    return false;
}

std::string Keys::names(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    if (!e) return "?";
    std::string n;
    for (const KeyChord& c : e->chords) n += (n.empty() ? "" : " / ") + chord_name(c);
    return n.empty() ? "-" : n;
}

std::string Keys::first(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    return e && !e->chords.empty() ? chord_name(e->chords[0]) : "-";
}

std::vector<Keys::Row> Keys::listing() const
{
    std::vector<Row> out;
    for (const Entry& e : entries_) out.push_back({e.action.section + "/" + e.action.id, e.action.section, e.action.name_key, names(e.action.section + "/" + e.action.id), e.keys != e.action.defaults});
    return out;
}

void Keys::set(const std::string& full_id, const std::string& keys)
{
    const auto it = index_.find(full_id);
    if (it == index_.end())
    {
        pending_[full_id] = keys; // (its system declares it later)
        return;
    }
    Entry& e = entries_[it->second];
    auto chords = parse_chords(keys);
    if (keys.empty() || chords.empty()) e.keys = e.action.defaults, e.chords = parse_chords(e.action.defaults);
    else e.keys = keys, e.chords = std::move(chords);
}

std::map<std::string, std::string> Keys::overrides() const
{
    std::map<std::string, std::string> out = pending_;
    for (const Entry& e : entries_)
        if (e.keys != e.action.defaults) out[e.action.section + "/" + e.action.id] = e.keys;
    return out;
}

std::vector<std::string> Keys::clashes() const
{
    std::vector<std::string> out;
    for (size_t i = 0; i < entries_.size(); ++i)
        for (size_t j = i + 1; j < entries_.size(); ++j)
        {
            if (entries_[i].action.section != entries_[j].action.section) continue;
            for (const KeyChord& a : entries_[i].chords)
                for (const KeyChord& b : entries_[j].chords)
                    if (a.key == b.key && a.mod == b.mod)
                        out.push_back(entries_[i].action.section + ": " + chord_name(a) + " is " + entries_[i].action.id + " and " + entries_[j].action.id);
        }
    return out;
}

} // namespace hallway
