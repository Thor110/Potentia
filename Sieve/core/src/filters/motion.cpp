// Sieve — the video line's own filters: how its frames follow one another (list Z5). The video line's
// other filters judge each frame as a picture (neighbour-agreement, palette-size, row-runs and the
// others) or the video as one run of symbols; these are the first about time.
//
//   video-moves-v1   not every frame the same: a still video is a picture of the image line shown
//                    F times, already on that line. Survivors: P^(nF) - P^n (n pixels a frame, P
//                    colours, F frames), counted exactly.
//   video-smooth-v1  each frame differs from the one before in at most a share of its pixels, as
//                    frames of a film do and noise does not. Survivors: P^n x (the frames within k of
//                    one frame)^(F - 1), where a frame has sum over j <= k of C(n, j) (P - 1)^j within
//                    k changed pixels: counted exactly.
//   life-causal-v1   each frame is the one before stepped once by a life-like rule (B3/S23, Conway's, at
//                    first; the field beyond dead), on a line of two colours: the first frame free,
//                    the rest following, so 2^n survivors (IDEAS 19.141, ROADMAP stage 12: "temporal or
//                    causal variants introduced into noise imagery").
//   video-pan-v1     each frame is the one before moved a pixel one way (left, right, up or down), the
//                    row or column coming in free: a camera panning across a scene. P^n x P^(e (F - 1)),
//                    e the pixels coming in a frame.
//   image-mirror-v1  (the image line) the picture is its own mirror image across, down, both, or turned
//                    half round: P^(the pixels of the half that decides the rest) (IDEAS 1512's figure
//                    symmetry, the faces, leaves, butterflies and letters of a picture).
// Each judges a unit against another part of itself, which no automaton over the symbols can hold, so
// none ranks: the line marks or hides what they set aside, with the survivors counted.

#include "sieve/filter.hpp"

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace sieve {

namespace {

bool video_line(const FilterLine& l)
{
    return l.kind == "video" && l.width && l.height && l.frames >= 2 && l.length == l.width * l.height * l.frames && l.base >= 2;
}

class Moves : public Filter
{
public:
    explicit Moves(const FilterLine& l) : n_(size_t(l.width) * l.height)
    {
        survivors_ = BigUint::pow(l.base, l.length);
        survivors_ -= BigUint::pow(l.base, n_);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t i = n_; i < unit.size(); ++i)
            if (unit[i] != unit[i - n_]) return true;
        return false;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    size_t n_;
    BigUint survivors_;
};

class Smooth : public Filter
{
public:
    Smooth(const FilterLine& l, int64_t percent) : n_(size_t(l.width) * l.height), most_(size_t(uint64_t(n_) * uint64_t(percent) / 100))
    {
        // The frames within `most` changes of one frame: C(n, j) (P - 1)^j for j = 0 .. most.
        BigUint within, term(1); // term = C(n, j) (P - 1)^j
        for (size_t j = 0; j <= most_; ++j)
        {
            within += term;
            if (j == n_) break;
            term.mul_small(uint32_t(n_ - j));
            term.mul_small(l.base - 1);
            (void)term.divmod_small(uint32_t(j + 1));
        }
        survivors_ = BigUint::pow(l.base, n_);
        survivors_ = BigUint::mul(survivors_, BigUint::pow(within, l.frames - 1));
        provenance_ = "percent=" + std::to_string(percent) + " pixels=" + std::to_string(most_);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t f = n_; f < unit.size(); f += n_)
        {
            size_t changed = 0;
            for (size_t i = 0; i < n_; ++i)
                if (unit[f + i] != unit[f + i - n_] && ++changed > most_) return false;
        }
        return true;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    size_t n_, most_;
    BigUint survivors_;
};

// ---------------------------------------------------------------- life-causal

struct LifeRule
{
    bool born[9] = {}, stays[9] = {};
};

LifeRule life_rule_of(const std::string& text)
{
    // "B3/S23": the counts of live neighbours that bear a cell, and that keep one.
    LifeRule r;
    const size_t slash = text.find('/');
    if (text.empty() || (text[0] != 'B' && text[0] != 'b') || slash == std::string::npos || slash + 1 >= text.size() ||
        (text[slash + 1] != 'S' && text[slash + 1] != 's'))
        throw std::invalid_argument("a life rule is written B<digits>/S<digits>, as B3/S23");
    for (size_t i = 1; i < slash; ++i)
    {
        if (text[i] < '0' || text[i] > '8') throw std::invalid_argument("a life rule's counts are 0 to 8");
        r.born[text[i] - '0'] = true;
    }
    for (size_t i = slash + 2; i < text.size(); ++i)
    {
        if (text[i] < '0' || text[i] > '8') throw std::invalid_argument("a life rule's counts are 0 to 8");
        r.stays[text[i] - '0'] = true;
    }
    return r;
}

class LifeCausal : public Filter
{
public:
    LifeCausal(const FilterLine& l, const std::string& rule) : w_(l.width), h_(l.height), rule_(life_rule_of(rule))
    {
        survivors_ = BigUint::pow(2, uint64_t(w_) * h_);
        provenance_ = "rule=" + rule;
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const size_t n = size_t(w_) * h_;
        for (size_t f = n; f < unit.size(); f += n)
            for (uint32_t y = 0; y < h_; ++y)
                for (uint32_t x = 0; x < w_; ++x)
                {
                    uint32_t live = 0;
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            if (!dx && !dy) continue;
                            const int64_t xx = int64_t(x) + dx, yy = int64_t(y) + dy;
                            if (xx < 0 || yy < 0 || xx >= w_ || yy >= h_) continue; // (the field beyond dead)
                            live += unit[f - n + size_t(yy) * w_ + size_t(xx)] != 0;
                        }
                    const bool was = unit[f - n + size_t(y) * w_ + x] != 0;
                    const bool now = was ? rule_.stays[live] : rule_.born[live];
                    if ((unit[f + size_t(y) * w_ + x] != 0) != now) return false;
                }
        return true;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    uint32_t w_, h_;
    LifeRule rule_;
    BigUint survivors_;
};

// ---------------------------------------------------------------- video-pan

class Pan : public Filter
{
public:
    Pan(const FilterLine& l, const std::string& way) : w_(l.width), h_(l.height), way_(way)
    {
        const uint64_t coming = (way == "left" || way == "right") ? h_ : w_;
        survivors_ = BigUint::pow(l.base, uint64_t(w_) * h_ + coming * (l.frames - 1));
        provenance_ = "direction=" + way;
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const size_t n = size_t(w_) * h_;
        for (size_t f = n; f < unit.size(); f += n)
            for (uint32_t y = 0; y < h_; ++y)
                for (uint32_t x = 0; x < w_; ++x)
                {
                    // (the scene moves the way named: the pixel here was its neighbour the other way, or comes in)
                    int64_t sx = x, sy = y;
                    if (way_ == "left") ++sx;
                    else if (way_ == "right") --sx;
                    else if (way_ == "up") ++sy;
                    else --sy;
                    if (sx < 0 || sy < 0 || sx >= w_ || sy >= h_) continue;
                    if (unit[f + size_t(y) * w_ + x] != unit[f - n + size_t(sy) * w_ + size_t(sx)]) return false;
                }
        return true;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    uint32_t w_, h_;
    std::string way_;
    BigUint survivors_;
};

// ---------------------------------------------------------------- image-mirror

bool image_line(const FilterLine& l) { return l.kind == "image" && l.width && l.height && l.length == l.width * l.height && l.base >= 2; }

class Mirror : public Filter
{
public:
    Mirror(const FilterLine& l, const std::string& axis) : w_(l.width), h_(l.height), axis_(axis)
    {
        const uint64_t hw = (w_ + 1) / 2, hh = (h_ + 1) / 2, n = uint64_t(w_) * h_;
        const uint64_t free = axis == "across" ? hw * h_ : axis == "down" ? w_ * hh : axis == "both" ? hw * hh : (n + 1) / 2;
        survivors_ = BigUint::pow(l.base, free);
        provenance_ = "axis=" + axis;
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (uint32_t y = 0; y < h_; ++y)
            for (uint32_t x = 0; x < w_; ++x)
            {
                const uint32_t here = unit[size_t(y) * w_ + x];
                const uint32_t mx = w_ - 1 - x, my = h_ - 1 - y;
                if ((axis_ == "across" || axis_ == "both") && here != unit[size_t(y) * w_ + mx]) return false;
                if ((axis_ == "down" || axis_ == "both") && here != unit[size_t(my) * w_ + x]) return false;
                if (axis_ == "turn" && here != unit[size_t(my) * w_ + mx]) return false;
            }
        return true;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    uint32_t w_, h_;
    std::string axis_;
    BigUint survivors_;
};

} // namespace

void add_motion_filters(std::vector<FilterSpec>& out)
{
    FilterSpec m;
    m.id = "video-moves";
    m.title = "video-moves";
    m.description = "Video: not every frame the same. A still video is a picture of the image line shown frame after frame, "
                    "already on that line; this sets it aside. Counted exactly (every video less the still ones); judged, as it "
                    "compares each frame with the one before.";
    m.applies = video_line;
    m.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> { return std::make_unique<Moves>(l); };
    out.push_back(m);

    FilterSpec s;
    s.id = "video-smooth";
    s.title = "video-smooth";
    s.description = "Video: each frame differs from the one before in at most a share of its pixels, as the frames of a film do and "
                    "noise does not (noise changes nearly every pixel, all but one in P of them). Counted exactly; judged, as it "
                    "compares each frame with the one before.";
    s.params = {{"percent", "the most pixels a frame may change from the one before, as a percentage of the frame", FilterParam::Kind::Integer,
                 "25", 0, 100, 5, {}}};
    s.applies = video_line;
    s.make = [own = s.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Smooth>(l, param_int(*find_filter(own), v, "percent"));
    };
    out.push_back(s);

    FilterSpec life;
    life.id = "life-causal";
    life.title = "life-causal";
    life.description = "Video, two colours: each frame is the one before stepped once by a life-like rule (B3/S23, Conway's Game of "
                       "Life, at first; the field beyond the frame dead), as a film of an automaton is: the first frame free, the "
                       "rest following from it, so 2^n survivors of n pixels. Counted exactly; judged, as it compares each frame "
                       "with the one before.";
    life.params = {{"rule", "the life-like rule, B<digits>/S<digits>: the live neighbours that bear a cell, and that keep one",
                    FilterParam::Kind::Text, "B3/S23", 0, 0, 1, {}}};
    life.applies = [](const FilterLine& l) { return video_line(l) && l.base == 2; };
    life.make = [own = life.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<LifeCausal>(l, param_value(*find_filter(own), v, "rule"));
    };
    out.push_back(life);

    FilterSpec pan;
    pan.id = "video-pan";
    pan.title = "video-pan";
    pan.description = "Video: each frame is the one before moved a pixel the way set (the scene moves left, right, up or down), the "
                      "row or column coming in free, as a camera panning across a scene. Counted exactly; judged.";
    pan.params = {{"direction", "the way the scene moves each frame", FilterParam::Kind::Text, "left", 0, 0, 1, {"left", "right", "up", "down"}}};
    pan.applies = video_line;
    pan.make = [own = pan.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Pan>(l, param_value(*find_filter(own), v, "direction"));
    };
    out.push_back(pan);

    FilterSpec mirror;
    mirror.id = "image-mirror";
    mirror.title = "image-mirror";
    mirror.description = "Pictures: the picture is its own mirror image across (left and right alike, as a face or a butterfly), down "
                         "(top and bottom alike), both, or turned half round (as a playing card): the half that decides the rest "
                         "free. Counted exactly; judged, as each pixel is compared with its mirror.";
    mirror.params = {{"axis", "across, down, both or turn", FilterParam::Kind::Text, "across", 0, 0, 1, {"across", "down", "both", "turn"}}};
    mirror.applies = image_line;
    mirror.make = [own = mirror.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Mirror>(l, param_value(*find_filter(own), v, "axis"));
    };
    out.push_back(mirror);
}

} // namespace sieve
