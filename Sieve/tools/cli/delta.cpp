// Sieve CLI -- a file as its differences from another (delta.hpp).

#include "delta.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

namespace sieve::cli::delta {

namespace {

// splitmix64: the gear table's numbers, fixed for ever (a different table cuts different blocks,
// which only changes which files are found alike, never what a script makes).
std::array<uint64_t, 256> gear_table()
{
    std::array<uint64_t, 256> g{};
    uint64_t x = 0x5167E5ull; // "sieve"
    for (uint64_t& v : g)
    {
        uint64_t z = (x += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        v = z ^ (z >> 31);
    }
    return g;
}

uint64_t mix64(uint64_t h)
{
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDull;
    h ^= h >> 33;
    h *= 0xC4CEB9FE1A85EC53ull;
    return h ^ (h >> 33);
}

// A block's fingerprint: eight bytes at a time, mixed.
uint64_t print_of(const uint8_t* p, size_t n)
{
    uint64_t h = 0x9E3779B97F4A7C15ull ^ n;
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        uint64_t w;
        std::memcpy(&w, p + i, 8);
        h = mix64(h ^ w) + 0x632BE59BD9B4E019ull;
    }
    uint64_t w = 0;
    std::memcpy(&w, p + i, n - i);
    return mix64(h ^ w ^ (uint64_t(n - i) << 56));
}

void put_varint(std::vector<uint8_t>& o, uint64_t v)
{
    while (v >= 0x80)
    {
        o.push_back(uint8_t(v | 0x80));
        v >>= 7;
    }
    o.push_back(uint8_t(v));
}

uint64_t get_varint(std::span<const uint8_t> in, size_t& at)
{
    uint64_t v = 0;
    for (int shift = 0;; shift += 7)
    {
        if (at >= in.size() || shift > 63) throw std::runtime_error("delta: its script ends inside a number");
        const uint8_t b = in[at++];
        v |= uint64_t(b & 0x7F) << shift;
        if (!(b & 0x80)) return v;
    }
}

constexpr size_t kWindow = 64; // the shortest copy, and the stretch the base is indexed by

} // namespace

std::vector<Block> blocks_of(std::span<const uint8_t> file)
{
    static const std::array<uint64_t, 256> gear = gear_table();
    constexpr uint64_t kMask = (uint64_t(1) << 12) - 1; // about every 4 KiB
    constexpr size_t kMin = 64, kMax = size_t(64) << 10;
    std::vector<Block> out;
    out.reserve(file.size() / 4096 + 1);
    uint64_t h = 0;
    size_t from = 0;
    for (size_t i = 0; i < file.size(); ++i)
    {
        h = (h << 1) + gear[file[i]];
        const size_t len = i + 1 - from;
        if ((len >= kMin && (h & kMask) == 0) || len >= kMax)
        {
            out.push_back({print_of(file.data() + from, len), uint32_t(len)});
            from = i + 1;
            h = 0;
        }
    }
    if (from < file.size()) out.push_back({print_of(file.data() + from, file.size() - from), uint32_t(file.size() - from)});
    return out;
}

Script make(std::span<const uint8_t> base, std::span<const uint8_t> target)
{
    Script s;
    // The base indexed every 64 bytes by its window's hash: a table of positions (plus one, 0 for
    // none), a power of two at least twice the entries; a later window of the same slot replaces an
    // earlier (some matches lost, never a wrong one: every match is compared byte for byte).
    constexpr uint64_t kMul = 0x100000001B3ull;
    uint64_t pow = 1; // kMul^(kWindow - 1), to take the oldest byte out of a rolling hash
    for (size_t k = 1; k < kWindow; ++k) pow *= kMul;
    auto hash_at = [&](const uint8_t* p) {
        uint64_t h = 0;
        for (size_t k = 0; k < kWindow; ++k) h = h * kMul + p[k];
        return h;
    };
    const size_t entries = base.size() / kWindow;
    size_t slots = 1;
    int bits = 0;
    while (slots < entries * 2) slots <<= 1, ++bits;
    const int shift = 64 - (bits ? bits : 1);
    std::vector<uint64_t> table(entries ? slots : 0, 0);
    for (size_t a = 0; a + kWindow <= base.size(); a += kWindow) table[size_t(mix64(hash_at(base.data() + a)) >> shift)] = uint64_t(a) + 1;

    uint64_t last_end = 0; // where the last copy ended in the base
    size_t fresh_from = 0; // the new bytes not yet written start here
    auto write_fresh = [&](size_t to) {
        if (to <= fresh_from) return;
        put_varint(s.bytes, uint64_t(to - fresh_from) << 1);
        s.bytes.insert(s.bytes.end(), target.begin() + std::ptrdiff_t(fresh_from), target.begin() + std::ptrdiff_t(to));
        s.fresh += to - fresh_from;
    };
    size_t i = 0;
    uint64_t h = target.size() >= kWindow && entries ? hash_at(target.data()) : 0;
    while (entries && i + kWindow <= target.size())
    {
        const uint64_t at = table[size_t(mix64(h) >> shift)];
        if (at && std::memcmp(base.data() + (at - 1), target.data() + i, kWindow) == 0)
        {
            // A match: forwards as far as it goes, and backwards over new bytes not yet written.
            size_t a = size_t(at - 1), b = i;
            while (b > fresh_from && a > 0 && base[a - 1] == target[b - 1]) --a, --b;
            size_t ea = size_t(at - 1) + kWindow, eb = i + kWindow;
            while (eb < target.size() && ea < base.size() && base[ea] == target[eb]) ++ea, ++eb;
            write_fresh(b);
            const uint64_t n = eb - b;
            put_varint(s.bytes, (n << 1) | 1);
            const int64_t move = int64_t(a) - int64_t(last_end);
            put_varint(s.bytes, move >= 0 ? uint64_t(move) << 1 : (uint64_t(-move) << 1) - 1);
            last_end = a + n;
            ++s.copies;
            s.copied += n;
            i = fresh_from = eb;
            if (i + kWindow <= target.size()) h = hash_at(target.data() + i);
            continue;
        }
        if (i + kWindow < target.size()) h = (h - pow * target[i]) * kMul + target[i + kWindow];
        ++i;
    }
    write_fresh(target.size());
    return s;
}

std::vector<uint8_t> apply(std::span<const uint8_t> base, std::span<const uint8_t> script, uint64_t size)
{
    std::vector<uint8_t> out;
    out.reserve(size_t(size));
    uint64_t last_end = 0;
    for (size_t at = 0; at < script.size();)
    {
        const uint64_t t = get_varint(script, at), n = t >> 1;
        if (n == 0) throw std::runtime_error("delta: an operation of no bytes");
        if (n > size - out.size()) throw std::runtime_error("delta: its script makes more than the file");
        if (!(t & 1))
        {
            if (n > script.size() - at) throw std::runtime_error("delta: its script ends inside new bytes");
            out.insert(out.end(), script.begin() + std::ptrdiff_t(at), script.begin() + std::ptrdiff_t(at + n));
            at += size_t(n);
            continue;
        }
        const uint64_t z = get_varint(script, at);
        const int64_t move = (z & 1) ? -int64_t((z + 1) >> 1) : int64_t(z >> 1);
        if ((move < 0 && uint64_t(-move) > last_end) || (move > 0 && uint64_t(move) > base.size()))
            throw std::runtime_error("delta: a copy from outside its base");
        const uint64_t from = uint64_t(int64_t(last_end) + move);
        if (from > base.size() || n > base.size() - from) throw std::runtime_error("delta: a copy from outside its base");
        out.insert(out.end(), base.begin() + std::ptrdiff_t(from), base.begin() + std::ptrdiff_t(from + n));
        last_end = from + n;
    }
    if (out.size() != size) throw std::runtime_error("delta: its script makes " + std::to_string(out.size()) + " bytes, not " + std::to_string(size));
    return out;
}

} // namespace sieve::cli::delta
