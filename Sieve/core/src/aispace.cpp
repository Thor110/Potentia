// Sieve — the AI line: every language model of one shape (see aispace.hpp).
#include "sieve/aispace.hpp"

#include "sieve/llm_tokenizer.hpp"
#include "sieve/safetensors.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
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

} // namespace sieve
