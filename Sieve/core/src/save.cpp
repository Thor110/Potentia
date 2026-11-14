// Sieve — a character's save (see save.hpp).
#include "sieve/save.hpp"

#include "sieve/hex.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve {

void CharacterSave::speak(const std::string& id, bool yes)
{
    const auto at = std::lower_bound(speaks.begin(), speaks.end(), id);
    const bool there = at != speaks.end() && *at == id;
    if (yes && !there) speaks.insert(at, id);
    if (!yes && there) speaks.erase(at);
}

std::string CharacterSave::text() const
{
    std::string t = std::string(actions.empty() ? kSaveVersion : kSaveVersion2) + "\ncharacter " + character + "\n";
    if (!animation.empty()) t += "animation " + animation + "\n";
    if (!actions.empty()) t += "actions " + actions + "\n";
    for (const auto& s : speaks) t += "speaks " + s + "\n";
    return t;
}

std::string CharacterSave::id() const { return Sha256::hex_hash(text()); }

CharacterSave read_save(const std::string& text)
{
    std::vector<std::string> lines;
    if (!lf_lines(text, lines)) throw std::invalid_argument("its last line has no line feed");
    const bool v2 = !lines.empty() && lines[0] == kSaveVersion2;
    if (lines.size() < 2 || (!v2 && lines[0] != kSaveVersion))
        throw std::invalid_argument(std::string("not a save: it does not start with ") + kSaveVersion + " or " + kSaveVersion2);
    CharacterSave s;
    if (lines[1].rfind("character ", 0) != 0 || !is_sha256_hex(lines[1].substr(10))) throw std::invalid_argument("its second line is \"character\" and a SHA-256");
    s.character = lines[1].substr(10);
    size_t i = 2;
    if (i < lines.size() && lines[i].rfind("animation ", 0) == 0)
    {
        if (!is_sha256_hex(lines[i].substr(10))) throw std::invalid_argument("its animation is not a SHA-256");
        s.animation = lines[i++].substr(10);
    }
    if (v2)
    {
        // (a v2 save has its map; one without is written as v1, so has no other way to be said)
        if (i >= lines.size() || lines[i].rfind("actions ", 0) != 0 || !is_sha256_hex(lines[i].substr(8)))
            throw std::invalid_argument("a sieve-save-v2 names its actions' map (\"actions\" and a SHA-256) after the animation");
        s.actions = lines[i++].substr(8);
    }
    for (; i < lines.size(); ++i)
    {
        if (lines[i].rfind("speaks ", 0) != 0 || !is_plain_id(lines[i].substr(7))) throw std::invalid_argument("line " + std::to_string(i + 1) + " is out of place: " + lines[i]);
        s.speaks.push_back(lines[i].substr(7));
    }
    for (size_t k = 1; k < s.speaks.size(); ++k)
        if (!(s.speaks[k - 1] < s.speaks[k])) throw std::invalid_argument("its languages are not in the order of their ids, each once");
    if (s.text() != text) throw std::invalid_argument("not in its canonical form");
    return s;
}

} // namespace sieve
