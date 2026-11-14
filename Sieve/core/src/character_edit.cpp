// Sieve — editing a character by its joints (sieve/character_edit.hpp; IDEAS 19.203).
#include "sieve/character_edit.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

namespace {

// A symmetry's move: x negated where `m`, then turned `k` eighths of a turn about the upright (from +x
// towards +z). Bilateral moves are (0, 0) and (1, 0); radial ones (0, k).
struct Tf
{
    uint8_t m = 0, k = 0;
};

// a after b. Mirroring reverses a turn about the upright, so a turn carried past a mirror changes sign.
Tf compose(Tf a, Tf b) { return Tf{uint8_t(a.m ^ b.m), uint8_t((a.k + (a.m ? 8 - b.k : b.k)) % 8)}; }
Tf inverse(Tf a) { return Tf{a.m, uint8_t(a.m ? a.k : (8 - a.k) % 8)}; }

std::array<double, 3> apply(Tf t, std::array<double, 3> v)
{
    if (t.m) v[0] = -v[0];
    if (t.k == 0) return v;
    // (the eight turns' sines and cosines written out, so a turn of a quarter is exact)
    static const double kHalf = 0.70710678118654752440;
    static const double kCos[8] = {1, kHalf, 0, -kHalf, -1, -kHalf, 0, kHalf};
    static const double kSin[8] = {0, kHalf, 1, kHalf, 0, -kHalf, -1, -kHalf};
    const double c = kCos[t.k], s = kSin[t.k];
    return {v[0] * c - v[2] * s, v[1], v[0] * s + v[2] * c};
}

double length(const std::array<double, 3>& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

// The character while an edit is made: its spheres (each marked alive or taken away), each one's set of
// images (the index of the set's first) and its move from that first, and its held models.
struct Work
{
    std::vector<Sphere> sp;
    std::vector<uint8_t> alive;
    std::vector<int32_t> group;
    std::vector<Tf> tf;
    std::vector<std::pair<uint32_t, std::string>> holds;
    Symmetry sym = Symmetry::None;
    CharacterSettings settings;

    explicit Work(const Character& c) : sp(c.spheres), alive(c.spheres.size(), 1), holds(c.holds), sym(c.given_symmetry), settings(c.settings)
    {
        group.resize(sp.size());
        tf.resize(sp.size());
        for (size_t i = 0; i < sp.size(); ++i)
        {
            const bool image = i < c.images.size() && c.images[i].of >= 0;
            group[i] = image ? c.images[i].of : int32_t(i);
            tf[i] = image ? Tf{c.images[i].mirror, c.images[i].turn} : Tf{};
        }
    }
    void check(uint32_t s, bool body_too) const
    {
        if (s >= sp.size() || !alive[s]) throw std::invalid_argument("no sphere " + std::to_string(s));
        if (!body_too && s == 0) throw std::invalid_argument("the body stays where it is: choose another sphere");
    }
    std::array<double, 3> at(uint32_t i) const { return {double(sp[i].x), double(sp[i].y), double(sp[i].z)}; }
    // The chosen sphere and its images, the chosen first.
    std::vector<uint32_t> members(uint32_t s) const
    {
        std::vector<uint32_t> out{s};
        for (uint32_t i = 0; i < sp.size(); ++i)
            if (i != s && alive[i] && group[i] == group[s]) out.push_back(i);
        return out;
    }
    // What a change at s is at t: carried back to the set's first, then out to t.
    std::array<double, 3> carried(uint32_t s, uint32_t t, const std::array<double, 3>& v) const { return apply(compose(tf[t], inverse(tf[s])), v); }
    std::vector<uint32_t> children(uint32_t t) const
    {
        std::vector<uint32_t> out;
        for (uint32_t i = 0; i < sp.size(); ++i)
            if (alive[i] && sp[i].parent == int32_t(t)) out.push_back(i);
        return out;
    }
    // A sphere and everything beyond it (its parents may come after it while an edit is made, so it is
    // walked by the tree, not by the order).
    std::vector<uint32_t> subtree(uint32_t t) const
    {
        std::vector<uint32_t> out{t};
        for (size_t k = 0; k < out.size(); ++k)
            for (const uint32_t c : children(out[k])) out.push_back(c);
        return out;
    }
    void shift(uint32_t t, int64_t dx, int64_t dy, int64_t dz)
    {
        for (const uint32_t i : subtree(t))
        {
            sp[i].x = int32_t(std::clamp<int64_t>(sp[i].x + dx, INT32_MIN, INT32_MAX));
            sp[i].y = int32_t(std::clamp<int64_t>(sp[i].y + dy, INT32_MIN, INT32_MAX));
            sp[i].z = int32_t(std::clamp<int64_t>(sp[i].z + dz, INT32_MIN, INT32_MAX));
        }
    }
    uint32_t add(const Sphere& s, int32_t g, Tf t)
    {
        sp.push_back(s);
        alive.push_back(1);
        group.push_back(g < 0 ? int32_t(sp.size() - 1) : g);
        tf.push_back(t);
        return uint32_t(sp.size() - 1);
    }

    // Back to a character: the spheres in their order (each parent before its children, children in the
    // order they were made), each set's first its earliest sphere, the held models with their spheres.
    // `choose` is given back as numbered now.
    Character finish(uint32_t& choose) const
    {
        std::vector<int32_t> renum(sp.size(), -1);
        std::vector<uint32_t> order;
        std::vector<uint32_t> stack{0};
        while (!stack.empty())
        {
            const uint32_t i = stack.back();
            stack.pop_back();
            renum[i] = int32_t(order.size());
            order.push_back(i);
            const std::vector<uint32_t> ch = children(i);
            for (auto it = ch.rbegin(); it != ch.rend(); ++it) stack.push_back(*it);
        }
        std::vector<Sphere> out;
        for (const uint32_t i : order)
        {
            Sphere s = sp[i];
            s.parent = s.parent < 0 ? -1 : renum[size_t(s.parent)];
            out.push_back(s);
        }
        std::vector<SphereImage> images;
        if (sym != Symmetry::None)
        {
            images.resize(out.size());
            std::map<int32_t, uint32_t> first; // each set's earliest sphere, as numbered now
            for (const uint32_t i : order)
                if (!first.count(group[i])) first[group[i]] = i;
            for (const uint32_t i : order)
            {
                const uint32_t f = first[group[i]];
                if (f == i) continue;
                const Tf rel = compose(inverse(tf[f]), tf[i]);
                images[size_t(renum[i])] = SphereImage{renum[f], rel.m, rel.k};
            }
        }
        Character c = given_character(std::move(out), sym, std::move(images));
        c.settings = settings;
        for (const auto& [i, hex] : holds)
            if (i < renum.size() && renum[i] >= 0) c.hold(uint32_t(renum[i]), hex);
        choose = choose < renum.size() && renum[choose] >= 0 ? uint32_t(renum[choose]) : 0;
        return c;
    }
};

// The images of a seed-made character, found from its unfolding (character_edit.hpp).
std::vector<SphereImage> found_images(const Character& c)
{
    const auto& sp = c.spheres;
    std::vector<SphereImage> im(sp.size());
    if (c.settings.symmetry == Symmetry::Bilateral)
    {
        std::vector<int32_t> partner(sp.size(), -1);
        for (size_t i = 1; i < sp.size(); ++i)
        {
            if (sp[i].x == 0) continue; // (on the centre plane: its own mirror)
            const int32_t pi = sp[i].parent;
            const int32_t want = pi >= 0 && partner[size_t(pi)] >= 0 ? partner[size_t(pi)] : pi;
            for (size_t j = 1; j < i; ++j)
            {
                const Sphere &a = sp[j], &b = sp[i];
                if (partner[j] >= 0 || im[j].of >= 0 || a.x != -b.x || a.y != b.y || a.z != b.z || a.r != b.r || a.kind != b.kind || a.parent != want) continue;
                partner[j] = int32_t(i), partner[i] = int32_t(j);
                im[i] = SphereImage{int32_t(j), 1, 0};
                break;
            }
        }
    }
    else if (c.settings.symmetry == Symmetry::Radial)
    {
        // Each sphere's limb and its place in it (which child at each step out from the limb's first joint).
        std::vector<uint32_t> roots;
        for (size_t i = 1; i < sp.size(); ++i)
            if (sp[i].parent == 0 && sp[i].kind == Sphere::Joint) roots.push_back(uint32_t(i));
        if (roots.empty()) return im;
        std::vector<int32_t> limb(sp.size(), -1);
        std::vector<std::vector<uint32_t>> path(sp.size());
        std::vector<uint32_t> made(sp.size(), 0); // children made so far, each sphere's
        std::map<std::vector<uint32_t>, uint32_t> first_limb;
        for (size_t i = 1; i < sp.size(); ++i)
        {
            const int32_t p = sp[i].parent;
            const uint32_t ordinal = made[size_t(p)]++;
            if (p == 0)
            {
                const auto it = std::find(roots.begin(), roots.end(), uint32_t(i));
                if (it == roots.end()) continue;
                limb[i] = int32_t(it - roots.begin());
            }
            else if (limb[size_t(p)] >= 0)
            {
                limb[i] = limb[size_t(p)];
                path[i] = path[size_t(p)];
                path[i].push_back(ordinal);
            }
            else continue;
            if (limb[i] == 0) first_limb[path[i]] = uint32_t(i);
            else if (const auto f = first_limb.find(path[i]); f != first_limb.end())
                im[i] = SphereImage{int32_t(f->second), 0, uint8_t(uint32_t(limb[i]) * 8 / uint32_t(roots.size()))};
        }
    }
    return im;
}

// The bone of t kept at its length: the point taken onto the sphere round t's parent at that length.
std::array<double, 3> keep_length(const Work& w, uint32_t t, std::array<double, 3> to)
{
    const int32_t p = w.sp[t].parent;
    if (p < 0) return to;
    const std::array<double, 3> pp = w.at(uint32_t(p)), old = w.at(t);
    const std::array<double, 3> v{to[0] - pp[0], to[1] - pp[1], to[2] - pp[2]};
    const double len = length(v), bone = length({old[0] - pp[0], old[1] - pp[1], old[2] - pp[2]});
    if (len < 1e-9) return old; // (onto the parent itself: no direction to keep it in)
    return {pp[0] + v[0] * bone / len, pp[1] + v[1] * bone / len, pp[2] + v[2] * bone / len};
}

} // namespace

Character editable_character(const Character& c)
{
    if (c.given) return c;
    Character e = given_character(c.spheres, c.settings.symmetry, c.settings.symmetry == Symmetry::None ? std::vector<SphereImage>() : found_images(c));
    e.settings = c.settings;
    for (const auto& [i, hex] : c.holds) e.hold(i, hex);
    return e;
}

std::vector<uint32_t> sphere_images(const Character& c, uint32_t sphere)
{
    const Work w(editable_character(c));
    w.check(sphere, true);
    return w.members(sphere);
}

uint32_t edit_move(Character& c, uint32_t s, const double to_in[3], bool stretch)
{
    Work w(editable_character(c));
    w.check(s, false);
    const std::vector<uint32_t> mem = w.members(s);
    std::array<double, 3> to{to_in[0], to_in[1], to_in[2]};
    // Alone under bilateral symmetry: on the centre plane, and kept there.
    if (w.sym == Symmetry::Bilateral && mem.size() == 1) to[0] = 0;
    if (!stretch) to = keep_length(w, s, to);
    const std::array<double, 3> from = w.at(s), by{to[0] - from[0], to[1] - from[1], to[2] - from[2]};
    // Each sphere's move worked out before any is made, so each is carried from the set as it stood.
    std::vector<std::array<int64_t, 3>> moves;
    for (const uint32_t t : mem)
    {
        const std::array<double, 3> old = w.at(t), d = w.carried(s, t, by);
        std::array<double, 3> target = t == s ? to : std::array<double, 3>{old[0] + d[0], old[1] + d[1], old[2] + d[2]};
        if (!stretch && t != s) target = keep_length(w, t, target);
        moves.push_back({std::llround(target[0]) - int64_t(old[0]), std::llround(target[1]) - int64_t(old[1]), std::llround(target[2]) - int64_t(old[2])});
    }
    for (size_t k = 0; k < mem.size(); ++k) w.shift(mem[k], moves[k][0], moves[k][1], moves[k][2]);
    c = w.finish(s);
    return s;
}

uint32_t edit_nudge(Character& c, uint32_t s, const double by[3], bool stretch)
{
    const Character e = editable_character(c);
    if (s >= e.spheres.size()) throw std::invalid_argument("no sphere " + std::to_string(s));
    const Sphere& p = e.spheres[s];
    const double to[3] = {double(p.x) + by[0], double(p.y) + by[1], double(p.z) + by[2]};
    return edit_move(c, s, to, stretch);
}

uint32_t edit_grow(Character& c, uint32_t s)
{
    Work w(editable_character(c));
    w.check(s, true);
    const std::vector<uint32_t> mem = w.members(s);
    // The direction: of the 25 not straight up (x, then y, then z, each -1, 0 or 1), the one whose
    // nearest bone at the sphere is farthest from it.
    std::vector<std::array<double, 3>> bones;
    const std::array<double, 3> here = w.at(s);
    auto unit = [](std::array<double, 3> v) {
        const double l = length(v);
        return l < 1e-9 ? std::array<double, 3>{0, 0, 0} : std::array<double, 3>{v[0] / l, v[1] / l, v[2] / l};
    };
    std::vector<uint32_t> near = w.children(s);
    if (w.sp[s].parent >= 0) near.push_back(uint32_t(w.sp[s].parent));
    for (const uint32_t n : near)
    {
        const std::array<double, 3> o = w.at(n);
        bones.push_back(unit({o[0] - here[0], o[1] - here[1], o[2] - here[2]}));
    }
    std::array<double, 3> best{0, -1, 0};
    double best_score = 2;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
            {
                if ((x == 0 && y == 0 && z == 0) || (x == 0 && y == 1 && z == 0)) continue;
                const std::array<double, 3> d = unit({double(x), double(y), double(z)});
                double score = -2;
                for (const auto& b : bones) score = std::max(score, d[0] * b[0] + d[1] * b[1] + d[2] * b[2]);
                if (score < best_score - 1e-9) best_score = score, best = d;
            }
    // Where each new joint goes: from each image of the sphere, the direction carried across; from a
    // sphere alone, one on each side its symmetry gives it.
    struct Grow
    {
        uint32_t from;
        std::array<double, 3> dir;
        Tf tf;
    };
    std::vector<Grow> grows;
    if (mem.size() > 1)
        for (const uint32_t t : mem) grows.push_back({t, w.carried(s, t, best), w.tf[t]});
    else if (w.sym == Symmetry::Bilateral && std::fabs(best[0]) > 1e-9)
    {
        grows.push_back({s, best, Tf{}});
        grows.push_back({s, apply(Tf{1, 0}, best), Tf{1, 0}});
    }
    else if (w.sym == Symmetry::Radial && w.sp[s].x == 0 && w.sp[s].z == 0)
    {
        // On the upright (the body): a limb at each turn the character's limbs take.
        std::vector<uint8_t> turns{0};
        for (size_t i = 0; i < w.sp.size(); ++i)
            if (w.alive[i] && std::find(turns.begin(), turns.end(), w.tf[i].k) == turns.end()) turns.push_back(w.tf[i].k);
        std::sort(turns.begin(), turns.end());
        for (const uint8_t k : turns) grows.push_back({s, apply(Tf{0, k}, best), Tf{0, k}});
    }
    else grows.push_back({s, best, w.tf[s]});
    int32_t g = -1;
    uint32_t chosen = 0;
    for (const Grow& gr : grows)
    {
        const Sphere& from = w.sp[gr.from];
        Sphere n;
        n.parent = int32_t(gr.from);
        n.r = std::max<int32_t>(8, int32_t(int64_t(from.r) * 3 / 4));
        n.kind = Sphere::Joint;
        const double out = double(from.r) + double(n.r);
        n.x = int32_t(std::llround(double(from.x) + gr.dir[0] * out));
        n.y = int32_t(std::llround(double(from.y) + gr.dir[1] * out));
        n.z = int32_t(std::llround(double(from.z) + gr.dir[2] * out));
        const uint32_t i = w.add(n, g, gr.tf);
        if (g < 0) g = int32_t(i), chosen = i;
    }
    c = w.finish(chosen);
    return chosen;
}

uint32_t edit_remove_limb(Character& c, uint32_t s)
{
    Work w(editable_character(c));
    w.check(s, false);
    uint32_t chosen = uint32_t(w.sp[s].parent);
    for (const uint32_t t : w.members(s))
        for (const uint32_t i : w.subtree(t)) w.alive[i] = 0;
    c = w.finish(chosen);
    return chosen;
}

uint32_t edit_add_joint(Character& c, uint32_t s)
{
    Work w(editable_character(c));
    w.check(s, false);
    int32_t g = -1;
    uint32_t chosen = s;
    for (const uint32_t t : w.members(s))
    {
        const Sphere at = w.sp[t], parent = w.sp[size_t(at.parent)];
        const int64_t bx = int64_t(at.x) - parent.x, by = int64_t(at.y) - parent.y, bz = int64_t(at.z) - parent.z;
        const std::vector<uint32_t> beyond = w.children(t);
        Sphere n = at;
        n.parent = int32_t(t);
        n.kind = Sphere::Joint;
        n.x = int32_t(std::clamp<int64_t>(at.x + bx, INT32_MIN, INT32_MAX));
        n.y = int32_t(std::clamp<int64_t>(at.y + by, INT32_MIN, INT32_MAX));
        n.z = int32_t(std::clamp<int64_t>(at.z + bz, INT32_MIN, INT32_MAX));
        const uint32_t i = w.add(n, g, w.tf[t]);
        if (g < 0) g = int32_t(i), chosen = i;
        for (const uint32_t b : beyond)
        {
            w.sp[b].parent = int32_t(i);
            w.shift(b, bx, by, bz);
        }
    }
    c = w.finish(chosen);
    return chosen;
}

uint32_t edit_remove_joint(Character& c, uint32_t s)
{
    Work w(editable_character(c));
    w.check(s, false);
    uint32_t chosen = uint32_t(w.sp[s].parent);
    for (const uint32_t t : w.members(s))
    {
        const Sphere at = w.sp[t], parent = w.sp[size_t(at.parent)];
        for (const uint32_t b : w.children(t))
        {
            w.sp[b].parent = at.parent;
            w.shift(b, int64_t(parent.x) - at.x, int64_t(parent.y) - at.y, int64_t(parent.z) - at.z);
        }
        w.alive[t] = 0;
    }
    c = w.finish(chosen);
    return chosen;
}

} // namespace sieve
