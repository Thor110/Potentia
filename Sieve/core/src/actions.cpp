// Sieve — actions to animations (see actions.hpp).
#include "sieve/actions.hpp"

#include "sieve/sha256.hpp"

#include <stdexcept>
#include <string>

namespace sieve {

namespace {

bool is_hex64(const std::string& s) { return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == std::string::npos; }

} // namespace

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

std::string ActionMap::id() const { return Sha256::hex(Sha256::hash(text())); }

ActionMap read_actions(const std::string& text)
{
    std::vector<std::string> lines;
    for (size_t i = 0; i < text.size();)
    {
        const size_t e = text.find('\n', i);
        if (e == std::string::npos) throw std::invalid_argument("its last line has no line feed");
        lines.push_back(text.substr(i, e - i));
        i = e + 1;
    }
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
        if (!is_hex64(v)) throw std::invalid_argument(std::string(kActionNames[i]) + "'s animation is not a SHA-256 or none");
        m.animation[i] = v;
    }
    return m;
}

bool animation_fits(const Character& c, const Animation& a) { return a.bones >= 1 && a.bones <= c.spheres.size(); }

} // namespace sieve
