// Sieve — a language model run by Sieve itself: the forward pass of a Llama model, sampling, and
// a chat in the model's own format (IDEAS §15; SPECIFICATIONS §12.0c). No outside runtime: the
// weights are read from the model's safetensors file, the tokenizer from its tokenizer.json.
//
// The forward pass, for one token at position p (as the transformers library computes it):
//   x = the token's row of the embedding
//   for each layer:
//     h = rmsnorm(x) * input_layernorm
//     q, k, v = Wq h, Wk h, Wv h; q and k turned by the rotary position embedding (each head's
//       first half and second half paired, angle p / theta^(2i / head_dim)); k and v kept
//     each query head attends over the keys and values of its group's head at 0..p, with scores
//       q.k / sqrt(head_dim), through a softmax
//     x += Wo (the heads' outputs, joined)
//     h = rmsnorm(x) * post_attention_layernorm
//     x += Wdown (silu(Wgate h) * Wup h)
//   logits = Wout (rmsnorm(x) * norm), Wout the embedding when tied
// rmsnorm(x) = x / sqrt(mean(x^2) + eps). Weights stay as they are stored (BF16 is widened as it is
// used); the arithmetic is in 32-bit floats, each sum taken in one fixed order, so the logits are the
// same whatever the number of threads. This is the model as a viewer runs it: floating point, so
// it may differ in the last bits between processors and compilers. Addresses under a model will
// need the pinned arithmetic IDEAS §15 describes.
//
// The chat (ChatML, the format SmolLM2's template writes):
//   <|im_start|>system\n{system}<|im_end|>\n<|im_start|>user\n{message}<|im_end|>\n<|im_start|>assistant\n
// then the model's reply up to <|im_end|>, and "\n" after it; each later turn adds its user and
// assistant parts the same way. The system line defaults to the one the template writes.
#pragma once

#include "sieve/json.hpp"
#include "sieve/llm_tokenizer.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sieve::llm {

struct Config
{
    uint32_t hidden = 0, layers = 0, inter = 0, vocab = 0, heads = 0, kv_heads = 0, head_dim = 0, max_positions = 0;
    float rms_eps = 1e-5f, rope_theta = 10000.0f;
    bool tied = false;
    // Throws std::invalid_argument for a configuration not covered (not llama, biases, rope scaling).
    static Config from_json(const json::Value& config);
};

class Model
{
public:
    // From config.json and the safetensors file of its weights. `threads` 0: as many as the machine has.
    Model(const json::Value& config, const std::filesystem::path& safetensors, unsigned threads = 0);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    const Config& config() const { return cfg_; }
    size_t position() const { return pos_; } // tokens seen
    void reset() { pos_ = 0; }
    // Feeds one token at the next position; returns the logits for the token after it.
    const std::vector<float>& step(uint32_t token);
    uint64_t weight_bytes() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Config cfg_;
    size_t pos_ = 0;
};

// Choosing the next token from the logits.
struct Sampling
{
    float temperature = 0.2f; // 0: always the most likely token
    float top_p = 0.9f;       // the fewest likeliest tokens whose probabilities reach this
    uint32_t top_k = 0;       // 0: no limit
    float repeat_penalty = 1.0f; // > 1: tokens already in the reply are made less likely
    uint64_t seed = 1;
};

class Sampler
{
public:
    explicit Sampler(Sampling s);
    uint32_t pick(std::vector<float> logits, const std::vector<uint32_t>& recent);

private:
    Sampling s_;
    uint64_t state_;
    double uniform(); // [0, 1), the same on every machine for a seed
};

// A conversation with a model in ChatML.
class Chat
{
public:
    static constexpr const char* kDefaultSystem = "You are a helpful AI assistant named SmolLM, trained by Hugging Face";
    Chat(Model& model, const Tokenizer& tok, std::string system = kDefaultSystem);
    // The model's reply to `message`; `on_text` is given the reply's text as it comes (whole UTF-8
    // characters), and may return false to stop it. At most `max_tokens` tokens.
    std::string reply(const std::string& message, Sampler& sampler, size_t max_tokens, const std::function<bool(const std::string&)>& on_text = {});
    size_t tokens() const { return model_.position(); }
    size_t last_reply_tokens() const { return replied_; } // the tokens of the last reply (not its prompt)

private:
    Model& model_;
    const Tokenizer& tok_;
    std::string system_;
    bool started_ = false;
    size_t replied_ = 0;
    uint32_t im_end_ = 0, end_of_text_ = 0;
    std::vector<float> logits_;
    void feed(const std::vector<uint32_t>& ids);
};

} // namespace sieve::llm
