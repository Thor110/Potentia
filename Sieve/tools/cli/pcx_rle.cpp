#include "cli/pcx_rle.hpp"

#include <stdexcept>

namespace sieve::cli {

namespace {

struct Head
{
    uint32_t planes = 0, bpl = 0, rows = 0, width = 0;
};

// The header's planes, bytes a plane line and rows, where it is one this carries.
std::optional<Head> head_of(const uint8_t* p, size_t n)
{
    if (n < 128 || p[0] != 0x0A || p[1] > 5 || p[2] != 1 || p[3] != 8) return std::nullopt;
    const uint32_t y0 = uint32_t(p[6]) | uint32_t(p[7]) << 8, y1 = uint32_t(p[10]) | uint32_t(p[11]) << 8;
    const uint32_t x0 = uint32_t(p[4]) | uint32_t(p[5]) << 8, x1 = uint32_t(p[8]) | uint32_t(p[9]) << 8;
    Head h;
    h.planes = p[65];
    h.bpl = uint32_t(p[66]) | uint32_t(p[67]) << 8;
    if (y1 < y0 || h.bpl == 0 || (h.planes != 1 && h.planes != 3 && h.planes != 4)) return std::nullopt;
    h.rows = y1 - y0 + 1;
    h.width = x1 >= x0 ? x1 - x0 + 1 : 0;
    return h;
}

// A plane line coded: by coder 0 whole; by coder 1, its first `width` bytes, then each after them alone.
void encode_line(const uint8_t* row, size_t n, std::vector<uint8_t>& o, int coder, size_t width)
{
    if (coder == 1 && width < n)
    {
        encode_line(row, width, o, 0, 0);
        for (size_t k = width; k < n; ++k)
        {
            if (row[k] >= 0xC0) o.push_back(0xC1);
            o.push_back(row[k]);
        }
        return;
    }
    for (size_t i = 0; i < n;)
    {
        size_t j = i;
        while (j < n && row[j] == row[i] && j - i < 63) ++j;
        const size_t run = j - i;
        if (run > 1 || row[i] >= 0xC0) o.push_back(uint8_t(0xC0 | run));
        o.push_back(row[i]);
        i = j;
    }
}

} // namespace

bool is_pcx(const std::vector<uint8_t>& b) { return head_of(b.data(), b.size()).has_value(); }

std::vector<uint8_t> pcx_remake(const std::vector<uint8_t>& inner, size_t skeleton)
{
    if (skeleton > inner.size() || skeleton < 129) throw std::runtime_error("a PCX's skeleton this Sieve does not write");
    const size_t px = inner.size() - skeleton;
    const int coder = inner[px];
    const uint8_t* sk = inner.data() + px + 1;
    const auto h = head_of(sk, skeleton - 1);
    const uint64_t line = uint64_t(h ? h->planes : 0) * (h ? h->bpl : 0);
    if (!h || coder > 1 || (coder == 1 && h->width >= h->bpl) || line * h->rows != px) throw std::runtime_error("a PCX's skeleton this Sieve does not write");
    std::vector<uint8_t> out(sk, sk + 128), plane(h->bpl);
    for (uint64_t r = 0; r < h->rows; ++r)
    {
        const uint8_t* at = inner.data() + r * line;
        for (uint32_t k = 0; k < h->planes; ++k)
        {
            for (uint32_t x = 0; x < h->bpl; ++x) plane[x] = at[uint64_t(x) * h->planes + k];
            encode_line(plane.data(), plane.size(), out, coder, h->width);
        }
    }
    out.insert(out.end(), sk + 128, sk + skeleton - 1);
    return out;
}

std::optional<PcxOpened> pcx_open(const std::vector<uint8_t>& b)
{
    const auto h = head_of(b.data(), b.size());
    if (!h) return std::nullopt;
    const uint64_t line = uint64_t(h->planes) * h->bpl, need = line * h->rows;
    if (need == 0 || need > (uint64_t(1) << 31)) return std::nullopt;
    // Decoded, plane line by plane line: no run may cross one.
    std::vector<uint8_t> planar;
    planar.reserve(size_t(need));
    size_t at = 128;
    for (uint64_t pl = 0; pl < uint64_t(h->rows) * h->planes; ++pl)
    {
        const size_t end = planar.size() + h->bpl;
        while (planar.size() < end)
        {
            if (at >= b.size()) return std::nullopt;
            const uint8_t c = b[at++];
            if (c >= 0xC0)
            {
                const size_t k = c & 0x3F;
                if (at >= b.size() || k == 0 || planar.size() + k > end) return std::nullopt;
                planar.insert(planar.end(), k, b[at++]);
            }
            else
                planar.push_back(c);
        }
    }
    PcxOpened o;
    o.inner.resize(size_t(need));
    for (uint64_t r = 0; r < h->rows; ++r)
        for (uint32_t k = 0; k < h->planes; ++k)
            for (uint32_t x = 0; x < h->bpl; ++x) o.inner[size_t(r * line + uint64_t(x) * h->planes + k)] = planar[size_t(r * line + uint64_t(k) * h->bpl + x)];
    const size_t skeleton = 129 + (b.size() - at), pixels = o.inner.size();
    o.inner.push_back(0);
    o.inner.insert(o.inner.end(), b.begin(), b.begin() + 128);
    o.inner.insert(o.inner.end(), b.begin() + std::ptrdiff_t(at), b.end());
    // Made again and compared, by each coder, before it is ever chosen.
    bool same = false;
    for (int coder = 0; coder <= (h->width < h->bpl ? 1 : 0) && !same; ++coder)
        try
        {
            o.inner[pixels] = uint8_t(coder);
            same = pcx_remake(o.inner, skeleton) == b;
        }
        catch (const std::exception&)
        {
        }
    if (!same) return std::nullopt;
    o.recipe = "pcx " + std::to_string(skeleton) + " " + std::to_string(o.inner.size());
    o.width = h->bpl;
    o.skeleton = skeleton;
    o.planes = h->planes;
    return o;
}

std::optional<std::pair<uint32_t, uint32_t>> pcx_shape(const std::vector<uint8_t>& inner, size_t skeleton)
{
    if (skeleton > inner.size()) return std::nullopt;
    if (skeleton < 129) return std::nullopt;
    const auto h = head_of(inner.data() + inner.size() - skeleton + 1, skeleton - 1);
    if (!h || h->bpl < 2 || h->bpl > (1u << 24)) return std::nullopt;
    return std::pair<uint32_t, uint32_t>{h->bpl, h->planes};
}

} // namespace sieve::cli
