#include "sieve/modelspace.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sieve {

namespace {

// Digits in chunks: as many as fit a 32-bit multiplier (base^k <= 2^32 - 1), one big-number
// operation per chunk instead of one per digit.
uint32_t chunk_digits(uint32_t base)
{
    uint32_t k = 1;
    for (uint64_t p = base; p * base < (uint64_t(1) << 32); p *= base) ++k;
    return k;
}

uint32_t small_pow(uint32_t base, uint32_t k)
{
    uint64_t p = 1;
    for (uint32_t i = 0; i < k; ++i) p *= base;
    return uint32_t(p);
}

void mul_pow(BigUint& n, uint32_t base, uint64_t e)
{
    const uint32_t k = chunk_digits(base), full = small_pow(base, k);
    for (; e >= k; e -= k) n.mul_small(full);
    for (; e > 0; --e) n.mul_small(base);
}

uint32_t log2_of(uint32_t v)
{
    uint32_t k = 0;
    while ((uint32_t(1) << k) < v) ++k;
    return k;
}

std::string shape_id(uint32_t v, uint32_t f, uint32_t c, const std::string& key)
{
    return "models/V" + std::to_string(v) + "/F" + std::to_string(f) + "/C" + std::to_string(c) + "/key=" + key + "/" +
           kModelSpaceVersion;
}

// One coordinate, exactly: digit d is (2d + 1 - C) / C, written with `decimals` places. The sign
// is always written, as '-' or '+', so every coordinate is the same width.
//
// It is '+' and not a space on purpose. The point of the canonical form is that the same object
// can live on the models line and, as text, on an ascii96 line, and canon-text-v2 collapses runs
// of spaces. Two spaces in a row anywhere here would not survive that trip, so there are none:
// every field is separated by exactly one space, and widths are made up with '+' and with
// leading zeros on the indices.
std::string coord_text(uint32_t d, uint32_t c, uint32_t decimals)
{
    // (2d + 1 - C) / C in units of 10^-decimals, exactly: C is a power of two and 10^decimals / C is
    // a whole number when decimals = log2(C), since 10^k / 2^k = 5^k.
    const int64_t num = int64_t(2) * d + 1 - int64_t(c);
    int64_t scaled = 1;
    for (uint32_t i = 0; i < decimals; ++i) scaled *= 10;
    const int64_t v = num * (scaled / c);
    const int64_t mag = v < 0 ? -v : v;
    std::string frac = std::to_string(mag % scaled);
    frac.insert(0, size_t(decimals) - frac.size(), '0');
    return (v < 0 ? "-" : "+") + std::to_string(mag / scaled) + "." + frac;
}

// The reverse, for the canonical form only: sign, one digit, a point, then `decimals` digits.
uint32_t coord_digit(std::string_view s, uint32_t c, uint32_t decimals)
{
    auto bad = [] { throw std::invalid_argument("a coordinate is not in the canonical form"); };
    if (s.size() != size_t(decimals) + 3 || (s[0] != '-' && s[0] != '+') || s[2] != '.') bad();
    int64_t units = 0, scaled = 1;
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (i == 2) continue; // the point
        if (s[i] < '0' || s[i] > '9') bad();
        units = units * 10 + (s[i] - '0');
        if (i >= 3) scaled *= 10;
    }
    if (s[0] == '-') units = -units;
    // digit d holds (2d + 1 - C) / C, so d = (units / 10^decimals * C + C - 1) / 2.
    const int64_t num = units * int64_t(c) + int64_t(c - 1) * scaled, denom = 2 * scaled;
    if (num < 0 || num % denom != 0) throw std::invalid_argument("a coordinate is not on this line's grid");
    const int64_t d = num / denom;
    if (d >= int64_t(c)) throw std::invalid_argument("a coordinate is off this line's grid");
    return uint32_t(d);
}

// An index in `width` characters, 1-based as .obj counts, padded with leading zeros rather than
// spaces for the reason given above: no two spaces in a row anywhere in the canonical form.
std::string index_text(uint32_t i, size_t width)
{
    std::string s = std::to_string(i + 1);
    if (s.size() < width) s.insert(0, width - s.size(), '0');
    return s;
}

uint32_t index_digit(std::string_view s, uint32_t v)
{
    if (s.empty()) throw std::invalid_argument("a face index is missing");
    size_t i = 0;
    uint64_t n = 0;
    for (; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9') throw std::invalid_argument("a face index is not a number");
        n = n * 10 + uint64_t(s[i] - '0');
        if (n > v) throw std::invalid_argument("a face index is past the end of this line's vertices");
    }
    if (n == 0) throw std::invalid_argument("face indices count from 1");
    return uint32_t(n - 1);
}

} // namespace

namespace {

// Validated before anything is built, so a bad shape says so rather than failing deeper down.
uint32_t checked_coords(uint32_t vertices, uint32_t faces, uint32_t coords)
{
    if (vertices < 3) throw std::invalid_argument("a models line needs at least 3 vertices");
    if (faces < 1) throw std::invalid_argument("a models line needs at least one face");
    if (coords < 2 || coords > 4096 || (coords & (coords - 1)) != 0)
        throw std::invalid_argument("a models line's coordinate grid must be a power of two between 2 and 4096");
    if (uint64_t(vertices) * 3 > 0xFFFFFFFFull || uint64_t(faces) * 3 > 0xFFFFFFFFull)
        throw std::invalid_argument("that many vertices or faces will not fit one unit");
    return coords;
}

BigUint model_count(uint32_t v, uint32_t f, uint32_t c)
{
    BigUint n(1);
    mul_pow(n, c, uint64_t(v) * 3);
    mul_pow(n, v, uint64_t(f) * 3);
    return n;
}

size_t width_of(const BigUint& n)
{
    BigUint top = n;
    top -= BigUint(1);
    return std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

} // namespace

ModelSpace::ModelSpace(uint32_t vertices, uint32_t faces, uint32_t coords, std::string key)
    : v_(vertices), f_(faces), c_(checked_coords(vertices, faces, coords)), decimals_(log2_of(coords)), key_(std::move(key)),
      vert_space_("models-coords/C" + std::to_string(coords), coords, 3 * vertices, key_),
      face_space_("models-faces/V" + std::to_string(vertices), vertices, 3 * faces, key_),
      size_(model_count(vertices, faces, coords)), hex_width_(width_of(size_)),
      shuffle_(size_, key_, shape_id(vertices, faces, coords, key_))
{
}

std::string ModelSpace::id() const { return shape_id(v_, f_, c_, key_); }

size_t ModelSpace::obj_length() const
{
    const size_t wc = size_t(decimals_) + 3;                      // " 0.1250"
    const size_t wi = std::to_string(uint64_t(v_)).size();        // the widest 1-based index
    return size_t(v_) * (2 + 3 * wc + 2 + 1) + size_t(f_) * (2 + 3 * wi + 2 + 1);
}

BigUint ModelSpace::index_of(const Parts& p, AddressMode m) const
{
    if (p.verts.size() != size_t(v_) * 3 || p.faces.size() != size_t(f_) * 3)
        throw std::invalid_argument("the model does not have this line's shape");
    BigUint value;
    auto put = [&](const Digits& d, uint32_t base) {
        const uint32_t k = chunk_digits(base);
        for (size_t i = 0; i < d.size();)
        {
            const size_t take = std::min<size_t>(k, d.size() - i);
            uint64_t chunk = 0;
            for (size_t j = 0; j < take; ++j, ++i)
            {
                if (d[i] >= base) throw std::invalid_argument("digit out of range");
                chunk = chunk * base + d[i];
            }
            value.mul_small(small_pow(base, uint32_t(take)));
            value.add_small(uint32_t(chunk));
        }
    };
    put(p.verts, c_);
    put(p.faces, v_);
    return m == AddressMode::Scrambled ? shuffle_.forward(value) : value;
}

ModelSpace::Parts ModelSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("model address beyond the line");
    BigUint value = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    Parts p;
    auto take = [&](Digits& d, uint32_t base, size_t length) { // least significant first
        const uint32_t k = chunk_digits(base);
        d.assign(length, 0);
        for (size_t i = d.size(); i > 0;)
        {
            const uint32_t n = uint32_t(std::min<size_t>(k, i));
            uint32_t r = value.divmod_small(small_pow(base, n));
            for (uint32_t j = 0; j < n; ++j)
            {
                d[--i] = r % base;
                r /= base;
            }
        }
    };
    take(p.faces, v_, size_t(f_) * 3);
    take(p.verts, c_, size_t(v_) * 3);
    return p;
}

BigUint ModelSpace::parse(std::string_view hex) const
{
    const BigUint value = BigUint::from_hex(hex);
    if (value >= size_) throw std::out_of_range("model address beyond the line");
    return value;
}

std::vector<ModelSpace::Vertex> ModelSpace::mesh_of(const Parts& p) const
{
    if (p.verts.size() != size_t(v_) * 3) throw std::invalid_argument("the model does not have this line's shape");
    std::vector<Vertex> out(v_);
    auto value = [&](uint32_t d) { return (2.0f * float(d) + 1.0f - float(c_)) / float(c_); };
    for (uint32_t i = 0; i < v_; ++i)
        out[i] = {value(p.verts[size_t(i) * 3]), value(p.verts[size_t(i) * 3 + 1]), value(p.verts[size_t(i) * 3 + 2])};
    return out;
}

std::vector<ModelSpace::Face> ModelSpace::faces_of(const Parts& p) const
{
    if (p.faces.size() != size_t(f_) * 3) throw std::invalid_argument("the model does not have this line's shape");
    std::vector<Face> out(f_);
    for (uint32_t i = 0; i < f_; ++i)
        out[i] = {p.faces[size_t(i) * 3], p.faces[size_t(i) * 3 + 1], p.faces[size_t(i) * 3 + 2]};
    return out;
}

std::string ModelSpace::to_obj(const Parts& p) const
{
    if (p.verts.size() != size_t(v_) * 3 || p.faces.size() != size_t(f_) * 3)
        throw std::invalid_argument("the model does not have this line's shape");
    const size_t wi = std::to_string(uint64_t(v_)).size();
    std::string out;
    out.reserve(obj_length());
    for (uint32_t i = 0; i < v_; ++i)
    {
        out += "v ";
        for (uint32_t k = 0; k < 3; ++k)
        {
            if (k) out += ' ';
            out += coord_text(p.verts[size_t(i) * 3 + k], c_, decimals_);
        }
        out += '\n';
    }
    for (uint32_t i = 0; i < f_; ++i)
    {
        out += "f ";
        for (uint32_t k = 0; k < 3; ++k)
        {
            if (k) out += ' ';
            out += index_text(p.faces[size_t(i) * 3 + k], wi);
        }
        out += '\n';
    }
    return out;
}

ModelSpace::Parts ModelSpace::from_obj(std::string_view obj) const
{
    const size_t wc = size_t(decimals_) + 3, wi = std::to_string(uint64_t(v_)).size();
    const size_t vline = 2 + 3 * wc + 2 + 1, fline = 2 + 3 * wi + 2 + 1;
    if (obj.size() != obj_length()) throw std::invalid_argument("that is not the canonical .obj text of this line's shape");
    Parts p;
    p.verts.resize(size_t(v_) * 3);
    p.faces.resize(size_t(f_) * 3);
    size_t at = 0;
    for (uint32_t i = 0; i < v_; ++i, at += vline)
    {
        const std::string_view line = obj.substr(at, vline);
        if (line.substr(0, 2) != "v " || line[vline - 1] != '\n') throw std::invalid_argument("a vertex line is not canonical");
        for (uint32_t k = 0; k < 3; ++k)
        {
            const size_t start = 2 + size_t(k) * (wc + 1);
            if (k && line[start - 1] != ' ') throw std::invalid_argument("a vertex line is not canonical");
            p.verts[size_t(i) * 3 + k] = coord_digit(line.substr(start, wc), c_, decimals_);
        }
    }
    for (uint32_t i = 0; i < f_; ++i, at += fline)
    {
        const std::string_view line = obj.substr(at, fline);
        if (line.substr(0, 2) != "f " || line[fline - 1] != '\n') throw std::invalid_argument("a face line is not canonical");
        for (uint32_t k = 0; k < 3; ++k)
        {
            const size_t start = 2 + size_t(k) * (wi + 1);
            if (k && line[start - 1] != ' ') throw std::invalid_argument("a face line is not canonical");
            p.faces[size_t(i) * 3 + k] = index_digit(line.substr(start, wi), v_);
        }
    }
    return p;
}

ModelSpace::Fitted ModelSpace::fit(const std::vector<Vertex>& verts, const std::vector<Face>& faces) const
{
    Fitted r;
    // Centre the mesh and scale it so its longest half-axis just reaches the edge of the grid.
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (const Vertex& v : verts)
    {
        const float p[3] = {v.x, v.y, v.z};
        for (int k = 0; k < 3; ++k)
        {
            lo[k] = std::min(lo[k], p[k]);
            hi[k] = std::max(hi[k], p[k]);
        }
    }
    float mid[3] = {0, 0, 0}, half = 0;
    if (!verts.empty())
        for (int k = 0; k < 3; ++k)
        {
            mid[k] = (lo[k] + hi[k]) * 0.5f;
            half = std::max(half, (hi[k] - lo[k]) * 0.5f);
        }
    r.scale = half > 1e-9f ? 1.0f / half : 1.0f;
    // The grid is C cells across [-1, 1], and digit d holds the middle of cell d, so a value x
    // falls in cell floor((x + 1) * C / 2).
    auto digit = [&](float x) {
        const float d = std::floor((x + 1.0f) * float(c_) * 0.5f);
        return uint32_t(std::clamp(d, 0.0f, float(c_ - 1)));
    };
    r.parts.verts.assign(size_t(v_) * 3, c_ / 2); // missing vertices sit at the origin
    const uint32_t take_v = uint32_t(std::min<size_t>(verts.size(), v_));
    for (uint32_t i = 0; i < take_v; ++i)
    {
        const float p[3] = {verts[i].x, verts[i].y, verts[i].z};
        for (int k = 0; k < 3; ++k) r.parts.verts[size_t(i) * 3 + size_t(k)] = digit((p[k] - mid[k]) * r.scale);
    }
    r.vertices_dropped = uint32_t(verts.size()) > v_ ? uint32_t(verts.size()) - v_ : 0;

    r.parts.faces.assign(size_t(f_) * 3, 0);
    const uint32_t take_f = uint32_t(std::min<size_t>(faces.size(), f_));
    for (uint32_t i = 0; i < take_f; ++i)
    {
        const uint32_t idx[3] = {faces[i].a, faces[i].b, faces[i].c};
        for (int k = 0; k < 3; ++k)
        {
            if (idx[k] >= v_) ++r.indices_clamped;
            r.parts.faces[size_t(i) * 3 + size_t(k)] = std::min(idx[k], v_ - 1);
        }
    }
    for (uint32_t i = take_f; i < f_; ++i) // a filler face, so a short mesh is still a mesh
    {
        r.parts.faces[size_t(i) * 3] = 0;
        r.parts.faces[size_t(i) * 3 + 1] = 1;
        r.parts.faces[size_t(i) * 3 + 2] = 2;
    }
    r.faces_dropped = uint32_t(faces.size()) > f_ ? uint32_t(faces.size()) - f_ : 0;
    r.faces_added = f_ - take_f;
    return r;
}

} // namespace sieve
