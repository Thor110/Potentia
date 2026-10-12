// Sieve — diffusion-v1: a model of the AI line that makes pictures of the image line, by undoing
// noise a level at a time (IDEAS §19.2). It is trained in training-v1's pinned integer arithmetic,
// so the same pictures in the same order give the same model on every machine, and every
// checkpoint is an address on the AI line, as a language model's is.
//
// The pictures are W by H cells of B colours (an image line's units), and the noise has S levels.
//   - Tokens: a cell's colour c is the byte c (0 .. B - 1); level t (1 .. S) is the byte B + t - 1.
//     So B + S <= 256, and W H + 1 <= 512 (the positions a model reads).
//   - Noise: each cell i has a mark u_i in [0, S) and a noise colour n_i in [0, B). At level t a
//     cell shows n_i if u_i < t, and its own colour otherwise. So at level S every cell is noise,
//     and each level clears about 1/S of the cells more.
//   - An example: the level's byte, then the cells as they show at that level; the targets are
//     the clean cells (none for the level's byte). Attention is not causal: the model sees the
//     whole picture at once, as a denoiser must.
//   - Training: one splitmix64 generator for the run, seeded with the recipe's seed. Each example
//     draws, in order: its picture (next() mod N, of the N training pictures), its level
//     (1 + next() mod S), every cell's mark (next() mod S), then every cell's noise colour
//     (next() mod B). A step's examples are drawn one after another.
//   - Making a picture: start from a noise picture (any picture of the line: an address), draw
//     every cell's mark from splitmix64 seeded with the sampler's seed (next() mod S), and for
//     t = S down to 1 read the model at level t; each cell whose mark is at least t - 1 takes the
//     colour the model gives most chance (the lowest on a tie), and the rest stay noise. At t = 1
//     every cell has been made. So a picture is a function of the model's address, the noise's
//     address and the seed.
#pragma once

#include "sieve/aispace.hpp"
#include "sieve/training.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sieve::diffusion {

inline constexpr const char* kDiffusionVersion = "diffusion-v1";

struct Setup
{
    uint32_t width = 10, height = 10, colours = 2, levels = 16;
    uint32_t cells() const { return width * height; }
    void check() const; // throws std::invalid_argument
};

// splitmix64, as order-v1 and the samplers use it.
class SplitMix
{
public:
    explicit SplitMix(uint64_t seed) : state_(seed) {}
    uint64_t next();
    uint64_t state() const { return state_; }

private:
    uint64_t state_;
};

// One example's draws, as training makes them.
struct Draw
{
    uint32_t picture = 0, level = 1;
    std::vector<uint32_t> marks, noise;
};
Draw draw(const Setup& s, SplitMix& g, uint32_t pictures);

// The example's input bytes and targets (see above).
void make_example(const Setup& s, std::span<const uint32_t> clean, uint32_t level, std::span<const uint32_t> marks,
                  std::span<const uint32_t> noise, std::vector<uint8_t>& input, std::vector<int16_t>& target);

// One training step: `batch` examples drawn from g, forward and back, the weights moved.
training::Score step(training::Trainer& trainer, const Setup& s, const std::vector<std::vector<uint32_t>>& pictures,
                     SplitMix& g, uint32_t batch);

// A picture made from `noise` (W H colours) with marks drawn from `seed`. With `levels_out`, the
// picture after each level is appended (level S first), for showing it being made.
std::vector<uint32_t> make_picture(const AiSpace& space, const AiSpace::Digits& digits, const Setup& s,
                                   std::span<const uint32_t> noise, uint64_t seed,
                                   std::vector<std::vector<uint32_t>>* levels_out = nullptr);

} // namespace sieve::diffusion
