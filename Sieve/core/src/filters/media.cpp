// Filters for pictures: neighbour-agreement (image and video).

#include "sieve/filter.hpp"

namespace sieve {

namespace {

// Pure noise has no spatial or temporal structure: neighbouring pixels agree only as often as
// chance allows (half the time in black and white). Counts pairs of neighbours (left-right and
// up-down within a frame, and the same pixel in consecutive frames) that share a colour.
//   passes iff  1000 * equal >= min_permille * pairs   (a unit with no pairs passes)
class NeighbourAgreement : public Filter
{
public:
    NeighbourAgreement(uint32_t w, uint32_t h, uint32_t frames, int64_t min_permille) : w_(w), h_(h), f_(frames), min_(min_permille)
    {
        provenance_ = "min_permille=" + std::to_string(min_permille);
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        uint64_t equal = 0, pairs = 0;
        const size_t frame = size_t(w_) * h_;
        for (uint32_t f = 0; f < f_; ++f)
            for (uint32_t y = 0; y < h_; ++y)
                for (uint32_t x = 0; x < w_; ++x)
                {
                    const size_t i = f * frame + size_t(y) * w_ + x;
                    if (x + 1 < w_) { ++pairs; equal += u[i] == u[i + 1]; }
                    if (y + 1 < h_) { ++pairs; equal += u[i] == u[i + w_]; }
                    if (f + 1 < f_) { ++pairs; equal += u[i] == u[i + frame]; }
                }
        return pairs == 0 || equal * 1000 >= uint64_t(min_) * pairs;
    }

private:
    uint32_t w_, h_, f_;
    int64_t min_;
};

} // namespace

void add_media_filters(std::vector<FilterSpec>& out)
{
    FilterSpec n;
    n.id = "neighbour-agreement";
    n.title = "neighbour-agreement";
    n.description = "At least min_permille of neighbouring pixels (and frames) share a colour. "
                    "Pure noise sits at chance (500 in black and white).";
    n.params = {{"min_permille", "agreeing neighbours required, per thousand", FilterParam::Kind::Integer, "600", 0, 1000, 10}};
    n.applies = [](const FilterLine& l) { return (l.kind == "image" || l.kind == "video") && l.width && l.height; };
    n.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        return std::make_unique<NeighbourAgreement>(l.width, l.height, l.frames ? l.frames : 1,
                                                    param_int(*find_filter("neighbour-agreement-v1"), v, "min_permille"));
    };
    out.push_back(n);
}

} // namespace sieve
