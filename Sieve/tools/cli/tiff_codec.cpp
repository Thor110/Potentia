#include "cli/tiff_codec.hpp"

#include <algorithm>

namespace sieve::cli {

namespace {

constexpr int kClear = 256, kEnd = 257, kFirst = 258, kCodeMax = 4095, kCheckGap = 10000;

struct BitsOut
{
    std::vector<uint8_t> out;
    uint64_t data = 0;
    int bits = 0;
    void put(int code, int width)
    {
        data = (data << width) | uint64_t(code);
        bits += width;
        while (bits >= 8)
        {
            out.push_back(uint8_t(data >> (bits - 8)));
            bits -= 8;
        }
    }
    void finish()
    {
        if (bits > 0) out.push_back(uint8_t(data << (8 - bits)));
        bits = 0;
    }
};

} // namespace

std::vector<uint8_t> tiff_lzw_encode(const uint8_t* p, size_t n)
{
    // The table: (string's code, next byte) to its code, -1 for none; the slots used, to empty it.
    std::vector<int16_t> table(size_t(kCodeMax + 1) * 256, -1);
    std::vector<uint32_t> used;
    used.reserve(kCodeMax);
    auto empty = [&] {
        for (const uint32_t u : used) table[u] = -1;
        used.clear();
    };
    BitsOut o;
    int width = 9, max_code = 511, free_code = kFirst;
    int64_t incount = 0, outcount = 0, checkpoint = kCheckGap, ratio = 0;
    auto put = [&](int code) {
        o.put(code, width);
        outcount += width;
    };
    auto clear = [&] {
        empty();
        ratio = 0;
        incount = 0;
        outcount = 0;
        free_code = kFirst;
        put(kClear);
        width = 9;
        max_code = 511;
    };
    int ent = -1;
    if (n > 0)
    {
        put(kClear);
        ent = p[0];
        incount = 1;
    }
    for (size_t i = 1; i < n; ++i)
    {
        const int c = p[i];
        ++incount;
        const uint32_t slot = uint32_t(ent) * 256 + uint32_t(c);
        if (table[slot] >= 0)
        {
            ent = table[slot];
            continue;
        }
        put(ent);
        ent = c;
        table[slot] = int16_t(free_code++);
        used.push_back(slot);
        if (free_code == kCodeMax - 1) clear();
        else if (free_code > max_code)
        {
            ++width;
            max_code = (1 << width) - 1;
        }
        else if (incount >= checkpoint)
        {
            checkpoint = incount + kCheckGap;
            int64_t rat;
            if (incount > 0x007fffff)
            {
                rat = outcount >> 8;
                rat = rat == 0 ? 0x7fffffff : incount / rat;
            }
            else
                rat = (incount << 8) / outcount;
            if (rat <= ratio) clear();
            else ratio = rat;
        }
    }
    if (ent >= 0)
    {
        put(ent);
        ++free_code;
        if (free_code == kCodeMax - 1)
        {
            put(kClear);
            width = 9;
        }
        else if (free_code > max_code)
            ++width;
    }
    put(kEnd);
    o.finish();
    return std::move(o.out);
}

std::optional<std::vector<uint8_t>> tiff_lzw_decode(const uint8_t* p, size_t n, size_t limit)
{
    // Each code's string as its prefix's code and its last byte (and its length), as written.
    std::vector<int16_t> prefix(kCodeMax + 1, -1);
    std::vector<uint8_t> last(kCodeMax + 1), first(kCodeMax + 1);
    std::vector<uint32_t> length(kCodeMax + 1, 0);
    for (int k = 0; k < 256; ++k) last[size_t(k)] = first[size_t(k)] = uint8_t(k), length[size_t(k)] = 1;
    std::vector<uint8_t> out;
    size_t bit = 0;
    int free_code = kFirst, old = -1;
    auto get = [&](int width) -> int {
        if (bit + size_t(width) > n * 8) return -1;
        int v = 0;
        for (int k = 0; k < width; ++k, ++bit) v = v << 1 | ((p[bit >> 3] >> (7 - (bit & 7))) & 1);
        return v;
    };
    auto emit = [&](int code) -> bool {
        const uint32_t len = length[size_t(code)];
        if (out.size() + len > limit) return false;
        const size_t at = out.size();
        out.resize(at + len);
        for (size_t k = at + len; k > at;)
        {
            out[--k] = last[size_t(code)];
            code = prefix[size_t(code)];
        }
        return true;
    };
    while (true)
    {
        // the width the coder wrote this code at: its next free code then is one more than ours
        int width = 9;
        while (width < 12 && free_code + 1 > (1 << width) - 1) ++width;
        const int code = get(width);
        if (code < 0) return std::nullopt;
        if (code == kEnd) break;
        if (code == kClear)
        {
            free_code = kFirst;
            old = -1;
            continue;
        }
        if (old < 0)
        {
            if (code > 255 || !emit(code)) return std::nullopt;
            old = code;
            continue;
        }
        if (code > free_code || free_code > kCodeMax) return std::nullopt;
        const uint8_t head = code == free_code ? first[size_t(old)] : first[size_t(code)];
        prefix[size_t(free_code)] = int16_t(old);
        last[size_t(free_code)] = head;
        first[size_t(free_code)] = first[size_t(old)];
        length[size_t(free_code)] = length[size_t(old)] + 1;
        ++free_code;
        if (!emit(code)) return std::nullopt;
        old = code;
    }
    return out;
}

namespace {

// One row as libtiff's PackBitsEncode codes it.
void packbits_row(const uint8_t* bp, size_t cc, std::vector<uint8_t>& o)
{
    enum
    {
        BASE,
        LITERAL,
        RUN,
        LITERAL_RUN
    } state = BASE;
    size_t lastliteral = 0;
    while (cc > 0)
    {
        const int b = *bp++;
        --cc;
        long n = 1;
        for (; cc > 0 && b == *bp; --cc, ++bp) ++n;
    again:
        switch (state)
        {
        case BASE:
            if (n > 1)
            {
                state = RUN;
                if (n > 128)
                {
                    o.push_back(uint8_t(-127));
                    o.push_back(uint8_t(b));
                    n -= 128;
                    goto again;
                }
                o.push_back(uint8_t(-(n - 1)));
                o.push_back(uint8_t(b));
            }
            else
            {
                lastliteral = o.size();
                o.push_back(0);
                o.push_back(uint8_t(b));
                state = LITERAL;
            }
            break;
        case LITERAL:
            if (n > 1)
            {
                state = LITERAL_RUN;
                if (n > 128)
                {
                    o.push_back(uint8_t(-127));
                    o.push_back(uint8_t(b));
                    n -= 128;
                    goto again;
                }
                o.push_back(uint8_t(-(n - 1)));
                o.push_back(uint8_t(b));
            }
            else
            {
                if (++o[lastliteral] == 127) state = BASE;
                o.push_back(uint8_t(b));
            }
            break;
        case RUN:
            if (n > 1)
            {
                if (n > 128)
                {
                    o.push_back(uint8_t(-127));
                    o.push_back(uint8_t(b));
                    n -= 128;
                    goto again;
                }
                o.push_back(uint8_t(-(n - 1)));
                o.push_back(uint8_t(b));
            }
            else
            {
                lastliteral = o.size();
                o.push_back(0);
                o.push_back(uint8_t(b));
                state = LITERAL;
            }
            break;
        case LITERAL_RUN:
            // a literal, a run of two, and now one byte: the three joined into the literal
            if (n == 1 && o[o.size() - 2] == uint8_t(-1) && o[lastliteral] < 126)
            {
                state = (o[lastliteral] += 2) == 127 ? BASE : LITERAL;
                o[o.size() - 2] = o[o.size() - 1];
            }
            else
                state = RUN;
            goto again;
        }
    }
}

} // namespace

std::vector<uint8_t> tiff_packbits_encode(const uint8_t* p, size_t n, size_t row)
{
    std::vector<uint8_t> o;
    if (row == 0) row = n;
    for (size_t at = 0; at < n; at += row) packbits_row(p + at, std::min(row, n - at), o);
    return o;
}

std::optional<std::vector<uint8_t>> tiff_packbits_decode(const uint8_t* p, size_t n, size_t size)
{
    std::vector<uint8_t> o;
    o.reserve(size);
    size_t at = 0;
    while (o.size() < size)
    {
        if (at >= n) return std::nullopt;
        const int c = int8_t(p[at++]);
        if (c >= 0)
        {
            const size_t k = size_t(c) + 1;
            if (k > n - at || o.size() + k > size) return std::nullopt;
            o.insert(o.end(), p + at, p + at + k);
            at += k;
        }
        else if (c != -128)
        {
            const size_t k = size_t(1 - c);
            if (at >= n || o.size() + k > size) return std::nullopt;
            o.insert(o.end(), k, p[at++]);
        }
    }
    if (at != n) return std::nullopt;
    return o;
}

namespace {

bool predict_rows(std::vector<uint8_t>& v, size_t row, size_t stride, int bytes, bool big, bool undo)
{
    if (row == 0 || v.size() % row != 0 || (bytes != 1 && bytes != 2) || stride == 0 || row % size_t(bytes) != 0) return false;
    const size_t samples = row / size_t(bytes);
    for (size_t r = 0; r < v.size(); r += row)
    {
        uint8_t* q = v.data() + r;
        auto at = [&](size_t k) -> uint32_t {
            if (bytes == 1) return q[k];
            const uint8_t* s = q + 2 * k;
            return big ? uint32_t(s[0]) << 8 | s[1] : uint32_t(s[1]) << 8 | s[0];
        };
        auto set = [&](size_t k, uint32_t x) {
            if (bytes == 1)
            {
                q[k] = uint8_t(x);
                return;
            }
            uint8_t* s = q + 2 * k;
            s[big ? 0 : 1] = uint8_t(x >> 8);
            s[big ? 1 : 0] = uint8_t(x);
        };
        const uint32_t mask = bytes == 1 ? 0xFF : 0xFFFF;
        if (undo)
            for (size_t k = stride; k < samples; ++k) set(k, (at(k) + at(k - stride)) & mask);
        else
            for (size_t k = samples; k-- > stride;) set(k, (at(k) - at(k - stride)) & mask);
    }
    return true;
}

} // namespace

bool tiff_predict(std::vector<uint8_t>& v, size_t row, size_t stride, int bytes, bool big) { return predict_rows(v, row, stride, bytes, big, false); }
bool tiff_unpredict(std::vector<uint8_t>& v, size_t row, size_t stride, int bytes, bool big) { return predict_rows(v, row, stride, bytes, big, true); }

} // namespace sieve::cli
