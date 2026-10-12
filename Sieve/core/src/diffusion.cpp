// Sieve — diffusion-v1 (sieve/diffusion.hpp): pictures made by undoing noise, trained in
// training-v1's integer arithmetic.

#include "sieve/diffusion.hpp"

#include <stdexcept>

namespace sieve::diffusion {

namespace {
[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument("diffusion-v1: " + why); }
} // namespace

void Setup::check() const
{
    if (width < 1 || height < 1) bad("a picture of at least one cell");
    if (uint64_t(width) * height + 1 > 512) bad("at most 511 cells (the level's byte and the cells fill 512 positions)");
    if (colours < 2) bad("at least two colours");
    if (levels < 1) bad("at least one level of noise");
    if (colours + levels > 256) bad("colours and levels together at most 256 (each is a byte)");
}

uint64_t SplitMix::next()
{
    uint64_t z = (state_ += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

Draw draw(const Setup& s, SplitMix& g, uint32_t pictures)
{
    if (pictures == 0) bad("no pictures to train on");
    Draw d;
    d.picture = uint32_t(g.next() % pictures);
    d.level = 1 + uint32_t(g.next() % s.levels);
    d.marks.resize(s.cells());
    d.noise.resize(s.cells());
    for (uint32_t& m : d.marks) m = uint32_t(g.next() % s.levels);
    for (uint32_t& n : d.noise) n = uint32_t(g.next() % s.colours);
    return d;
}

void make_example(const Setup& s, std::span<const uint32_t> clean, uint32_t level, std::span<const uint32_t> marks,
                  std::span<const uint32_t> noise, std::vector<uint8_t>& input, std::vector<int16_t>& target)
{
    const uint32_t n = s.cells();
    if (clean.size() != n || marks.size() != n || noise.size() != n) bad("a picture of " + std::to_string(n) + " cells");
    if (level < 1 || level > s.levels) bad("a level from 1 to " + std::to_string(s.levels));
    input.assign(1, uint8_t(s.colours + level - 1));
    target.assign(1, -1);
    for (uint32_t i = 0; i < n; ++i)
    {
        if (clean[i] >= s.colours || noise[i] >= s.colours) bad("a colour beyond the palette");
        input.push_back(uint8_t(marks[i] < level ? noise[i] : clean[i]));
        target.push_back(int16_t(clean[i]));
    }
}

training::Score step(training::Trainer& trainer, const Setup& s, const std::vector<std::vector<uint32_t>>& pictures,
                     SplitMix& g, uint32_t batch)
{
    s.check();
    std::vector<std::vector<uint8_t>> inputs(batch);
    std::vector<std::vector<int16_t>> targets(batch);
    std::vector<training::Example> ex(batch);
    for (uint32_t j = 0; j < batch; ++j)
    {
        const Draw d = draw(s, g, uint32_t(pictures.size()));
        make_example(s, pictures[d.picture], d.level, d.marks, d.noise, inputs[j], targets[j]);
        ex[j] = training::Example{inputs[j], targets[j], false};
    }
    return trainer.step(ex);
}

std::vector<uint32_t> make_picture(const AiSpace& space, const AiSpace::Digits& digits, const Setup& s,
                                   std::span<const uint32_t> noise, uint64_t seed, std::vector<std::vector<uint32_t>>* levels_out)
{
    s.check();
    const uint32_t n = s.cells();
    if (noise.size() != n) bad("a noise picture of " + std::to_string(n) + " cells");
    SplitMix g(seed);
    std::vector<uint32_t> marks(n);
    for (uint32_t& m : marks) m = uint32_t(g.next() % s.levels);
    std::vector<uint32_t> x(noise.begin(), noise.end());
    std::vector<uint8_t> input;
    std::vector<int16_t> target(n + 1, -1);
    for (uint32_t t = s.levels; t >= 1; --t)
    {
        input.assign(1, uint8_t(s.colours + t - 1));
        for (uint32_t i = 0; i < n; ++i)
        {
            if (x[i] >= s.colours) bad("a colour beyond the palette");
            input.push_back(uint8_t(x[i]));
        }
        const std::vector<int32_t> logits = training::model_logits(space, digits, training::Example{input, target, false});
        for (uint32_t i = 0; i < n; ++i)
        {
            if (marks[i] + 1 < t) continue; // still noise at level t - 1
            const int32_t* z = &logits[size_t(i + 1) * 256];
            uint32_t best = 0;
            for (uint32_t c = 1; c < s.colours; ++c)
                if (z[c] > z[best]) best = c;
            x[i] = best;
        }
        if (levels_out) levels_out->push_back(x);
    }
    return x;
}

} // namespace sieve::diffusion
