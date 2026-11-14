// Sieve — actions to animations (see actions.hpp).
#include "sieve/actions.hpp"

#include "sieve/hex.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace sieve {

const char* action_name(Action a) { return kActionNames[size_t(a)]; }

Action action_from_name(std::string_view name)
{
    for (size_t i = 0; i < kActionCount; ++i)
        if (name == kActionNames[i]) return Action(i);
    throw std::invalid_argument("not an action of " + std::string(kActionsVersion) + ": " + std::string(name));
}

bool ActionMap::empty() const
{
    for (const auto& a : animation)
        if (!a.empty()) return false;
    return true;
}

std::string ActionMap::text() const
{
    std::string t = std::string(kActionsVersion) + "\n";
    for (size_t i = 0; i < kActionCount; ++i) t += std::string("action ") + kActionNames[i] + " " + (animation[i].empty() ? "none" : animation[i]) + "\n";
    return t;
}

std::string ActionMap::id() const { return Sha256::hex_hash(text()); }

ActionMap read_actions(const std::string& text)
{
    std::vector<std::string> lines;
    if (!lf_lines(text, lines)) throw std::invalid_argument("its last line has no line feed");
    if (lines.empty() || lines[0] != kActionsVersion) throw std::invalid_argument(std::string("not an action map: it does not start with ") + kActionsVersion);
    if (lines.size() != kActionCount + 1)
        throw std::invalid_argument("it has " + std::to_string(lines.size() - 1) + " actions, not the " + std::to_string(kActionCount) + " of " + kActionsVersion);
    ActionMap m;
    for (size_t i = 0; i < kActionCount; ++i)
    {
        const std::string want = std::string("action ") + kActionNames[i] + " ";
        const std::string& l = lines[i + 1];
        if (l.rfind(want, 0) != 0) throw std::invalid_argument("line " + std::to_string(i + 2) + " is not action " + kActionNames[i] + " (each once, in order)");
        const std::string v = l.substr(want.size());
        if (v == "none") continue;
        if (!is_sha256_hex(v)) throw std::invalid_argument(std::string(kActionNames[i]) + "'s animation is not a SHA-256 or none");
        m.animation[i] = v;
    }
    return m;
}

bool animation_fits(const Character& c, const Animation& a) { return a.bones >= 1 && a.bones <= c.spheres.size(); }

} // namespace sieve
