// Sieve — a JPEG at an eighth of its size, from each block's average alone (see jpeg_dc.hpp).
//
// ITU T.81: the markers read are SOF0, SOF1 and SOF2 (the frame), DHT (Huffman tables), DQT (the
// quantisers, of which only each table's first value is used), DRI (the restart interval) and SOS
// (a scan); the rest are stepped over by their lengths.
#include "sieve/jpeg_dc.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace sieve {

namespace {

struct Huffman
{
    bool defined = false;
    // Codes of each length, as T.81 Annex C makes them: the first code of each length, how many,
    // and where their values begin.
    std::array<int32_t, 17> maxcode{}, mincode{};
    std::array<int32_t, 17> valptr{};
    std::vector<uint8_t> values;
    // A look-up of the next 9 bits: the value and its length, where the code is that short.
    std::array<uint8_t, 512> fast_len{}, fast_val{};
};

bool build(Huffman& h, const uint8_t counts[16], std::vector<uint8_t> values)
{
    h = Huffman{};
    h.values = std::move(values);
    int32_t code = 0, k = 0;
    for (int len = 1; len <= 16; ++len)
    {
        const int n = counts[len - 1];
        h.valptr[size_t(len)] = k;
        h.mincode[size_t(len)] = code;
        code += n;
        k += n;
        h.maxcode[size_t(len)] = n ? code - 1 : -1;
        if (len <= 9)
            for (int c = code - n; c < code; ++c)
                for (int fill = 0; fill < (1 << (9 - len)); ++fill)
                {
                    const size_t at = (size_t(c) << (9 - len)) | size_t(fill);
                    if (at >= 512) return false;
                    h.fast_len[at] = uint8_t(len);
                    h.fast_val[at] = h.values[size_t(h.valptr[size_t(len)] + (c - h.mincode[size_t(len)]))];
                }
        if (code > (1 << len)) return false;
        code <<= 1;
    }
    if (size_t(k) != h.values.size()) return false;
    h.defined = true;
    return true;
}

// The scan's bits: stuffed zero bytes after FF taken out; at a marker, zeros (and `marker` set).
struct Bits
{
    std::span<const uint8_t> d;
    size_t at = 0;
    uint64_t buf = 0;
    int have = 0;
    bool marker = false;

    void fill()
    {
        while (have <= 56)
        {
            uint8_t b = 0;
            if (!marker && at < d.size())
            {
                b = d[at];
                if (b == 0xFF)
                {
                    const uint8_t next = at + 1 < d.size() ? d[at + 1] : 0;
                    if (next == 0x00) at += 2;
                    else
                    {
                        marker = true;
                        b = 0;
                    }
                }
                else ++at;
            }
            buf |= uint64_t(b) << (56 - have);
            have += 8;
        }
    }
    uint32_t peek(int n)
    {
        if (have < n) fill();
        return uint32_t(buf >> (64 - n));
    }
    void skip(int n)
    {
        buf <<= n;
        have -= n;
    }
    uint32_t get(int n)
    {
        if (n == 0) return 0;
        const uint32_t v = peek(n);
        skip(n);
        return v;
    }
    // Byte-aligned again, for a restart: the bits left in this byte dropped, then the RST marker.
    bool restart()
    {
        buf = 0;
        have = 0;
        // (past any fill bytes FF to the marker itself)
        while (at + 1 < d.size() && !(d[at] == 0xFF && d[at + 1] >= 0xD0 && d[at + 1] <= 0xD7)) ++at;
        if (at + 1 >= d.size()) return false;
        at += 2;
        marker = false;
        return true;
    }
};

int decode(Bits& b, const Huffman& h)
{
    const uint32_t look = b.peek(9);
    if (const uint8_t len = h.fast_len[look]) // (its value where the code is 9 bits or fewer)
    {
        b.skip(len);
        return h.fast_val[look];
    }
    const uint32_t code16 = b.peek(16);
    for (int len = 10; len <= 16; ++len)
    {
        const int32_t code = int32_t(code16 >> (16 - len));
        if (h.maxcode[size_t(len)] >= 0 && code <= h.maxcode[size_t(len)] && code >= h.mincode[size_t(len)])
        {
            b.skip(len);
            return h.values[size_t(h.valptr[size_t(len)] + code - h.mincode[size_t(len)])];
        }
    }
    return -1;
}

int32_t extend(uint32_t v, int s) { return s == 0 ? 0 : (v < (1u << (s - 1)) ? int32_t(v) - (1 << s) + 1 : int32_t(v)); }

struct Component
{
    int id = 0, h = 1, v = 1, tq = 0;
    uint32_t bw = 0, bh = 0;       // its blocks across and down, padded to whole MCUs
    uint32_t own_bw = 0, own_bh = 0; // its own (a scan of it alone has these)
    std::vector<int32_t> dc;       // each block's DC, quantised
};

struct Frame
{
    bool ok = false, progressive = false;
    uint32_t width = 0, height = 0;
    int hmax = 1, vmax = 1;
    uint32_t mcux = 0, mcuy = 0;
    std::vector<Component> comps;
};

uint32_t be16(std::span<const uint8_t> d, size_t at) { return uint32_t(d[at]) << 8 | d[at + 1]; }

bool read_frame(std::span<const uint8_t> seg, uint8_t marker, Frame& f)
{
    if (seg.size() < 6) return false;
    if (seg[0] != 8) return false; // (8-bit samples only)
    f.height = be16(seg, 1);
    f.width = be16(seg, 3);
    const size_t n = seg[5];
    if ((n != 1 && n != 3) || seg.size() < 6 + 3 * n || f.width == 0 || f.height == 0) return false;
    f.progressive = marker == 0xC2;
    f.comps.resize(n);
    for (size_t k = 0; k < n; ++k)
    {
        Component& c = f.comps[k];
        c.id = seg[6 + 3 * k];
        c.h = seg[7 + 3 * k] >> 4;
        c.v = seg[7 + 3 * k] & 15;
        c.tq = seg[8 + 3 * k];
        if (c.h < 1 || c.h > 4 || c.v < 1 || c.v > 4 || c.tq > 3) return false;
        f.hmax = std::max(f.hmax, c.h);
        f.vmax = std::max(f.vmax, c.v);
    }
    f.mcux = (f.width + 8 * uint32_t(f.hmax) - 1) / (8 * uint32_t(f.hmax));
    f.mcuy = (f.height + 8 * uint32_t(f.vmax) - 1) / (8 * uint32_t(f.vmax));
    for (Component& c : f.comps)
    {
        c.bw = f.mcux * uint32_t(c.h);
        c.bh = f.mcuy * uint32_t(c.v);
        const uint32_t cw = (f.width * uint32_t(c.h) + uint32_t(f.hmax) - 1) / uint32_t(f.hmax);
        const uint32_t ch = (f.height * uint32_t(c.v) + uint32_t(f.vmax) - 1) / uint32_t(f.vmax);
        c.own_bw = (cw + 7) / 8;
        c.own_bh = (ch + 7) / 8;
        c.dc.assign(size_t(c.bw) * c.bh, 0);
    }
    f.ok = true;
    return true;
}

// Steps past a scan's coded data to the next marker that is not a restart (where it begins).
size_t past_scan(std::span<const uint8_t> d, size_t at)
{
    while (at + 1 < d.size())
    {
        if (d[at] == 0xFF && d[at + 1] != 0x00 && !(d[at + 1] >= 0xD0 && d[at + 1] <= 0xD7) && d[at + 1] != 0xFF) return at;
        ++at;
    }
    return d.size();
}

} // namespace

std::optional<std::pair<uint32_t, uint32_t>> jpeg_size(std::span<const uint8_t> d)
{
    if (d.size() < 4 || d[0] != 0xFF || d[1] != 0xD8) return std::nullopt;
    size_t at = 2;
    while (at + 4 <= d.size())
    {
        if (d[at] != 0xFF) return std::nullopt;
        const uint8_t m = d[at + 1];
        if (m == 0xFF)
        {
            ++at;
            continue;
        }
        const size_t len = be16(d, at + 2);
        if (len < 2 || at + 2 + len > d.size()) return std::nullopt;
        if (m == 0xC0 || m == 0xC1 || m == 0xC2)
        {
            Frame f;
            if (!read_frame(d.subspan(at + 4, len - 2), m, f)) return std::nullopt;
            return std::make_pair(f.width, f.height);
        }
        if (m == 0xDA || m == 0xD9) return std::nullopt;
        at += 2 + len;
    }
    return std::nullopt;
}

std::optional<EighthJpeg> jpeg_eighth(std::span<const uint8_t> d)
{
    if (d.size() < 4 || d[0] != 0xFF || d[1] != 0xD8) return std::nullopt;
    Frame f;
    std::array<Huffman, 4> dc_tables, ac_tables;
    std::array<int32_t, 4> quant0{1, 1, 1, 1};
    uint32_t restart_interval = 0;
    bool any_dc = false;
    size_t at = 2;
    while (at + 4 <= d.size())
    {
        if (d[at] != 0xFF) return std::nullopt;
        const uint8_t m = d[at + 1];
        if (m == 0xFF)
        {
            ++at;
            continue;
        }
        if (m == 0xD9) break; // (the end)
        const size_t len = be16(d, at + 2);
        if (len < 2 || at + 2 + len > d.size()) return std::nullopt;
        const std::span<const uint8_t> seg = d.subspan(at + 4, len - 2);
        at += 2 + len;
        if (m == 0xC0 || m == 0xC1 || m == 0xC2)
        {
            if (f.ok || !read_frame(seg, m, f)) return std::nullopt;
            // (a sixty-fourth of the pixels for each component, padded to whole MCUs: held to 1 GiB)
            uint64_t blocks = 0;
            for (const Component& c : f.comps) blocks += uint64_t(c.bw) * c.bh;
            if (blocks * 4 > (uint64_t(1) << 30)) return std::nullopt;
        }
        else if ((m >= 0xC3 && m <= 0xCF) && m != 0xC4 && m != 0xC8 && m != 0xCC) return std::nullopt; // (lossless, hierarchical, arithmetic)
        else if (m == 0xC4)
        {
            for (size_t p = 0; p < seg.size();)
            {
                if (p + 17 > seg.size()) return std::nullopt;
                const int tc = seg[p] >> 4, th = seg[p] & 15;
                if (tc > 1 || th > 3) return std::nullopt;
                size_t total = 0;
                for (int k = 0; k < 16; ++k) total += seg[p + 1 + size_t(k)];
                if (p + 17 + total > seg.size()) return std::nullopt;
                std::vector<uint8_t> values(seg.begin() + std::ptrdiff_t(p + 17), seg.begin() + std::ptrdiff_t(p + 17 + total));
                if (!build((tc == 0 ? dc_tables : ac_tables)[size_t(th)], &seg[p + 1], std::move(values))) return std::nullopt;
                p += 17 + total;
            }
        }
        else if (m == 0xDB)
        {
            for (size_t p = 0; p < seg.size();)
            {
                const int pq = seg[p] >> 4, tq = seg[p] & 15;
                if (tq > 3 || pq > 1) return std::nullopt;
                const size_t n = pq ? 128 : 64;
                if (p + 1 + n > seg.size()) return std::nullopt;
                quant0[size_t(tq)] = pq ? int32_t(be16(seg, p + 1)) : seg[p + 1];
                p += 1 + n;
            }
        }
        else if (m == 0xDD)
        {
            if (seg.size() < 2) return std::nullopt;
            restart_interval = be16(seg, 0);
        }
        else if (m == 0xDA)
        {
            if (!f.ok || seg.empty()) return std::nullopt;
            const size_t ns = seg[0];
            if (ns < 1 || ns > 4 || seg.size() < 1 + 2 * ns + 3) return std::nullopt;
            std::vector<Component*> in;
            std::vector<int> td, ta;
            for (size_t k = 0; k < ns; ++k)
            {
                const int cs = seg[1 + 2 * k];
                auto it = std::find_if(f.comps.begin(), f.comps.end(), [&](const Component& c) { return c.id == cs; });
                if (it == f.comps.end()) return std::nullopt;
                in.push_back(&*it);
                td.push_back(seg[2 + 2 * k] >> 4);
                ta.push_back(seg[2 + 2 * k] & 15);
                if (td.back() > 3 || ta.back() > 3) return std::nullopt;
            }
            const int ss = seg[1 + 2 * ns], se = seg[2 + 2 * ns], ah = seg[3 + 2 * ns] >> 4, al = seg[3 + 2 * ns] & 15;
            const size_t data_start = at, data_end = past_scan(d, at);
            at = data_end;
            // A progressive scan of the other coefficients: stepped over.
            if (f.progressive && ss != 0) continue;
            if (f.progressive && se != 0) return std::nullopt; // (a DC scan has Ss = Se = 0)
            for (size_t k = 0; k < ns; ++k)
                if ((!(f.progressive && ah != 0) && !dc_tables[size_t(td[k])].defined) || (!f.progressive && !ac_tables[size_t(ta[k])].defined)) return std::nullopt;
            Bits b{d.subspan(0, data_end), data_start};
            std::vector<int32_t> pred(ns, 0);
            // The blocks in the scan's order: an MCU of each component's H by V blocks where it holds
            // several, else the one component's own blocks, row by row.
            auto one_block = [&](size_t k, uint32_t bx, uint32_t by) -> bool {
                Component& c = *in[k];
                int32_t* dc = (bx < c.bw && by < c.bh) ? &c.dc[size_t(by) * c.bw + bx] : nullptr;
                if (f.progressive && ah != 0)
                {
                    if (dc && b.get(1)) *dc |= 1 << al; // (a refinement: one bit more of each)
                    return true;
                }
                const int t = decode(b, dc_tables[size_t(td[k])]);
                if (t < 0 || t > 16) return false;
                pred[k] += extend(b.get(t), t);
                if (dc) *dc = f.progressive ? pred[k] * (1 << al) : pred[k];
                if (f.progressive) return true;
                for (int z = 1; z < 64;) // (the other coefficients' codes, read past)
                {
                    const int rs = decode(b, ac_tables[size_t(ta[k])]);
                    if (rs < 0) return false;
                    const int r = rs >> 4, s = rs & 15;
                    if (s == 0)
                    {
                        if (r != 15) break;
                        z += 16;
                        continue;
                    }
                    b.get(s);
                    z += r + 1;
                }
                return true;
            };
            uint32_t units = 0; // (MCUs since the last restart)
            auto restart = [&]() -> bool {
                if (!restart_interval || units < restart_interval) return true;
                units = 0;
                std::fill(pred.begin(), pred.end(), 0);
                return b.restart();
            };
            if (ns > 1)
            {
                for (uint32_t my = 0; my < f.mcuy; ++my)
                    for (uint32_t mx = 0; mx < f.mcux; ++mx)
                    {
                        if (!restart()) return std::nullopt;
                        for (size_t k = 0; k < ns; ++k)
                            for (int v = 0; v < in[k]->v; ++v)
                                for (int h = 0; h < in[k]->h; ++h)
                                    if (!one_block(k, mx * uint32_t(in[k]->h) + uint32_t(h), my * uint32_t(in[k]->v) + uint32_t(v))) return std::nullopt;
                        ++units;
                    }
            }
            else
            {
                const Component& c = *in[0];
                for (uint32_t by = 0; by < c.own_bh; ++by)
                    for (uint32_t bx = 0; bx < c.own_bw; ++bx)
                    {
                        if (!restart()) return std::nullopt;
                        if (!one_block(0, bx, by)) return std::nullopt;
                        ++units;
                    }
            }
            any_dc = true;
        }
        else if (m >= 0xD0 && m <= 0xD7) continue;
    }
    if (!f.ok || !any_dc) return std::nullopt;
    // Each block's average: DC times its quantiser, over 8, plus 128; then the picture's colours.
    EighthJpeg out;
    out.width = f.width;
    out.height = f.height;
    out.eighth_w = (f.width + 7) / 8;
    out.eighth_h = (f.height + 7) / 8;
    out.rgba.assign(size_t(out.eighth_w) * out.eighth_h * 4, 255);
    // (In integers, so every compiler gives the same pixels: a level times 8 is DC times its
    // quantiser plus 1024; JFIF's colours by its constants times 65536, rounded to the nearest.)
    auto level8 = [&](const Component& c, uint32_t x, uint32_t y) -> int64_t {
        // (x, y: a block at the picture's full resolution; this component's block over it)
        const uint32_t bx = std::min(c.bw - 1, x * uint32_t(c.h) / uint32_t(f.hmax)), by = std::min(c.bh - 1, y * uint32_t(c.v) / uint32_t(f.vmax));
        return int64_t(c.dc[size_t(by) * c.bw + bx]) * quant0[size_t(c.tq)] + 1024;
    };
    auto to8 = [](int64_t v65536x8) { // (a value times 8 times 65536, to 0 .. 255, nearest)
        const int64_t d = int64_t(8) << 16;
        const int64_t q = v65536x8 >= 0 ? (v65536x8 + d / 2) / d : -((-v65536x8 + d / 2) / d);
        return uint8_t(std::clamp<int64_t>(q, 0, 255));
    };
    for (uint32_t y = 0; y < out.eighth_h; ++y)
        for (uint32_t x = 0; x < out.eighth_w; ++x)
        {
            uint8_t* p = &out.rgba[(size_t(y) * out.eighth_w + x) * 4];
            const int64_t yy = level8(f.comps[0], x, y) << 16;
            if (f.comps.size() == 1)
            {
                p[0] = p[1] = p[2] = to8(yy);
                continue;
            }
            const int64_t cb = level8(f.comps[1], x, y) - 1024, cr = level8(f.comps[2], x, y) - 1024; // (JFIF's YCbCr)
            p[0] = to8(yy + 91881 * cr);
            p[1] = to8(yy - 22554 * cb - 46802 * cr);
            p[2] = to8(yy + 116130 * cb);
        }
    return out;
}

} // namespace sieve
