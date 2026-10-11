// Sieve — the AI line: every language model of one shape (see aispace.hpp).
#include "sieve/aispace.hpp"

#include "sieve/llm_tokenizer.hpp"
#include "sieve/safetensors.hpp"
#include "sieve/sha256.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace sieve {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("aispace-v1: " + what); }

json::Value number(uint64_t v) { return json::Value::make_number(std::to_string(v)); }

bool is_norm(const std::string& name) { return name.find("norm.weight") != std::string::npos; }

} // namespace

void AiShape::check() const
{
    if (layers < 1 || layers > 64) bad("layers from 1 to 64");
    if (width < 2 || width > 4096) bad("a width from 2 to 4096");
    if (heads < 1 || width % heads || (width / heads) % 2) bad("heads that divide the width into heads of an even size");
    if (bits < 1 || bits > 8) bad("from 1 to 8 bits a weight");
}

AiSpace::AiSpace(AiShape shape, std::string key) : shape_(shape), key_(std::move(key)), shuffle_(BigUint(1), "", "")
{
    shape_.check();
    std::vector<safetensors::Tensor> all = safetensors::llama_tensors(config());
    std::sort(all.begin(), all.end(), [](const safetensors::Tensor& a, const safetensors::Tensor& b) { return a.name < b.name; });
    for (const safetensors::Tensor& t : all)
    {
        if (is_norm(t.name)) continue;
        const uint32_t rows = uint32_t(t.shape[0]), cols = uint32_t(t.shape[1]);
        const double scale = t.name == "model.embed_tokens.weight" ? 1.0 : std::sqrt(3.0 / double(cols));
        tensors_.push_back({t.name, rows, cols, scale, weights_});
        weights_ += uint64_t(rows) * cols;
    }
    size_ = BigUint::pow(2, bits());
    BigUint top = size_;
    top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, top.to_hex().size());
    shuffle_ = Shuffle(size_, key_, id());
    // Each layer's tensors, for the canonical form.
    layers_.assign(shape_.layers, Layer{});
    for (size_t i = 0; i < tensors_.size(); ++i)
    {
        const std::string& n = tensors_[i].name;
        if (n == "model.embed_tokens.weight")
        {
            embed_ = i;
            continue;
        }
        const size_t at = std::string("model.layers.").size(), dot = n.find('.', at);
        Layer& l = layers_.at(std::stoul(n.substr(at, dot - at)));
        const std::string part = n.substr(dot + 1);
        size_t* slot = part == "mlp.down_proj.weight"        ? &l.down
                       : part == "mlp.gate_proj.weight"      ? &l.gate
                       : part == "mlp.up_proj.weight"        ? &l.up
                       : part == "self_attn.k_proj.weight"   ? &l.k
                       : part == "self_attn.o_proj.weight"   ? &l.o
                       : part == "self_attn.q_proj.weight"   ? &l.q
                       : part == "self_attn.v_proj.weight"   ? &l.v
                                                             : nullptr;
        if (!slot) bad("a tensor it does not know: " + n);
        *slot = i;
    }
}

std::string AiSpace::id() const
{
    return "ai/L" + std::to_string(shape_.layers) + "/H" + std::to_string(shape_.width) + "/A" + std::to_string(shape_.heads) + "/F" +
           std::to_string(shape_.ffn()) + "/B" + std::to_string(shape_.bits) + "/V256/key=" + key_ + "/" + kAiSpaceVersion;
}

AiSpace::Digits AiSpace::digits_at(const BigUint& index, AddressMode m) const
{
    if (!(index < size_)) bad("an index past the line");
    const BigUint place = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    // The digits are the number's bits, B at a time, the first most significant.
    const uint64_t total = bits(), bytes = (total + 7) / 8;
    const std::vector<uint8_t> raw = place.to_bytes(size_t(bytes));
    const uint64_t lead = bytes * 8 - total; // unused bits at the front
    Digits d(static_cast<size_t>(weights_));
    const uint32_t b = shape_.bits;
    for (uint64_t i = 0; i < weights_; ++i)
    {
        uint32_t v = 0;
        for (uint32_t k = 0; k < b; ++k)
        {
            const uint64_t bit = lead + i * b + k;
            v = (v << 1) | ((raw[size_t(bit / 8)] >> (7 - bit % 8)) & 1u);
        }
        d[size_t(i)] = uint8_t(v);
    }
    return d;
}

BigUint AiSpace::index_of(const Digits& d, AddressMode m) const
{
    if (d.size() != weights_) bad("a model of " + std::to_string(weights_) + " weights, not " + std::to_string(d.size()));
    const uint32_t b = shape_.bits;
    const uint64_t total = bits(), bytes = (total + 7) / 8, lead = bytes * 8 - total;
    std::vector<uint8_t> raw(size_t(bytes), 0);
    for (uint64_t i = 0; i < weights_; ++i)
    {
        if (d[size_t(i)] >> b) bad("a digit past " + std::to_string(b) + " bits");
        for (uint32_t k = 0; k < b; ++k)
            if ((d[size_t(i)] >> (b - 1 - k)) & 1u)
            {
                const uint64_t bit = lead + i * b + k;
                raw[size_t(bit / 8)] |= uint8_t(1u << (7 - bit % 8));
            }
    }
    const BigUint place = BigUint::from_bytes(raw);
    return m == AddressMode::Scrambled ? shuffle_.forward(place) : place;
}

BigUint AiSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (!(v < size_)) bad("an address past the line");
    return v;
}

float AiSpace::value(const Tensor& t, uint8_t digit) const
{
    const double m = double((1u << shape_.bits) - 1);
    return float((2.0 * double(digit) - m) / m * t.scale);
}

json::Value AiSpace::config() const
{
    using json::Value;
    return Value::make_object({{"architectures", Value::make_array({Value::make_string("LlamaForCausalLM")})},
                               {"model_type", Value::make_string("llama")},
                               {"hidden_size", number(shape_.width)},
                               {"intermediate_size", number(shape_.ffn())},
                               {"num_attention_heads", number(shape_.heads)},
                               {"num_key_value_heads", number(shape_.heads)},
                               {"num_hidden_layers", number(shape_.layers)},
                               {"vocab_size", number(256)},
                               {"max_position_embeddings", number(512)},
                               {"rms_norm_eps", Value::make_number("1e-05")},
                               {"rope_theta", Value::make_number("10000.0")},
                               {"hidden_act", Value::make_string("silu")},
                               {"tie_word_embeddings", Value::make_bool(true)},
                               {"attention_bias", Value::make_bool(false)},
                               {"mlp_bias", Value::make_bool(false)},
                               {"torch_dtype", Value::make_string("float32")}});
}

std::map<std::string, std::vector<float>> AiSpace::tensors_of(const Digits& d) const
{
    if (d.size() != weights_) bad("a model of " + std::to_string(weights_) + " weights, not " + std::to_string(d.size()));
    std::map<std::string, std::vector<float>> out;
    for (const Tensor& t : tensors_)
    {
        std::vector<float>& v = out[t.name];
        v.resize(size_t(t.rows) * t.cols);
        for (size_t i = 0; i < v.size(); ++i) v[i] = value(t, d[size_t(t.first + i)]);
    }
    for (const safetensors::Tensor& t : safetensors::llama_tensors(config()))
        if (is_norm(t.name)) out[t.name].assign(size_t(t.shape[0]), 1.0f);
    return out;
}

std::string AiSpace::safetensors_of(const Digits& d) const
{
    const std::map<std::string, std::vector<float>> ts = tensors_of(d);
    std::vector<safetensors::Tensor> laid;
    std::string file = safetensors::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, safetensors::llama_tensors(config()), &laid);
    for (const safetensors::Tensor& t : laid)
        for (const float f : ts.at(t.name))
        {
            uint32_t u;
            std::memcpy(&u, &f, 4);
            for (int k = 0; k < 4; ++k) file.push_back(char((u >> (8 * k)) & 0xFF));
        }
    return file;
}

std::string AiSpace::tokenizer_json()
{
    using json::Value;
    std::vector<std::pair<std::string, Value>> vocab;
    for (int b = 0; b < 256; ++b) vocab.emplace_back(utf8_encode(llm::byte_char(uint8_t(b))), number(uint64_t(b)));
    const Value byte_level = Value::make_object({{"type", Value::make_string("ByteLevel")},
                                                 {"add_prefix_space", Value::make_bool(false)},
                                                 {"trim_offsets", Value::make_bool(true)},
                                                 {"use_regex", Value::make_bool(true)}});
    return json::write(Value::make_object({{"version", Value::make_string("1.0")},
                                           {"added_tokens", Value::make_array({})},
                                           {"normalizer", Value()},
                                           {"pre_tokenizer", byte_level},
                                           {"post_processor", Value()},
                                           {"decoder", byte_level},
                                           {"model", Value::make_object({{"type", Value::make_string("BPE")},
                                                                         {"dropout", Value()},
                                                                         {"unk_token", Value()},
                                                                         {"continuing_subword_prefix", Value()},
                                                                         {"end_of_word_suffix", Value()},
                                                                         {"fuse_unk", Value::make_bool(false)},
                                                                         {"byte_fallback", Value::make_bool(false)},
                                                                         {"ignore_merges", Value::make_bool(false)},
                                                                         {"vocab", Value::make_object(std::move(vocab))},
                                                                         {"merges", Value::make_array({})}})}})) +
           "\n";
}

std::optional<AiSpace::Digits> AiSpace::digits_of_safetensors(std::string_view file) const
{
    safetensors::Header h;
    try
    {
        h = safetensors::parse_header(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(file.data()), file.size()));
    }
    catch (const std::invalid_argument&)
    {
        return std::nullopt;
    }
    if (file.size() != h.start_bytes() + h.data_bytes()) return std::nullopt;
    std::map<std::string, const safetensors::Tensor*> by;
    for (const safetensors::Tensor& t : h.tensors) by[t.name] = &t;
    const std::vector<safetensors::Tensor> want = safetensors::llama_tensors(config());
    if (by.size() != want.size()) return std::nullopt;
    auto read = [&](const safetensors::Tensor& t, std::vector<float>& out) {
        out.resize(size_t(t.elements()));
        const char* p = file.data() + h.start_bytes() + t.begin;
        for (size_t i = 0; i < out.size(); ++i)
        {
            if (t.dtype == "F32") std::memcpy(&out[i], p + 4 * i, 4);
            else if (t.dtype == "BF16" || t.dtype == "F16")
                out[i] = float(safetensors::half_to_double(uint16_t(uint8_t(p[2 * i]) | (uint8_t(p[2 * i + 1]) << 8)), t.dtype == "BF16"));
            else return false;
        }
        return true;
    };
    std::vector<float> v;
    for (const safetensors::Tensor& w : want)
    {
        const auto it = by.find(w.name);
        if (it == by.end() || it->second->shape != w.shape || !read(*it->second, v)) return std::nullopt;
        if (is_norm(w.name) && std::any_of(v.begin(), v.end(), [](float x) { return x != 1.0f; })) return std::nullopt;
    }
    Digits d(static_cast<size_t>(weights_));
    const uint32_t levels = 1u << shape_.bits;
    for (const Tensor& t : tensors_)
    {
        if (!read(*by.at(t.name), v)) return std::nullopt;
        for (size_t i = 0; i < v.size(); ++i)
        {
            // The digit whose value this is, if any: the nearest, checked exactly.
            const double m = double(levels - 1);
            const double guess = (double(v[i]) / t.scale * m + m) / 2.0;
            if (!std::isfinite(guess)) return std::nullopt;
            const long g = std::lround(guess);
            if (g < 0 || g >= long(levels) || value(t, uint8_t(g)) != v[i]) return std::nullopt;
            d[size_t(t.first + i)] = uint8_t(g);
        }
    }
    return d;
}

// ---------------------------------------------------------------- the canonical form

namespace {

// A tensor's digits in place: element (r, c).
struct View
{
    uint8_t* d;
    uint32_t rows, cols;
    uint8_t& at(uint32_t r, uint32_t c) const { return d[size_t(r) * cols + c]; }
    void row(uint32_t r, std::vector<uint8_t>& out) const { out.insert(out.end(), d + size_t(r) * cols, d + size_t(r + 1) * cols); }
    void column(uint32_t c, std::vector<uint8_t>& out) const
    {
        for (uint32_t r = 0; r < rows; ++r) out.push_back(at(r, c));
    }
};

// A member's key: SHA-256 of "canonical-model-v1" and its digits (one byte each, in address order),
// then the digits, so members compare as noise does and only equal members tie.
std::vector<uint8_t> key_of(const std::vector<uint8_t>& digits)
{
    Sha256 h;
    h.update(std::string_view(kCanonicalModelVersion));
    h.update(std::span<const uint8_t>(digits));
    const Sha256::Digest dg = h.finish();
    std::vector<uint8_t> k(dg.begin(), dg.end());
    k.insert(k.end(), digits.begin(), digits.end());
    return k;
}

// Two keys of one group (always of one length).
bool key_less(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b)
{
    return std::memcmp(a.data(), b.data(), std::min(a.size(), b.size())) < 0;
}

// C(N + n - 1, n): the sorted runs of n things of N (a product of n numbers, split in halves).
BigUint rising(const BigUint& n0, uint64_t lo, uint64_t hi) // (N + lo) (N + lo + 1) ... (N + hi - 1)
{
    if (hi - lo == 1)
    {
        BigUint x = n0;
        x += BigUint(lo);
        return x;
    }
    const uint64_t mid = lo + (hi - lo) / 2;
    return BigUint::mul(rising(n0, lo, mid), rising(n0, mid, hi));
}
BigUint factorial(uint64_t lo, uint64_t hi) // lo (lo + 1) ... (hi - 1)
{
    if (hi - lo == 1) return BigUint(lo);
    const uint64_t mid = lo + (hi - lo) / 2;
    return BigUint::mul(factorial(lo, mid), factorial(mid, hi));
}
BigUint multisets(const BigUint& n, uint64_t k)
{
    if (k == 0) return BigUint(1);
    BigUint q, r;
    BigUint::divmod(rising(n, 0, k), factorial(1, k + 1), q, r);
    return q;
}
// log2 C(N + n - 1, n), N = 2^bits: N^n / n! to many places, N being far beyond n at every shape.
double log2_multisets(double bits, double n)
{
    if (n == 0) return 0;
    return bits * n - std::lgamma(n + 1) / std::log(2.0);
}

} // namespace

bool AiSpace::is_canonical(const Digits& d) const
{
    return canonical_of(d) == d;
}

AiSpace::Digits AiSpace::canonical_of(const Digits& d0) const
{
    if (d0.size() != weights_) bad("a model of " + std::to_string(weights_) + " weights, not " + std::to_string(d0.size()));
    Digits d = d0;
    const uint32_t m = (1u << shape_.bits) - 1;
    const uint32_t H = shape_.width, F = shape_.ffn(), A = shape_.heads, hd = H / A, hh = hd / 2;
    auto view = [&](size_t i) { return View{d.data() + tensors_[i].first, tensors_[i].rows, tensors_[i].cols}; };
    auto neg = [&](uint8_t& x) { x = uint8_t(m - x); };
    auto negated = [&](std::vector<uint8_t> g) {
        for (uint8_t& x : g) x = uint8_t(m - x);
        return g;
    };
    // The order of a group's members by their keys (ties, equal members, keep their order).
    auto order_of = [](const std::vector<std::vector<uint8_t>>& keys) {
        std::vector<uint32_t> order(keys.size());
        for (uint32_t j = 0; j < order.size(); ++j) order[j] = j;
        std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return key_less(keys[a], keys[b]); });
        return order;
    };
    // Places [first, first + n * block) of rows (or columns) put in `order`, a block at a time.
    auto permute_rows = [](const View& t, uint32_t first, const std::vector<uint32_t>& order, uint32_t block) {
        const std::vector<uint8_t> was(t.d, t.d + size_t(t.rows) * t.cols);
        for (uint32_t j = 0; j < order.size(); ++j)
            for (uint32_t b = 0; b < block; ++b)
                std::memcpy(t.d + size_t(first + j * block + b) * t.cols, was.data() + size_t(first + order[j] * block + b) * t.cols, t.cols);
    };
    auto permute_cols = [](const View& t, uint32_t first, const std::vector<uint32_t>& order, uint32_t block) {
        const std::vector<uint8_t> was(t.d, t.d + size_t(t.rows) * t.cols);
        for (uint32_t r = 0; r < t.rows; ++r)
            for (uint32_t j = 0; j < order.size(); ++j)
                for (uint32_t b = 0; b < block; ++b)
                    t.d[size_t(r) * t.cols + first + j * block + b] = was[size_t(r) * t.cols + first + order[j] * block + b];
    };

    // The residual stream: a dimension is its embedding column; negated where that has the smaller
    // key (with every tensor's column or row of it), then in order.
    const View E = view(embed_);
    std::vector<View> ins, outs; // tensors reading the stream (its dimension a column) and writing it (a row)
    for (const Layer& l : layers_)
    {
        for (size_t i : {l.q, l.k, l.v, l.gate, l.up}) ins.push_back(view(i));
        for (size_t i : {l.o, l.down}) outs.push_back(view(i));
    }
    {
        std::vector<std::vector<uint8_t>> keys(H);
        for (uint32_t i = 0; i < H; ++i)
        {
            std::vector<uint8_t> g;
            E.column(i, g);
            keys[i] = key_of(g);
            std::vector<uint8_t> other = key_of(negated(g));
            if (key_less(other, keys[i]))
            {
                for (uint32_t r = 0; r < E.rows; ++r) neg(E.at(r, i));
                for (const View& t : ins)
                    for (uint32_t r = 0; r < t.rows; ++r) neg(t.at(r, i));
                for (const View& t : outs)
                    for (uint32_t c = 0; c < t.cols; ++c) neg(t.at(i, c));
                keys[i] = std::move(other);
            }
        }
        const std::vector<uint32_t> order = order_of(keys);
        permute_cols(E, 0, order, 1);
        for (const View& t : ins) permute_cols(t, 0, order, 1);
        for (const View& t : outs) permute_rows(t, 0, order, 1);
    }
    for (const Layer& l : layers_)
    {
        // The feed-forward: a unit is its down column, gate row and up row; negated (its down column
        // and up row) where that has the smaller key, then in order.
        const View down = view(l.down), gate = view(l.gate), up = view(l.up);
        {
            std::vector<std::vector<uint8_t>> keys(F);
            for (uint32_t u = 0; u < F; ++u)
            {
                std::vector<uint8_t> g, n;
                down.column(u, g);
                gate.row(u, g);
                up.row(u, g);
                n = g;
                for (uint32_t x = 0; x < H; ++x) n[x] = uint8_t(m - n[x]), n[2 * H + x] = uint8_t(m - n[2 * H + x]);
                keys[u] = key_of(g);
                std::vector<uint8_t> other = key_of(n);
                if (key_less(other, keys[u]))
                {
                    for (uint32_t r = 0; r < H; ++r) neg(down.at(r, u));
                    for (uint32_t c = 0; c < H; ++c) neg(up.at(u, c));
                    keys[u] = std::move(other);
                }
            }
            const std::vector<uint32_t> order = order_of(keys);
            permute_cols(down, 0, order, 1);
            permute_rows(gate, 0, order, 1);
            permute_rows(up, 0, order, 1);
        }
        // The heads.
        const View k = view(l.k), o = view(l.o), q = view(l.q), v = view(l.v);
        auto turn = [&](uint32_t a, uint32_t b) { // a rotary pair a quarter turn: (a, b) to (-b, a), in q and k
            for (const View* t : {&q, &k})
                for (uint32_t c = 0; c < H; ++c)
                {
                    const uint8_t x = t->at(a, c), y = t->at(b, c);
                    t->at(a, c) = uint8_t(m - y);
                    t->at(b, c) = x;
                }
        };
        for (uint32_t h = 0; h < A; ++h)
        {
            // Each rotary pair (its k rows, then its q rows) turned to the turn with the smallest key.
            for (uint32_t i = 0; i < hh; ++i)
            {
                const uint32_t a = h * hd + i, b = a + hh;
                std::vector<uint8_t> best;
                int at = 0;
                for (int n = 0; n < 4; ++n, turn(a, b))
                {
                    std::vector<uint8_t> g;
                    k.row(a, g);
                    k.row(b, g);
                    q.row(a, g);
                    q.row(b, g);
                    std::vector<uint8_t> key = key_of(g);
                    if (n == 0 || key_less(key, best)) best = std::move(key), at = n;
                }
                for (int n = 0; n < at; ++n) turn(a, b);
            }
            // Its value dimensions: a dimension is its o column and v row; negated where that has the
            // smaller key, then in order.
            std::vector<std::vector<uint8_t>> keys(hd);
            for (uint32_t j = 0; j < hd; ++j)
            {
                const uint32_t c = h * hd + j;
                std::vector<uint8_t> g;
                o.column(c, g);
                v.row(c, g);
                keys[j] = key_of(g);
                std::vector<uint8_t> other = key_of(negated(g));
                if (key_less(other, keys[j]))
                {
                    for (uint32_t r = 0; r < H; ++r) neg(o.at(r, c));
                    for (uint32_t x = 0; x < H; ++x) neg(v.at(c, x));
                    keys[j] = std::move(other);
                }
            }
            const std::vector<uint32_t> order = order_of(keys);
            permute_cols(o, h * hd, order, 1);
            permute_rows(v, h * hd, order, 1);
        }
        // Then the heads in order, a head being its k rows, its o columns (row by row), its q rows
        // and its v rows.
        std::vector<std::vector<uint8_t>> keys(A);
        for (uint32_t h = 0; h < A; ++h)
        {
            std::vector<uint8_t> g;
            for (uint32_t r = h * hd; r < (h + 1) * hd; ++r) k.row(r, g);
            for (uint32_t r = 0; r < H; ++r)
                for (uint32_t c = h * hd; c < (h + 1) * hd; ++c) g.push_back(o.at(r, c));
            for (uint32_t r = h * hd; r < (h + 1) * hd; ++r) q.row(r, g);
            for (uint32_t r = h * hd; r < (h + 1) * hd; ++r) v.row(r, g);
            keys[h] = key_of(g);
        }
        const std::vector<uint32_t> order = order_of(keys);
        permute_rows(k, 0, order, hd);
        permute_rows(q, 0, order, hd);
        permute_rows(v, 0, order, hd);
        permute_cols(o, 0, order, hd);
    }
    return d;
}

const BigUint& AiSpace::canonical_count() const
{
    std::call_once(*counted_, [&] {
        const uint32_t B = shape_.bits, H = shape_.width, F = shape_.ffn(), A = shape_.heads, hd = H / A, hh = hd / 2;
        auto two_to = [](uint64_t e) { return BigUint::pow(2, e); };
        BigUint total = multisets(two_to(uint64_t(B) * 256 - 1), H);
        const BigUint ffn = multisets(two_to(uint64_t(B) * 3 * H - 1), F);
        const BigUint head = BigUint::mul(two_to((uint64_t(B) * 4 * H - 2) * hh), multisets(two_to(uint64_t(B) * 2 * H - 1), hd));
        const BigUint layer = BigUint::mul(ffn, multisets(head, A));
        total = BigUint::mul(total, BigUint::pow(layer, shape_.layers));
        *canonical_count_ = std::move(total);
    });
    return *canonical_count_;
}

double AiSpace::canonical_log2_count() const
{
    const double B = shape_.bits, H = shape_.width, F = shape_.ffn(), A = shape_.heads, hd = H / A, hh = hd / 2;
    const double head = (B * 4 * H - 2) * hh + log2_multisets(B * 2 * H - 1, hd);
    const double layer = log2_multisets(B * 3 * H - 1, F) + log2_multisets(head, A);
    return log2_multisets(B * 256 - 1, H) + shape_.layers * layer;
}

} // namespace sieve
