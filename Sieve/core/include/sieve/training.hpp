// Sieve — training a model of the AI line in pinned integer arithmetic, so that the same data in the
// same order gives the same model, bit for bit, on every machine (SPECIFICATIONS §12.0e,
// "training-v1"; IDEAS §15). Floating point would not: its last bits move with the processor, the
// compiler and the thread count, and a training run that cannot be repeated cannot be cited.
//
// A model in training is a model of the AI line (sieve/aispace.hpp): its forward pass uses the
// values of its digits, and each digit is the line's value nearest a master weight kept to 32
// fractional bits (training with the quantisation in the loop, the master weights taking the
// gradient as if the rounding were not there). So every checkpoint is an address on the line,
// and a training run is a path along it.
//
// Every number is a 64-bit integer: activations, gradients and the weights' values with 16
// fractional bits (Q16), the master weights and their gradients with 32. Every rounding is down
// (floor), every stored value is held to [-2^23, 2^23], and the functions it needs (2^x, sine and
// cosine, the square root) are fixed polynomials and integer steps, all given in the
// specification, which the oracle (reference/sieve_ref.py) follows apart. Integer sums do not
// depend on their order, so threads change nothing.
#pragma once

#include "sieve/aispace.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::training {

inline constexpr const char* kTrainingVersion = "training-v1";

// The fixed-point functions, exposed for the tests and the specification.
namespace fx {
inline constexpr int64_t kOne = int64_t(1) << 16; // 1.0 in Q16
inline constexpr int64_t kLimit = int64_t(1) << 23; // every stored Q16 value is held to [-kLimit, kLimit]
int64_t fdiv(int64_t a, int64_t b);                 // floor(a / b), b > 0
uint64_t isqrt(uint64_t n);                         // floor(sqrt(n))
int64_t pow2_frac(int64_t f30);                     // 2^(f / 2^30) in Q30, for f in [0, 2^30)
int64_t exp_q16(int64_t z);                         // e^z in Q16, for z <= 0 in Q16
int64_t sigmoid_q16(int64_t x);                     // 1 / (1 + e^-x) in Q16
int64_t pow2_q32(int64_t y);                        // 2^y in Q32, for y <= 0 in Q32
void sincos_q16(int64_t angle_q32, int64_t& s, int64_t& c); // of an angle >= 0 in Q32
int64_t scale_q32(uint32_t cols, bool embedding);   // a tensor's scale (aispace-v1) in Q32
} // namespace fx

// How a run trains: its training.ini.
struct Recipe
{
    AiShape shape;
    std::string key = "sieve";
    AddressMode start_mode = AddressMode::Scrambled; // where it starts on the line
    std::string start = "0";                         // that address, in hex
    std::string learning_rate = "0.0005";             // decimals as written; used as round(x * 2^16)
    std::string momentum = "0.9";
    uint32_t batch = 4;                              // windows a step
    uint32_t checkpoint_every = 100;                 // steps between the checkpoints the result lists
    int64_t learning_rate_q16() const;
    int64_t momentum_q16() const;
    void check() const;                              // throws std::invalid_argument
    std::string text() const;                        // training.ini, canonical
    static Recipe parse(std::string_view text);
};

// One window of the order: `length` bytes of a corpus file from `offset`, read as `length - 1`
// predictions of the next byte.
struct Window
{
    std::string path; // the corpus file, relative to the run's folder
    uint64_t offset = 0;
    uint32_t length = 0;
};

// order.txt: "sieve-order-v1", then a window a line, path TAB offset TAB length.
std::string order_text(const std::vector<Window>& order);
std::vector<Window> parse_order(std::string_view text);
// How `sieve ai-train --record` makes an order (the order itself is what is recorded, not this): each
// file cut into windows of `length` bytes that overlap by one (so every byte after a file's first is
// predicted once an epoch; a last window shorter than two bytes is dropped), files in the order
// given; then, for each epoch, every window once, shuffled by Fisher-Yates driven by splitmix64
// seeded with seed + epoch.
std::vector<Window> make_order(const std::vector<std::pair<std::string, uint64_t>>& files, uint32_t length, uint32_t epochs,
                               uint64_t seed);

// What a step saw: the sum of the probabilities (Q16) it gave the right next byte, and how many.
struct Score
{
    uint64_t prob_sum = 0, predictions = 0;
    double bits = 0; // display only, never recorded: the sum of -log2 of those probabilities
    double mean_probability() const { return predictions ? double(prob_sum) / 65536.0 / double(predictions) : 0.0; }
    double bits_per_byte() const;
};

class Trainer
{
public:
    Trainer(const AiSpace& space, const AiSpace::Digits& start, const Recipe& recipe, unsigned threads = 0);

    // One step: every window of the batch forward and back, the gradients summed, the weights moved.
    Score step(const std::vector<std::span<const uint8_t>>& batch);
    // The model's score on windows, without training (no weights move).
    Score evaluate(const std::vector<std::span<const uint8_t>>& windows) const;

    const AiSpace::Digits& digits() const { return digits_; }
    const AiSpace& space() const { return space_; }

    struct Net; // the tensors' places and values, private to training.cpp

private:
    const AiSpace& space_;
    AiSpace::Digits digits_;
    std::vector<int64_t> master_, velocity_; // Q32, one a weight
    std::vector<int64_t> scale_;             // Q32, one a tensor
    std::vector<int64_t> values_;            // Q16, each weight's digit's value
    int64_t lr_ = 0, momentum_ = 0;
    unsigned threads_ = 1;
    void refresh_values();
};

// result.txt: what a run gave, so that a replay can be checked against it line for line:
//     sieve-training-result-v1
//     version training-v1
//     steps <N>
//     checkpoint <step> <prob sum> <predictions> <positional address>   (step 0 is the start)
//     final <positional address>
//     model-sha256 <the SHA-256 of the final model's model.safetensors>
struct Checkpoint
{
    uint64_t step = 0;
    Score score; // over the steps since the checkpoint before
    std::string address;
};
struct Result
{
    uint64_t steps = 0;
    std::vector<Checkpoint> checkpoints;
    std::string final_address, model_sha256;
    std::string text() const;
    static Result parse(std::string_view text);
};

} // namespace sieve::training
