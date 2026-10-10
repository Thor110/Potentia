// Sieve — the AI line: every language model of one shape, numbered by its weights (SPECIFICATIONS
// §12.0d, "aispace-v1"; IDEAS §15). Edward's choice: an item on this line is a model, to take off the
// shelf and talk to; its weights are its address, so walking the shelves needs no download.
//
// The shape (AiShape): a Llama model (sieve/llm.hpp) of L layers, width H, A heads (head size H / A,
// even; as many key/value heads), a feed-forward of 4H, and a vocabulary of the 256 bytes (token b
// is byte b), its output tied to its embedding, its norms all 1. Its weights are the embedding
// [256, H] and, in each layer, q, k, v, o [H, H], gate and up [4H, H] and down [H, 4H]: W of them,
// each a digit of B bits (`bits`, 1 to 8). Digit d of a tensor with c columns stands for
//     value = (2d - m) / m * scale,   m = 2^B - 1,
// evenly spaced over [-scale, scale], scale = 1 for the embedding and sqrt(3 / c) for the rest (so a
// model's activations keep their size), in 64-bit arithmetic rounded once to a 32-bit float.
//
// The line holds 2^(B W) models. Positional order reads the digits as one number in base 2^B, the
// first digit most significant, the tensors in safetensors-layout-v1's order (by name) and each
// row-major: an address is the weights' digits, written out. Scrambled order passes that index
// through shuffle-sha256-v1 over the whole line, keyed with the line's key, domain = the space's id,
// "ai/L<L>/H<H>/A<A>/F<4H>/B<B>/V256/key=<key>/aispace-v1". Addresses are hex, zero-padded to the
// width 2^(B W) - 1 needs.
//
// A model's files (to keep, or to talk to with `sieve chat --raw`): config.json, model.safetensors
// (F32, every tensor of the shape, the norms' ones included, laid out by safetensors-layout-v1) and
// tokenizer.json (byte-level BPE with the 256 bytes and no merges).
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/json.hpp"
#include "sieve/space.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kAiSpaceVersion = "aispace-v1";

struct AiShape
{
    uint32_t layers = 1, width = 16, heads = 2, bits = 4;
    uint32_t ffn() const { return 4 * width; }
    void check() const; // throws std::invalid_argument for a shape this line cannot have
};

class AiSpace
{
public:
    using Digits = std::vector<uint8_t>; // W digits, each below 2^bits

    struct Tensor
    {
        std::string name;
        uint32_t rows, cols;
        double scale;
        uint64_t first; // its first digit's place among the W
    };

    AiSpace(AiShape shape, std::string key);

    const AiShape& shape() const { return shape_; }
    const std::string& key() const { return key_; }
    uint64_t weights() const { return weights_; }              // W
    uint64_t bits() const { return weights_ * shape_.bits; }   // log2 of the line's size
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    std::string id() const;
    const std::vector<Tensor>& tensors() const { return tensors_; } // the weights, in address order

    Digits digits_at(const BigUint& index, AddressMode m) const; // index < size()
    BigUint index_of(const Digits& d, AddressMode m) const;      // throws if not W digits of `bits` bits
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked

    float value(const Tensor& t, uint8_t digit) const;

    // The model: its config.json, its tensors (every one of the shape, norms included) by name, its
    // files.
    json::Value config() const;
    std::map<std::string, std::vector<float>> tensors_of(const Digits& d) const;
    std::string safetensors_of(const Digits& d) const;
    static std::string tokenizer_json(); // byte-level BPE: the 256 bytes, no merges
    // A model file back to its digits: nullopt unless it is a model of this shape whose every
    // weight is one of the line's values exactly (and whose norms are all 1).
    std::optional<Digits> digits_of_safetensors(std::string_view file) const;

private:
    AiShape shape_;
    std::string key_;
    std::vector<Tensor> tensors_;
    uint64_t weights_ = 0;
    BigUint size_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
};

} // namespace sieve
