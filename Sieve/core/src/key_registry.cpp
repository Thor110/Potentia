// Sieve — every key in one registry (see key_registry.hpp).
#include "sieve/key_registry.hpp"

#include "sieve/text.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace sieve {

namespace {

// Without a naming given (the unit tests, a program without a keyboard): a single printable character
// names the key that types it, a letter by its capital as SDL names it.
uint32_t plain_code(const std::string& n)
{
    if (n.size() != 1 || n[0] <= ' ' || n[0] > '~') return 0;
    return uint32_t(std::tolower(static_cast<unsigned char>(n[0])));
}

std::string plain_name(uint32_t code)
{
    if (code <= ' ' || code > '~') return {};
    return std::string(1, char(std::toupper(int(code))));
}

KeyNaming& naming()
{
    static KeyNaming n{plain_code, plain_name};
    return n;
}

} // namespace

void set_key_naming(KeyNaming n)
{
    if (!n.code) n.code = plain_code;
    if (!n.name) n.name = plain_name;
    naming() = std::move(n);
}

uint16_t plain_mods(uint16_t m)
{
    uint16_t out = 0;
    if (m & kKeyShift) out |= kKeyShift;
    if (m & kKeyCtrl) out |= kKeyCtrl;
    if (m & kKeyAlt) out |= kKeyAlt;
    return out;
}

KeyChord parse_chord(const std::string& text)
{
    KeyChord c;
    std::string t = trim(text, " "); // (spaces only, as a chord has always been read)
    for (;;)
    {
        if (t.rfind("Shift+", 0) == 0) c.mod |= kKeyShift, t = t.substr(6);
        else if (t.rfind("Ctrl+", 0) == 0) c.mod |= kKeyCtrl, t = t.substr(5);
        else if (t.rfind("Alt+", 0) == 0) c.mod |= kKeyAlt, t = t.substr(4);
        else break;
    }
    // (the names a comma list cannot hold are written as words)
    if (t == "Comma") t = ",";
    else if (t == "Plus") t = "+";
    c.key = t.empty() ? 0 : naming().code(t);
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
        if (c.key != 0) out.push_back(c);
        start = end + 1;
    }
    return out;
}

std::string chord_name(const KeyChord& c)
{
    std::string n;
    if (c.mod & kKeyCtrl) n += "Ctrl+";
    if (c.mod & kKeyAlt) n += "Alt+";
    if (c.mod & kKeyShift) n += "Shift+";
    std::string kn = naming().name(c.key);
    if (kn.empty()) kn = "?";
    if (kn == ",") kn = "Comma";
    return n + kn;
}

const std::string& KeyRegistry::declare(const KeyAction& a)
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

const KeyRegistry::Entry* KeyRegistry::find(const std::string& full_id) const
{
    const auto it = index_.find(full_id);
    return it == index_.end() ? nullptr : &entries_[it->second];
}

bool KeyRegistry::is(const std::string& full_id, uint32_t key, uint16_t mod) const
{
    const Entry* e = find(full_id);
    if (!e) return false;
    const uint16_t m = plain_mods(mod);
    for (const KeyChord& c : e->chords)
        if (c.key == key && c.mod == m) return true;
    return false;
}

bool KeyRegistry::within(const std::string& full_id, uint32_t key, uint16_t mod) const
{
    const Entry* e = find(full_id);
    if (!e) return false;
    const uint16_t m = plain_mods(mod);
    for (const KeyChord& c : e->chords)
        if (c.key == key && (c.mod & m) == c.mod) return true;
    return false;
}

std::string KeyRegistry::which(const std::vector<std::string>& full_ids, uint32_t key, uint16_t mod) const
{
    for (const std::string& id : full_ids)
        if (is(id, key, mod)) return id;
    const uint16_t m = plain_mods(mod);
    auto count = [](uint16_t bits) { return int(bool(bits & kKeyShift)) + int(bool(bits & kKeyCtrl)) + int(bool(bits & kKeyAlt)); };
    std::string best;
    int most = -1;
    for (const std::string& id : full_ids)
        if (const Entry* e = find(id))
            for (const KeyChord& c : e->chords)
                if (c.key == key && (c.mod & m) == c.mod && count(c.mod) > most) best = id, most = count(c.mod);
    return best;
}

std::string KeyRegistry::names(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    if (!e) return "?";
    std::string n;
    for (const KeyChord& c : e->chords) n += (n.empty() ? "" : " / ") + chord_name(c);
    return n.empty() ? "-" : n;
}

std::string KeyRegistry::first(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    return e && !e->chords.empty() ? chord_name(e->chords[0]) : "-";
}

bool KeyRegistry::changed(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    return e && e->keys != e->action.defaults;
}

std::vector<KeyChord> KeyRegistry::chords(const std::string& full_id) const
{
    const Entry* e = find(full_id);
    return e ? e->chords : std::vector<KeyChord>{};
}

std::string KeyRegistry::keyed(const std::string& text) const
{
    std::string out;
    size_t at = 0;
    for (;;)
    {
        const size_t open = text.find("{key", at);
        if (open == std::string::npos) break;
        const bool one = text.compare(open, 6, "{key1:") == 0;
        const size_t id_at = open + (one ? 6 : 5);
        const size_t bar = text.find('|', id_at), close = text.find('}', id_at);
        // (not one of these: "{key" followed by anything else, or one left open, stays as it is)
        if ((!one && text.compare(open, 5, "{key:") != 0) || bar == std::string::npos || close == std::string::npos || bar > close)
        {
            out += text.substr(at, open + 4 - at);
            at = open + 4;
            continue;
        }
        const std::string id = text.substr(id_at, bar - id_at), shown = text.substr(bar + 1, close - bar - 1);
        out += text.substr(at, open - at);
        out += !changed(id) ? shown : one ? first(id) : names(id);
        at = close + 1;
    }
    return out + text.substr(at);
}

std::vector<KeyRegistry::Row> KeyRegistry::listing() const
{
    std::vector<Row> out;
    for (const Entry& e : entries_) out.push_back({e.action.section + "/" + e.action.id, e.action.section, e.action.name_key, names(e.action.section + "/" + e.action.id), e.keys != e.action.defaults});
    return out;
}

void KeyRegistry::set(const std::string& full_id, const std::string& keys)
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

std::map<std::string, std::string> KeyRegistry::overrides() const
{
    std::map<std::string, std::string> out = pending_;
    for (const Entry& e : entries_)
        if (e.keys != e.action.defaults) out[e.action.section + "/" + e.action.id] = e.keys;
    return out;
}

void KeyRegistry::together(const std::string& section_a, const std::string& section_b)
{
    together_.emplace_back(section_a, section_b);
}

std::vector<KeyRegistry::Clash> KeyRegistry::clashes() const
{
    auto asked_together = [&](const std::string& a, const std::string& b) {
        if (a == b) return true;
        for (const auto& [x, y] : together_)
            if ((x == a && y == b) || (x == b && y == a)) return true;
        return false;
    };
    std::vector<Clash> out;
    for (size_t i = 0; i < entries_.size(); ++i)
        for (size_t j = i + 1; j < entries_.size(); ++j)
        {
            if (!asked_together(entries_[i].action.section, entries_[j].action.section)) continue;
            for (const KeyChord& a : entries_[i].chords)
                for (const KeyChord& b : entries_[j].chords)
                    if (a.key == b.key && a.mod == b.mod)
                        out.push_back({chord_name(a), entries_[i].action.section + "/" + entries_[i].action.id, entries_[j].action.section + "/" + entries_[j].action.id});
        }
    return out;
}

} // namespace sieve
