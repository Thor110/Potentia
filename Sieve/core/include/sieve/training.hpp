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
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::training {

inline constexpr const char* kTrainingVersion = "training-v1";
// training-v2: v1 with the loss scaled by 2^k (the gradients carried k bits larger, so that fewer
// of them fall below Q16 on their way back through deep and wide models) and the learning rate held
// to 16 + k fractional bits; with k = 0 it trains exactly as v1 does.
inline constexpr const char* kTrainingVersion2 = "training-v2";
// training-v3 (9 November 2026; IDEAS §19.119, Edward: "no one can say with absolute certainty that
// something has no chance of following something else ... 'waffle' may be very unlikely after
// 'quantised', but that doesn't stop me typing 'quantised waffle'"): v2 with every byte's probability
// held between a floor and a ceiling, in training as in use. The softmax over the 256 bytes gives
// byte j  1 + floor(e_j (2^16 - 256) / sum e), so each is at least 1/65,536 (never ruled out) and at
// most 65,281/65,536 (never certain), and they add to at most 2^16; the gradient follows these
// probabilities. A model trained so can drive an arithmetic coder, which needs every byte possible.
inline constexpr const char* kTrainingVersion3 = "training-v3";
const char* training_version_name(int version); // "training-v1", -v2 or -v3

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
int64_t mul_shift(int64_t a, int64_t b, unsigned s); // floor(a b / 2^s), the product exact (it may pass 64 bits)
} // namespace fx

// The inner loops' vector paths (AVX2, and AVX-512 where the processor has it; the widest is chosen
// at start-up): the same integer sums whichever, so the switch is only for the tests, which run each.
bool vector_kernels_available();      // AVX2
bool wide_kernels_available();        // AVX-512
bool use_vector_kernels(bool enable); // the widest there is, or plain; returns whether one is in use
bool use_kernels(int level);          // 0 plain, 1 AVX2, 2 AVX-512; false (unchanged) where not there
bool using_vector_kernels();
const char* kernels_name();           // "plain", "vectorised (AVX2)" or "vectorised (AVX-512)"

// How a run reads its corpus, when its order is made by order-v1 rather than written out.
enum class Reading { InOrder, ShuffledWithinFiles, Shuffled };
const char* to_string(Reading r);       // "in-order", "shuffled-within-files", "shuffled"
Reading reading_from_string(std::string_view s);

// How a run trains: its training.ini.
struct Recipe
{
    int version = 3;                   // training-v1, -v2 or -v3 (the default for new runs)
    uint32_t loss_scale_bits = 0;      // v2 and v3: k, the loss scaled by 2^k (0 to 7)
    AiShape shape;
    std::string key = "sieve";
    AddressMode start_mode = AddressMode::Scrambled; // where it starts on the line
    std::string start = "0";                         // that address, in hex
    std::string learning_rate = "0.0005";             // decimals as written; used as round(x * 2^16)
    std::string momentum = "0.9";
    uint32_t batch = 4;                              // windows a step
    uint32_t checkpoint_every = 100;                 // steps between the checkpoints the result lists
    // The order (run format 2): "order-v1", made from the lines below; "order.txt", written out in
    // that file; or empty, the first runs' format (order.txt, and a result listing every
    // checkpoint's address).
    std::string order = "order-v1";
    Reading reading = Reading::Shuffled;
    uint32_t window = 65, epochs = 30;
    uint64_t seed = 1;
    std::vector<std::string> files;                  // order-v1: the corpus files, in the order read
    bool checkpoint_addresses = false;               // the result lists each checkpoint's address, not only its hash
    int result_version() const { return order.empty() ? 1 : 2; }
    int64_t learning_rate_q16() const;
    int64_t learning_rate_q() const; // round(x * 2^(16 + k)): what the update uses (v1: k = 0)
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
// order-v1: each file cut into windows of `length` bytes that overlap by one (so every byte after a
// file's first is predicted once an epoch; a last window shorter than two bytes is dropped), then,
// for each epoch, every window once: in-order, file by file in the order given and each file from
// its start; shuffled-within-files, file by file but each file's windows shuffled; shuffled, all of
// them shuffled together (the files' windows listed in the order given first). Shuffling is
// Fisher-Yates driven by splitmix64 seeded with seed + epoch, one generator an epoch, used file
// after file.
std::vector<Window> make_order(const std::vector<std::pair<std::string, uint64_t>>& files, uint32_t length, uint32_t epochs,
                               uint64_t seed, Reading reading = Reading::Shuffled);

// What a step saw: the sum of the probabilities (Q16) it gave the right next byte, and how many.
struct Score
{
    uint64_t prob_sum = 0, predictions = 0;
    double bits = 0; // display only, never recorded: the sum of -log2 of those probabilities
    double mean_probability() const { return predictions ? double(prob_sum) / 65536.0 / double(predictions) : 0.0; }
    double bits_per_byte() const;
};

// One example: the model reads `input` and, at each position whose target is not -1, is scored on
// (and trained towards) that byte. `causal`: each position attends to itself and those before it,
// as a language model must; otherwise to every position (a denoiser, which sees the whole picture).
// A window of text is the example of its bytes but the last, each targeting the byte after it,
// causal (Trainer::step on windows makes exactly these).
// How often training met its arithmetic's limits (Edward, 4 November 2026: does a value ever reach
// the floor or the ceiling?). Counted as it runs, for measurement only: nothing here changes a
// number. `held` of `holds` were values held at ±2^23 (±128 in Q16); of `updates` (a weight moved
// once a step; the holds of the backward pass counted apart), `grad_zero` had a gradient of exactly 0, `grad_clamped` and `velocity_clamped`
// reached their limits, and `master_clamped` met an end of the weight's range ([-S, S]), where
// `master_at_end` weights sat after the last step. Counted over every Trainer since the last reset,
// while count_limits(true).
struct LimitCounts
{
    uint64_t holds = 0, held = 0;           // the forward pass: activations
    uint64_t holds_back = 0, held_back = 0; // the backward pass: gradients flowing back
    uint64_t updates = 0, grad_zero = 0, grad_clamped = 0, velocity_clamped = 0, master_clamped = 0;
    uint64_t weights = 0, master_at_end = 0, master_at_low = 0; // (at_low: of those at an end, the low one, -S)
    // The probabilities (Q16, so a byte's can round to exactly 0): predictions made, the right byte
    // given 0, a prediction giving one byte all of it, and bytes given 0 (of 256 a prediction).
    uint64_t predictions = 0, right_zero = 0, certain = 0, zero_probs = 0;
};
LimitCounts limit_counts();
void reset_limit_counts();
void count_limits(bool on); // off by default: counting slows training (sieve ai-train --limits)

struct Example
{
    std::span<const uint8_t> input;
    std::span<const int16_t> target;
    bool causal = true;
};

class Trainer
{
public:
    Trainer(const AiSpace& space, const AiSpace::Digits& start, const Recipe& recipe, unsigned threads = 0);
    ~Trainer();
    Trainer(const Trainer&) = delete;
    Trainer& operator=(const Trainer&) = delete;

    // One step: every window of the batch forward and back, the gradients summed, the weights moved.
    Score step(const std::vector<std::span<const uint8_t>>& batch);
    // One step on examples (see Example): the same arithmetic, other inputs and targets.
    Score step(const std::vector<Example>& batch);
    Score evaluate(const std::vector<Example>& batch) const;
    // The model's score on windows, without training (no weights move).
    Score evaluate(const std::vector<std::span<const uint8_t>>& windows) const;

    const AiSpace::Digits& digits() const { return digits_; }

    // The whole state a step leaves (each weight's master value and momentum, Q32): saved, it lets
    // training stop and carry on later exactly as if it had not stopped. Restoring sets the digits
    // and values from the masters, as a step does.
    const std::vector<int64_t>& masters() const { return master_; }
    const std::vector<int64_t>& velocities() const { return velocity_; }
    void restore(const std::vector<int64_t>& masters, const std::vector<int64_t>& velocities);
    const AiSpace& space() const { return space_; }

    struct Net; // the tensors' places and values, private to training.cpp

private:
    const AiSpace& space_;
    AiSpace::Digits digits_;
    std::vector<int64_t> master_, velocity_; // Q32, one a weight
    std::vector<int64_t> scale_;             // Q32, one a tensor
    std::vector<int32_t> values_;            // Q16, each weight's digit's value
    int64_t lr_ = 0, momentum_ = 0;
    unsigned shift_ = 0; // training-v2's loss scale, in bits
    bool floor_ = false; // training-v3: every byte's probability between 1/65,536 and 65,281/65,536
    unsigned threads_ = 1;
    std::unique_ptr<class Pool> pool_;       // the trainer's threads, kept for its life
    void refresh_values();
    void apply(const std::vector<int64_t>& grad); // the update every step makes
};

// A model's score on windows of text, the forward pass alone (as Trainer::evaluate, with no trainer
// and on one thread): exact, the same on every machine.
Score evaluate_model(const AiSpace& space, const AiSpace::Digits& digits, const std::vector<std::span<const uint8_t>>& windows);

// The head's logits for one example (input.size() x 256, Q16), the forward pass alone: what a
// sampler reads to choose.
std::vector<int32_t> model_logits(const AiSpace& space, const AiSpace::Digits& digits, const Example& ex);

// result.txt: what a run gave, so that a replay can be checked against it line for line:
//     sieve-training-result-v2
//     version training-v1
//     steps <N>
//     checkpoint <step> <prob sum> <predictions> <SHA-256 of its digits> [<positional address>]
//     final <positional address>
//     model-sha256 <the SHA-256 of the final model's model.safetensors>
// (step 0 is the start). Version 1, the first runs', lists each checkpoint's address and no hash.
struct Checkpoint
{
    uint64_t step = 0;
    Score score; // over the steps since the checkpoint before
    std::string digest;  // v2
    std::string address; // v1, and v2 when the recipe keeps them
};
struct Result
{
    std::string trainer = kTrainingVersion; // the result's "version" line: the training version of its run
    int version = 2;
    uint64_t steps = 0;
    std::vector<Checkpoint> checkpoints;
    std::string final_address, model_sha256;
    std::string text() const;
    static Result parse(std::string_view text);
};

} // namespace sieve::training
