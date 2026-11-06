// A GIF carried as its skeleton and each frame's colour indices (gif_lzw.hpp).

#include "cli/gif_lzw.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace sieve::cli {

namespace {

// (Most indices one GIF may open to: a frame of one colour codes to almost nothing, so a small file
// could otherwise ask for gigabytes.)
constexpr uint64_t kMostIndices = uint64_t(1) << 30;

uint16_t le16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }

// Each frame's LZW undone: its indices put after what `out` holds, or false where the stream is not
// one this reads (a code not yet in the table, no end code, more or fewer indices than the frame
// holds); `out` is then as it was. (Onto the end of one vector, every frame's: review, 6 November
// 2026, each frame held apart and then copied had held them all twice.)
bool lzw_decode(int mcs, const std::vector<uint8_t>& data, uint64_t pixels, std::vector<uint8_t>& out)
{
    const size_t base = out.size();
    auto fail = [&] {
        out.resize(base);
        return false;
    };
    const uint32_t clear = 1u << mcs, eoi = clear + 1;
    std::array<uint16_t, 4096> prefix{};
    std::array<uint8_t, 4096> suffix{}, first{};
    std::array<uint16_t, 4096> length{};
    for (uint32_t i = 0; i < clear; ++i) suffix[i] = first[i] = uint8_t(i), length[i] = 1;
    int nbits = mcs + 1;
    uint32_t next = clear + 2;
    int32_t prev = -1;
    uint64_t bit = 0;
    const uint64_t bits = uint64_t(data.size()) * 8;
    auto emit = [&](uint32_t code) {
        const size_t n = length[code];
        if (out.size() - base + n > pixels) return false;
        out.resize(out.size() + n);
        uint8_t* p = out.data() + out.size();
        for (uint32_t c = code;; c = prefix[c])
        {
            *--p = suffix[c];
            if (c < clear) break;
        }
        return true;
    };
    for (;;)
    {
        if (bit + uint64_t(nbits) > bits) return fail(); // (no end code)
        uint32_t code = 0;
        for (int i = 0; i < nbits; ++i, ++bit) code |= uint32_t(data[bit >> 3] >> (bit & 7) & 1) << i;
        if (code == clear)
        {
            nbits = mcs + 1;
            next = clear + 2;
            prev = -1;
            continue;
        }
        if (code == eoi) break;
        if (prev < 0)
        {
            if (code >= clear || !emit(code)) return fail();
            prev = int32_t(code);
            continue;
        }
        if (code < next && code != clear && code != eoi)
        {
            if (!emit(code)) return fail();
            if (next < 4096)
            {
                prefix[next] = uint16_t(prev), suffix[next] = first[code], first[next] = first[prev], length[next] = uint16_t(length[prev] + 1);
                ++next;
            }
        }
        else if (code == next && next < 4096)
        {
            prefix[next] = uint16_t(prev), suffix[next] = first[prev], first[next] = first[prev], length[next] = uint16_t(length[prev] + 1);
            ++next;
            if (!emit(code)) return fail();
        }
        else return fail();
        prev = int32_t(code);
        if (next == (1u << nbits) && nbits < 12) ++nbits;
    }
    if (out.size() - base != pixels) return fail();
    return true;
}

// A frame's indices coded as compress codes them (Weiner's LZWEncoder, as gifenc has it): the code
// width grows once the next free code passes the widest, a clear code (and a new table) when it
// would pass 4096. Each index below 1 << mcs (the caller sees to it).
void lzw_encode(int mcs, const uint8_t* px, size_t n, std::vector<uint8_t>& out)
{
    const int init = mcs + 1;
    const uint32_t clear = 1u << mcs, eoi = clear + 1;
    uint32_t acc = 0;
    int accbits = 0, nbits = init;
    uint32_t maxcode = (1u << nbits) - 1, free = clear + 2;
    bool clear_flg = false;
    // (The table, a string's code by its prefix's code and its last index: open addressing, 8192
    // places for at most 4096, emptied on a clear.)
    constexpr uint32_t kSlots = 8192;
    std::vector<uint32_t> keys(kSlots, 0); // (key + 1; 0 an empty place)
    std::vector<uint16_t> codes(kSlots, 0);
    auto output = [&](uint32_t code) {
        acc |= code << accbits;
        accbits += nbits;
        while (accbits >= 8)
        {
            out.push_back(uint8_t(acc));
            acc >>= 8;
            accbits -= 8;
        }
        if (free > maxcode || clear_flg)
        {
            if (clear_flg)
            {
                nbits = init;
                maxcode = (1u << nbits) - 1;
                clear_flg = false;
            }
            else
            {
                ++nbits;
                maxcode = nbits == 12 ? 4096 : (1u << nbits) - 1;
            }
        }
        if (code == eoi)
            while (accbits > 0)
            {
                out.push_back(uint8_t(acc));
                acc >>= 8;
                accbits -= 8;
            }
    };
    output(clear);
    uint32_t ent = px[0];
    for (size_t i = 1; i < n; ++i)
    {
        const uint32_t key = (ent << 8 | px[i]) + 1;
        uint32_t at = (key * 2654435761u) >> 19;
        while (keys[at] && keys[at] != key) at = (at + 1) & (kSlots - 1);
        if (keys[at] == key)
        {
            ent = codes[at];
            continue;
        }
        output(ent);
        ent = px[i];
        if (free < 4096)
        {
            keys[at] = key;
            codes[at] = uint16_t(free++);
        }
        else
        {
            std::fill(keys.begin(), keys.end(), 0u);
            free = clear + 2;
            clear_flg = true;
            output(clear);
        }
    }
    output(ent);
    output(eoi);
}

[[noreturn]] void bad(const char* why) { throw std::runtime_error(std::string("a GIF's skeleton ") + why); }

// A skeleton's blocks walked, as the remaker and the shape both walk them: each frame handed to
// on_frame(where its LZW code size byte ends, its width, its height, that code size), in order.
// Returns where the trailer starts; throws on anything not as gif_open writes it.
template <class OnFrame> size_t walk_skeleton(const uint8_t* s, size_t n, OnFrame on_frame)
{
    if (n < 13 || (std::memcmp(s, "GIF87a", 6) != 0 && std::memcmp(s, "GIF89a", 6) != 0)) bad("is not a GIF's");
    size_t at = 13;
    auto need = [&](size_t k) {
        if (at > n || n - at < k) bad("ends early");
    };
    if (s[10] & 0x80) at += size_t(3) << ((s[10] & 7) + 1);
    for (;;)
    {
        need(1);
        const uint8_t t = s[at];
        if (t == 0x3B) return at;
        if (t == 0x21)
        {
            need(2);
            at += 2;
            for (;;)
            {
                need(1);
                if (!s[at]) break;
                at += 1 + size_t(s[at]);
            }
            ++at;
        }
        else if (t == 0x2C)
        {
            need(11);
            const uint32_t w = le16(s + at + 5), h = le16(s + at + 7);
            const uint8_t f = s[at + 9];
            at += 10;
            if (f & 0x80) at += size_t(3) << ((f & 7) + 1);
            need(1);
            const int mcs = s[at++];
            if (mcs < 2 || mcs > 8) bad("names an LZW code size GIFs do not have");
            if (!w || !h) bad("holds a frame of no pixels");
            on_frame(at, w, h, mcs);
        }
        else bad("holds a block GIFs do not have");
    }
}

} // namespace

bool is_gif(const std::vector<uint8_t>& b)
{
    return b.size() >= 13 && (std::memcmp(b.data(), "GIF87a", 6) == 0 || std::memcmp(b.data(), "GIF89a", 6) == 0);
}

std::vector<uint8_t> gif_remake(const std::vector<uint8_t>& inner, size_t skeleton, size_t block)
{
    if (skeleton > inner.size() || skeleton < 13 || block < 1 || block > 255) bad("does not fit its recipe");
    const size_t indices = inner.size() - skeleton;
    const uint8_t* s = inner.data() + indices;
    std::vector<uint8_t> out;
    out.reserve(inner.size());
    size_t copied = 0, idx = 0; // (the skeleton's bytes before copied are in out already; the indices before idx used)
    std::vector<uint8_t> lzw;
    const size_t trailer = walk_skeleton(s, skeleton, [&](size_t at, uint32_t w, uint32_t h, int mcs) {
        const uint64_t pixels = uint64_t(w) * h;
        if (indices - idx < pixels) bad("asks for more indices than are carried");
        const uint8_t* px = inner.data() + idx;
        for (uint64_t k = 0; k < pixels; ++k)
            if (px[k] >> mcs) bad("carries an index past its frame's colours");
        out.insert(out.end(), s + copied, s + at);
        copied = at;
        lzw.clear();
        lzw_encode(mcs, px, size_t(pixels), lzw);
        idx += size_t(pixels);
        for (size_t k = 0; k < lzw.size(); k += block)
        {
            const size_t m = std::min(block, lzw.size() - k);
            out.push_back(uint8_t(m));
            out.insert(out.end(), lzw.begin() + std::ptrdiff_t(k), lzw.begin() + std::ptrdiff_t(k + m));
        }
        out.push_back(0);
    });
    (void)trailer;
    if (idx != indices) bad("leaves indices over");
    out.insert(out.end(), s + copied, s + skeleton); // (the trailer, and anything after it, as it was)
    return out;
}

std::optional<uint32_t> gif_width(const std::vector<uint8_t>& inner, size_t skeleton)
{
    if (skeleton > inner.size()) return std::nullopt;
    uint32_t width = 0;
    bool same = true;
    try
    {
        walk_skeleton(inner.data() + inner.size() - skeleton, skeleton, [&](size_t, uint32_t w, uint32_t, int) {
            same = same && (!width || w == width);
            width = w;
        });
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
    if (!same || width < 2) return std::nullopt;
    return width;
}

std::optional<GifOpened> gif_open(const std::vector<uint8_t>& b)
{
    if (!is_gif(b)) return std::nullopt;
    const size_t n = b.size();
    size_t at = 13;
    if (b[10] & 0x80) at += size_t(3) << ((b[10] & 7) + 1);
    std::vector<uint8_t> skeleton;
    std::vector<uint8_t> indices; // (every frame's, one after another)
    size_t frames = 0;
    size_t copied = 0;
    int block = 0; // (the sub-block size every frame cuts its LZW into, the last cut shorter)
    uint64_t total = 0;
    const uint64_t most = std::min<uint64_t>(kMostIndices, uint64_t(n) * 1024);
    for (;;)
    {
        if (at >= n) return std::nullopt; // (no trailer)
        const uint8_t t = b[at];
        if (t == 0x3B) break;
        if (t == 0x21)
        {
            at += 2;
            for (;;)
            {
                if (at >= n) return std::nullopt;
                if (!b[at]) break;
                at += 1 + size_t(b[at]);
            }
            ++at;
        }
        else if (t == 0x2C)
        {
            if (n - at < 11) return std::nullopt;
            const uint64_t pixels = uint64_t(le16(&b[at + 5])) * le16(&b[at + 7]);
            const uint8_t f = b[at + 9];
            at += 10;
            if (f & 0x80) at += size_t(3) << ((f & 7) + 1);
            if (at >= n) return std::nullopt;
            const int mcs = b[at++];
            if (mcs < 2 || mcs > 8 || pixels == 0 || (total += pixels) > most) return std::nullopt;
            skeleton.insert(skeleton.end(), b.begin() + std::ptrdiff_t(copied), b.begin() + std::ptrdiff_t(at));
            std::vector<uint8_t> data;
            std::vector<uint8_t> sizes;
            for (;;)
            {
                if (at >= n) return std::nullopt;
                const size_t len = b[at];
                if (!len) break;
                if (n - at - 1 < len) return std::nullopt;
                sizes.push_back(uint8_t(len));
                data.insert(data.end(), b.begin() + std::ptrdiff_t(at + 1), b.begin() + std::ptrdiff_t(at + 1 + len));
                at += 1 + len;
            }
            ++at;
            copied = at;
            if (sizes.size() > 1 && !block) block = sizes[0];
            if (!lzw_decode(mcs, data, pixels, indices)) return std::nullopt;
            ++frames;
        }
        else return std::nullopt;
    }
    if (frames == 0) return std::nullopt;
    skeleton.insert(skeleton.end(), b.begin() + std::ptrdiff_t(copied), b.end());
    if (!block) block = 255;
    GifOpened g;
    g.frames = frames;
    const size_t skeleton_size = skeleton.size();
    g.inner = std::move(indices);
    g.inner.insert(g.inner.end(), skeleton.begin(), skeleton.end());
    // (Given back exactly, or not taken: the one check that the GIF was coded as compress codes.)
    try
    {
        if (gif_remake(g.inner, skeleton_size, size_t(block)) != b) return std::nullopt;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
    g.recipe = "gif " + std::to_string(block) + " " + std::to_string(skeleton_size) + " " + std::to_string(g.inner.size());
    return g;
}

} // namespace sieve::cli
