// Sieve — model files in the safetensors format, taken apart (see safetensors.hpp).
#include "sieve/safetensors.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

namespace sieve::safetensors {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("safetensors: " + what); }

uint64_t mul(uint64_t a, uint64_t b)
{
    if (a && b > UINT64_MAX / a) bad("a tensor too large to count");
    return a * b;
}

json::Value number(uint64_t v) { return json::Value::make_number(std::to_string(v)); }

} // namespace

int dtype_rank(const std::string& dtype)
{
    for (size_t i = 0; i < kDTypes.size(); ++i)
        if (dtype == kDTypes[i]) return int(i);
    return -1;
}

size_t dtype_bytes(const std::string& dtype)
{
    static const std::map<std::string, size_t> sizes = {{"BOOL", 1}, {"U8", 1},  {"I8", 1},  {"F8_E5M2", 1}, {"F8_E4M3", 1},
                                                        {"I16", 2},  {"U16", 2}, {"F16", 2}, {"BF16", 2},    {"I32", 4},
                                                        {"U32", 4},  {"F32", 4}, {"F64", 8}, {"I64", 8},     {"U64", 8}};
    const auto it = sizes.find(dtype);
    return it == sizes.end() ? 0 : it->second;
}

uint64_t Tensor::elements() const
{
    uint64_t n = 1;
    for (const uint64_t d : shape) n = mul(n, d);
    return n;
}

uint64_t Header::data_bytes() const
{
    uint64_t e = 0;
    for (const Tensor& t : tensors) e = std::max(e, t.end);
    return e;
}

Header parse_header(std::span<const uint8_t> start)
{
    if (start.size() < 8) bad("shorter than its 8-byte length");
    uint64_t n = 0;
    for (int i = 7; i >= 0; --i) n = (n << 8) | start[size_t(i)];
    if (n > start.size() - 8) bad("the header runs past the bytes given");
    Header h;
    h.json_bytes = n;
    h.json.assign(reinterpret_cast<const char*>(start.data()) + 8, size_t(n));
    const json::Value root = json::parse(h.json);
    if (!root.is_object()) bad("the header is not a JSON object");
    for (const auto& [name, v] : root.object())
    {
        if (name == "__metadata__")
        {
            if (h.metadata) bad("two __metadata__ members");
            std::vector<std::pair<std::string, std::string>> m;
            for (const auto& [k, s] : v.object()) m.emplace_back(k, s.string());
            h.metadata = std::move(m);
            continue;
        }
        Tensor t;
        t.name = name;
        t.dtype = v.at("dtype").string();
        if (!dtype_bytes(t.dtype)) bad("tensor " + name + ": an unknown type " + t.dtype);
        for (const json::Value& d : v.at("shape").array()) t.shape.push_back(d.u64());
        const auto& off = v.at("data_offsets").array();
        if (off.size() != 2) bad("tensor " + name + ": data_offsets is not two numbers");
        t.begin = off[0].u64();
        t.end = off[1].u64();
        if (t.end < t.begin || t.end - t.begin != mul(t.elements(), dtype_bytes(t.dtype)))
            bad("tensor " + name + ": its offsets do not hold its shape");
        h.tensors.push_back(std::move(t));
    }
    // The tensors fill the data end to end, with no gaps and no overlaps.
    std::vector<const Tensor*> by(h.tensors.size());
    for (size_t i = 0; i < by.size(); ++i) by[i] = &h.tensors[i];
    std::sort(by.begin(), by.end(), [](const Tensor* a, const Tensor* b) { return a->begin != b->begin ? a->begin < b->begin : a->end < b->end; });
    uint64_t at = 0;
    for (const Tensor* t : by)
    {
        if (t->begin != at) bad("tensor " + t->name + (t->begin < at ? " overlaps the one before it" : " leaves a gap before it"));
        at = t->end;
    }
    for (size_t i = 1; i < by.size(); ++i)
        if (by[i]->name == by[i - 1]->name) bad("two tensors named " + by[i]->name);
    return h;
}

Header read_header(std::istream& in, uint64_t max_json)
{
    uint8_t len[8];
    if (!in.read(reinterpret_cast<char*>(len), 8)) bad("shorter than its 8-byte length");
    uint64_t n = 0;
    for (int i = 7; i >= 0; --i) n = (n << 8) | len[i];
    if (n > max_json) bad("a header of " + std::to_string(n) + " bytes, more than " + std::to_string(max_json));
    std::vector<uint8_t> start(size_t(8 + n));
    std::memcpy(start.data(), len, 8);
    if (!in.read(reinterpret_cast<char*>(start.data()) + 8, std::streamsize(n))) bad("the header runs past the end of the file");
    return parse_header(start);
}

std::string layout_start(const std::optional<std::vector<std::pair<std::string, std::string>>>& metadata, std::vector<Tensor> tensors,
                         std::vector<Tensor>* laid)
{
    for (const Tensor& t : tensors)
        if (dtype_rank(t.dtype) < 0) bad("tensor " + t.name + ": an unknown type " + t.dtype);
    std::sort(tensors.begin(), tensors.end(), [](const Tensor& a, const Tensor& b) {
        const int ra = dtype_rank(a.dtype), rb = dtype_rank(b.dtype);
        return ra != rb ? ra > rb : a.name < b.name;
    });
    std::vector<std::pair<std::string, json::Value>> members;
    if (metadata)
    {
        std::vector<std::pair<std::string, json::Value>> m;
        for (const auto& [k, v] : *metadata) m.emplace_back(k, json::Value::make_string(v));
        members.emplace_back("__metadata__", json::Value::make_object(std::move(m)));
    }
    uint64_t at = 0;
    for (Tensor& t : tensors)
    {
        t.begin = at;
        t.end = at = at + mul(t.elements(), dtype_bytes(t.dtype));
        std::vector<json::Value> shape;
        for (const uint64_t d : t.shape) shape.push_back(number(d));
        members.emplace_back(t.name, json::Value::make_object({{"dtype", json::Value::make_string(t.dtype)},
                                                               {"shape", json::Value::make_array(std::move(shape))},
                                                               {"data_offsets", json::Value::make_array({number(t.begin), number(t.end)})}}));
    }
    std::string j = json::write(json::Value::make_object(std::move(members)));
    j.append((8 - j.size() % 8) % 8, ' ');
    std::string out(8, '\0');
    uint64_t n = j.size();
    for (int i = 0; i < 8; ++i, n >>= 8) out[size_t(i)] = char(n & 0xFF);
    if (laid) *laid = std::move(tensors);
    return out + j;
}

bool is_canonical(const Header& h)
{
    const std::string s = layout_start(h.metadata, h.tensors);
    return s.size() == h.start_bytes() && s.compare(8, std::string::npos, h.json) == 0;
}

namespace {

uint64_t size_of(const json::Value& c, const char* key)
{
    const json::Value* v = c.find(key);
    if (!v) bad(std::string("config.json has no ") + key);
    return v->u64();
}

bool flag(const json::Value& c, const char* key, bool otherwise)
{
    const json::Value* v = c.find(key);
    return v && !v->is_null() ? v->boolean() : otherwise;
}

struct Llama
{
    uint64_t hidden, layers, inter, vocab, heads, kv_heads, head_dim;
    bool tied;
    std::string dtype;
};

Llama llama_of(const json::Value& c)
{
    if (!c.is_object()) bad("config.json is not a JSON object");
    const json::Value* type = c.find("model_type");
    if (!type || type->string() != "llama") bad("config.json is not a llama model's (model_type)");
    if (flag(c, "attention_bias", false) || flag(c, "mlp_bias", false)) bad("a llama model with biases is not covered yet");
    Llama m;
    m.hidden = size_of(c, "hidden_size");
    m.layers = size_of(c, "num_hidden_layers");
    m.inter = size_of(c, "intermediate_size");
    m.vocab = size_of(c, "vocab_size");
    m.heads = size_of(c, "num_attention_heads");
    m.kv_heads = c.find("num_key_value_heads") ? size_of(c, "num_key_value_heads") : m.heads;
    if (!m.heads || !m.kv_heads || m.heads % m.kv_heads) bad("config.json's heads do not divide");
    if (const json::Value* hd = c.find("head_dim"); hd && !hd->is_null()) m.head_dim = hd->u64();
    else
    {
        if (m.hidden % m.heads) bad("config.json's hidden size is not a whole number of heads");
        m.head_dim = m.hidden / m.heads;
    }
    m.tied = flag(c, "tie_word_embeddings", false); // LlamaConfig's default
    const json::Value* td = c.find("torch_dtype");
    const std::string t = td ? td->string() : "float32";
    m.dtype = t == "bfloat16" ? "BF16" : t == "float16" ? "F16" : t == "float32" ? "F32" : "";
    if (m.dtype.empty()) bad("config.json's torch_dtype " + t + " is not covered");
    return m;
}

} // namespace

std::vector<Tensor> llama_tensors(const json::Value& config)
{
    const Llama m = llama_of(config);
    std::vector<Tensor> out;
    auto add = [&](std::string name, std::vector<uint64_t> shape) {
        Tensor t;
        t.name = std::move(name);
        t.dtype = m.dtype;
        t.shape = std::move(shape);
        out.push_back(std::move(t));
    };
    const uint64_t q = m.heads * m.head_dim, kv = m.kv_heads * m.head_dim;
    add("model.embed_tokens.weight", {m.vocab, m.hidden});
    for (uint64_t l = 0; l < m.layers; ++l)
    {
        const std::string p = "model.layers." + std::to_string(l) + ".";
        add(p + "input_layernorm.weight", {m.hidden});
        add(p + "post_attention_layernorm.weight", {m.hidden});
        add(p + "self_attn.q_proj.weight", {q, m.hidden});
        add(p + "self_attn.k_proj.weight", {kv, m.hidden});
        add(p + "self_attn.v_proj.weight", {kv, m.hidden});
        add(p + "self_attn.o_proj.weight", {m.hidden, q});
        add(p + "mlp.gate_proj.weight", {m.inter, m.hidden});
        add(p + "mlp.up_proj.weight", {m.inter, m.hidden});
        add(p + "mlp.down_proj.weight", {m.hidden, m.inter});
    }
    add("model.norm.weight", {m.hidden});
    if (!m.tied) add("lm_head.weight", {m.vocab, m.hidden});
    return out;
}

std::string llama_summary(const json::Value& config)
{
    const Llama m = llama_of(config);
    return "llama, " + std::to_string(m.layers) + " layers, hidden " + std::to_string(m.hidden) + ", feed-forward " + std::to_string(m.inter) + ", " +
           std::to_string(m.heads) + " heads (" + std::to_string(m.kv_heads) + " key/value) of " + std::to_string(m.head_dim) + ", vocabulary " +
           std::to_string(m.vocab) + (m.tied ? ", embedding tied to the output" : ", output layer of its own") + ", " + m.dtype;
}

bool has_stats(const std::string& dtype) { return dtype == "BF16" || dtype == "F16"; }

double half_to_double(uint16_t bits, bool bf16)
{
    if (bf16)
    {
        const uint32_t u = uint32_t(bits) << 16;
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }
    const int sign = bits >> 15, exp = (bits >> 10) & 0x1F, frac = bits & 0x3FF;
    double v;
    if (exp == 0) v = std::ldexp(double(frac), -24);
    else if (exp == 31) v = frac ? std::nan("") : INFINITY;
    else v = std::ldexp(double(frac + 1024), exp - 25);
    return sign ? -v : v;
}

StatsBuilder::StatsBuilder(const std::string& dtype) : bf16_(dtype == "BF16"), counts_(65536, 0)
{
    if (!has_stats(dtype)) bad("statistics are for BF16 and F16 tensors, not " + dtype);
}

void StatsBuilder::add(std::span<const uint8_t> bytes)
{
    if (bytes.size() % 2) bad("half an element");
    for (size_t i = 0; i < bytes.size(); i += 2)
    {
        const uint16_t v = uint16_t(bytes[i] | (bytes[i + 1] << 8));
        ++counts_[v];
        const double x = half_to_double(v, bf16_);
        sum_ += x;
        sum2_ += x * x;
        ++n_;
    }
}

TensorStats StatsBuilder::finish() const
{
    TensorStats s;
    s.values = n_;
    if (!n_) return s;
    std::array<uint64_t, 256> hi{}, lo{};
    for (size_t v = 0; v < counts_.size(); ++v)
        if (counts_[v])
        {
            ++s.distinct;
            hi[v >> 8] += counts_[v];
            lo[v & 0xFF] += counts_[v];
        }
    const double n = double(n_);
    auto entropy = [n](const auto& c) {
        double h = 0;
        for (const uint64_t k : c)
            if (k)
            {
                const double p = double(k) / n;
                h -= p * std::log2(p);
            }
        return h;
    };
    s.h_value = entropy(counts_);
    s.h_high = entropy(hi);
    s.h_low = entropy(lo);
    s.mean = sum_ / n;
    s.sd = std::sqrt(std::max(0.0, sum2_ / n - s.mean * s.mean));
    return s;
}

} // namespace sieve::safetensors
