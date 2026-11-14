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

SkinCell skin_cell(const std::vector<Sphere>& spheres, float voxel_cell, uint32_t halvings)
{
    SkinCell out;
    out.cell = std::max(1e-3f, voxel_cell) / float(1u << std::min<uint32_t>(halvings, 3));
    if (spheres.empty()) return out;
    float lo[3], hi[3];
    box_of(spheres, lo, hi);
    const float widest = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
    while (widest / out.cell > float(kSkinMostCells))
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

SkinMesh skin_mesh(const std::vector<Sphere>& spheres, float cell, float blend)
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
    const float pad = 0.5f * k + 2.0f * cell;
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
    std::vector<int32_t> owner(field.size(), -1);
    std::vector<float> nearest(field.size(), reach);
    for (size_t si = 0; si < spheres.size(); ++si)
    {
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

} // namespace sieve
