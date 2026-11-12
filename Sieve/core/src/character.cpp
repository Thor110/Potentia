// Sieve — characters: a seed unfolding a tree of spheres (sieve/character.hpp; IDEAS §19.148).
#include "sieve/character.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string>

namespace sieve {

namespace {

struct Dir
{
    int x, y, z;
};

// The 26 directions in their order: x, then y, then z, each -1, 0, 1, skipping (0, 0, 0).
std::array<Dir, 26> all_directions()
{
    std::array<Dir, 26> d{};
    size_t n = 0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
                if (x || y || z) d[n++] = {x, y, z};
    return d;
}

// The k-th draw of a seed (character.hpp).
class Draws
{
public:
    explicit Draws(uint64_t seed) : seed_(seed) {}
    uint32_t next()
    {
        Sha256 h;
        h.update(kCharacterVersion);
        uint8_t s[8];
        for (int i = 0; i < 8; ++i) s[i] = uint8_t(seed_ >> (8 * i));
        h.update(std::span<const uint8_t>(s, 8));
        h.update_u32le(k_++);
        const auto d = h.finish();
        return uint32_t(d[0]) | uint32_t(d[1]) << 8 | uint32_t(d[2]) << 16 | uint32_t(d[3]) << 24;
    }

private:
    uint64_t seed_;
    uint32_t k_ = 0;
};

// A step of `len` along a direction: each nonzero component len * unit / 1024, the unit 1024,
// 724 or 591 by how many components are nonzero (1024 / sqrt of that, rounded).
void step(const Dir& d, int64_t len, int32_t& x, int32_t& y, int32_t& z)
{
    const int nz = (d.x != 0) + (d.y != 0) + (d.z != 0);
    const int64_t unit = nz == 1 ? 1024 : nz == 2 ? 724 : 591;
    x += int32_t(d.x * len * unit / 1024);
    y += int32_t(d.y * len * unit / 1024);
    z += int32_t(d.z * len * unit / 1024);
}

// A direction drawn from the 25 that are not straight up.
Dir not_up(uint32_t d)
{
    static const std::array<Dir, 26> all = all_directions();
    std::array<Dir, 25> v{};
    size_t n = 0;
    for (const Dir& a : all)
        if (!(a.x == 0 && a.y == 1 && a.z == 0)) v[n++] = a;
    return v[d % 25];
}

// One limb (or branch) of `joints` joints from sphere `from`: its first joint `out_radii` of that
// sphere's radii from its centre along `dir`, of radius `r0` (at least 8). Appended to `s`; returns
// the index of its last joint. `centre`: the limb stays on the plane x = 0 (its x never bends).
int limb(std::vector<Sphere>& s, Draws& draws, int from, Dir dir, int32_t r0, uint32_t joints, int out_radii, bool centre)
{
    int32_t x = s[size_t(from)].x, y = s[size_t(from)].y, z = s[size_t(from)].z;
    step(dir, int64_t(s[size_t(from)].r) * out_radii, x, y, z);
    int parent = from;
    int32_t r = std::max<int32_t>(8, r0);
    for (uint32_t j = 0; j < joints; ++j)
    {
        if (j > 0)
        {
            const uint32_t d = draws.next();
            if (d % 4 == 0)
            {
                int* c[3] = {&dir.x, &dir.y, &dir.z};
                int& comp = *c[(d / 4) % 3];
                const int before = comp;
                // On the centre plane (a bilateral character's odd limb) x never bends, so the
                // limb stays its own mirror.
                if (!(centre && &comp == &dir.x)) comp = std::clamp(comp + ((d / 12) % 2 ? 1 : -1), -1, 1);
                if (dir.x == 0 && dir.y == 0 && dir.z == 0) comp = before;
            }
            step(dir, int64_t(r) * 2, x, y, z);
            r = std::max<int32_t>(8, int32_t(int64_t(r) * (75 + (d / 24) % 18) / 100));
        }
        Sphere sp;
        sp.parent = parent;
        sp.x = x;
        sp.y = y;
        sp.z = z;
        sp.r = r;
        sp.kind = Sphere::Joint;
        s.push_back(sp);
        parent = int(s.size()) - 1;
    }
    return parent;
}

// Spheres [first, last) copied with x negated, their parents moved by `shift` (the copies' own
// spheres), or kept where the parent is before `first` (the body, a limb's joint).
void mirror_copy(std::vector<Sphere>& s, size_t first, size_t last)
{
    const int32_t shift = int32_t(last - first);
    for (size_t i = first; i < last; ++i)
    {
        Sphere m = s[i];
        m.x = -m.x;
        if (m.parent >= int32_t(first)) m.parent += shift;
        s.push_back(m);
    }
}

} // namespace

void CharacterSettings::check() const
{
    if (limbs > 16) throw std::invalid_argument("limbs: " + std::to_string(limbs) + " is more than 16");
    if (joints < 1 || joints > 12) throw std::invalid_argument("joints: " + std::to_string(joints) + " is not 1 to 12");
    if (branches > 3) throw std::invalid_argument("branches: " + std::to_string(branches) + " is more than 3");
    if (symmetry != Symmetry::None && symmetry != Symmetry::Bilateral && symmetry != Symmetry::Radial)
        throw std::invalid_argument("symmetry: not none, bilateral or radial");
}

const char* symmetry_name(Symmetry s)
{
    switch (s)
    {
    case Symmetry::None: return "none";
    case Symmetry::Bilateral: return "bilateral";
    case Symmetry::Radial: return "radial";
    }
    return "?";
}

Symmetry symmetry_from_name(const std::string& name)
{
    if (name == "none") return Symmetry::None;
    if (name == "bilateral") return Symmetry::Bilateral;
    if (name == "radial") return Symmetry::Radial;
    throw std::invalid_argument("symmetry: '" + name + "' is not none, bilateral or radial");
}

Character unfold_character(const CharacterSettings& st)
{
    st.check();
    Character c;
    c.settings = st;
    auto& s = c.spheres;
    Draws draws(st.seed);
    Sphere body;
    body.r = int32_t(384 + draws.next() % 256);
    body.kind = Sphere::Body;
    s.push_back(body);
    if (st.head)
    {
        Sphere head;
        head.parent = 0;
        head.r = int32_t(int64_t(body.r) * (50 + draws.next() % 26) / 100);
        head.y = body.r + head.r;
        head.kind = Sphere::Head;
        s.push_back(head);
    }
    static const std::array<Dir, 8> kRing = {{{1, 0, 0}, {1, 0, 1}, {0, 0, 1}, {-1, 0, 1}, {-1, 0, 0}, {-1, 0, -1}, {0, 0, -1}, {1, 0, -1}}};
    const uint32_t branch_joints = (st.joints + 1) / 2;
    // One limb and its branches; returns nothing, appends to s.
    auto grow = [&](Dir dir, bool centre) {
        const int32_t r0 = int32_t(int64_t(body.r) * (35 + draws.next() % 21) / 100);
        const int end = limb(s, draws, 0, dir, r0, st.joints, 1, centre);
        for (uint32_t b = 0; b < st.branches; ++b)
        {
            Dir bd = not_up(draws.next());
            if (centre)
            {
                bd.x = 0; // on the centre plane, its branches too
                if (bd.y == 0 && bd.z == 0) bd.y = -1;
            }
            limb(s, draws, end, bd, int32_t(int64_t(s[size_t(end)].r) * 3 / 4), branch_joints, 1, centre);
        }
    };
    uint32_t i = 0;
    while (i < st.limbs)
    {
        if (st.symmetry == Symmetry::Radial)
        {
            grow(kRing[i * 8 / st.limbs], false);
            ++i;
        }
        else if (st.symmetry == Symmetry::Bilateral && i + 1 < st.limbs)
        {
            // The first of a pair: one of the 8 directions with x = +1 other than (1, 1, 0), which
            // would point the pair up and out like a second head; the second is its mirror.
            const uint32_t d = draws.next();
            const int yz = int(d % 8);
            static const std::array<std::array<int, 2>, 8> kYZ = {{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 0}, {0, 1}, {1, -1}, {1, 1}}};
            const size_t first = s.size();
            grow(Dir{1, kYZ[size_t(yz)][0], kYZ[size_t(yz)][1]}, false);
            mirror_copy(s, first, s.size());
            i += 2;
        }
        else if (st.symmetry == Symmetry::Bilateral)
        {
            grow(draws.next() % 2 ? Dir{0, 0, 1} : Dir{0, -1, 0}, true);
            ++i;
        }
        else
        {
            grow(not_up(draws.next()), false);
            ++i;
        }
    }
    return c;
}

Character read_character(const std::string& text)
{
    const size_t nl = text.find('\n');
    std::string first = text.substr(0, nl);
    if (!first.empty() && first.back() == '\r') first.pop_back();
    std::vector<std::string> words;
    for (size_t i = 0; i <= first.size();)
    {
        const size_t sp = std::min(first.find(' ', i), first.size());
        if (sp > i) words.push_back(first.substr(i, sp - i));
        i = sp + 1;
    }
    if (words.empty() || words[0] != kCharacterVersion)
        throw std::invalid_argument(std::string("not a character: it does not start with ") + kCharacterVersion);
    CharacterSettings s;
    bool seen[6] = {};
    for (size_t i = 1; i < words.size(); ++i)
    {
        const size_t eq = words[i].find('=');
        if (eq == std::string::npos) throw std::invalid_argument("not a setting: " + words[i]);
        const std::string k = words[i].substr(0, eq), v = words[i].substr(eq + 1);
        auto number = [&](uint64_t hi) {
            if (v.empty() || v.size() > 20 || v.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument(k + ": not a number: " + v);
            const unsigned long long n = std::stoull(v);
            if (n > hi) throw std::invalid_argument(k + ": " + v + " is out of range");
            return uint64_t(n);
        };
        if (k == "seed") s.seed = number(UINT64_MAX), seen[0] = true;
        else if (k == "limbs") s.limbs = uint32_t(number(1000)), seen[1] = true;
        else if (k == "joints") s.joints = uint32_t(number(1000)), seen[2] = true;
        else if (k == "branches") s.branches = uint32_t(number(1000)), seen[3] = true;
        else if (k == "symmetry") s.symmetry = symmetry_from_name(v), seen[4] = true;
        else if (k == "head")
        {
            if (v != "yes" && v != "no") throw std::invalid_argument("head: not yes or no: " + v);
            s.head = v == "yes";
            seen[5] = true;
        }
        else throw std::invalid_argument("not a setting of " + std::string(kCharacterVersion) + ": " + k);
    }
    static const char* const kNames[] = {"seed", "limbs", "joints", "branches", "symmetry", "head"};
    for (int i = 0; i < 6; ++i)
        if (!seen[i]) throw std::invalid_argument(std::string("missing: ") + kNames[i]);
    Character c = unfold_character(s);
    // The spheres, where given, must be the unfolding's, line for line (line endings aside).
    std::string rest = nl == std::string::npos ? std::string() : text.substr(nl + 1);
    std::string clean;
    for (const char ch : rest)
        if (ch != '\r') clean += ch;
    while (!clean.empty() && clean.back() == '\n') clean.pop_back();
    if (!clean.empty())
    {
        std::string want = c.text().substr(c.text().find('\n') + 1);
        while (!want.empty() && want.back() == '\n') want.pop_back();
        if (clean != want) throw std::invalid_argument("the spheres do not match their settings (a changed file, or another version)");
    }
    return c;
}

std::string Character::text() const
{
    std::string t = std::string(kCharacterVersion) + " seed=" + std::to_string(settings.seed) + " limbs=" + std::to_string(settings.limbs) +
                    " joints=" + std::to_string(settings.joints) + " branches=" + std::to_string(settings.branches) +
                    " symmetry=" + symmetry_name(settings.symmetry) + " head=" + (settings.head ? "yes" : "no") + "\n";
    static const char* const kKinds[] = {"body", "head", "joint"};
    for (size_t i = 0; i < spheres.size(); ++i)
    {
        const Sphere& p = spheres[i];
        t += "s " + std::to_string(i) + " " + std::to_string(p.parent) + " " + std::to_string(p.x) + " " + std::to_string(p.y) + " " +
             std::to_string(p.z) + " " + std::to_string(p.r) + " " + kKinds[p.kind] + "\n";
    }
    return t;
}

std::string Character::id() const { return Sha256::hex(Sha256::hash(text())); }

void Character::extent(int32_t& x, int32_t& y, int32_t& z) const
{
    x = y = z = 0;
    for (const Sphere& p : spheres)
    {
        x = std::max(x, std::abs(p.x) + p.r);
        y = std::max(y, std::abs(p.y) + p.r);
        z = std::max(z, std::abs(p.z) + p.r);
    }
}

} // namespace sieve
