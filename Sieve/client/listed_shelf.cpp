// Sieve hallway — listed lines' shelves (listed_shelf.hpp): the fractal dimension's.
#include "listed_shelf.hpp"

#include "strings.hpp"

#include "cli/dictionaries.hpp" // find_data_folder
#include "sieve/fractal.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

namespace {

// The deepest a fractal is drawn on a shelf, in the hands or in the viewer: past this many symbols a
// word takes too long to draw a frame (a limit chosen, not measured; `sieve fractal draw` goes as deep
// as a ruleset grows).
constexpr size_t kDrawnMost = size_t(1) << 18;

// The fractal dimension: item 0 the index of every fractal charted, items 1 on the fractals in the
// chart's order (data/fractals/chart.txt).
class FractalShelf : public ListedShelf
{
public:
    FractalShelf()
    {
        try
        {
            chart_ = sieve::read_fractal_chart(sieve::cli::find_data_folder("fractals", "chart.txt"));
        }
        catch (const std::exception& e)
        {
            trouble_ = e.what();
        }
        std::string all;
        for (const auto& c : chart_)
        {
            all += c.id + "\n" + c.text;
            uint32_t d = 0;
            while (d < c.fractal.deepest())
            {
                const auto w = c.fractal.grow(d + 1);
                if (!w || w->size() > kDrawnMost) break;
                ++d;
            }
            deepest_.push_back(d);
        }
        // (named by what it holds, so a chart changed is a shelf of its own and its favourites stay apart)
        id_ = "fractals/" + sieve::Sha256::hex_hash(all).substr(0, 16) + "/fractal-chart-v1";
    }
    size_t size() const override { return chart_.size() + 1; }
    std::string id() const override { return id_; }
    std::string title(size_t i) const override { return i == 0 ? tr("fractals.index") : chart_.at(i - 1).title; }
    std::string summary(size_t i) const override
    {
        if (i == 0) return trf("fractals.index.summary", {std::to_string(chart_.size())});
        const auto& f = chart_.at(i - 1).fractal;
        return f.family == sieve::Fractal::Family::Substitution ? tr("fractals.substitution") : trf("fractals.lsystem.angle", {std::to_string(f.angle)});
    }
    std::optional<sieve::RgbaImage> picture(size_t i, uint32_t depth, uint32_t width, uint32_t height) const override
    {
        if (i == 0)
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
                const auto t = sieve::draw_fractal(chart_[k].fractal, std::min<uint32_t>(deepest_[k], 4), s, s);
                if (!t) continue;
                const uint32_t ox = uint32_t(k % cols) * cw + (cw - s) / 2, oy = uint32_t(k / cols) * ch + (ch - s) / 2;
                for (uint32_t y = 0; y < s; ++y)
                    std::copy_n(&t->rgba[size_t(y) * s * 4], size_t(s) * 4, &img.rgba[(size_t(oy + y) * width + ox) * 4]);
            }
            return img;
        }
        return sieve::draw_fractal(chart_.at(i - 1).fractal, std::min(depth, deepest_.at(i - 1)), width, height);
    }
    std::vector<std::string> text(size_t i) const override
    {
        std::vector<std::string> out;
        if (i == 0)
        {
            for (size_t k = 0; k < chart_.size(); ++k)
                out.push_back(std::to_string(k + 1) + ". " + chart_[k].title + " (" + chart_[k].id + ")");
            if (!trouble_.empty()) out.push_back(trf("fractals.unreadable", {trouble_}));
            return out;
        }
        const std::string& t = chart_.at(i - 1).text;
        for (size_t at = 0; at < t.size();)
        {
            const size_t lf = t.find('\n', at);
            out.push_back(t.substr(at, lf - at));
            at = lf == std::string::npos ? t.size() : lf + 1;
        }
        return out;
    }
    // (a fractal as its ruleset, sieve-fractal-v1, as its file in data/fractals holds it; the index as text)
    std::string file_ext(size_t i) const override { return i == 0 ? ".txt" : ".fractal"; }
    uint32_t deepest(size_t i) const override { return i == 0 ? 0 : deepest_.at(i - 1); }
    uint32_t first_depth(size_t i) const override { return i == 0 ? 0 : std::min<uint32_t>(deepest_.at(i - 1), 5); }
    std::optional<size_t> find(const std::string& name) const override
    {
        if (name == "index") return size_t(0);
        for (size_t k = 0; k < chart_.size(); ++k)
            if (chart_[k].id == name) return k + 1;
        return std::nullopt;
    }
    std::string trouble() const override { return trouble_; }

private:
    std::vector<sieve::ChartedFractal> chart_;
    std::vector<uint32_t> deepest_;
    std::string id_, trouble_;
};

} // namespace

std::unique_ptr<ListedShelf> make_listed_shelf(Media m)
{
    if (m == Media::Fractals) return std::make_unique<FractalShelf>();
    return nullptr;
}

} // namespace hallway
