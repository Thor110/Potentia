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
// Each judges a unit against another part of itself, which no automaton over the symbols can hold.
// Until list N they were counted but never ranked, so the line could only mark or hide what they set
// aside. Since list N (18 November 2026) all but video-moves rank too, each with a ranker of its own:
//   - life-causal, video-pan and image-mirror leave each place either free (any colour) or forced (the
//     one colour the places before it decide: the cell the rule steps to, the pixel the scene brought
//     along, the mirror's). Two survivors first differ at a free place (a forced one follows from places
//     they share), so in address order the survivors go as their free places read as one number in base
//     P: the k-th is k written in the free places, the forced ones filled in after (ForcedRanker).
//   - video-smooth-v2 (v1 with the still videos set aside: a film that moves, smoothly) counts the ways
//     to finish a unit from how far along it is, how many pixels the frame has changed so far and
//     whether anything has moved yet, whatever the pixels are: at each place one colour is the one
//     before it (no change) and the other P - 1 are changes. So it ranks by those counts (ChangesRanker, changes_ranker.hpp).
// Each also walks symbol by symbol for the guided orderings, its state the place and the frame before
// (kept as it is first met: History).

#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"

#include "changes_ranker.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace sieve {

namespace {

// ---------------------------------------------------------------- rankers (list N)

// Places free or forced (see the top of the file). forced(pos, at) is the colour place pos must have,
// at(j) the symbol at an earlier place j, no further back than `lag`; free[pos] says which places are free.
class ForcedRanker : public Ranker
{
public:
    using At = std::function<uint32_t(size_t)>;
    using Rule = std::function<uint32_t(size_t, const At&)>;
    ForcedRanker(uint32_t base, std::vector<char> free, size_t lag, Rule forced)
        : base_(base), free_(std::move(free)), forced_(std::move(forced)), states_(lag)
    {
        suffix_.assign(free_.size() + 1, 0);
        for (size_t i = free_.size(); i-- > 0;) suffix_[i] = suffix_[i + 1] + (free_[i] ? 1 : 0);
        set_count();
    }
    uint32_t length() const override { return uint32_t(free_.size()); }
    uint32_t base() const override { return base_; }
    State start() const override { return states_.id_of(0, 0, {}); }
    State next(State s, uint32_t c) const override
    {
        if (s == kDead || c >= base_) return kDead;
        History::Rec r = states_.get(s);
        if (r.pos >= free_.size()) return kDead;
        if (!free_[r.pos])
        {
            const size_t pos = r.pos, first = pos - r.tail.size();
            if (c != forced_(pos, [&](size_t j) { return r.tail.at(j - first); })) return kDead;
        }
        r.tail.push_back(c);
        return states_.id_of(r.pos + 1, 0, std::move(r.tail));
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        const History::Rec r = states_.get(s);
        if (uint64_t(r.pos) + remaining != free_.size()) return BigUint();
        return BigUint::pow(base_, suffix_[r.pos]);
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (k >= count()) throw std::out_of_range("rank beyond the survivors");
        const auto digits = k.to_digits(base_, suffix_[0]);
        std::vector<uint32_t> u(free_.size());
        const At at = [&](size_t j) { return u[j]; };
        size_t d = 0;
        for (size_t i = 0; i < u.size(); ++i) u[i] = free_[i] ? digits[d++] : forced_(i, at);
        return u;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != free_.size()) throw std::invalid_argument("unit has the wrong length");
        const At at = [&](size_t j) { return unit[j]; };
        BigUint k;
        for (size_t i = 0; i < unit.size(); ++i)
        {
            if (unit[i] >= base_) throw std::invalid_argument("unit is not a survivor");
            if (free_[i])
            {
                k.mul_small(base_);
                k.add_small(unit[i]);
            }
            else if (unit[i] != forced_(i, at)) throw std::invalid_argument("unit is not a survivor");
        }
        return k;
    }

private:
    uint32_t base_;
    std::vector<char> free_;
    Rule forced_;
    History states_;
    std::vector<uint64_t> suffix_; // free places from each place on
};

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
        // (list N: ranked as every frame within any number of changes of the one before, one change at least)
        if (ChangesRanker::table_bytes(l.base, n_, n_) <= filter_memory())
        {
            std::vector<char> free(l.frames, 0);
            free[0] = 1;
            ranker_ = std::make_unique<ChangesRanker>(l.base, n_, std::move(free), n_, true);
        }
        else set_table_bytes(ChangesRanker::table_bytes(l.base, n_, n_));
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t i = n_; i < unit.size(); ++i)
            if (unit[i] != unit[i - n_]) return true;
        return false;
    }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &survivors_; }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    size_t n_;
    BigUint survivors_;
    std::unique_ptr<Ranker> ranker_;
};

class Smooth : public Filter
{
public:
    // moving: v2, the still videos set aside too, and ranked
    Smooth(const FilterLine& l, int64_t percent, bool moving = false)
        : n_(size_t(l.width) * l.height), most_(size_t(uint64_t(n_) * uint64_t(percent) / 100)), moving_(moving)
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
        if (moving_)
        {
            survivors_ -= BigUint::pow(l.base, n_);
            // (ranked where its table fits the filter memory; past that, counted and judged as v1 is)
            if (ChangesRanker::table_bytes(l.base, n_, most_) <= filter_memory())
            {
                std::vector<char> free(l.frames, 0);
                free[0] = 1; // (the frames are the parts, the first free)
                ranker_ = std::make_unique<ChangesRanker>(l.base, n_, std::move(free), most_, true);
            }
            else set_table_bytes(ChangesRanker::table_bytes(l.base, n_, most_));
        }
        provenance_ = "percent=" + std::to_string(percent) + " pixels=" + std::to_string(most_);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        bool moved = false;
        for (size_t f = n_; f < unit.size(); f += n_)
        {
            size_t changed = 0;
            for (size_t i = 0; i < n_; ++i)
                if (unit[f + i] != unit[f + i - n_] && ++changed > most_) return false;
            moved = moved || changed > 0;
        }
        return moved || !moving_;
    }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &survivors_; }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    size_t n_, most_;
    bool moving_;
    BigUint survivors_;
    std::unique_ptr<Ranker> ranker_;
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
        const size_t n = size_t(w_) * h_;
        std::vector<char> free(l.length, 0);
        for (size_t i = 0; i < n; ++i) free[i] = 1; // (the first frame)
        const uint32_t w = w_, h = h_;
        const LifeRule r = rule_;
        ranker_ = std::make_unique<ForcedRanker>(2, std::move(free), n + w + 1, [w, h, r, n](size_t pos, const ForcedRanker::At& at) -> uint32_t {
            const size_t o = pos % n;
            const uint32_t y = uint32_t(o / w), x = uint32_t(o % w);
            uint32_t live = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if (!dx && !dy) continue;
                    const int64_t xx = int64_t(x) + dx, yy = int64_t(y) + dy;
                    if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                    live += at(pos - n + size_t(yy) * w + size_t(xx) - o) != 0;
                }
            const bool was = at(pos - n) != 0;
            return (was ? r.stays[live] : r.born[live]) ? 1 : 0;
        });
    }
    const Ranker* ranker() const override { return ranker_.get(); }
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
    std::unique_ptr<Ranker> ranker_;
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
        // (the place each pixel comes from, a frame back, or none where it comes in free)
        const size_t n = size_t(w_) * h_;
        std::vector<char> free(l.length, 1);
        std::vector<int64_t> from(l.length, -1);
        for (size_t pos = n; pos < l.length; ++pos)
        {
            const size_t o = pos % n;
            int64_t sx = int64_t(o % w_), sy = int64_t(o / w_);
            if (way == "left") ++sx;
            else if (way == "right") --sx;
            else if (way == "up") ++sy;
            else --sy;
            if (sx < 0 || sy < 0 || sx >= w_ || sy >= h_) continue;
            free[pos] = 0;
            from[pos] = int64_t(pos - o - n) + sy * w_ + sx;
        }
        ranker_ = std::make_unique<ForcedRanker>(l.base, std::move(free), n + w_, [from](size_t pos, const ForcedRanker::At& at) {
            return at(size_t(from[pos]));
        });
    }
    const Ranker* ranker() const override { return ranker_.get(); }
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
    std::unique_ptr<Ranker> ranker_;
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
        // (each pixel's mirror that comes first, in reading order: itself where it is free)
        std::vector<char> is_free(n, 0);
        std::vector<size_t> from(n, 0);
        for (uint32_t y = 0; y < h_; ++y)
            for (uint32_t x = 0; x < w_; ++x)
            {
                const uint32_t mx = w_ - 1 - x, my = h_ - 1 - y;
                size_t src = size_t(y) * w_ + x;
                if (axis == "across") src = size_t(y) * w_ + std::min(x, mx);
                else if (axis == "down") src = size_t(std::min(y, my)) * w_ + x;
                else if (axis == "both") src = size_t(std::min(y, my)) * w_ + std::min(x, mx);
                else src = std::min(src, size_t(my) * w_ + mx);
                const size_t here = size_t(y) * w_ + x;
                is_free[here] = src == here;
                from[here] = src;
            }
        ranker_ = std::make_unique<ForcedRanker>(l.base, std::move(is_free), size_t(n), [from](size_t pos, const ForcedRanker::At& at) {
            return at(from[pos]);
        });
    }
    const Ranker* ranker() const override { return ranker_.get(); }
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
    std::unique_ptr<Ranker> ranker_;
};

} // namespace

void add_motion_filters(std::vector<FilterSpec>& out)
{
    FilterSpec m;
    m.id = "video-moves";
    m.title = "video-moves";
    m.description = "Video: not every frame the same. A still video is a picture of the image line shown frame after frame, "
                    "already on that line; this sets it aside. Counted and ranked exactly (every video less the still ones; "
                    "since list N, ranked by whether a frame has changed yet).";
    m.applies = video_line;
    m.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> { return std::make_unique<Moves>(l); };
    m.counts_as = "own"; // (list N: ranked by its changes, changes_ranker.hpp)
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
    s.judge_only = true; // (it compares a unit with another part of itself: counted, never ranked)
    s.retired = true;    // (list N: v2 sets the still videos aside too, and ranks)
    s.replaced_by = "video-smooth-v2";
    out.push_back(s);

    FilterSpec s2 = s;
    s2.version = 2;
    s2.retired = false;
    s2.replaced_by.clear();
    s2.judge_only = false;
    s2.counts_as = "own";
    s2.implies = {"video-moves-v1"};
    s2.description = "Video: a film that moves, smoothly: each frame differs from the one before in at most a share of its pixels, as "
                     "the frames of a film do and noise does not (noise changes nearly every pixel, all but one in P of them), and "
                     "some frame differs (a still video is a picture of the image line). Counted and ranked exactly, so the line "
                     "compacts: at each pixel one colour is the one before it and the rest are changes, whatever the pixels are.";
    s2.make = [own = s2.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Smooth>(l, param_int(*find_filter(own), v, "percent"), true);
    };
    out.push_back(s2);

    FilterSpec life;
    life.id = "life-causal";
    life.title = "life-causal";
    life.description = "Video, two colours: each frame is the one before stepped once by a life-like rule (B3/S23, Conway's Game of "
                       "Life, at first; the field beyond the frame dead), as a film of an automaton is: the first frame free, the "
                       "rest following from it, so 2^n survivors of n pixels. Counted and ranked exactly: the first frame is "
                       "free and the rest follow from it.";
    life.params = {{"rule", "the life-like rule, B<digits>/S<digits>: the live neighbours that bear a cell, and that keep one",
                    FilterParam::Kind::Text, "B3/S23", 0, 0, 1, {}}};
    life.applies = [](const FilterLine& l) { return video_line(l) && l.base == 2; };
    life.make = [own = life.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<LifeCausal>(l, param_value(*find_filter(own), v, "rule"));
    };
    life.counts_as = "own";  // (list N: each place free or forced by the places before it, so it ranks)
    life.picks_a_kind = true; // (a film of an automaton: a kind of item, not noise set aside)
    out.push_back(life);

    FilterSpec pan;
    pan.id = "video-pan";
    pan.title = "video-pan";
    pan.description = "Video: each frame is the one before moved a pixel the way set (the scene moves left, right, up or down), the "
                      "row or column coming in free, as a camera panning across a scene. Counted and ranked exactly: the first frame "
                      "and the pixels coming in are free, and the rest follow.";
    pan.params = {{"direction", "the way the scene moves each frame", FilterParam::Kind::Text, "left", 0, 0, 1, {"left", "right", "up", "down"}}};
    pan.applies = video_line;
    pan.make = [own = pan.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Pan>(l, param_value(*find_filter(own), v, "direction"));
    };
    pan.counts_as = "own";  // (list N: each place free or forced by the places before it, so it ranks)
    pan.picks_a_kind = true; // (a camera panning: a kind of item, not noise set aside)
    out.push_back(pan);

    FilterSpec mirror;
    mirror.id = "image-mirror";
    mirror.title = "image-mirror";
    mirror.description = "Pictures: the picture is its own mirror image across (left and right alike, as a face or a butterfly), down "
                         "(top and bottom alike), both, or turned half round (as a playing card): the half that decides the rest "
                         "free. Counted and ranked exactly: the half that decides is free, and its mirror follows.";
    mirror.params = {{"axis", "across, down, both or turn", FilterParam::Kind::Text, "across", 0, 0, 1, {"across", "down", "both", "turn"}}};
    mirror.applies = image_line;
    mirror.make = [own = mirror.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Mirror>(l, param_value(*find_filter(own), v, "axis"));
    };
    mirror.counts_as = "own";  // (list N: each place free or forced by the places before it, so it ranks)
    mirror.picks_a_kind = true; // (a picture with a mirror: a kind of item, not noise set aside)
    out.push_back(mirror);
}

} // namespace sieve
