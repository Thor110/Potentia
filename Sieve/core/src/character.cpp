// Sieve — characters: a seed unfolding a tree of spheres (sieve/character.hpp; IDEAS §19.148).
#include "sieve/character.hpp"
#include "sieve/sha256.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/importers.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cstdint>
#include <climits>

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

namespace {
const ModelSpace& held_models()
{
    static const ModelSpace m(kHeldVertices, kHeldFaces, kHeldCoords);
    return m;
}
} // namespace

void Character::hold(uint32_t sphere, const std::string& hex)
{
    if (sphere >= spheres.size()) throw std::invalid_argument("hold: no sphere " + std::to_string(sphere));
    auto it = std::find_if(holds.begin(), holds.end(), [&](const auto& h) { return h.first == sphere; });
    if (hex.empty())
    {
        if (it != holds.end()) holds.erase(it);
        return;
    }
    const ModelSpace& m = held_models();
    if (hex.size() != m.hex_width() || hex.find_first_not_of("0123456789abcdef") != std::string::npos)
        throw std::invalid_argument("hold: not a model's address (" + std::to_string(m.hex_width()) + " hex digits): " + hex);
    (void)m.parse(hex); // range-checked
    if (it != holds.end()) it->second = hex;
    else
    {
        holds.push_back({sphere, hex});
        std::sort(holds.begin(), holds.end());
    }
}

const std::string* Character::held(uint32_t sphere) const
{
    for (const auto& h : holds)
        if (h.first == sphere) return &h.second;
    return nullptr;
}

std::vector<HeldEdge> held_model_edges(const std::string& hex)
{
    const ModelSpace& m = held_models();
    const ModelSpace::Parts parts = m.parts_at(m.parse(hex), AddressMode::Positional);
    const auto v = m.mesh_of(parts);
    std::vector<std::pair<uint32_t, uint32_t>> seen;
    std::vector<HeldEdge> out;
    for (const auto& f : m.faces_of(parts))
        for (auto [a, b] : {std::pair{f.a, f.b}, std::pair{f.b, f.c}, std::pair{f.c, f.a}})
        {
            if (a == b || a >= v.size() || b >= v.size()) continue;
            const auto key = std::minmax(a, b);
            if (std::find(seen.begin(), seen.end(), std::pair{key.first, key.second}) != seen.end()) continue;
            seen.push_back({key.first, key.second});
            out.push_back({{v[a].x, v[a].y, v[a].z}, {v[b].x, v[b].y, v[b].z}});
        }
    return out;
}

std::string held_model_step(const std::string& hex, int64_t n)
{
    const ModelSpace& m = held_models();
    if (hex.empty()) return m.hex_of(BigUint(0));
    BigUint i = m.parse(hex);
    const BigUint& size = m.size();
    const BigUint s = BigUint::mod(BigUint(uint64_t(n < 0 ? -n : n)), size);
    if (n >= 0)
    {
        i += s;
        i = BigUint::mod(i, size);
    }
    else if (i >= s) i -= s;
    else
    {
        BigUint w = size;
        w -= s;
        w += i;
        i = w;
    }
    return m.hex_of(i);
}

std::string held_model_from(uint64_t r)
{
    const ModelSpace& m = held_models();
    // The number's 64 bits spread over the whole line: shifted up to the line's own width, then
    // reduced, so any number lands somewhere along all of it.
    BigUint i(r);
    const size_t bits = m.size().bit_length();
    if (bits > 64) i <<= (bits - 64);
    return m.hex_of(BigUint::mod(i, m.size()));
}

std::string species_text(const CharacterSettings& s)
{
    return "sieve-species-v1 limbs=" + std::to_string(s.limbs) + " joints=" + std::to_string(s.joints) + " branches=" + std::to_string(s.branches) +
           " symmetry=" + symmetry_name(s.symmetry) + " head=" + (s.head ? "yes" : "no");
}

std::string species_id(const CharacterSettings& s) { return Sha256::hex(Sha256::hash(species_text(s))); }

CharacterSettings walk_character(const CharacterSettings& from, uint32_t steps)
{
    CharacterSettings s = from;
    for (uint32_t k = 0; k < steps; ++k)
    {
        const std::string t = unfold_character(s).text();
        const auto d = Sha256::hash("sieve-character-walk-v1" + t.substr(0, t.find('\n')));
        uint64_t seed = 0;
        for (int i = 7; i >= 0; --i) seed = seed << 8 | d[size_t(i)];
        s.seed = seed;
        s.limbs = d[8] % 9;
        s.joints = 1 + d[9] % 6;
        s.branches = d[10] % 3;
        s.symmetry = Symmetry(d[11] % 3);
        s.head = d[12] % 4 != 0;
    }
    return s;
}

namespace {
HeldImport import_mesh_for_hold(const ModelSpace::AnyMesh& any);
}

HeldImport import_obj_for_hold(std::string_view obj_text)
{
    HeldImport out = import_mesh_for_hold(ModelSpace::read_any_obj(obj_text));
    out.format = "obj";
    return out;
}

HeldImport import_model_for_hold(std::string_view bytes, const std::function<std::optional<std::string>(const std::string&)>& load)
{
    const ImportedModel m = read_model(bytes, load);
    if (m.mesh.verts.empty()) (void)import_mesh_for_hold(m.mesh); // (refused, and why)
    return held_import_of(m);
}

HeldImport held_import_of(const ImportedModel& m)
{
    HeldImport out = m.mesh.verts.empty() ? HeldImport{} : import_mesh_for_hold(m.mesh);
    out.format = m.format;
    out.left_out = m.left_out;
    out.joints = m.skeleton.size();
    out.animations = m.sampled.size();
    return out;
}

FormatStat import_format_stat(const HeldImport& h, const std::string& machine)
{
    FormatStat s;
    s.date = format_stats_today();
    s.machine = format_stats_machine(machine);
    s.what = "import";
    const uint64_t vk = std::min<uint64_t>(h.vertices_in, kHeldVertices);
    s.result = h.left_out.empty() && vk == h.vertices_in && h.faces_kept == h.faces_in ? "whole" : "partial";
    s.figures = {{"animations", h.animations}, {"faces-in", h.faces_in}, {"faces-kept", h.faces_kept}, {"joints", h.joints},
                 {"left-out", h.left_out.size()}, {"vertices-in", h.vertices_in}, {"vertices-kept", vk}};
    return s;
}

namespace {
HeldImport import_mesh_for_hold(const ModelSpace::AnyMesh& any)
{
    if (any.verts.empty()) throw std::invalid_argument("no mesh in it to hold (a skeleton alone: Ctrl+K takes that)");
    HeldImport out;
    out.vertices_in = any.verts.size();
    out.faces_in = any.faces.size();
    const auto& v = any.verts;
    auto d2 = [&](const ModelSpace::Vertex& a, float x, float y, float z) {
        return (a.x - x) * (a.x - x) + (a.y - y) * (a.y - y) + (a.z - z) * (a.z - z);
    };
    float lo[3] = {v[0].x, v[0].y, v[0].z}, hi[3] = {v[0].x, v[0].y, v[0].z};
    for (const auto& p : v)
    {
        lo[0] = std::min(lo[0], p.x), lo[1] = std::min(lo[1], p.y), lo[2] = std::min(lo[2], p.z);
        hi[0] = std::max(hi[0], p.x), hi[1] = std::max(hi[1], p.y), hi[2] = std::max(hi[2], p.z);
    }
    const float cx = (lo[0] + hi[0]) / 2, cy = (lo[1] + hi[1]) / 2, cz = (lo[2] + hi[2]) / 2;
    // Farthest-point sampling: the first the farthest from the centre, each next the farthest from
    // those kept (the first such in file order on a tie).
    std::vector<uint32_t> kept;
    std::vector<float> nearest(v.size(), 0);
    {
        uint32_t best = 0;
        for (uint32_t i = 0; i < v.size(); ++i)
            if (d2(v[i], cx, cy, cz) > d2(v[best], cx, cy, cz)) best = i;
        kept.push_back(best);
        for (uint32_t i = 0; i < v.size(); ++i) nearest[i] = d2(v[i], v[best].x, v[best].y, v[best].z);
    }
    while (kept.size() < kHeldVertices && kept.size() < v.size())
    {
        uint32_t best = 0;
        for (uint32_t i = 0; i < v.size(); ++i)
            if (nearest[i] > nearest[best]) best = i;
        if (nearest[best] <= 0) break; // every vertex sits on one kept already
        kept.push_back(best);
        for (uint32_t i = 0; i < v.size(); ++i) nearest[i] = std::min(nearest[i], d2(v[i], v[best].x, v[best].y, v[best].z));
    }
    auto snap = [&](uint32_t i) {
        uint32_t best = 0;
        for (uint32_t k = 1; k < kept.size(); ++k)
            if (d2(v[i], v[kept[k]].x, v[kept[k]].y, v[kept[k]].z) < d2(v[i], v[kept[best]].x, v[kept[best]].y, v[kept[best]].z)) best = k;
        return best;
    };
    // The largest faces first (by twice their area), each moved onto the kept vertices.
    std::vector<std::pair<float, size_t>> by_area;
    for (size_t f = 0; f < any.faces.size(); ++f)
    {
        const auto& F = any.faces[f];
        if (F.a >= v.size() || F.b >= v.size() || F.c >= v.size()) continue;
        const float ux = v[F.b].x - v[F.a].x, uy = v[F.b].y - v[F.a].y, uz = v[F.b].z - v[F.a].z;
        const float wx = v[F.c].x - v[F.a].x, wy = v[F.c].y - v[F.a].y, wz = v[F.c].z - v[F.a].z;
        const float nx = uy * wz - uz * wy, ny = uz * wx - ux * wz, nz = ux * wy - uy * wx;
        by_area.push_back({-(nx * nx + ny * ny + nz * nz), f});
    }
    std::stable_sort(by_area.begin(), by_area.end());
    std::vector<ModelSpace::Face> faces;
    for (const auto& [area, f] : by_area)
    {
        if (faces.size() >= kHeldFaces) break;
        const auto& F = any.faces[f];
        const ModelSpace::Face g{snap(F.a), snap(F.b), snap(F.c)};
        if (g.a == g.b || g.b == g.c || g.a == g.c) continue; // closed up
        if (std::find_if(faces.begin(), faces.end(), [&](const ModelSpace::Face& h) { return h.a == g.a && h.b == g.b && h.c == g.c; }) != faces.end()) continue;
        faces.push_back(g);
    }
    out.faces_kept = faces.size();
    std::vector<ModelSpace::Vertex> verts;
    for (const uint32_t i : kept) verts.push_back(v[i]);
    const ModelSpace& m = held_models();
    const ModelSpace::Fitted fit = m.fit(verts, faces);
    out.hex = m.hex_of(m.index_of(fit.parts, AddressMode::Positional));
    return out;
}
} // namespace

namespace {

// The given spheres of a v3 character, checked.
void check_given(const std::vector<Sphere>& sp)
{
    if (sp.empty()) throw std::invalid_argument("a character has at least one sphere");
    if (sp.size() > 4096) throw std::invalid_argument("more than 4,096 spheres");
    constexpr int32_t kFar = int32_t(1) << 28;
    for (size_t i = 0; i < sp.size(); ++i)
    {
        const Sphere& p = sp[i];
        const std::string n = "sphere " + std::to_string(i);
        if (i == 0 && p.parent != -1) throw std::invalid_argument("sphere 0 is the body: its parent is -1");
        if (i > 0 && (p.parent < 0 || size_t(p.parent) >= i)) throw std::invalid_argument(n + "'s parent must come before it");
        if (p.r < 1 || p.r > kFar) throw std::invalid_argument(n + "'s radius is out of range");
        if (std::abs(int64_t(p.x)) > kFar || std::abs(int64_t(p.y)) > kFar || std::abs(int64_t(p.z)) > kFar) throw std::invalid_argument(n + " is too far out");
        if (p.kind > Sphere::Joint) throw std::invalid_argument(n + "'s kind is not body, head or joint");
    }
}

Character read_given(const std::vector<std::string>& lines)
{
    // lines[0]: "sieve-character-v3 spheres=N"
    const std::string head = std::string(kCharacterVersion3) + " spheres=";
    if (lines[0].rfind(head, 0) != 0) throw std::invalid_argument("a sieve-character-v3's first line is \"sieve-character-v3 spheres=N\"");
    const std::string n = lines[0].substr(head.size());
    if (n.empty() || n.size() > 5 || n.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument("spheres: not a number: " + n);
    const size_t count = std::stoul(n);
    if (lines.size() < count + 1) throw std::invalid_argument("it names " + n + " spheres but has fewer lines");
    std::vector<Sphere> sp;
    for (size_t i = 0; i < count; ++i)
    {
        std::istringstream in(lines[i + 1]);
        std::string tag, kind;
        long long idx, parent, x, y, z, r;
        if (!(in >> tag >> idx >> parent >> x >> y >> z >> r >> kind) || tag != "s" || idx != (long long)i)
            throw std::invalid_argument("sphere " + std::to_string(i) + "'s line is not \"s INDEX PARENT X Y Z R KIND\"");
        std::string extra;
        if (in >> extra) throw std::invalid_argument("sphere " + std::to_string(i) + "'s line has more after its kind");
        auto fits = [](long long v) { return v >= INT32_MIN && v <= INT32_MAX; };
        if (!fits(parent) || !fits(x) || !fits(y) || !fits(z) || !fits(r)) throw std::invalid_argument("sphere " + std::to_string(i) + " is out of range");
        Sphere s;
        s.parent = int32_t(parent), s.x = int32_t(x), s.y = int32_t(y), s.z = int32_t(z), s.r = int32_t(r);
        if (kind == "body") s.kind = Sphere::Body;
        else if (kind == "head") s.kind = Sphere::Head;
        else if (kind == "joint") s.kind = Sphere::Joint;
        else throw std::invalid_argument("sphere " + std::to_string(i) + "'s kind is not body, head or joint");
        sp.push_back(s);
    }
    Character c = given_character(std::move(sp));
    for (size_t i = count + 1; i < lines.size(); ++i)
    {
        const std::string& h = lines[i];
        const size_t sp2 = h.find(' ', 2);
        if (h.rfind("h ", 0) != 0 || sp2 == std::string::npos) throw std::invalid_argument("not a held model's line: " + h);
        const std::string idx = h.substr(2, sp2 - 2);
        if (idx.empty() || idx.size() > 6 || idx.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument("not a sphere's number: " + h);
        if (c.held(uint32_t(std::stoul(idx)))) throw std::invalid_argument("two models in one sphere: " + h);
        c.hold(uint32_t(std::stoul(idx)), h.substr(sp2 + 1));
    }
    if (c.text() != [&] { std::string t; for (const auto& l : lines) t += l + "\n"; return t; }())
        throw std::invalid_argument("not in its canonical form (spheres or held models out of order, or numbers written otherwise)");
    return c;
}

} // namespace

Character given_character(std::vector<Sphere> spheres)
{
    check_given(spheres);
    Character c;
    c.given = true;
    c.spheres = std::move(spheres);
    return c;
}

std::string Character::one_line() const
{
    std::string t = text();
    while (!t.empty() && t.back() == '\n') t.pop_back();
    for (char& ch : t)
        if (ch == '\n') ch = ';';
    return t;
}

Character read_character(const std::string& text_in)
{
    // One line with ';' between its lines (as the settings file keeps it) reads as the text.
    std::string text = text_in;
    if (text.find('\n') == std::string::npos && text.find(';') != std::string::npos)
        for (char& ch : text)
            if (ch == ';') ch = '\n';
    if (text.rfind(std::string(kCharacterVersion3) + " ", 0) == 0)
    {
        std::vector<std::string> lines;
        for (size_t i = 0; i < text.size();)
        {
            size_t e = text.find('\n', i);
            if (e == std::string::npos) e = text.size();
            std::string l = text.substr(i, e - i);
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if (!l.empty()) lines.push_back(l);
            i = e + 1;
        }
        return read_given(lines);
    }
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
    if (words.empty() || (words[0] != kCharacterVersion && words[0] != kCharacterVersion2))
        throw std::invalid_argument(std::string("not a character: it does not start with ") + kCharacterVersion + ", " + kCharacterVersion2 + " or " + kCharacterVersion3);
    const bool v2 = words[0] == kCharacterVersion2;
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
        else throw std::invalid_argument("not a setting of " + words[0] + ": " + k);
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
    // v2: the held models' lines, after the spheres, taken off before the spheres are checked.
    if (v2)
    {
        std::vector<std::string> lines;
        for (size_t i = 0; i <= clean.size();)
        {
            const size_t e = std::min(clean.find('\n', i), clean.size());
            lines.push_back(clean.substr(i, e - i));
            i = e + 1;
        }
        while (!lines.empty() && lines.back().rfind("h ", 0) == 0)
        {
            const std::string& h = lines.back();
            const size_t sp = h.find(' ', 2);
            if (sp == std::string::npos) throw std::invalid_argument("not a held model's line: " + h);
            const std::string idx = h.substr(2, sp - 2);
            if (idx.empty() || idx.size() > 6 || idx.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument("not a sphere's number: " + h);
            if (c.held(uint32_t(std::stoul(idx)))) throw std::invalid_argument("two models in one sphere: " + h);
            c.hold(uint32_t(std::stoul(idx)), h.substr(sp + 1));
            lines.pop_back();
        }
        if (c.holds.empty()) throw std::invalid_argument("a sieve-character-v2 holds at least one model (one that holds none is v1)");
        clean.clear();
        for (size_t i = 0; i < lines.size(); ++i) clean += (i ? "\n" : "") + lines[i];
        while (!clean.empty() && clean.back() == '\n') clean.pop_back();
    }
    if (!clean.empty())
    {
        Character bare = c;
        bare.holds.clear();
        std::string want = bare.text().substr(bare.text().find('\n') + 1);
        while (!want.empty() && want.back() == '\n') want.pop_back();
        if (clean != want) throw std::invalid_argument("the spheres do not match their settings (a changed file, or another version)");
    }
    return c;
}

std::string Character::text() const
{
    std::string t = given ? std::string(kCharacterVersion3) + " spheres=" + std::to_string(spheres.size()) + "\n"
                          : std::string(holds.empty() ? kCharacterVersion : kCharacterVersion2) + " seed=" + std::to_string(settings.seed) + " limbs=" + std::to_string(settings.limbs) +
                                " joints=" + std::to_string(settings.joints) + " branches=" + std::to_string(settings.branches) +
                                " symmetry=" + symmetry_name(settings.symmetry) + " head=" + (settings.head ? "yes" : "no") + "\n";
    static const char* const kKinds[] = {"body", "head", "joint"};
    for (size_t i = 0; i < spheres.size(); ++i)
    {
        const Sphere& p = spheres[i];
        t += "s " + std::to_string(i) + " " + std::to_string(p.parent) + " " + std::to_string(p.x) + " " + std::to_string(p.y) + " " +
             std::to_string(p.z) + " " + std::to_string(p.r) + " " + kKinds[p.kind] + "\n";
    }
    for (const auto& [i, hex] : holds) t += "h " + std::to_string(i) + " " + hex + "\n";
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
