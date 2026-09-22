#include "sieve/model.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace sieve {

namespace {

// Largest-remainder quantisation to kModelTotal with every entry at least 1 (see model.hpp).
void quantise(const uint64_t* a, size_t N, uint64_t D, uint32_t* cum)
{
    const uint64_t free = kModelTotal - N;
    uint32_t f[256];
    uint64_t rem[256];
    uint64_t used = 0;
    for (size_t s = 0; s < N; ++s)
    {
        const uint64_t x = free * a[s], q = x / D;
        f[s] = static_cast<uint32_t>(1 + q);
        rem[s] = x - q * D;
        used += f[s];
    }
    // The leftover (fewer than N units) goes to the largest remainders, ties to the lowest symbol.
    const size_t left = size_t(kModelTotal - used);
    if (left)
    {
        uint32_t order[256];
        for (size_t s = 0; s < N; ++s) order[s] = uint32_t(s);
        std::partial_sort(order, order + left, order + N,
                          [&](uint32_t x, uint32_t y) { return rem[x] != rem[y] ? rem[x] > rem[y] : x < y; });
        for (size_t i = 0; i < left; ++i) ++f[order[i]];
    }
    cum[0] = 0;
    for (size_t s = 0; s < N; ++s) cum[s + 1] = cum[s] + f[s];
    if (cum[N] != kModelTotal) throw std::logic_error("quantisation did not reach the total");
}

std::string hex2(uint32_t v)
{
    static const char* d = "0123456789abcdef";
    return {d[v >> 4], d[v & 15]};
}

uint64_t parse_u64(const std::string& s, const char* what)
{
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos || s.size() > 19)
        throw std::runtime_error(std::string("model file: bad ") + what + " '" + s + "'");
    return std::stoull(s);
}

} // namespace

uint64_t CharModel::key(const uint32_t* symbols, size_t k) const
{
    uint64_t v = 0;
    for (size_t i = 0; i < k; ++i) v = v * params_.base + symbols[i];
    return v * 16 + k;
}

CharModel CharModel::train(std::span<const uint32_t> stream, const ModelParams& p)
{
    if (p.base < 2 || p.base > 256) throw std::invalid_argument("model base must be 2..256");
    if (p.order > 12) throw std::invalid_argument("model order must be at most 12");
    for (uint32_t s : stream)
        if (s >= p.base) throw std::invalid_argument("training symbol out of range");

    CharModel m;
    m.params_ = p;
    m.trained_ = stream.size();
    const uint32_t N = p.base;
    std::vector<uint64_t> pw(p.order + 1, 1);
    for (size_t i = 1; i <= p.order; ++i) pw[i] = pw[i - 1] * N;

    // Count every (context, next symbol) pair. A context of length k is keyed by (k, value).
    std::unordered_map<uint64_t, uint32_t> slot;
    std::vector<uint64_t> counts; // N per slot
    std::vector<std::pair<uint32_t, uint64_t>> keys; // (k, value) per slot
    for (size_t t = 0; t < stream.size(); ++t)
    {
        uint64_t v = 0;
        for (size_t k = 0; k <= p.order && k <= t; ++k)
        {
            if (k) v += stream[t - k] * pw[k - 1];
            const uint64_t key = v * 16 + k;
            auto [it, fresh] = slot.try_emplace(key, uint32_t(keys.size()));
            if (fresh)
            {
                keys.emplace_back(uint32_t(k), v);
                counts.resize(counts.size() + N, 0);
            }
            ++counts[size_t(it->second) * N + stream[t]];
        }
    }

    // Keep contexts seen at least min_count times whose parent is kept, shortest first.
    std::vector<uint32_t> order(keys.size());
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return keys[a] < keys[b]; });
    std::unordered_map<uint64_t, bool> kept;
    for (uint32_t id : order)
    {
        const auto [k, v] = keys[id];
        uint64_t n = 0;
        for (uint32_t s = 0; s < N; ++s) n += counts[size_t(id) * N + s];
        bool keep = k == 0 || n >= p.min_count;
        if (keep && k > 0) keep = kept.count((v % pw[k - 1]) * 16 + (k - 1)) != 0; // parent drops the oldest
        if (!keep) continue;
        kept[v * 16 + k] = true;
        Context c;
        c.symbols.resize(k);
        uint64_t x = v;
        for (size_t i = k; i-- > 0;) { c.symbols[i] = uint32_t(x % N); x /= N; }
        for (uint32_t s = 0; s < N; ++s)
            if (const uint64_t n_s = counts[size_t(id) * N + s]) c.counts.emplace_back(s, n_s);
        m.contexts_.push_back(std::move(c));
    }
    if (m.contexts_.empty()) // empty stream: still a valid (uniform-ish) model
        m.contexts_.push_back({});
    m.sha256_ = Sha256::hex(Sha256::hash(m.serialise()));
    m.finish();
    return m;
}

void CharModel::finish()
{
    const uint32_t N = params_.base;
    if (N < 2 || N > 256) throw std::runtime_error("model file: base must be 2..256");
    if (params_.order > 12) throw std::runtime_error("model file: order must be at most 12");
    // Keys must fit: N^order * 16 < 2^64.
    long double room = std::ldexp(1.0L, 60);
    for (uint32_t i = 0; i < params_.order; ++i) room /= N;
    if (room < 1) throw std::runtime_error("model file: base^order is too large");
    if (params_.padding && *params_.padding >= N) throw std::runtime_error("model file: padding symbol out of range");

    cum_.assign((contexts_.size() + (params_.padding && params_.order >= 2 ? 1 : 0)) * (N + 1), 0);
    index_.clear();
    index_.reserve(contexts_.size() * 2);
    for (size_t i = 0; i < contexts_.size(); ++i)
    {
        const Context& c = contexts_[i];
        const size_t k = c.symbols.size();
        if (k > params_.order) throw std::runtime_error("model file: context longer than the order");
        if (i == 0 && k != 0) throw std::runtime_error("model file: the first context must be the empty one");
        if (i > 0)
        {
            const Context& prev = contexts_[i - 1];
            const bool ordered = prev.symbols.size() < k || (prev.symbols.size() == k && prev.symbols < c.symbols);
            if (!ordered)
                throw std::runtime_error("model file: contexts must be sorted and distinct");
        }
        uint64_t n = 0;
        uint32_t last = 0;
        for (size_t j = 0; j < c.counts.size(); ++j)
        {
            const auto [s, n_s] = c.counts[j];
            if (s >= N || n_s == 0 || (j && s <= last)) throw std::runtime_error("model file: bad count list");
            last = s;
            n += n_s;
        }
        if (n + N >= (uint64_t(1) << 32)) throw std::runtime_error("model file: a context count is too large");
        for (uint32_t s : c.symbols)
            if (s >= N) throw std::runtime_error("model file: context symbol out of range");

        uint64_t a[256] = {};
        uint64_t D;
        if (k == 0)
        {
            for (uint32_t s = 0; s < N; ++s) a[s] = 1;
            for (const auto& [s, n_s] : c.counts) a[s] += n_s;
            D = n + N;
        }
        else
        {
            const auto parent = index_.find(key(c.symbols.data() + 1, k - 1));
            if (parent == index_.end()) throw std::runtime_error("model file: a context's parent is missing");
            const uint32_t* pc = &cum_[size_t(parent->second) * (N + 1)];
            const uint64_t u = c.counts.size();
            for (uint32_t s = 0; s < N; ++s) a[s] = u * (pc[s + 1] - pc[s]);
            for (const auto& [s, n_s] : c.counts) a[s] += n_s * kModelTotal;
            D = (n + u) * kModelTotal;
        }
        quantise(a, N, D, &cum_[i * (N + 1)]);
        index_.emplace(key(c.symbols.data(), k), uint32_t(i));
    }
    if (params_.padding && params_.order >= 2)
    {
        // The fixed padding table overrides whatever the corpus says about "p p".
        const uint32_t p = *params_.padding;
        const size_t t = contexts_.size();
        uint32_t* cum = &cum_[t * (N + 1)];
        for (uint32_t s = 0; s < N; ++s) cum[s + 1] = cum[s] + (s == p ? kModelTotal - (N - 1) : 1);
        const uint32_t pp[2] = {p, p};
        index_[key(pp, 2)] = uint32_t(t);
        padding_table_ = uint32_t(t);
    }
}

const uint32_t* CharModel::cumulative(std::span<const uint32_t> history) const
{
    const size_t n = history.size();
    for (size_t k = std::min<size_t>(params_.order, n);; --k)
    {
        const auto it = index_.find(key(history.data() + (n - k), k));
        if (it != index_.end()) return &cum_[size_t(it->second) * (params_.base + 1)];
        if (k == 0) break;
    }
    throw std::logic_error("model has no empty context");
}

double CharModel::stream_bits(std::span<const uint32_t> stream) const
{
    double bits = 0;
    for (size_t t = 0; t < stream.size(); ++t)
    {
        const size_t k = std::min<size_t>(params_.order, t);
        const uint32_t* cum = cumulative(stream.subspan(t - k, k));
        const uint32_t s = stream[t];
        bits += kModelTotalBits - std::log2(double(cum[s + 1] - cum[s]));
    }
    return bits;
}

std::string CharModel::serialise() const
{
    std::string o;
    o.reserve(64 + contexts_.size() * 48);
    auto num = [&](uint64_t v) {
        char b[24];
        const auto r = std::to_chars(b, b + sizeof b, v);
        o.append(b, r.ptr);
    };
    auto line = [&](const char* key, const std::string& value) { o.append(key).append(" ").append(value).append("\n"); };
    o.append(kCharModelFormat).append("\n");
    line("symbols", params_.symbols_id);
    line("base", std::to_string(params_.base));
    line("order", std::to_string(params_.order));
    line("min_count", std::to_string(params_.min_count));
    line("total", std::to_string(kModelTotal));
    line("smoothing", kSmoothing);
    line("padding", params_.padding ? std::to_string(*params_.padding) : std::string("none"));
    line("trained_symbols", std::to_string(trained_));
    line("corpus", params_.corpus);
    line("contexts", std::to_string(contexts_.size()));
    for (const Context& c : contexts_)
    {
        if (c.symbols.empty()) o.push_back('-');
        for (uint32_t s : c.symbols) o.append(hex2(s));
        o.push_back('\t');
        for (size_t j = 0; j < c.counts.size(); ++j)
        {
            if (j) o.push_back(' ');
            num(c.counts[j].first);
            o.push_back(':');
            num(c.counts[j].second);
        }
        o.push_back('\n');
    }
    return o;
}

CharModel CharModel::parse(std::string_view text)
{
    CharModel m;
    size_t pos = 0;
    auto next_line = [&]() -> std::string {
        if (pos >= text.size()) throw std::runtime_error("model file: unexpected end");
        const size_t e = text.find('\n', pos);
        if (e == std::string_view::npos) throw std::runtime_error("model file: missing final newline");
        std::string l(text.substr(pos, e - pos));
        pos = e + 1;
        return l;
    };
    auto field = [&](const char* name) -> std::string {
        const std::string l = next_line();
        const std::string pre = std::string(name) + " ";
        if (l.compare(0, pre.size(), pre) != 0) throw std::runtime_error(std::string("model file: expected '") + name + "'");
        return l.substr(pre.size());
    };
    if (next_line() != kCharModelFormat) throw std::runtime_error(std::string("model file: not ") + kCharModelFormat);
    m.params_.symbols_id = field("symbols");
    m.params_.base = uint32_t(parse_u64(field("base"), "base"));
    m.params_.order = uint32_t(parse_u64(field("order"), "order"));
    m.params_.min_count = uint32_t(parse_u64(field("min_count"), "min_count"));
    if (field("total") != std::to_string(kModelTotal)) throw std::runtime_error("model file: unsupported total");
    if (field("smoothing") != kSmoothing) throw std::runtime_error("model file: unsupported smoothing");
    const std::string pad = field("padding");
    if (pad != "none") m.params_.padding = uint32_t(parse_u64(pad, "padding"));
    m.trained_ = parse_u64(field("trained_symbols"), "trained_symbols");
    m.params_.corpus = field("corpus");
    const uint64_t n = parse_u64(field("contexts"), "contexts");
    if (m.params_.base < 2 || m.params_.base > 256) throw std::runtime_error("model file: base must be 2..256");
    for (uint64_t i = 0; i < n; ++i)
    {
        const std::string l = next_line();
        const size_t tab = l.find('\t');
        if (tab == std::string::npos) throw std::runtime_error("model file: context line without a tab");
        Context c;
        const std::string_view ctx = std::string_view(l).substr(0, tab);
        auto hexval = [&](char ch) -> uint32_t {
            if (ch >= '0' && ch <= '9') return uint32_t(ch - '0');
            if (ch >= 'a' && ch <= 'f') return uint32_t(ch - 'a' + 10);
            throw std::runtime_error("model file: bad context '" + std::string(ctx) + "'");
        };
        if (ctx != "-")
        {
            if (ctx.empty() || ctx.size() % 2) throw std::runtime_error("model file: bad context '" + std::string(ctx) + "'");
            for (size_t j = 0; j < ctx.size(); j += 2) c.symbols.push_back(hexval(ctx[j]) * 16 + hexval(ctx[j + 1]));
        }
        // "sym:count sym:count ..." with plain decimal numbers.
        const char* q = l.data() + tab + 1;
        const char* end = l.data() + l.size();
        while (q < end)
        {
            uint64_t sym = 0, cnt = 0;
            auto r1 = std::from_chars(q, end, sym);
            if (r1.ec != std::errc() || r1.ptr == end || *r1.ptr != ':') throw std::runtime_error("model file: bad count list");
            auto r2 = std::from_chars(r1.ptr + 1, end, cnt);
            if (r2.ec != std::errc()) throw std::runtime_error("model file: bad count list");
            if (sym > 0xFFFFFFFFull) throw std::runtime_error("model file: bad symbol");
            c.counts.emplace_back(uint32_t(sym), cnt);
            q = r2.ptr;
            if (q < end)
            {
                if (*q != ' ') throw std::runtime_error("model file: bad count list");
                ++q;
            }
        }
        m.contexts_.push_back(std::move(c));
    }
    if (pos != text.size()) throw std::runtime_error("model file: trailing data after the last context");
    m.finish();
    // A model file has exactly one spelling, so its hash identifies it.
    if (m.serialise() != text) throw std::runtime_error("model file: not in canonical form (re-save it with sieve train)");
    m.sha256_ = Sha256::hex(Sha256::hash(text));
    return m;
}

CharModel CharModel::load_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open model '" + path + "'");
    std::string bytes;
    in.seekg(0, std::ios::end);
    bytes.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0);
    in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return parse(bytes);
}

} // namespace sieve
