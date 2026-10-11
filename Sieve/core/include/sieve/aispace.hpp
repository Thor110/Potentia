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
//
// Many models of the line are one model: its symmetries (canonical-model-v1, SPECIFICATIONS §12.0f)
// change the weights and not what the model computes. With a digit d's negation m - d (the values
// are symmetric about 0, and none is 0, since m is odd):
//   the residual stream   its H dimensions permuted, and any of them negated: the embedding's
//                         columns, the columns of q, k, v, gate and up, and the rows of o and down
//                         together (the norms' weights are all 1, so they see no difference);
//   a layer's feed-forward  its 4H units permuted (gate and up rows, down columns together), and
//                         any unit's up row and down column negated (silu(g) u: -u, then -down);
//   a layer's heads       permuted (their blocks of q, k and v rows and o columns together);
//   a head's values       its value dimensions permuted and any negated (v rows, o columns);
//   a head's rotary pairs each pair of dimensions (i, i + size/2) of q and k turned a quarter
//                         turn at once, (a, b) to (-b, a), which rotary embedding commutes with.
// Negation and the quarter turn change no number the model computes; a permutation changes the
// order of some sums, so a permuted model's output may differ in the last bits of a float.
// The canonical form picks one model of each class. Each member of a group (a dimension of the
// stream, a unit, a rotary pair, a value dimension, a head) is read as its digits in address order
// and keyed by SHA-256("canonical-model-v1" || those digits), then the digits: every negation or turn
// is the one with the smallest key, and every permuted group is in order of its keys. Chosen by a
// hash, not by the digits themselves, so a canonical model looks like any other (sorting by a first
// row would leave that row in order, which is the structure weight-rows-v1 sets aside).
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/json.hpp"
#include "sieve/space.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kAiSpaceVersion = "aispace-v1";
inline constexpr const char* kCanonicalModelVersion = "canonical-model-v1";

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

    // The canonical form (above): whether a model is in it, and the model of its class that is.
    bool is_canonical(const Digits& d) const;
    Digits canonical_of(const Digits& d) const;
    // How many models are canonical, exactly: C(K^256/2 + H - 1, H), times, for each layer, the
    // feed-forward's C(K^(3H)/2 + 4H - 1, 4H) and the heads' C(N + A - 1, A), where a head is one of
    // N = (K^(4H)/4)^(size/2) C(K^(2H)/2 + size - 1, size), K = 2^B (C(N + n - 1, n): the sorted
    // runs of n of N things). Worked out once, when first asked for (seconds at a width of 1024).
    const BigUint& canonical_count() const;
    double canonical_log2_count() const; // the same, as a log, at once, for any shape

private:
    AiShape shape_;
    std::string key_;
    std::vector<Tensor> tensors_;
    uint64_t weights_ = 0;
    BigUint size_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
    struct Layer // each tensor's place in tensors_
    {
        size_t down, gate, up, k, o, q, v;
    };
    size_t embed_ = 0;
    std::vector<Layer> layers_;
    mutable std::shared_ptr<std::once_flag> counted_ = std::make_shared<std::once_flag>();
    mutable std::shared_ptr<BigUint> canonical_count_ = std::make_shared<BigUint>();
};

} // namespace sieve
