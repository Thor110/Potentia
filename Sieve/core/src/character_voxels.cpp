// Sieve — a character's voxels (sieve/character_voxels.hpp; IDEAS 19.214). Moved here from the Character
// Creator's own drawing, its arithmetic unchanged, so that every space draws the same cubes; with the
// lattice's cell size (IDEAS 19.216) and the joining of faces by greedy meshing (IDEAS 19.215) beside it.
#include "sieve/character_voxels.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace sieve {

float voxel_cell(const Character& at_rest)
{
    return voxel_cell_of(at_rest, VoxelSizing{}).cell;
}

VoxelCell voxel_cell_of(const Character& at_rest, const VoxelSizing& sizing)
{
    VoxelCell out;
    if (sizing.rule != VoxelSizing::Lattice)
    {
        // Its height in 16 cells; but a character far wider than it is tall (a flat one, or one sphere
        // set far out) would ask for a grid too large to hold, so its cells grow until its widest side
        // is within kVoxelLatticeMostCells, as the lattice's do.
        int32_t ex, ey, ez;
        at_rest.extent(ex, ey, ez);
        out.cell = std::max(1.0f, float(std::max(ey, 1)) / 16.0f);
        const double widest = 2.0 * double(std::max({ex, ey, ez, int32_t(1)}));
        if (widest / double(out.cell) > double(kVoxelLatticeMostCells))
        {
            out.cell = float(widest / double(kVoxelLatticeMostCells));
            out.coarser = true;
        }
        return out;
    }
    // The base in the character's units (kCharacterUnitsPerMetre of them a metre), and the cell its
    // thinnest part asks for: its smallest sphere's width in `across` cells.
    const double base = double(std::clamp<uint32_t>(sizing.base_mm, 1, 100)) * double(kCharacterUnitsPerMetre) / 1000.0;
    const uint32_t across = std::clamp<uint32_t>(sizing.across, 1, 8);
    int32_t least = 0;
    for (const Sphere& p : at_rest.spheres)
        if (p.r > 0 && (least == 0 || p.r < least)) least = p.r;
    int level = 0;
    if (least > 0)
        while (level < 24 && base * double(int64_t(1) << (level + 1)) <= 2.0 * double(least) / double(across)) ++level;
    // Within the limit: the widest side (the box round every sphere) in at most kVoxelLatticeMostCells.
    int32_t ex, ey, ez;
    at_rest.extent(ex, ey, ez);
    const double widest = 2.0 * double(std::max({ex, ey, ez, int32_t(1)}));
    while (level < 30 && widest / (base * double(int64_t(1) << level)) > double(kVoxelLatticeMostCells))
    {
        ++level;
        out.coarser = true;
    }
    out.level = level;
    out.cell = float(base * double(int64_t(1) << level));
    return out;
}

std::vector<VoxelFace> voxel_faces(const std::vector<Sphere>& spheres, float cellu, bool anchored)
{
    std::vector<VoxelFace> out;
    if (spheres.empty() || !(cellu > 0)) return out;
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (const Sphere& p : spheres)
    {
        const float c[3] = {float(p.x), float(p.y), float(p.z)};
        for (int k = 0; k < 3; ++k) lo[k] = std::min(lo[k], c[k] - float(p.r)), hi[k] = std::max(hi[k], c[k] + float(p.r));
    }
    if (anchored)
        for (int k = 0; k < 3; ++k) lo[k] = std::floor(lo[k] / cellu) * cellu;
    const int nx = int(std::ceil((hi[0] - lo[0]) / cellu)) + 1, ny = int(std::ceil((hi[1] - lo[1]) / cellu)) + 1, nz = int(std::ceil((hi[2] - lo[2]) / cellu)) + 1;
    // Each cell: the sphere it lies deepest in, or -1 where it lies in none.
    std::vector<int> owner(size_t(nx) * size_t(ny) * size_t(nz), -1);
    auto idx = [&](int x, int y, int z) { return (size_t(z) * size_t(ny) + size_t(y)) * size_t(nx) + size_t(x); };
    for (size_t si = 0; si < spheres.size(); ++si)
    {
        const Sphere& p = spheres[si];
        const float r = float(p.r);
        const int x0 = std::max(0, int((float(p.x) - r - lo[0]) / cellu)), x1 = std::min(nx - 1, int((float(p.x) + r - lo[0]) / cellu));
        const int y0 = std::max(0, int((float(p.y) - r - lo[1]) / cellu)), y1 = std::min(ny - 1, int((float(p.y) + r - lo[1]) / cellu));
        const int z0 = std::max(0, int((float(p.z) - r - lo[2]) / cellu)), z1 = std::min(nz - 1, int((float(p.z) + r - lo[2]) / cellu));
        for (int z = z0; z <= z1; ++z)
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x)
                {
                    const float cx3 = lo[0] + (float(x) + 0.5f) * cellu - float(p.x), cy3 = lo[1] + (float(y) + 0.5f) * cellu - float(p.y),
                                cz3 = lo[2] + (float(z) + 0.5f) * cellu - float(p.z);
                    const float in = r * r - (cx3 * cx3 + cy3 * cy3 + cz3 * cz3);
                    if (in < 0) continue;
                    int& o = owner[idx(x, y, z)];
                    if (o < 0) o = int(si);
                    else
                    {
                        const Sphere& q = spheres[size_t(o)];
                        const float qx = lo[0] + (float(x) + 0.5f) * cellu - float(q.x), qy = lo[1] + (float(y) + 0.5f) * cellu - float(q.y), qz = lo[2] + (float(z) + 0.5f) * cellu - float(q.z);
                        if (in > float(q.r) * float(q.r) - (qx * qx + qy * qy + qz * qz)) o = int(si);
                    }
                }
    }
    auto filled = [&](int x, int y, int z) { return x >= 0 && y >= 0 && z >= 0 && x < nx && y < ny && z < nz && owner[idx(x, y, z)] >= 0; };
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
            {
                const int o = owner[idx(x, y, z)];
                if (o < 0) continue;
                const float bx = lo[0] + float(x) * cellu, by = lo[1] + float(y) * cellu, bz = lo[2] + float(z) * cellu;
                for (int d = 0; d < 6; ++d)
                {
                    const int* dv = kVoxelDirs[d];
                    if (filled(x + dv[0], y + dv[1], z + dv[2])) continue; // (inside: hidden)
                    VoxelFace f;
                    f.sphere = uint32_t(o);
                    f.dir = uint8_t(d);
                    f.mid[0] = bx + cellu / 2, f.mid[1] = by + cellu / 2, f.mid[2] = bz + cellu / 2;
                    f.cell[0] = x, f.cell[1] = y, f.cell[2] = z;
                    // The face's four corners: on the side dv points to.
                    const int ax = dv[0] ? 0 : dv[1] ? 1 : 2, u = (ax + 1) % 3, v = (ax + 2) % 3;
                    for (int k = 0; k < 4; ++k)
                    {
                        float w[3] = {bx, by, bz};
                        w[ax] += dv[ax] > 0 ? cellu : 0;
                        w[u] += (k == 1 || k == 2) ? cellu : 0;
                        w[v] += (k >= 2) ? cellu : 0;
                        for (int j = 0; j < 3; ++j) f.corner[k][j] = w[j];
                    }
                    out.push_back(f);
                }
            }
    return out;
}

std::vector<VoxelQuad> join_voxel_faces(const std::vector<VoxelFace>& faces, const std::vector<uint32_t>& colours)
{
    std::vector<VoxelQuad> out;
    if (faces.empty() || colours.size() != faces.size()) return out;
    // A face's plane is its direction and its cell along the axis it faces; within the plane, its cell
    // across (u) and up (v), the axes its corners run along (u from corner 0 to 1, v from 1 to 2).
    auto axis = [](const VoxelFace& f) {
        const int* dv = kVoxelDirs[f.dir];
        return dv[0] ? 0 : dv[1] ? 1 : 2;
    };
    // Sorted by plane, then row, then place along it, as one number each (cheaper to sort than to compare
    // field by field): the direction, then each cell index offset to be positive, 16 bits apiece (a grid
    // is never near 65,536 cells across).
    int32_t least = INT32_MAX;
    for (const VoxelFace& f : faces)
        for (int j = 0; j < 3; ++j) least = std::min(least, f.cell[j]);
    std::vector<std::pair<uint64_t, uint32_t>> keyed(faces.size());
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const VoxelFace& f = faces[i];
        const int ax = axis(f);
        auto part = [&](int j) { return uint64_t(uint32_t(f.cell[j] - least) & 0xFFFFu); };
        keyed[i] = {uint64_t(f.dir) << 48 | part(ax) << 32 | part((ax + 2) % 3) << 16 | part((ax + 1) % 3), uint32_t(i)};
    }
    std::sort(keyed.begin(), keyed.end());
    std::vector<uint32_t> order(faces.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = keyed[i].second;
    // The plane's faces by place (an index into faces, or -1), kept between planes: every face placed in it
    // is taken by a rectangle before the plane is done, so it is all -1 again for the next without clearing.
    std::vector<int64_t> grid;
    for (size_t g0 = 0; g0 < order.size();)
    {
        // One plane: the faces from g0 to g1.
        const VoxelFace& first = faces[order[g0]];
        const int ax = axis(first), u = (ax + 1) % 3, v = (ax + 2) % 3;
        size_t g1 = g0;
        int32_t u0 = INT32_MAX, u1 = INT32_MIN, v0 = INT32_MAX, v1 = INT32_MIN;
        while (g1 < order.size() && faces[order[g1]].dir == first.dir && faces[order[g1]].cell[ax] == first.cell[ax])
        {
            const VoxelFace& f = faces[order[g1]];
            u0 = std::min(u0, f.cell[u]), u1 = std::max(u1, f.cell[u]), v0 = std::min(v0, f.cell[v]), v1 = std::max(v1, f.cell[v]);
            ++g1;
        }
        const size_t gw = size_t(int64_t(u1) - u0 + 1), gh = size_t(int64_t(v1) - v0 + 1);
        if (grid.size() < gw * gh) grid.assign(gw * gh, -1);
        for (size_t i = g0; i < g1; ++i)
        {
            const VoxelFace& f = faces[order[i]];
            grid[size_t(f.cell[v] - v0) * gw + size_t(f.cell[u] - u0)] = int64_t(order[i]);
        }
        auto at = [&](size_t x, size_t y) { return grid[y * gw + x]; };
        // (the faces in row order, as sorted: the places to start from, skipping those already taken)
        for (size_t i = g0; i < g1; ++i)
        {
            const size_t x = size_t(faces[order[i]].cell[u] - u0), y = size_t(faces[order[i]].cell[v] - v0);
            const int64_t start = at(x, y);
            if (start < 0) continue;
            const uint32_t colour = colours[size_t(start)];
            auto same = [&](size_t xx, size_t yy) {
                const int64_t there = at(xx, yy);
                return there >= 0 && colours[size_t(there)] == colour;
            };
            // Along its row as far as the faces match, then down as many rows as match its width.
            size_t w = 1;
            while (x + w < gw && same(x + w, y)) ++w;
            size_t h = 1;
            for (bool whole = true; whole && y + h < gh;)
            {
                for (size_t k = 0; k < w && whole; ++k) whole = same(x + k, y + h);
                if (whole) ++h;
            }
            VoxelQuad q;
            q.face = uint32_t(start);
            q.dir = first.dir;
            q.cells = uint32_t(w * h);
            const VoxelFace* corner_of[4] = {&faces[size_t(at(x, y))], &faces[size_t(at(x + w - 1, y))], &faces[size_t(at(x + w - 1, y + h - 1))],
                                             &faces[size_t(at(x, y + h - 1))]};
            for (int k = 0; k < 4; ++k)
                for (int j = 0; j < 3; ++j) q.corner[k][j] = corner_of[k]->corner[k][j];
            for (size_t yy = y; yy < y + h; ++yy)
                for (size_t xx = x; xx < x + w; ++xx) grid[yy * gw + xx] = -1; // (joined: taken)
            out.push_back(q);
        }
        g0 = g1;
    }
    return out;
}

} // namespace sieve
