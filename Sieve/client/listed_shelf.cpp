// Sieve hallway — listed lines' shelves (listed_shelf.hpp): the fractal dimension's.
#include "listed_shelf.hpp"

#include "strings.hpp"

#include "cli/dictionaries.hpp" // find_data_folder
#include "cli/filter_config.hpp" // fractal_filter_line
#include "sieve/fractal.hpp"
#include "sieve/fractal_space.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

namespace {

using sieve::BigUint;

// The deepest a fractal is drawn on a shelf, in the hands or in the viewer: past this many symbols a
// word takes too long to draw a frame (a limit chosen, not measured; `sieve fractal draw` goes as deep
// as a ruleset grows).
constexpr size_t kDrawnMost = size_t(1) << 18;
// How many rulesets' texts and depths are kept once worked out (a tile's shelves, and some to spare).
constexpr size_t kKept = 4096;

uint32_t drawn_deepest(const sieve::Fractal& f) { return sieve::fractal_deepest_within(f, kDrawnMost); }

// The fractal dimension (list X1): with the known fractals included (the default), its first book the
// index of every fractal charted and after it every ruleset of the whole space (fractal-space-v1) up to
// the longest set, the charted ones named where they stand; only, the index and then the charted ones
// alone, in the chart's order (data/fractals/chart.txt), as the shelves were before the space was
// generated; excluded, every ruleset but the charted ones, and no index, so nothing known is on the
// line at all (Edward, 15 November 2026: "the known fractals would be the first possible filter for its
// state space, with exclude/include/only options"; 16 November: "That should also be under the
// exclude/include/only setting").
class FractalShelf : public ListedShelf
{
public:
    explicit FractalShelf(const ListedOptions& o) : known_(o.fractal_known), space_(std::clamp(o.fractal_longest, sieve::kFractalSpaceShortest, sieve::kFractalSpaceLongestMost))
    {
        if (known_ != "exclude" && known_ != "only") known_ = "include";
        index_ = known_ != "exclude";
        try
        {
            chart_ = sieve::read_fractal_chart(sieve::cli::find_data_folder("fractals", "chart.txt"));
        }
        catch (const std::exception& e)
        {
            trouble_ = e.what();
        }
        std::string all;
        for (size_t k = 0; k < chart_.size(); ++k)
        {
            const auto& c = chart_[k];
            all += c.id + "\n" + c.text;
            chart_deepest_.push_back(drawn_deepest(c.fractal));
            if (const auto i = space_.index_of(c.text))
            {
                charted_at_.emplace(*i, k);
                chart_index_.push_back(*i);
            }
            else chart_index_.push_back(std::nullopt);
        }
        // (named by what it holds, so a chart or a shape changed is a shelf of its own and its
        // favourites stay apart; excluded, "exclude-no-index" since its items moved down one place when
        // the index left it, 16 November 2026)
        id_ = "fractals/" + sieve::Sha256::hex_hash(all).substr(0, 16) + "/" + (index_ ? known_ : "exclude-no-index") + "/" + sieve::kFractalSpaceVersion + "-" +
              std::to_string(space_.longest());
        if (known_ == "only") size_ = BigUint(uint64_t(chart_.size()) + 1);
        else if (known_ == "include")
        {
            size_ = space_.count();
            size_.add_small(1);
        }
        else
        {
            size_ = space_.count();
            size_ -= BigUint(uint64_t(charted_at_.size()));
        }
    }
    BigUint size() const override { return size_; }
    std::string id() const override { return id_; }
    std::string title(const BigUint& i) const override
    {
        if (is_index(i)) return tr("fractals.index");
        if (const auto k = charted(i)) return chart_[*k].title;
        const auto f = sieve::read_fractal(ruleset(i));
        if (!f) return "";
        if (!f->rewrites())
        {
            if (f->family == sieve::Fractal::Family::Ifs) return trf("fractals.generated.ifs", {std::to_string(f->maps.size())});
            std::string formula;
            for (const std::string& t : f->formula) formula += (formula.empty() ? "" : " ") + t;
            if (formula.size() > 16) formula = formula.substr(0, 16) + "..";
            return trf(f->family == sieve::Fractal::Family::Escape ? "fractals.generated.escape" : "fractals.generated.converge", {formula});
        }
        const std::string axiom = f->axiom.size() > 12 ? f->axiom.substr(0, 12) + ".." : f->axiom;
        return f->family == sieve::Fractal::Family::Substitution ? trf("fractals.generated.substitution", {axiom})
                                                                 : trf("fractals.generated.lsystem", {f->angle_written(), axiom});
    }
    std::string summary(const BigUint& i) const override
    {
        if (is_index(i)) return trf("fractals.index.summary", {std::to_string(chart_.size()), tr("fractals.known." + known_)});
        const auto f = sieve::read_fractal(ruleset(i));
        if (!f) return "";
        std::string formula;
        for (const std::string& t : f->formula) formula += (formula.empty() ? "" : " ") + t;
        const std::string family = f->family == sieve::Fractal::Family::Ifs ? trf("fractals.ifs", {std::to_string(f->maps.size())})
                                   : f->family == sieve::Fractal::Family::Escape || f->family == sieve::Fractal::Family::Converge
                                       ? trf(f->family == sieve::Fractal::Family::Escape ? "fractals.escape" : "fractals.converge", {formula}) +
                                             (f->julia ? trf("fractals.julia", {f->julia_text[0] + " " + f->julia_text[1]}) : std::string())
                                   : f->family == sieve::Fractal::Family::Substitution ? tr("fractals.substitution")
                                   : !f->scale_written().empty() ? trf("fractals.lsystem.scale", {f->angle_written(), f->scale_written()})
                                                                 : trf("fractals.lsystem.angle", {f->angle_written()});
        // (a charted one says where it was transcribed from)
        if (const auto k = charted(i); k && !chart_[*k].source.empty()) return family + trf("fractals.source", {chart_[*k].source});
        return family;
    }
    std::optional<sieve::RgbaImage> picture(const BigUint& i, uint32_t depth, uint32_t width, uint32_t height) const override
    {
        if (is_index(i))
        {
            // The index: a small drawing of each fractal charted, in rows.
            if (chart_.empty() || width == 0 || height == 0) return std::nullopt;
            sieve::RgbaImage img;
            img.width = width, img.height = height;
            img.rgba.assign(size_t(width) * height * 4, 0);
            for (size_t p = 0; p < size_t(width) * height; ++p) img.rgba[p * 4] = 16, img.rgba[p * 4 + 1] = 20, img.rgba[p * 4 + 2] = 24, img.rgba[p * 4 + 3] = 255;
            const size_t n = chart_.size(), cols = std::max<size_t>(1, size_t(std::ceil(std::sqrt(double(n)))));
            const size_t rows = (n + cols - 1) / cols;
            const uint32_t cw = width / uint32_t(cols), ch = height / uint32_t(rows), s = std::max<uint32_t>(1, std::min(cw, ch));
            for (size_t k = 0; k < n; ++k)
            {
                const auto t = sieve::draw_fractal(chart_[k].fractal, std::min<uint32_t>(chart_deepest_[k], 4), s, s);
                if (!t) continue;
                const uint32_t ox = uint32_t(k % cols) * cw + (cw - s) / 2, oy = uint32_t(k / cols) * ch + (ch - s) / 2;
                for (uint32_t y = 0; y < s; ++y)
                    std::copy_n(&t->rgba[size_t(y) * s * 4], size_t(s) * 4, &img.rgba[(size_t(oy + y) * width + ox) * 4]);
            }
            return img;
        }
        const auto f = sieve::read_fractal(ruleset(i));
        if (!f) return std::nullopt;
        return sieve::draw_fractal(*f, std::min(depth, deepest(i)), width, height);
    }
    std::vector<std::string> text(const BigUint& i) const override
    {
        std::vector<std::string> out;
        if (is_index(i))
        {
            // Each charted fractal, its name, its id and where it stands on this line.
            for (size_t k = 0; k < chart_.size(); ++k)
            {
                std::string where;
                if (known_ == "only") where = trf("fractals.at", {BigUint(uint64_t(k) + 1).to_hex()});
                else if (chart_index_[k])
                {
                    BigUint p = *chart_index_[k];
                    p.add_small(1);
                    where = trf("fractals.at", {p.to_hex()});
                }
                else where = trf("fractals.too_long", {std::to_string(chart_[k].text.size()), std::to_string(space_.longest())});
                out.push_back(std::to_string(k + 1) + ". " + chart_[k].title + " (" + chart_[k].id + "), " + where);
            }
            if (!trouble_.empty()) out.push_back(trf("fractals.unreadable", {trouble_}));
            return out;
        }
        const std::string t = ruleset(i);
        for (size_t at = 0; at < t.size();)
        {
            const size_t lf = t.find('\n', at);
            out.push_back(t.substr(at, lf - at));
            at = lf == std::string::npos ? t.size() : lf + 1;
        }
        return out;
    }
    std::string file_ext(const BigUint& i) const override { return is_index(i) ? ".txt" : ".fractal"; } // (a ruleset as sieve-fractal-v1; the index as text)
    uint32_t deepest(const BigUint& i) const override
    {
        if (is_index(i)) return 0;
        if (const auto k = charted(i)) return chart_deepest_[*k];
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (const auto it = deepest_.find(i); it != deepest_.end()) return it->second;
        }
        const auto f = sieve::read_fractal(ruleset(i));
        const uint32_t d = f ? drawn_deepest(*f) : 0;
        std::lock_guard<std::mutex> lock(mx_);
        if (deepest_.size() >= kKept) deepest_.clear();
        deepest_.emplace(i, d);
        return d;
    }
    uint32_t first_depth(const BigUint& i) const override { return std::min<uint32_t>(deepest(i), 5); }
    std::optional<BigUint> find(const std::string& name) const override
    {
        if (name == "index") return index_ ? std::optional<BigUint>(BigUint()) : std::nullopt;
        for (size_t k = 0; k < chart_.size(); ++k)
            if (chart_[k].id == name)
            {
                if (known_ == "only") return BigUint(uint64_t(k) + 1);
                if (known_ == "exclude" || !chart_index_[k]) return std::nullopt; // (set aside, or longer than the longest)
                BigUint p = *chart_index_[k];
                p.add_small(1);
                return p;
            }
        return std::nullopt;
    }
    std::string not_found(const std::string& name) const override
    {
        const std::string where = tr("fractals.why.where");
        if (name == "index" && !index_) return trf("fractals.why.index", {where});
        for (const auto& c : chart_)
            if (c.id == name)
            {
                if (known_ == "exclude") return trf("fractals.why.set_aside", {name, where});
                if (c.text.size() > space_.longest()) return trf("fractals.why.too_long", {name, std::to_string(c.text.size()), std::to_string(space_.longest()), where});
            }
        return "";
    }
    std::optional<std::vector<uint32_t>> unit(const BigUint& i) const override
    {
        // (the index is no ruleset, and a known fractal is never judged: the known setting rules it)
        if (is_index(i) || charted(i)) return std::nullopt;
        const std::string t = ruleset(i);
        if (t.empty()) return std::nullopt;
        return std::vector<uint32_t>(t.begin(), t.end());
    }
    sieve::FilterLine filter_line() const override { return sieve::cli::fractal_filter_line(space_.longest()); }
    std::string trouble() const override { return trouble_; }
    std::string units_text() const override
    {
        if (known_ == "only") return trf("fractals.units.only", {std::to_string(chart_.size())});
        return trf(known_ == "exclude" ? "fractals.units.exclude" : "fractals.units.include", {std::to_string(space_.longest())});
    }

private:
    bool is_index(const BigUint& i) const { return index_ && i.is_zero(); }
    // The ruleset an item (but the index) holds: a charted one's text, or the whole space's at its address.
    std::string ruleset(const BigUint& i) const
    {
        if (known_ == "only") return i.is_zero() || i > BigUint(uint64_t(chart_.size())) ? std::string() : chart_[size_t(i.mod_small(1u << 31)) - 1].text;
        const BigUint j = space_index(i);
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (const auto it = texts_.find(j); it != texts_.end()) return it->second;
        }
        std::string t = j < space_.count() ? space_.at(j) : std::string();
        std::lock_guard<std::mutex> lock(mx_);
        if (texts_.size() >= kKept) texts_.clear();
        texts_.emplace(j, t);
        return t;
    }
    // An item's address in the whole space (included: one less, after the index; excluded: past every
    // charted one at or before it, the charted ones' addresses in order).
    BigUint space_index(const BigUint& i) const
    {
        BigUint j = i;
        if (index_) j -= BigUint(1);
        if (known_ == "exclude")
        {
            for (const auto& [c, k] : charted_at_)
            {
                if (c > j) break;
                j.add_small(1);
            }
        }
        return j;
    }
    // The charted fractal an item is, if it is one.
    std::optional<size_t> charted(const BigUint& i) const
    {
        if (is_index(i)) return std::nullopt;
        if (known_ == "only") return i > BigUint(uint64_t(chart_.size())) ? std::nullopt : std::optional<size_t>(size_t(i.mod_small(1u << 31)) - 1);
        if (known_ == "exclude") return std::nullopt;
        const auto it = charted_at_.find(space_index(i));
        return it == charted_at_.end() ? std::nullopt : std::optional<size_t>(it->second);
    }

    std::string known_;
    bool index_ = true; // whether item 0 is the index (not when the known fractals are excluded)
    sieve::FractalSpace space_;
    std::vector<sieve::ChartedFractal> chart_;
    std::vector<uint32_t> chart_deepest_;
    std::vector<std::optional<BigUint>> chart_index_; // each charted one's address in the space (none: longer than the longest)
    std::map<BigUint, size_t> charted_at_;              // and by address, in order
    BigUint size_;
    std::string id_, trouble_;
    // (kept as worked out; the shelves' faces are painted on worker threads, so under a lock)
    mutable std::mutex mx_;
    mutable std::map<BigUint, std::string> texts_;
    mutable std::map<BigUint, uint32_t> deepest_;
};

} // namespace

std::unique_ptr<ListedShelf> make_listed_shelf(Media m, const ListedOptions& options)
{
    if (m == Media::Fractals) return std::make_unique<FractalShelf>(options);
    return nullptr;
}

} // namespace hallway
