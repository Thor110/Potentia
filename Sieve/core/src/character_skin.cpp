// Sieve — a character's skin (sieve/character_skin.hpp; IDEAS 19.225): the spheres as one field, its
// surface made into triangles by surface nets over a grid as fine as the voxels.
#include "sieve/character_skin.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace sieve {

float skin_blend(const std::vector<Sphere>& spheres, uint32_t percent)
{
    double sum = 0;
    size_t n = 0;
    for (const Sphere& p : spheres)
        if (p.r > 0) sum += double(p.r), ++n;
    if (n == 0) return 0;
    return float(sum / double(n) * double(std::min<uint32_t>(percent, 1000)) / 100.0);
}

// The box round the spheres, in the character's units.
static void box_of(const std::vector<Sphere>& spheres, float lo[3], float hi[3])
{
    for (int k = 0; k < 3; ++k) lo[k] = 1e30f, hi[k] = -1e30f;
    for (const Sphere& p : spheres)
    {
        const float c[3] = {float(p.x), float(p.y), float(p.z)};
        for (int k = 0; k < 3; ++k) lo[k] = std::min(lo[k], c[k] - float(p.r)), hi[k] = std::max(hi[k], c[k] + float(p.r));
    }
}

// How far the grid reaches past the spheres' box on each side: the blend in full and two cells. The smooth minimum is
// taken over the spheres one after another, and each can lower the field again, by up to the whole blend where
// many spheres overlap (until review 4 the grid reached half the blend past the box, and such a skin was cut open
// where the grid ended).
static float skin_pad(float blend, float cell) { return std::max(0.0f, blend) + 2.0f * cell; }

SkinCell skin_cell(const std::vector<Sphere>& spheres, float voxel_cell, uint32_t halvings, float blend)
{
    SkinCell out;
    out.cell = std::max(1e-3f, voxel_cell) / float(1u << std::min<uint32_t>(halvings, 3));
    if (spheres.empty()) return out;
    float lo[3], hi[3];
    box_of(spheres, lo, hi);
    const float widest = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
    while ((widest + 2 * skin_pad(blend, out.cell)) / out.cell > float(kSkinMostCells))
    {
        out.cell *= 2;
        out.coarser = true;
    }
    return out;
}

// The smooth minimum of two distances (the quadratic one): the plain minimum where they differ by the
// blend or more, and below it, by at most a quarter of the blend, where they are near, so the crease
// between two spheres is filled as far as the blend reaches and no further.
static float smooth_min(float a, float b, float k)
{
    if (k <= 0) return std::min(a, b);
    const float h = std::max(k - std::fabs(a - b), 0.0f) / k;
    return std::min(a, b) - h * h * k * 0.25f;
}

std::vector<SkinLimb> skin_limbs(const std::vector<Sphere>& spheres, float blend)
{
    std::vector<SkinLimb> out;
    const float k = std::max(0.0f, blend);
    for (size_t i = 0; i < spheres.size(); ++i)
    {
        const Sphere& c = spheres[i];
        if (c.parent < 0 || size_t(c.parent) >= spheres.size() || size_t(c.parent) == i) continue;
        const Sphere& p = spheres[size_t(c.parent)];
        SkinLimb l;
        l.parent = uint32_t(c.parent), l.child = uint32_t(i);
        l.a[0] = float(p.x), l.a[1] = float(p.y), l.a[2] = float(p.z);
        l.b[0] = float(c.x), l.b[1] = float(c.y), l.b[2] = float(c.z);
        const float rp = std::max(0.0f, float(p.r));
        // (set back by the blend, but never by more than half the parent, so a small joint still sends out
        // a limb as wide as it nearly is)
        l.ra = rp - std::min(k, 0.5f * rp);
        l.rb = std::max(0.0f, float(c.r));
        out.push_back(l);
    }
    return out;
}

// A limb's cone with what does not depend on the point worked out once, for the many grid corners it is
// asked about. Worked in doubles: the squares below run to many figures more than a float keeps.
namespace {
struct Cone
{
    double a[3], ba[3], r1, r2, l2, rr, a2, il2;
    bool ball;       // one end's sphere holds the other's: the cone is the larger sphere
    explicit Cone(const SkinLimb& limb)
    {
        for (int k = 0; k < 3; ++k) a[k] = limb.a[k], ba[k] = double(limb.b[k]) - double(limb.a[k]);
        r1 = limb.ra, r2 = limb.rb;
        l2 = ba[0] * ba[0] + ba[1] * ba[1] + ba[2] * ba[2];
        rr = r1 - r2;
        a2 = l2 - rr * rr;
        ball = !(a2 > 0);
        il2 = ball ? 0.0 : 1.0 / l2;
    }
    double distance(const float p[3]) const
    {
        const double pa[3] = {double(p[0]) - a[0], double(p[1]) - a[1], double(p[2]) - a[2]};
        if (ball)
        {
            double d2 = 0;
            for (int k = 0; k < 3; ++k)
            {
                const double d = pa[k] - (r1 >= r2 ? 0.0 : ba[k]);
                d2 += d * d;
            }
            return std::sqrt(d2) - std::max(r1, r2);
        }
        // The exact distance with one square root (as Inigo Quilez gives it): y is how far along the bone
        // the point lies (scaled by its length squared), x2 how far from the bone; the point is nearest the
        // child's sphere, the parent's, or the cone's side between them, as the side's slope decides.
        const double y = pa[0] * ba[0] + pa[1] * ba[1] + pa[2] * ba[2];
        const double z = y - l2;
        double x2 = 0;
        for (int k = 0; k < 3; ++k)
        {
            const double q = pa[k] * l2 - ba[k] * y;
            x2 += q * q;
        }
        const double y2 = y * y * l2, z2 = z * z * l2;
        auto sign = [](double v) { return v > 0 ? 1.0 : v < 0 ? -1.0 : 0.0; };
        const double k = sign(rr) * rr * rr * x2;
        if (sign(z) * a2 * z2 > k) return std::sqrt(x2 + z2) * il2 - r2;
        if (sign(y) * a2 * y2 < k) return std::sqrt(x2 + y2) * il2 - r1;
        return (std::sqrt(x2 * a2 * il2) + y * rr) * il2 - r1;
    }
    // At least how far the point is from the cone: its distance from the bone less the wider end's radius
    // (the cone lies within the spheres of that radius along the bone), cheaper than the exact distance.
    double at_least(const float p[3]) const
    {
        const double pa[3] = {double(p[0]) - a[0], double(p[1]) - a[1], double(p[2]) - a[2]};
        const double t = l2 > 0 ? std::clamp((pa[0] * ba[0] + pa[1] * ba[1] + pa[2] * ba[2]) / l2, 0.0, 1.0) : 0.0;
        double d2 = 0;
        for (int k = 0; k < 3; ++k)
        {
            const double d = pa[k] - t * ba[k];
            d2 += d * d;
        }
        return std::sqrt(d2) - std::max(r1, r2);
    }
};
} // namespace

float skin_limb_distance(const SkinLimb& limb, const float p[3]) { return float(Cone(limb).distance(p)); }

SkinMesh skin_mesh(const std::vector<Sphere>& spheres, float cell, float blend, const std::vector<SkinLimb>& limbs)
{
    SkinMesh out;
    if (spheres.empty() || !(cell > 0)) return out;
    const float k = std::max(0.0f, blend);
    // The field is wanted exactly only near the surface: a sphere further than this from a point cannot
    // change the field there where it lies within two cells of nought, so each sphere is worked out only
    // over the grid corners within this of it, and the field elsewhere left at this (outside).
    const float reach = k + 2.0f * cell;
    float lo[3], hi[3];
    box_of(spheres, lo, hi);
    // The grid: whole cells from the character's centre (so it holds still on the character as it moves),
    // reaching past the box by enough for the blend's fullness and a cell besides, so the surface never
    // meets its edge.
    const float pad = skin_pad(k, cell);
    int g0[3], n[3];
    for (int a = 0; a < 3; ++a)
    {
        g0[a] = int(std::floor((lo[a] - pad) / cell));
        n[a] = int(std::ceil((hi[a] + pad) / cell)) - g0[a] + 1;
        out.grid[a] = uint32_t(n[a]);
    }
    const size_t nx = size_t(n[0]), ny = size_t(n[1]), nz = size_t(n[2]);
    auto idx = [&](size_t x, size_t y, size_t z) { return (z * ny + y) * nx + x; };
    std::vector<float> field(nx * ny * nz, reach);
    // Each corner's nearest sphere (by the distance from its surface), for the vertices' colours.
    // (The nearest starts unreached, so a limb's corners beyond every sphere's reach still find one; a
    // sphere's own are all within its reach, so for the spheres alone this is as it was.)
    std::vector<int32_t> owner(field.size(), -1);
    std::vector<float> nearest(field.size(), 1e30f);
    // The limbs, by the sphere each takes the place of (its child).
    std::vector<int32_t> limb_of(spheres.size(), -1);
    for (size_t li = 0; li < limbs.size(); ++li)
        if (limbs[li].child < spheres.size()) limb_of[limbs[li].child] = int32_t(li);
    // Which limb last visited each corner, so a corner within the boxes of two pieces of one limb (below)
    // takes that limb into its field once. (Only kept when there are limbs.)
    std::vector<uint32_t> visited(limbs.empty() ? 0 : field.size(), 0);
    for (size_t si = 0; si < spheres.size(); ++si)
    {
        if (limb_of[si] >= 0)
        {
            // A limb, in its child's place in the order: worked out over the corners within reach of the
            // box round both its ends, its corners' nearest sphere the nearer of the two it joins.
            const SkinLimb& l = limbs[size_t(limb_of[si])];
            const Cone cone(l);
            const Sphere& sp = spheres[l.parent];
            const Sphere& sc = spheres[l.child];
            // A long slanting bone's one box would hold most of the grid (a chain zigzagging corner to
            // corner took 49 s with 4,096 spheres), so the bone is cut into pieces, each with the box of
            // its own stretch: the round cone is the union of the spheres along it, their centres and radii
            // going evenly from one end to the other, so each piece's box holds the spheres at its two ends.
            const float dab[3] = {l.b[0] - l.a[0], l.b[1] - l.a[1], l.b[2] - l.a[2]};
            const float length = std::sqrt(dab[0] * dab[0] + dab[1] * dab[1] + dab[2] * dab[2]);
            const float piece = std::max(4.0f * cell, std::max(l.ra, l.rb) + reach);
            const int pieces = std::clamp(int(std::ceil(length / piece)), 1, 4096);
            const uint32_t stamp = uint32_t(limb_of[si]) + 1;
            for (int pc = 0; pc < pieces; ++pc)
            {
            const float t0 = float(pc) / float(pieces), t1 = float(pc + 1) / float(pieces);
            const float r0 = l.ra + (l.rb - l.ra) * t0, r1 = l.ra + (l.rb - l.ra) * t1;
            int a0[3], a1[3];
            for (int a = 0; a < 3; ++a)
            {
                const float e0 = l.a[a] + dab[a] * t0, e1 = l.a[a] + dab[a] * t1;
                const float lo_a = std::min(e0 - r0, e1 - r1), hi_a = std::max(e0 + r0, e1 + r1);
                a0[a] = std::max(0, int(std::floor((lo_a - reach) / cell)) - g0[a]);
                a1[a] = std::min(n[a] - 1, int(std::ceil((hi_a + reach) / cell)) - g0[a]);
            }
            for (int z = a0[2]; z <= a1[2]; ++z)
                for (int y = a0[1]; y <= a1[1]; ++y)
                    for (int x = a0[0]; x <= a1[0]; ++x)
                    {
                        const size_t vi = idx(size_t(x), size_t(y), size_t(z));
                        if (visited[vi] == stamp) continue;
                        visited[vi] = stamp;
                        const float q[3] = {float(g0[0] + x) * cell, float(g0[1] + y) * cell, float(g0[2] + z) * cell};
                        // (most of a slanting limb's box is far from it: those corners are passed over
                        // by the cheaper bound before the exact distance is asked)
                        if (cone.at_least(q) >= double(reach)) continue;
                        const float d = float(cone.distance(q));
                        if (d >= reach) continue;
                        const size_t i = idx(size_t(x), size_t(y), size_t(z));
                        field[i] = smooth_min(field[i], d, k);
                        auto surface = [&](const Sphere& s) {
                            const float dx = q[0] - float(s.x), dy = q[1] - float(s.y), dz = q[2] - float(s.z);
                            return std::sqrt(dx * dx + dy * dy + dz * dz) - float(s.r);
                        };
                        const float dp = surface(sp), dc = surface(sc);
                        const float dn = std::min(dp, dc);
                        if (dn < nearest[i]) nearest[i] = dn, owner[i] = int32_t(dc <= dp ? l.child : l.parent);
                    }
            }
            continue;
        }
        const Sphere& p = spheres[si];
        const float c[3] = {float(p.x), float(p.y), float(p.z)}, r = float(p.r);
        int a0[3], a1[3];
        for (int a = 0; a < 3; ++a)
        {
            a0[a] = std::max(0, int(std::floor((c[a] - r - reach) / cell)) - g0[a]);
            a1[a] = std::min(n[a] - 1, int(std::ceil((c[a] + r + reach) / cell)) - g0[a]);
        }
        for (int z = a0[2]; z <= a1[2]; ++z)
        {
            const float dz = float(g0[2] + z) * cell - c[2];
            for (int y = a0[1]; y <= a1[1]; ++y)
            {
                const float dy = float(g0[1] + y) * cell - c[1];
                for (int x = a0[0]; x <= a1[0]; ++x)
                {
                    const float dx = float(g0[0] + x) * cell - c[0];
                    const float d = std::sqrt(dx * dx + dy * dy + dz * dz) - r;
                    if (d >= reach) continue;
                    const size_t i = idx(size_t(x), size_t(y), size_t(z));
                    field[i] = smooth_min(field[i], d, k);
                    if (d < nearest[i]) nearest[i] = d, owner[i] = int32_t(si);
                }
            }
        }
    }
    // One vertex in every cell the surface passes through: the average of where it crosses the cell's
    // twelve edges (each crossing found along its edge as the field runs straight between the corners).
    static constexpr int kCorner[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};
    static constexpr int kEdge[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    const size_t cx = nx - 1, cy = ny - 1, cz = nz - 1;
    auto cidx = [&](size_t x, size_t y, size_t z) { return (z * cy + y) * cx + x; };
    std::vector<int32_t> vert(cx * cy * cz, -1);
    for (size_t z = 0; z < cz; ++z)
        for (size_t y = 0; y < cy; ++y)
            for (size_t x = 0; x < cx; ++x)
            {
                float f[8];
                int inside = 0;
                for (int q = 0; q < 8; ++q)
                {
                    f[q] = field[idx(x + size_t(kCorner[q][0]), y + size_t(kCorner[q][1]), z + size_t(kCorner[q][2]))];
                    inside += f[q] < 0;
                }
                if (inside == 0 || inside == 8) continue;
                float sum[3] = {0, 0, 0};
                int crossings = 0;
                for (const auto& e : kEdge)
                {
                    const float fa = f[e[0]], fb = f[e[1]];
                    if ((fa < 0) == (fb < 0)) continue;
                    const float t = fa / (fa - fb);
                    for (int a = 0; a < 3; ++a) sum[a] += float(kCorner[e[0]][a]) + t * float(kCorner[e[1]][a] - kCorner[e[0]][a]);
                    ++crossings;
                }
                // Within the cell (u, v, w from 0 to 1), then in the character's units.
                const float u = sum[0] / float(crossings), v = sum[1] / float(crossings), w = sum[2] / float(crossings);
                SkinVertex sv;
                sv.p[0] = (float(g0[0]) + float(x) + u) * cell;
                sv.p[1] = (float(g0[1]) + float(y) + v) * cell;
                sv.p[2] = (float(g0[2]) + float(z) + w) * cell;
                // Its normal: the field's rise there, as the field runs between the cell's eight corners
                // (straight along each axis, so the slope along one axis is the four edges' slopes
                // weighted by how near the vertex is to each).
                auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
                const float gx = lerp(lerp(f[1] - f[0], f[3] - f[2], v), lerp(f[5] - f[4], f[7] - f[6], v), w);
                const float gy = lerp(lerp(f[2] - f[0], f[3] - f[1], u), lerp(f[6] - f[4], f[7] - f[5], u), w);
                const float gz = lerp(lerp(f[4] - f[0], f[5] - f[1], u), lerp(f[6] - f[2], f[7] - f[3], u), v);
                const float gl = std::sqrt(gx * gx + gy * gy + gz * gz);
                sv.n[0] = gl > 0 ? gx / gl : 0, sv.n[1] = gl > 0 ? gy / gl : 1, sv.n[2] = gl > 0 ? gz / gl : 0;
                // Its sphere: the one nearest the cell's corner nearest the vertex (of the corners some
                // sphere reached; a cell the surface passes through always has one inside).
                float best = 1e30f;
                for (int q = 0; q < 8; ++q)
                {
                    const int32_t o = owner[idx(x + size_t(kCorner[q][0]), y + size_t(kCorner[q][1]), z + size_t(kCorner[q][2]))];
                    if (o < 0) continue;
                    const float du = u - float(kCorner[q][0]), dv = v - float(kCorner[q][1]), dw = w - float(kCorner[q][2]);
                    const float d2 = du * du + dv * dv + dw * dw;
                    if (d2 < best) best = d2, sv.sphere = uint32_t(o);
                }
                vert[cidx(x, y, z)] = int32_t(out.vertices.size());
                out.vertices.push_back(sv);
            }
    // One four-sided face across every grid edge the surface crosses, joining the four cells round that
    // edge, turned so it faces where the field rises (outwards), split in two along its shorter diagonal.
    for (int a = 0; a < 3; ++a)
    {
        const int b = (a + 1) % 3, c = (a + 2) % 3;
        int s[3];
        for (s[2] = 0; s[2] < n[2]; ++s[2])
            for (s[1] = 0; s[1] < n[1]; ++s[1])
                for (s[0] = 0; s[0] < n[0]; ++s[0])
                {
                    if (s[a] + 1 >= n[a] || s[b] < 1 || s[c] < 1 || s[b] >= n[b] - 1 || s[c] >= n[c] - 1) continue;
                    int t[3] = {s[0], s[1], s[2]};
                    ++t[a];
                    const float fa = field[idx(size_t(s[0]), size_t(s[1]), size_t(s[2]))], fb = field[idx(size_t(t[0]), size_t(t[1]), size_t(t[2]))];
                    if ((fa < 0) == (fb < 0)) continue;
                    // The four cells round the edge, anticlockwise seen from along +a.
                    static constexpr int kRound[4][2] = {{-1, -1}, {0, -1}, {0, 0}, {-1, 0}};
                    int32_t q[4];
                    bool whole = true;
                    for (int m = 0; m < 4 && whole; ++m)
                    {
                        int e[3] = {s[0], s[1], s[2]};
                        e[b] += kRound[m][0], e[c] += kRound[m][1];
                        q[m] = vert[cidx(size_t(e[0]), size_t(e[1]), size_t(e[2]))];
                        whole = q[m] >= 0;
                    }
                    if (!whole) continue;
                    if (fa >= 0) std::swap(q[1], q[3]); // (outside at the edge's start: outwards is -a)
                    auto d2 = [&](int32_t i, int32_t j) {
                        const float* p = out.vertices[size_t(i)].p;
                        const float* r = out.vertices[size_t(j)].p;
                        return (p[0] - r[0]) * (p[0] - r[0]) + (p[1] - r[1]) * (p[1] - r[1]) + (p[2] - r[2]) * (p[2] - r[2]);
                    };
                    const bool first = d2(q[0], q[2]) <= d2(q[1], q[3]);
                    const int32_t tri[6] = {q[0], q[1], q[2], q[0], q[2], q[3]};
                    const int32_t alt[6] = {q[1], q[2], q[3], q[1], q[3], q[0]};
                    for (int m = 0; m < 6; ++m) out.tris.push_back(uint32_t(first ? tri[m] : alt[m]));
                    // (the face whole, from the corner its first triangle starts at, so it is wound the same)
                    for (int m = 0; m < 4; ++m) out.quads.push_back(uint32_t(q[(m + (first ? 0 : 1)) % 4]));
                }
    }
    return out;
}

std::vector<float> skin_smooth_normals(const SkinMesh& skin)
{
    std::vector<float> n(skin.vertices.size() * 3, 0.0f);
    for (size_t t = 0; t < skin.triangles(); ++t)
    {
        const uint32_t i[3] = {skin.tris[t * 3], skin.tris[t * 3 + 1], skin.tris[t * 3 + 2]};
        const float* a = skin.vertices[i[0]].p;
        const float* b = skin.vertices[i[1]].p;
        const float* c = skin.vertices[i[2]].p;
        const float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        // (the cross product: across the face, outwards as it is wound, as long as twice its area)
        const float g[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        for (uint32_t v : i)
            for (int k = 0; k < 3; ++k) n[size_t(v) * 3 + size_t(k)] += g[k];
    }
    for (size_t v = 0; v < skin.vertices.size(); ++v)
    {
        float* m = &n[v * 3];
        const float l = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
        if (l > 0)
            for (int k = 0; k < 3; ++k) m[k] /= l;
        else
            for (int k = 0; k < 3; ++k) m[k] = skin.vertices[v].n[k];
    }
    return n;
}

// The faces a subdivision step starts from, and the smoothing's: the quads when every triangle is half of one (the
// skin as surface nets makes it, and every step's output), else the triangles (the rule's general form).
static bool skin_by_quads(const SkinMesh& skin) { return !skin.quads.empty() && skin.quad_count() * 2 == skin.triangles(); }

// Each vertex's neighbours along the faces' sides, once each and sorted, laid out as one list (`next`) with where
// each vertex's run starts (`start`, nv + 1 of them): each face's sides counted at both ends, laid out by vertex,
// then each vertex's few made unique in place (a skin's neighbours are four or so, so this is linear in the faces,
// with no sort of them all). skin_taubin's and skin_subdivide's (review 4: they were two copies).
static void skin_neighbours(const std::vector<uint32_t>& faces, size_t corners, size_t nv, std::vector<uint32_t>& start,
                            std::vector<uint32_t>& next)
{
    start.assign(nv + 1, 0);
    auto each_side = [&](auto&& take) {
        for (size_t f = 0; f + corners <= faces.size(); f += corners)
            for (size_t k = 0; k < corners; ++k)
            {
                const uint32_t i = faces[f + k], j = faces[f + (k + 1) % corners];
                if (i == j || i >= nv || j >= nv) continue;
                take(i, j);
                take(j, i);
            }
    };
    each_side([&](uint32_t i, uint32_t) { ++start[size_t(i) + 1]; });
    for (size_t v = 0; v < nv; ++v) start[v + 1] += start[v];
    next.assign(start[nv], 0);
    std::vector<uint32_t> fill(start.begin(), start.end() - 1);
    each_side([&](uint32_t i, uint32_t j) { next[fill[i]++] = j; });
    size_t kept = 0;
    for (size_t v = 0; v < nv; ++v)
    {
        const auto b = next.begin() + std::ptrdiff_t(start[v]), e = next.begin() + std::ptrdiff_t(start[v + 1]);
        std::sort(b, e);
        const auto u = std::unique(b, e);
        start[v] = uint32_t(kept);
        for (auto it = b; it != u; ++it) next[kept++] = *it;
    }
    start[nv] = uint32_t(kept);
    next.resize(kept);
}

SkinMesh skin_taubin(const SkinMesh& skin, uint32_t rounds, float lambda, float mu)
{
    SkinMesh out = skin;
    const size_t nv = out.vertices.size();
    if (rounds == 0 || nv == 0) return out;
    // Each vertex's neighbours, once each, kept as one list with where each vertex's run starts (built
    // once, as the faces do not change between rounds). The quads' sides when every triangle is half of
    // one (the skin as surface nets makes it), else the triangles' sides.
    const bool by_quads = skin_by_quads(out);
    const std::vector<uint32_t>& faces = by_quads ? out.quads : out.tris;
    const size_t corners = by_quads ? 4 : 3;
    std::vector<uint32_t> start, next;
    skin_neighbours(faces, corners, nv, start, next);
    // The steps, each from where every vertex stood before it (so the order the vertices are visited in
    // does not matter), the positions kept in doubles between steps so many rounds do not gather the
    // floats' rounding.
    std::vector<double> p(nv * 3), q(nv * 3);
    for (size_t v = 0; v < nv; ++v)
        for (int k = 0; k < 3; ++k) p[v * 3 + size_t(k)] = double(out.vertices[v].p[k]);
    auto step = [&](double weight) {
        for (size_t v = 0; v < nv; ++v)
        {
            const uint32_t b = start[v], e = start[v + 1];
            if (b == e)
            {
                for (int k = 0; k < 3; ++k) q[v * 3 + size_t(k)] = p[v * 3 + size_t(k)];
                continue;
            }
            double mean[3] = {0, 0, 0};
            for (uint32_t m = b; m < e; ++m)
                for (int k = 0; k < 3; ++k) mean[k] += p[size_t(next[m]) * 3 + size_t(k)];
            for (int k = 0; k < 3; ++k)
            {
                const double here = p[v * 3 + size_t(k)];
                q[v * 3 + size_t(k)] = here + weight * (mean[k] / double(e - b) - here);
            }
        }
        p.swap(q);
    };
    for (uint32_t r = 0; r < rounds; ++r)
    {
        step(double(lambda));
        step(double(mu));
    }
    for (size_t v = 0; v < nv; ++v)
        for (int k = 0; k < 3; ++k) out.vertices[v].p[k] = float(p[v * 3 + size_t(k)]);
    const std::vector<float> n = skin_smooth_normals(out);
    for (size_t v = 0; v < nv; ++v)
        for (int k = 0; k < 3; ++k) out.vertices[v].n[k] = n[v * 3 + size_t(k)];
    out.smooth_normals = true;
    return out;
}

double skin_volume(const SkinMesh& skin)
{
    double six = 0;
    for (size_t t = 0; t < skin.triangles(); ++t)
    {
        const float* a = skin.vertices[skin.tris[t * 3]].p;
        const float* b = skin.vertices[skin.tris[t * 3 + 1]].p;
        const float* c = skin.vertices[skin.tris[t * 3 + 2]].p;
        // (a . (b x c): six times the signed volume of the triangle and the origin)
        six += double(a[0]) * (double(b[1]) * c[2] - double(b[2]) * c[1]) + double(a[1]) * (double(b[2]) * c[0] - double(b[0]) * c[2]) +
               double(a[2]) * (double(b[0]) * c[1] - double(b[1]) * c[0]);
    }
    return six / 6.0;
}


size_t skin_subdivided_quads(const SkinMesh& skin, uint32_t steps)
{
    if (steps == 0) return skin.quad_count();
    // (the first step: one quad for each corner of each face; every step after it, four for each)
    size_t n = skin_by_quads(skin) ? skin.quads.size() : skin.tris.size();
    for (uint32_t s = 1; s < steps; ++s)
    {
        if (n > (size_t(1) << 60)) return n; // (far past any bound; kept from overflowing)
        n *= 4;
    }
    return n;
}

uint32_t skin_subdivision_steps(const SkinMesh& skin, uint32_t asked, size_t most_quads)
{
    uint32_t steps = 0;
    while (steps < asked && skin_subdivided_quads(skin, steps + 1) <= most_quads) ++steps;
    return steps;
}

// One step of Catmull and Clark's rule (skin_subdivide's), positions in doubles.
static SkinMesh skin_subdivide_once(const SkinMesh& skin)
{
    const size_t nv = skin.vertices.size();
    const bool by_quads = skin_by_quads(skin);
    const std::vector<uint32_t>& faces = by_quads ? skin.quads : skin.tris;
    const size_t corners = by_quads ? 4 : 3;
    const size_t nf = faces.size() / corners;
    SkinMesh out;
    for (int a = 0; a < 3; ++a) out.grid[a] = skin.grid[a];
    if (nv == 0 || nf == 0) return out;
    // Each vertex's neighbours, once each and sorted, laid out as one list with where each vertex's run
    // starts (as skin_taubin finds them), and each edge's number given at its lower end and copied to its
    // upper, so an edge is found from either end by a short search of that end's few neighbours.
    std::vector<uint32_t> start, next;
    skin_neighbours(faces, corners, nv, start, next);
    const size_t kept = next.size();
    auto slot = [&](uint32_t i, uint32_t j) {
        const auto b = next.begin() + std::ptrdiff_t(start[i]), e = next.begin() + std::ptrdiff_t(start[size_t(i) + 1]);
        return size_t(std::lower_bound(b, e, j) - next.begin());
    };
    std::vector<uint32_t> edge_of(kept, 0);
    uint32_t ne = 0;
    for (uint32_t v = 0; v < uint32_t(nv); ++v)
        for (uint32_t m = start[v]; m < start[size_t(v) + 1]; ++m)
            if (next[m] > v) edge_of[m] = ne++;
    for (uint32_t v = 0; v < uint32_t(nv); ++v)
        for (uint32_t m = start[v]; m < start[size_t(v) + 1]; ++m)
            if (next[m] < v) edge_of[m] = edge_of[slot(next[m], v)];
    auto edge = [&](uint32_t i, uint32_t j) { return edge_of[slot(i, j)]; };
    // The face points: each face's corners' average.
    std::vector<double> fp(nf * 3, 0.0);
    for (size_t f = 0; f < nf; ++f)
    {
        for (size_t k = 0; k < corners; ++k)
            for (int a = 0; a < 3; ++a) fp[f * 3 + size_t(a)] += double(skin.vertices[faces[f * corners + k]].p[a]);
        for (int a = 0; a < 3; ++a) fp[f * 3 + size_t(a)] /= double(corners);
    }
    // Each edge's ends, and the faces either side of it (their points summed, and how many: two on a
    // closed surface, one on an outline, more where the skin touches itself along an edge).
    std::vector<uint32_t> ends(size_t(ne) * 2, 0);
    for (uint32_t v = 0; v < uint32_t(nv); ++v)
        for (uint32_t m = start[v]; m < start[size_t(v) + 1]; ++m)
            if (next[m] > v) ends[size_t(edge_of[m]) * 2] = v, ends[size_t(edge_of[m]) * 2 + 1] = next[m];
    std::vector<double> edge_faces(size_t(ne) * 3, 0.0);
    std::vector<uint32_t> edge_count(ne, 0);
    // Each vertex's faces: their points summed, and how many.
    std::vector<double> vertex_faces(nv * 3, 0.0);
    std::vector<uint32_t> vertex_count(nv, 0);
    for (size_t f = 0; f < nf; ++f)
        for (size_t k = 0; k < corners; ++k)
        {
            const uint32_t i = faces[f * corners + k], j = faces[f * corners + (k + 1) % corners];
            if (i >= nv || j >= nv) continue;
            for (int a = 0; a < 3; ++a) vertex_faces[size_t(i) * 3 + size_t(a)] += fp[f * 3 + size_t(a)];
            ++vertex_count[i];
            if (i == j) continue;
            const uint32_t e = edge(i, j);
            for (int a = 0; a < 3; ++a) edge_faces[size_t(e) * 3 + size_t(a)] += fp[f * 3 + size_t(a)];
            ++edge_count[e];
        }
    auto pos = [&](uint32_t v, int a) { return double(skin.vertices[v].p[a]); };
    // The new vertices: the old corners moved (first, so each keeps its number), then the edges' points,
    // then the faces'.
    out.vertices.resize(nv + size_t(ne) + nf);
    std::vector<double> ep(size_t(ne) * 3);
    for (uint32_t e = 0; e < ne; ++e)
    {
        const uint32_t i = ends[size_t(e) * 2], j = ends[size_t(e) * 2 + 1];
        const uint32_t c = edge_count[e];
        for (int a = 0; a < 3; ++a)
            ep[size_t(e) * 3 + size_t(a)] = c >= 2 ? (pos(i, a) + pos(j, a) + edge_faces[size_t(e) * 3 + size_t(a)]) / double(2 + c) : 0.5 * (pos(i, a) + pos(j, a));
        SkinVertex& w = out.vertices[nv + e];
        for (int a = 0; a < 3; ++a) w.p[a] = float(ep[size_t(e) * 3 + size_t(a)]);
        w.sphere = skin.vertices[i].sphere; // (i is the lower-numbered end)
    }
    for (uint32_t v = 0; v < uint32_t(nv); ++v)
    {
        SkinVertex& w = out.vertices[v];
        w = skin.vertices[v];
        const uint32_t b = start[v], e = start[size_t(v) + 1];
        if (b == e || vertex_count[v] == 0) continue; // (in no face: left where it is)
        uint32_t outline = 0, along[2] = {0, 0};
        for (uint32_t m = b; m < e; ++m)
            if (edge_count[edge_of[m]] == 1)
            {
                if (outline < 2) along[outline] = next[m];
                ++outline;
            }
        if (outline == 2)
        {
            for (int a = 0; a < 3; ++a) w.p[a] = float((6.0 * pos(v, a) + pos(along[0], a) + pos(along[1], a)) / 8.0);
            continue;
        }
        if (outline > 0) continue; // (where outlines meet: a corner, kept)
        const double n = double(e - b);
        for (int a = 0; a < 3; ++a)
        {
            double mids = 0;
            for (uint32_t m = b; m < e; ++m) mids += 0.5 * (pos(v, a) + pos(next[m], a));
            const double F = vertex_faces[size_t(v) * 3 + size_t(a)] / double(vertex_count[v]), R = mids / n;
            w.p[a] = float((F + 2.0 * R + (n - 3.0) * pos(v, a)) / n);
        }
    }
    for (size_t f = 0; f < nf; ++f)
    {
        SkinVertex& w = out.vertices[nv + ne + f];
        for (int a = 0; a < 3; ++a) w.p[a] = float(fp[f * 3 + size_t(a)]);
        // (the sphere most corners share; the first corner's wins a tie)
        size_t best = 0;
        w.sphere = skin.vertices[faces[f * corners]].sphere;
        for (size_t i = 0; i < corners; ++i)
        {
            size_t count = 0;
            for (size_t j = 0; j < corners; ++j) count += skin.vertices[faces[f * corners + j]].sphere == skin.vertices[faces[f * corners + i]].sphere;
            if (count > best) best = count, w.sphere = skin.vertices[faces[f * corners + i]].sphere;
        }
    }
    // The new faces: one quad at each corner of each face (the corner, its side's point onwards, the face's
    // point, the point of the side before it), wound as the face was, split along its shorter diagonal as
    // skin_mesh splits its own.
    out.quads.reserve(nf * corners * 4);
    out.tris.reserve(nf * corners * 6);
    auto d2 = [&](uint32_t i, uint32_t j) {
        const float* p = out.vertices[i].p;
        const float* r = out.vertices[j].p;
        return (p[0] - r[0]) * (p[0] - r[0]) + (p[1] - r[1]) * (p[1] - r[1]) + (p[2] - r[2]) * (p[2] - r[2]);
    };
    for (size_t f = 0; f < nf; ++f)
        for (size_t k = 0; k < corners; ++k)
        {
            const uint32_t prev = faces[f * corners + (k + corners - 1) % corners], here = faces[f * corners + k], after = faces[f * corners + (k + 1) % corners];
            if (here >= nv || prev >= nv || after >= nv || here == prev || here == after) continue;
            const uint32_t q[4] = {here, uint32_t(nv) + edge(here, after), uint32_t(nv + ne + f), uint32_t(nv) + edge(prev, here)};
            const bool first = d2(q[0], q[2]) <= d2(q[1], q[3]);
            const uint32_t tri[6] = {q[0], q[1], q[2], q[0], q[2], q[3]};
            const uint32_t alt[6] = {q[1], q[2], q[3], q[1], q[3], q[0]};
            for (int m = 0; m < 6; ++m) out.tris.push_back(first ? tri[m] : alt[m]);
            for (int m = 0; m < 4; ++m) out.quads.push_back(q[(m + (first ? 0 : 1)) % 4]);
        }
    return out;
}

SkinMesh skin_subdivide(const SkinMesh& skin, uint32_t steps)
{
    if (steps == 0) return skin;
    SkinMesh out = skin_subdivide_once(skin);
    for (uint32_t s = 1; s < steps; ++s) out = skin_subdivide_once(out);
    const std::vector<float> n = skin_smooth_normals(out);
    for (size_t v = 0; v < out.vertices.size(); ++v)
        for (int k = 0; k < 3; ++k) out.vertices[v].n[k] = n[v * 3 + size_t(k)];
    out.smooth_normals = true;
    return out;
}

} // namespace sieve
