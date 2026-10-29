// Sieve — sieve-weights-v1's tables and rANS coder (see weights.hpp), on their own: used by the
// model-file packer (weights.cpp) and by the tightened rope (rope_wave.cpp), which sieve-install
// links without the packer's safetensors and JSON.
#include "sieve/weights.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve::weights {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("sieve-weights-v1: " + what); }

constexpr uint64_t kL = uint64_t(1) << 31;
constexpr uint32_t kM = uint32_t(1) << kScaleBits;

// The cumulative frequencies: cum[s] is the sum of the frequencies of the symbols below s.
std::vector<uint32_t> cumulative(const std::vector<uint32_t>& freq)
{
    std::vector<uint32_t> cum(freq.size() + 1, 0);
    for (size_t s = 0; s < freq.size(); ++s) cum[s + 1] = cum[s] + freq[s];
    return cum;
}

} // namespace

std::vector<uint32_t> quantise(const std::vector<uint64_t>& counts)
{
    if (counts.size() != 65536 && counts.size() != 256) bad("a table is of 65,536 values (or 256, of bytes)");
    uint64_t n = 0, k = 0;
    size_t top = 0;
    for (size_t s = 0; s < counts.size(); ++s)
        if (counts[s])
        {
            n += counts[s];
            ++k;
            if (counts[s] > counts[top] || !counts[top]) top = s;
        }
    if (!n) bad("a table with no values");
    std::vector<uint32_t> f(counts.size(), 0);
    uint64_t sum = 0;
    for (size_t s = 0; s < counts.size(); ++s)
        if (counts[s])
        {
            // c * (M - k) < 2^64: c is below 2^40 for any kind that fits in memory, M - k below 2^24.
            f[s] = uint32_t(1 + counts[s] * (kM - k) / n);
            sum += f[s];
        }
    f[top] += uint32_t(kM - sum);
    return f;
}

std::vector<uint32_t> encode(const std::vector<uint16_t>& values, const std::vector<uint32_t>& freq)
{
    const std::vector<uint32_t> cum = cumulative(freq);
    if (cum.back() != kM) bad("a table that does not sum to 2^24");
    std::vector<uint32_t> out; // the words put out, first first
    uint64_t x = kL;
    for (size_t i = values.size(); i-- > 0;)
    {
        const uint32_t f = freq[values[i]];
        if (!f) bad("a value its table does not have");
        if (x >= ((kL >> kScaleBits) << 32) * f)
        {
            out.push_back(uint32_t(x));
            x >>= 32;
        }
        x = ((x / f) << kScaleBits) + (x % f) + cum[values[i]];
    }
    std::vector<uint32_t> stream;
    stream.reserve(out.size() + 2);
    stream.push_back(uint32_t(x));
    stream.push_back(uint32_t(x >> 32));
    stream.insert(stream.end(), out.rbegin(), out.rend());
    return stream;
}

std::vector<uint16_t> decode(const std::vector<uint32_t>& words, const std::vector<uint32_t>& freq, uint64_t n)
{
    const std::vector<uint32_t> cum = cumulative(freq);
    if (cum.back() != kM) bad("a table that does not sum to 2^24");
    // The symbols present, by where their ranges start, to find the one a slot falls in.
    std::vector<uint32_t> starts;
    std::vector<uint16_t> syms;
    for (size_t s = 0; s < freq.size(); ++s)
        if (freq[s])
        {
            starts.push_back(cum[s]);
            syms.push_back(uint16_t(s));
        }
    // Where to start looking for a slot's symbol: the symbol holding the first slot of each block
    // of 256 slots (the symbols in a block are few, so a short walk finds the one).
    std::vector<uint32_t> first(size_t(kM >> 8));
    for (size_t b = 0, j = 0; b < first.size(); ++b)
    {
        const uint32_t slot = uint32_t(b << 8);
        while (j + 1 < starts.size() && starts[j + 1] <= slot) ++j;
        first[b] = uint32_t(j);
    }
    if (words.size() < 2) bad("a stream shorter than its state");
    uint64_t x = uint64_t(words[0]) | (uint64_t(words[1]) << 32);
    size_t next = 2;
    std::vector<uint16_t> out;
    out.reserve(size_t(std::min<uint64_t>(n, uint64_t(1) << 24))); // (n is from the file: grown past this as values come)
    for (uint64_t i = 0; i < n; ++i)
    {
        const uint32_t slot = uint32_t(x & (kM - 1));
        size_t j = first[slot >> 8];
        while (j + 1 < starts.size() && starts[j + 1] <= slot) ++j;
        const uint16_t s = syms[j];
        x = uint64_t(freq[s]) * (x >> kScaleBits) + slot - cum[s];
        while (x < kL)
        {
            if (next >= words.size()) bad("a stream that ends too soon");
            x = (x << 32) | words[next++];
        }
        out.push_back(s);
    }
    if (x != kL || next != words.size()) bad("a stream that does not end where its values do");
    return out;
}

} // namespace sieve::weights
