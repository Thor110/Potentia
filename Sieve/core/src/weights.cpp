// Sieve — "sieve-weights-v1": a model file coded under a prior over its weights (see weights.hpp).
#include "sieve/weights.hpp"

#include "sieve/safetensors.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <span>
#include <stdexcept>

namespace sieve::weights {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("sieve-weights-v1: " + what); }

constexpr uint64_t kL = uint64_t(1) << 31;
constexpr uint32_t kM = uint32_t(1) << kScaleBits;
const std::string kMagic = std::string(kVersion) + "\n";

void put_varint(std::ostream& out, uint64_t v, uint64_t& written)
{
    do
    {
        uint8_t b = uint8_t(v & 0x7F);
        v >>= 7;
        if (v) b |= 0x80;
        out.put(char(b));
        ++written;
    } while (v);
}

uint64_t get_varint(std::istream& in)
{
    uint64_t v = 0;
    for (int shift = 0; shift < 64; shift += 7)
    {
        const int c = in.get();
        if (c == EOF) bad("the stream ends inside a number");
        v |= uint64_t(c & 0x7F) << shift;
        if (!(c & 0x80))
        {
            if (shift == 63 && c > 1) bad("a number too large");
            return v;
        }
    }
    bad("a number too long");
}

void put_word(std::ostream& out, uint32_t w)
{
    const char b[4] = {char(w & 0xFF), char((w >> 8) & 0xFF), char((w >> 16) & 0xFF), char(w >> 24)};
    out.write(b, 4);
}

void read_exact(std::istream& in, char* p, size_t n, const char* what)
{
    if (n && !in.read(p, std::streamsize(n))) bad(std::string("the stream ends inside ") + what);
}

// The cumulative frequencies: cum[s] is the sum of the frequencies of the symbols below s.
std::vector<uint32_t> cumulative(const std::vector<uint32_t>& freq)
{
    std::vector<uint32_t> cum(freq.size() + 1, 0);
    for (size_t s = 0; s < freq.size(); ++s) cum[s + 1] = cum[s] + freq[s];
    return cum;
}

bool is_coded(const std::string& dtype) { return dtype == "BF16" || dtype == "F16"; }
std::string group_of(const safetensors::Tensor& t) { return kind_of(t.name) + "|" + t.dtype; }

} // namespace

std::string kind_of(const std::string& name)
{
    const std::string p = "model.layers.";
    if (name.rfind(p, 0) != 0) return name;
    size_t i = p.size();
    while (i < name.size() && name[i] >= '0' && name[i] <= '9') ++i;
    return i > p.size() && i < name.size() && name[i] == '.' ? p + "*" + name.substr(i) : name;
}

std::vector<uint32_t> quantise(const std::vector<uint64_t>& counts)
{
    if (counts.size() != 65536) bad("a table is of 65,536 values");
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
    out.reserve(size_t(n));
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

PackReport pack(std::istream& in, std::ostream& out)
{
    PackReport r;
    in.clear();
    in.seekg(0);
    const safetensors::Header h = safetensors::read_header(in);
    in.clear();
    in.seekg(0, std::ios::end);
    r.in_bytes = uint64_t(in.tellg());
    if (r.in_bytes != h.start_bytes() + h.data_bytes()) bad("the file is not the size its header says");
    std::vector<const safetensors::Tensor*> order;
    for (const safetensors::Tensor& t : h.tensors) order.push_back(&t);
    std::sort(order.begin(), order.end(), [](const safetensors::Tensor* a, const safetensors::Tensor* b) { return a->begin < b->begin; });
    auto read_tensor = [&](const safetensors::Tensor& t) {
        std::vector<char> bytes(size_t(t.bytes()));
        in.clear();
        in.seekg(std::streamoff(h.start_bytes() + t.begin));
        read_exact(in, bytes.data(), bytes.size(), "a tensor");
        return bytes;
    };

    // The first pass: the file's hash, and each kind's counts.
    std::vector<std::string> groups; // in the order their first tensor comes
    std::map<std::string, std::vector<uint64_t>> counts;
    Sha256 sha;
    {
        std::string start(size_t(h.start_bytes()), '\0');
        in.clear();
        in.seekg(0);
        read_exact(in, start.data(), start.size(), "the start");
        sha.update(start);
    }
    for (const safetensors::Tensor* t : order)
    {
        const std::vector<char> bytes = read_tensor(*t);
        sha.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()));
        if (!is_coded(t->dtype) || !t->elements()) continue;
        const std::string g = group_of(*t);
        auto [it, fresh] = counts.try_emplace(g, std::vector<uint64_t>(65536, 0));
        if (fresh) groups.push_back(g);
        for (size_t i = 0; i < bytes.size(); i += 2) ++it->second[uint16_t(uint8_t(bytes[i]) | (uint8_t(bytes[i + 1]) << 8))];
    }
    r.sha256 = sha.finish();

    // The start, as it was; the tables.
    out.write(kMagic.data(), std::streamsize(kMagic.size()));
    r.out_bytes += kMagic.size();
    {
        std::string start(size_t(h.start_bytes()), '\0');
        in.clear();
        in.seekg(0);
        read_exact(in, start.data(), start.size(), "the start");
        put_varint(out, start.size(), r.out_bytes);
        out.write(start.data(), std::streamsize(start.size()));
        r.out_bytes += start.size();
        r.start_bytes = r.out_bytes - kMagic.size();
    }
    std::map<std::string, std::vector<uint32_t>> tables;
    {
        const uint64_t before = r.out_bytes;
        put_varint(out, groups.size(), r.out_bytes);
        for (const std::string& g : groups)
        {
            const std::vector<uint32_t> f = quantise(counts[g]);
            uint64_t k = 0;
            for (const uint32_t v : f) k += v != 0;
            put_varint(out, k, r.out_bytes);
            uint64_t prev = 0;
            bool first = true;
            for (size_t s = 0; s < f.size(); ++s)
                if (f[s])
                {
                    put_varint(out, first ? s : s - prev, r.out_bytes);
                    put_varint(out, f[s], r.out_bytes);
                    prev = s;
                    first = false;
                }
            tables[g] = f;
        }
        r.table_bytes = r.out_bytes - before;
        r.kinds = groups.size();
    }

    // The second pass: each tensor, coded under its kind's table, or as it is.
    for (const safetensors::Tensor* t : order)
    {
        const std::vector<char> bytes = read_tensor(*t);
        if (is_coded(t->dtype) && t->elements())
        {
            std::vector<uint16_t> values(bytes.size() / 2);
            for (size_t i = 0; i < values.size(); ++i) values[i] = uint16_t(uint8_t(bytes[2 * i]) | (uint8_t(bytes[2 * i + 1]) << 8));
            const std::vector<uint32_t> words = encode(values, tables.at(group_of(*t)));
            const uint64_t before = r.out_bytes;
            put_varint(out, words.size(), r.out_bytes);
            for (const uint32_t w : words) put_word(out, w);
            r.out_bytes += 4 * words.size();
            r.coded_bytes += r.out_bytes - before;
            ++r.coded_tensors;
        }
        else
        {
            out.write(bytes.data(), std::streamsize(bytes.size()));
            r.out_bytes += bytes.size();
            r.raw_bytes += bytes.size();
            ++r.raw_tensors;
        }
    }
    out.write(reinterpret_cast<const char*>(r.sha256.data()), 32);
    r.out_bytes += 32;
    if (!out) bad("cannot write the output");
    return r;
}

UnpackReport unpack(std::istream& in, std::ostream& out)
{
    {
        std::string magic(kMagic.size(), '\0');
        read_exact(in, magic.data(), magic.size(), "the version line");
        if (magic != kMagic) bad("not a sieve-weights-v1 stream");
    }
    const uint64_t s = get_varint(in);
    if (s > (uint64_t(100) << 20) + 8) bad("a start larger than any header");
    std::string start(size_t(s), '\0');
    read_exact(in, start.data(), start.size(), "the start");
    const safetensors::Header h = safetensors::parse_header(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(start.data()), start.size()));
    if (h.start_bytes() != s) bad("a start of the wrong length");

    std::vector<const safetensors::Tensor*> order;
    for (const safetensors::Tensor& t : h.tensors) order.push_back(&t);
    std::sort(order.begin(), order.end(), [](const safetensors::Tensor* a, const safetensors::Tensor* b) { return a->begin < b->begin; });
    std::vector<std::string> groups;
    for (const safetensors::Tensor* t : order)
        if (is_coded(t->dtype) && t->elements() && std::find(groups.begin(), groups.end(), group_of(*t)) == groups.end()) groups.push_back(group_of(*t));

    const uint64_t g = get_varint(in);
    if (g != groups.size()) bad("the tables are not one for each kind of tensor");
    std::map<std::string, std::vector<uint32_t>> tables;
    for (const std::string& name : groups)
    {
        const uint64_t k = get_varint(in);
        if (!k || k > 65536) bad("a table of " + std::to_string(k) + " values");
        std::vector<uint32_t> f(65536, 0);
        uint64_t sym = 0, sum = 0;
        for (uint64_t i = 0; i < k; ++i)
        {
            const uint64_t gap = get_varint(in);
            if (i && !gap) bad("a table whose values do not increase");
            sym = i ? sym + gap : gap;
            if (sym >= 65536) bad("a table value past 16 bits");
            const uint64_t v = get_varint(in);
            if (!v || v > kM) bad("a frequency out of range");
            f[size_t(sym)] = uint32_t(v);
            sum += v;
        }
        if (sum != kM) bad("a table that does not sum to 2^24");
        tables[name] = std::move(f);
    }

    UnpackReport r;
    Sha256 sha;
    auto emit = [&](const char* p, size_t n) {
        out.write(p, std::streamsize(n));
        sha.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(p), n));
        r.bytes += n;
    };
    emit(start.data(), start.size());
    for (const safetensors::Tensor* t : order)
    {
        if (is_coded(t->dtype) && t->elements())
        {
            const uint64_t w = get_varint(in);
            if (w > t->elements() + 2) bad("tensor " + t->name + ": a stream longer than its values could need");
            std::vector<uint32_t> words(static_cast<size_t>(w));
            std::vector<char> raw(size_t(w) * 4);
            read_exact(in, raw.data(), raw.size(), "a tensor's stream");
            for (size_t i = 0; i < words.size(); ++i)
                words[i] = uint32_t(uint8_t(raw[4 * i])) | uint32_t(uint8_t(raw[4 * i + 1])) << 8 | uint32_t(uint8_t(raw[4 * i + 2])) << 16 |
                           uint32_t(uint8_t(raw[4 * i + 3])) << 24;
            const std::vector<uint16_t> values = decode(words, tables.at(group_of(*t)), t->elements());
            std::vector<char> bytes(values.size() * 2);
            for (size_t i = 0; i < values.size(); ++i)
            {
                bytes[2 * i] = char(values[i] & 0xFF);
                bytes[2 * i + 1] = char(values[i] >> 8);
            }
            emit(bytes.data(), bytes.size());
        }
        else
        {
            std::vector<char> bytes(size_t(t->bytes()));
            read_exact(in, bytes.data(), bytes.size(), "a tensor");
            emit(bytes.data(), bytes.size());
        }
    }
    r.sha256 = sha.finish();
    Sha256::Digest want{};
    read_exact(in, reinterpret_cast<char*>(want.data()), 32, "the SHA-256");
    if (in.peek() != EOF) bad("bytes after the SHA-256");
    if (want != r.sha256) bad("what it rebuilds is not the file it was made from (SHA-256 " + Sha256::hex(r.sha256) + ", not " + Sha256::hex(want) + ")");
    if (!out) bad("cannot write the output");
    return r;
}

} // namespace sieve::weights
