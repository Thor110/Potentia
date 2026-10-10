// Sieve — a language model run by Sieve itself (see llm.hpp).
#include "sieve/llm.hpp"

#include "sieve/safetensors.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>

namespace sieve::llm {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("model: " + what); }

uint32_t size_of(const json::Value& c, const char* key)
{
    const json::Value* v = c.find(key);
    if (!v) bad(std::string("config.json has no ") + key);
    const uint64_t n = v->u64();
    if (n > 1'000'000'000) bad(std::string("config.json's ") + key + " is out of range");
    return uint32_t(n);
}

bool flag(const json::Value& c, const char* key, bool otherwise)
{
    const json::Value* v = c.find(key);
    return v && !v->is_null() ? v->boolean() : otherwise;
}

inline float widen(uint16_t b)
{
    const uint32_t u = uint32_t(b) << 16;
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

// A weight matrix (rows x cols, row-major) as stored: BF16 kept as it is, anything else widened.
struct Matrix
{
    size_t rows = 0, cols = 0;
    std::vector<uint16_t> bf16;
    std::vector<float> f32;
    uint64_t bytes() const { return bf16.size() * 2 + f32.size() * 4; }
    // One row's dot product with x, summed in eight lanes and then lane by lane: one fixed order.
    float dot(size_t r, const float* x) const
    {
        float acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        size_t c = 0;
        if (!bf16.empty())
        {
            const uint16_t* w = bf16.data() + r * cols;
            for (; c + 8 <= cols; c += 8)
                for (int k = 0; k < 8; ++k) acc[k] += widen(w[c + size_t(k)]) * x[c + size_t(k)];
            for (; c < cols; ++c) acc[c % 8] += widen(w[c]) * x[c];
        }
        else
        {
            const float* w = f32.data() + r * cols;
            for (; c + 8 <= cols; c += 8)
                for (int k = 0; k < 8; ++k) acc[k] += w[c + size_t(k)] * x[c + size_t(k)];
            for (; c < cols; ++c) acc[c % 8] += w[c] * x[c];
        }
        return ((acc[0] + acc[1]) + (acc[2] + acc[3])) + ((acc[4] + acc[5]) + (acc[6] + acc[7]));
    }
    float at(size_t r, size_t c) const { return bf16.empty() ? f32[r * cols + c] : widen(bf16[r * cols + c]); }
};

// A small pool of threads that share out the rows of a product.
class Pool
{
public:
    explicit Pool(unsigned n)
    {
        for (unsigned i = 1; i < n; ++i) workers_.emplace_back([this, i] { run(i); });
        n_ = n;
    }
    ~Pool()
    {
        {
            std::lock_guard<std::mutex> l(m_);
            quit_ = true;
        }
        cv_.notify_all();
        for (std::thread& t : workers_) t.join();
    }
    // Calls f(begin, end) over [0, n) in n_ parts, one a thread, and waits.
    void each(size_t n, const std::function<void(size_t, size_t)>& f)
    {
        if (n_ <= 1 || n < 64)
        {
            f(0, n);
            return;
        }
        {
            std::lock_guard<std::mutex> l(m_);
            job_ = &f;
            total_ = n;
            pending_ = n_ - 1;
            ++generation_;
        }
        cv_.notify_all();
        part(0, f);
        std::unique_lock<std::mutex> l(m_);
        done_.wait(l, [this] { return pending_ == 0; });
    }

private:
    std::vector<std::thread> workers_;
    unsigned n_ = 1;
    std::mutex m_;
    std::condition_variable cv_, done_;
    const std::function<void(size_t, size_t)>* job_ = nullptr;
    size_t total_ = 0;
    unsigned pending_ = 0;
    uint64_t generation_ = 0;
    bool quit_ = false;

    void part(unsigned i, const std::function<void(size_t, size_t)>& f)
    {
        const size_t a = total_ * i / n_, b = total_ * (i + 1) / n_;
        if (a < b) f(a, b);
    }
    void run(unsigned i)
    {
        uint64_t seen = 0;
        for (;;)
        {
            const std::function<void(size_t, size_t)>* f;
            {
                std::unique_lock<std::mutex> l(m_);
                cv_.wait(l, [&] { return quit_ || generation_ != seen; });
                if (quit_) return;
                seen = generation_;
                f = job_;
            }
            part(i, *f);
            {
                std::lock_guard<std::mutex> l(m_);
                if (--pending_ == 0) done_.notify_one();
            }
        }
    }
};

} // namespace

Config Config::from_json(const json::Value& c)
{
    if (!c.is_object()) bad("config.json is not a JSON object");
    const json::Value* type = c.find("model_type");
    if (!type || type->string() != "llama") bad("only llama models are covered (config.json's model_type)");
    if (flag(c, "attention_bias", false) || flag(c, "mlp_bias", false)) bad("a llama model with biases is not covered yet");
    if (const json::Value* rs = c.find("rope_scaling"); rs && !rs->is_null()) bad("rope scaling is not covered yet");
    if (const json::Value* act = c.find("hidden_act"); act && act->string() != "silu") bad("an activation other than silu is not covered");
    Config m;
    m.hidden = size_of(c, "hidden_size");
    m.layers = size_of(c, "num_hidden_layers");
    m.inter = size_of(c, "intermediate_size");
    m.vocab = size_of(c, "vocab_size");
    m.heads = size_of(c, "num_attention_heads");
    m.kv_heads = c.find("num_key_value_heads") ? size_of(c, "num_key_value_heads") : m.heads;
    if (!m.heads || !m.kv_heads || m.heads % m.kv_heads) bad("config.json's heads do not divide");
    if (const json::Value* hd = c.find("head_dim"); hd && !hd->is_null()) m.head_dim = uint32_t(hd->u64());
    else
    {
        if (m.hidden % m.heads) bad("config.json's hidden size is not a whole number of heads");
        m.head_dim = m.hidden / m.heads;
    }
    if (m.head_dim % 2) bad("an odd head size");
    m.max_positions = c.find("max_position_embeddings") ? size_of(c, "max_position_embeddings") : 2048;
    if (const json::Value* e = c.find("rms_norm_eps")) m.rms_eps = float(e->number());
    if (const json::Value* t = c.find("rope_theta")) m.rope_theta = float(t->number());
    m.tied = flag(c, "tie_word_embeddings", false);
    return m;
}

struct Model::Impl
{
    struct Layer
    {
        Matrix q, k, v, o, gate, up, down, in_norm, post_norm;
        std::vector<float> keys, values; // position-major: [pos][kv_heads * head_dim]
    };
    std::vector<Layer> layers;
    Matrix embed, norm, out;
    bool tied = false;
    Pool pool;
    std::vector<float> x, h, q, k, v, att, gate, up, scores, logits, inv_freq;
    explicit Impl(unsigned threads) : pool(threads) {}

    void mul(const Matrix& w, const float* in, float* outp)
    {
        pool.each(w.rows, [&](size_t a, size_t b) {
            for (size_t r = a; r < b; ++r) outp[r] = w.dot(r, in);
        });
    }
};

namespace {

Matrix load(std::ifstream& in, uint64_t data_start, const safetensors::Tensor& t, size_t rows, size_t cols)
{
    if (t.elements() != uint64_t(rows) * cols) bad("tensor " + t.name + " is not the shape config.json gives");
    Matrix m;
    m.rows = rows;
    m.cols = cols;
    in.seekg(std::streamoff(data_start + t.begin));
    std::vector<char> raw(size_t(t.bytes()));
    if (!in.read(raw.data(), std::streamsize(raw.size()))) bad("the file ends inside " + t.name);
    const size_t n = rows * cols;
    if (t.dtype == "BF16")
    {
        m.bf16.resize(n);
        for (size_t i = 0; i < n; ++i) m.bf16[i] = uint16_t(uint8_t(raw[2 * i]) | (uint8_t(raw[2 * i + 1]) << 8));
    }
    else if (t.dtype == "F16")
    {
        m.f32.resize(n);
        for (size_t i = 0; i < n; ++i) m.f32[i] = float(safetensors::half_to_double(uint16_t(uint8_t(raw[2 * i]) | (uint8_t(raw[2 * i + 1]) << 8)), false));
    }
    else if (t.dtype == "F32")
    {
        m.f32.resize(n);
        std::memcpy(m.f32.data(), raw.data(), n * 4);
    }
    else bad("tensor " + t.name + " is " + t.dtype + ", which is not covered");
    return m;
}

void rmsnorm(const float* x, const Matrix& w, float eps, size_t n, float* out)
{
    double ss = 0;
    for (size_t i = 0; i < n; ++i) ss += double(x[i]) * x[i];
    const float scale = float(1.0 / std::sqrt(ss / double(n) + double(eps)));
    for (size_t i = 0; i < n; ++i) out[i] = x[i] * scale * w.at(0, i);
}

} // namespace

Model::Model(const json::Value& config, const std::filesystem::path& file, unsigned threads)
    : impl_(std::make_unique<Impl>(threads ? threads : std::max(1u, std::thread::hardware_concurrency()))), cfg_(Config::from_json(config))
{
    std::ifstream in(file, std::ios::binary);
    if (!in) bad("cannot open " + file.string());
    const safetensors::Header h = safetensors::read_header(in);
    std::map<std::string, const safetensors::Tensor*> by;
    for (const safetensors::Tensor& t : h.tensors) by[t.name] = &t;
    auto get = [&](const std::string& name, size_t rows, size_t cols) {
        const auto it = by.find(name);
        if (it == by.end()) bad("the file has no tensor " + name);
        return load(in, h.start_bytes(), *it->second, rows, cols);
    };
    const Config& c = cfg_;
    const size_t qd = size_t(c.heads) * c.head_dim, kvd = size_t(c.kv_heads) * c.head_dim;
    impl_->embed = get("model.embed_tokens.weight", c.vocab, c.hidden);
    impl_->norm = get("model.norm.weight", 1, c.hidden);
    impl_->tied = c.tied || !by.count("lm_head.weight");
    if (!impl_->tied) impl_->out = get("lm_head.weight", c.vocab, c.hidden);
    impl_->layers.resize(c.layers);
    for (uint32_t l = 0; l < c.layers; ++l)
    {
        const std::string p = "model.layers." + std::to_string(l) + ".";
        Impl::Layer& L = impl_->layers[l];
        L.in_norm = get(p + "input_layernorm.weight", 1, c.hidden);
        L.post_norm = get(p + "post_attention_layernorm.weight", 1, c.hidden);
        L.q = get(p + "self_attn.q_proj.weight", qd, c.hidden);
        L.k = get(p + "self_attn.k_proj.weight", kvd, c.hidden);
        L.v = get(p + "self_attn.v_proj.weight", kvd, c.hidden);
        L.o = get(p + "self_attn.o_proj.weight", c.hidden, qd);
        L.gate = get(p + "mlp.gate_proj.weight", c.inter, c.hidden);
        L.up = get(p + "mlp.up_proj.weight", c.inter, c.hidden);
        L.down = get(p + "mlp.down_proj.weight", c.hidden, c.inter);
    }
    Impl& I = *impl_;
    I.x.resize(c.hidden);
    I.h.resize(c.hidden);
    I.q.resize(qd);
    I.k.resize(kvd);
    I.v.resize(kvd);
    I.att.resize(qd);
    I.gate.resize(c.inter);
    I.up.resize(c.inter);
    I.logits.resize(c.vocab);
    // The rotary embedding's frequencies, as transformers computes them (in 32-bit floats).
    for (uint32_t i = 0; i < c.head_dim / 2; ++i) I.inv_freq.push_back(1.0f / std::pow(c.rope_theta, float(2 * i) / float(c.head_dim)));
}

Model::~Model() = default;

uint64_t Model::weight_bytes() const
{
    uint64_t b = impl_->embed.bytes() + impl_->norm.bytes() + impl_->out.bytes();
    for (const Impl::Layer& L : impl_->layers)
        b += L.q.bytes() + L.k.bytes() + L.v.bytes() + L.o.bytes() + L.gate.bytes() + L.up.bytes() + L.down.bytes() + L.in_norm.bytes() + L.post_norm.bytes();
    return b;
}

const std::vector<float>& Model::step(uint32_t token)
{
    const Config& c = cfg_;
    if (token >= c.vocab) bad("token " + std::to_string(token) + " is past the vocabulary");
    if (pos_ >= c.max_positions) bad("the conversation is longer than the model's context (" + std::to_string(c.max_positions) + " tokens)");
    Impl& I = *impl_;
    const size_t hd = c.head_dim, half = hd / 2, kvd = size_t(c.kv_heads) * hd, group = c.heads / c.kv_heads;
    for (size_t i = 0; i < c.hidden; ++i) I.x[i] = I.embed.at(token, i);
    // The rotation at this position.
    std::vector<float> cosv(half), sinv(half);
    for (size_t i = 0; i < half; ++i)
    {
        const float angle = float(pos_) * I.inv_freq[i];
        cosv[i] = std::cos(angle);
        sinv[i] = std::sin(angle);
    }
    auto rotate = [&](float* v) {
        for (size_t i = 0; i < half; ++i)
        {
            const float a = v[i], b = v[i + half];
            v[i] = a * cosv[i] - b * sinv[i];
            v[i + half] = b * cosv[i] + a * sinv[i];
        }
    };
    const float inv_sqrt = 1.0f / std::sqrt(float(hd));
    I.scores.resize(pos_ + 1);
    for (Impl::Layer& L : I.layers)
    {
        rmsnorm(I.x.data(), L.in_norm, c.rms_eps, c.hidden, I.h.data());
        I.mul(L.q, I.h.data(), I.q.data());
        I.mul(L.k, I.h.data(), I.k.data());
        I.mul(L.v, I.h.data(), I.v.data());
        for (size_t hq = 0; hq < c.heads; ++hq) rotate(I.q.data() + hq * hd);
        for (size_t hk = 0; hk < c.kv_heads; ++hk) rotate(I.k.data() + hk * hd);
        L.keys.insert(L.keys.end(), I.k.begin(), I.k.end());
        L.values.insert(L.values.end(), I.v.begin(), I.v.end());
        for (size_t hq = 0; hq < c.heads; ++hq)
        {
            const size_t hk = hq / group;
            const float* qv = I.q.data() + hq * hd;
            float best = -INFINITY;
            for (size_t t = 0; t <= pos_; ++t)
            {
                const float* kv = L.keys.data() + t * kvd + hk * hd;
                float s = 0;
                for (size_t i = 0; i < hd; ++i) s += qv[i] * kv[i];
                I.scores[t] = s * inv_sqrt;
                best = std::max(best, I.scores[t]);
            }
            float sum = 0;
            for (size_t t = 0; t <= pos_; ++t) sum += (I.scores[t] = std::exp(I.scores[t] - best));
            float* o = I.att.data() + hq * hd;
            std::fill(o, o + hd, 0.0f);
            for (size_t t = 0; t <= pos_; ++t)
            {
                const float p = I.scores[t] / sum;
                const float* vv = L.values.data() + t * kvd + hk * hd;
                for (size_t i = 0; i < hd; ++i) o[i] += p * vv[i];
            }
        }
        I.mul(L.o, I.att.data(), I.h.data());
        for (size_t i = 0; i < c.hidden; ++i) I.x[i] += I.h[i];
        rmsnorm(I.x.data(), L.post_norm, c.rms_eps, c.hidden, I.h.data());
        I.mul(L.gate, I.h.data(), I.gate.data());
        I.mul(L.up, I.h.data(), I.up.data());
        for (size_t i = 0; i < c.inter; ++i)
        {
            const float g = I.gate[i];
            I.gate[i] = g / (1.0f + std::exp(-g)) * I.up[i];
        }
        I.mul(L.down, I.gate.data(), I.h.data());
        for (size_t i = 0; i < c.hidden; ++i) I.x[i] += I.h[i];
    }
    rmsnorm(I.x.data(), I.norm, c.rms_eps, c.hidden, I.h.data());
    I.mul(I.tied ? I.embed : I.out, I.h.data(), I.logits.data());
    ++pos_;
    return I.logits;
}

// ---------------------------------------------------------------- sampling

Sampler::Sampler(Sampling s) : s_(s), state_(s.seed) {}

double Sampler::uniform()
{
    // splitmix64: the same numbers on every machine for a seed.
    uint64_t z = (state_ += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return double(z >> 11) * (1.0 / 9007199254740992.0);
}

uint32_t Sampler::pick(std::vector<float> logits, const std::vector<uint32_t>& recent)
{
    if (logits.empty()) throw std::invalid_argument("no logits to choose from");
    if (s_.repeat_penalty > 0 && s_.repeat_penalty != 1.0f)
        for (const uint32_t t : recent)
            if (t < logits.size()) logits[t] = logits[t] > 0 ? logits[t] / s_.repeat_penalty : logits[t] * s_.repeat_penalty;
    if (s_.temperature <= 0) return uint32_t(std::max_element(logits.begin(), logits.end()) - logits.begin());
    std::vector<uint32_t> order(logits.size());
    std::iota(order.begin(), order.end(), 0u);
    std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return logits[a] > logits[b]; });
    if (s_.top_k && s_.top_k < order.size()) order.resize(s_.top_k);
    std::vector<double> p(order.size());
    const double top = logits[order[0]];
    double sum = 0;
    for (size_t i = 0; i < order.size(); ++i) sum += (p[i] = std::exp((double(logits[order[i]]) - top) / s_.temperature));
    size_t keep = order.size();
    if (s_.top_p > 0 && s_.top_p < 1)
    {
        double acc = 0;
        for (size_t i = 0; i < order.size(); ++i)
        {
            acc += p[i] / sum;
            if (acc >= s_.top_p)
            {
                keep = i + 1;
                break;
            }
        }
    }
    double kept = 0;
    for (size_t i = 0; i < keep; ++i) kept += p[i];
    double r = uniform() * kept;
    for (size_t i = 0; i < keep; ++i)
    {
        r -= p[i];
        if (r < 0) return order[i];
    }
    return order[keep - 1];
}

// ---------------------------------------------------------------- the chat

Chat::Chat(Model& model, const Tokenizer& tok, std::string system) : model_(model), tok_(tok), system_(std::move(system))
{
    const auto e = tok.find("<|im_end|>"), s = tok.find("<|im_start|>");
    if (!e || !s) throw std::invalid_argument("the tokenizer has no <|im_start|> and <|im_end|>: not a ChatML model");
    im_end_ = *e;
    end_of_text_ = tok.find("<|endoftext|>").value_or(*e);
}

void Chat::feed(const std::vector<uint32_t>& ids)
{
    for (const uint32_t id : ids) logits_ = model_.step(id);
}

namespace {

// How many bytes at the start of `s` are whole UTF-8 characters.
size_t whole(const std::string& s)
{
    size_t i = 0, ok = 0;
    while (i < s.size())
    {
        const uint8_t c = uint8_t(s[i]);
        const size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + n > s.size()) break;
        i += n;
        ok = i;
    }
    return ok;
}

} // namespace

std::string Chat::reply(const std::string& message, Sampler& sampler, size_t max_tokens, const std::function<bool(const std::string&)>& on_text)
{
    std::string prompt;
    if (!started_) prompt = "<|im_start|>system\n" + system_ + "<|im_end|>\n";
    prompt += "<|im_start|>user\n" + message + "<|im_end|>\n<|im_start|>assistant\n";
    feed(tok_.encode(prompt, true));
    started_ = true;
    std::vector<uint32_t> said;
    std::string text, pending;
    bool closed = false;
    while (said.size() < max_tokens)
    {
        const uint32_t t = sampler.pick(logits_, said);
        logits_ = model_.step(t);
        if (t == im_end_ || t == end_of_text_)
        {
            closed = t == im_end_;
            break;
        }
        said.push_back(t);
        pending += tok_.bytes_of(t);
        const size_t n = whole(pending);
        if (n)
        {
            const std::string piece = pending.substr(0, n);
            pending.erase(0, n);
            text += piece;
            if (on_text && !on_text(piece)) break;
        }
    }
    replied_ = said.size();
    text += pending;
    if (on_text && !pending.empty()) on_text(pending);
    // The turn closed as ChatML closes it, so the next one follows on.
    if (!closed) feed({im_end_});
    feed(tok_.encode("\n", false));
    return text;
}

} // namespace sieve::llm
