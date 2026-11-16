// Sieve — the fractal dimension's rulesets (sieve/fractal.hpp; IDEAS 19.191, 19.235).
#include "sieve/fractal.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sieve {

namespace {

bool substitution_symbol(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'); }
bool letter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
bool lsystem_symbol(char c) { return letter(c) || c == '+' || c == '-' || c == '[' || c == ']' || c == '|'; }
bool scaled_symbol(char c) { return lsystem_symbol(c) || c == '"' || c == '\''; }

// A whole number written in decimal with no sign or leading zero, from lo to hi.
std::optional<uint32_t> decimal(std::string_view s, uint32_t lo, uint32_t hi)
{
    if (s.empty() || s.size() > 9 || (s.size() > 1 && s[0] == '0')) return std::nullopt;
    uint32_t v = 0;
    for (const char c : s)
    {
        if (c < '0' || c > '9') return std::nullopt;
        v = v * 10 + uint32_t(c - '0');
    }
    if (v < lo || v > hi) return std::nullopt;
    return v;
}

// A colour for a symbol of a substitution: its place among the rule's symbols round a wheel of hues.
std::array<uint8_t, 3> hue(size_t i, size_t n)
{
    const double h = 6.0 * double(i) / double(std::max<size_t>(n, 1));
    const int k = int(h) % 6;
    const double f = h - std::floor(h);
    const double q = 1 - f;
    double r = 0, g = 0, b = 0;
    switch (k)
    {
    case 0: r = 1, g = f, b = 0; break;
    case 1: r = q, g = 1, b = 0; break;
    case 2: r = 0, g = 1, b = f; break;
    case 3: r = 0, g = q, b = 1; break;
    case 4: r = f, g = 0, b = 1; break;
    default: r = 1, g = 0, b = q; break;
    }
    // (softened towards white, so they read on the dark ground)
    return {uint8_t(80 + 175 * r), uint8_t(80 + 175 * g), uint8_t(80 + 175 * b)};
}

constexpr std::array<uint8_t, 3> kGround = {16, 20, 24};

RgbaImage blank(uint32_t w, uint32_t h)
{
    RgbaImage img;
    img.width = w, img.height = h;
    img.rgba.assign(size_t(w) * h * 4, 255);
    for (size_t i = 0; i < size_t(w) * h; ++i)
        for (int c = 0; c < 3; ++c) img.rgba[i * 4 + size_t(c)] = kGround[size_t(c)];
    return img;
}

void put(RgbaImage& img, int x, int y, const std::array<uint8_t, 3>& c)
{
    if (x < 0 || y < 0 || uint32_t(x) >= img.width || uint32_t(y) >= img.height) return;
    uint8_t* p = &img.rgba[(size_t(y) * img.width + size_t(x)) * 4];
    p[0] = c[0], p[1] = c[1], p[2] = c[2], p[3] = 255;
}

// A line by Bresenham's walk, its end points included.
void line(RgbaImage& img, int x0, int y0, int x1, int y1, const std::array<uint8_t, 3>& c)
{
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;)
    {
        put(img, x0, y0, c);
        if (x0 == x1 && y0 == y1) return;
        const int e2 = 2 * err;
        if (e2 >= dy) err += dy, x0 += sx;
        if (e2 <= dx) err += dx, y0 += sy;
    }
}

// An angle as v2 writes it: whole degrees (1 to 359), or degrees and a tenth not 0 (0.1 to 359.9); in tenths.
std::optional<uint32_t> angle_tenths_of(std::string_view s)
{
    const size_t dot = s.find('.');
    if (dot == std::string_view::npos)
    {
        const auto a = decimal(s, 1, 359);
        return a ? std::optional<uint32_t>(*a * 10) : std::nullopt;
    }
    const auto whole = decimal(s.substr(0, dot), 0, 359);
    if (!whole || s.size() != dot + 2 || s[dot + 1] < '1' || s[dot + 1] > '9') return std::nullopt;
    return *whole * 10 + uint32_t(s[dot + 1] - '0');
}

// A ratio: "0." and one to three digits, the last not 0; in thousandths.
std::optional<uint32_t> scale_of(std::string_view s)
{
    if (s.size() < 3 || s.size() > 5 || s.substr(0, 2) != "0." || s.back() == '0') return std::nullopt;
    uint32_t v = 0;
    for (size_t k = 2; k < 5; ++k)
    {
        const char c = k < s.size() ? s[k] : '0';
        if (c < '0' || c > '9') return std::nullopt;
        v = v * 10 + uint32_t(c - '0');
    }
    return v;
}

} // namespace

std::string fractal_angle_text(uint32_t tenths) { return std::to_string(tenths / 10) + (tenths % 10 ? "." + std::to_string(tenths % 10) : ""); }

std::string fractal_scale_text(uint32_t thousandths)
{
    std::string t = std::to_string(1000 + thousandths % 1000).substr(1);
    while (!t.empty() && t.back() == '0') t.pop_back();
    return "0." + t;
}

std::string Fractal::text() const
{
    std::string t = std::string(version == 2 ? kFractalVersion2 : kFractalVersion) + "\n";
    t += family == Family::Substitution ? std::string("substitution\n")
                                        : "lsystem " + fractal_angle_text(angle_tenths) + (scale ? " scale " + fractal_scale_text(scale) : std::string()) + "\n";
    t += "axiom " + axiom + "\n";
    for (const auto& [s, w] : rules) t += std::string("rule ") + s + " " + w + "\n";
    return t;
}

std::optional<std::string> Fractal::grow(uint32_t depth) const
{
    std::array<const std::string*, 256> rule{};
    for (const auto& [s, w] : rules) rule[uint8_t(s)] = &w;
    std::string word = axiom;
    for (uint32_t d = 0; d < depth; ++d)
    {
        std::string next;
        for (const char c : word)
        {
            if (const std::string* w = rule[uint8_t(c)]) next += *w;
            else next += c;
            if (next.size() > kFractalMost) return std::nullopt;
        }
        word = std::move(next);
    }
    return word;
}

uint32_t Fractal::deepest() const { return fractal_deepest_within(*this, kFractalMost); }

uint32_t fractal_deepest_within(const Fractal& f, size_t most)
{
    // (each step grows the word by at most the longest rule, so the sizes are worked out, not grown)
    const uint64_t limit = std::min<uint64_t>(most, kFractalMost);
    const auto& rules = f.rules;
    const auto& axiom = f.axiom;
    std::array<uint64_t, 256> size{};
    for (int c = 0; c < 256; ++c) size[size_t(c)] = 1;
    uint32_t d = 0;
    for (; d < kFractalDepthMost; ++d)
    {
        std::array<uint64_t, 256> next = size;
        for (const auto& [s, w] : rules)
        {
            uint64_t n = 0;
            for (const char c : w) n = std::min<uint64_t>(n + size[uint8_t(c)], limit + 1);
            next[uint8_t(s)] = n;
        }
        uint64_t total = 0;
        for (const char c : axiom) total = std::min<uint64_t>(total + next[uint8_t(c)], limit + 1);
        if (total > limit) break;
        size = next;
    }
    return d;
}

std::optional<Fractal> read_fractal(const std::string& text, std::string* why)
{
    auto bad = [&](const std::string& w) -> std::optional<Fractal> {
        if (why) *why = w;
        return std::nullopt;
    };
    std::vector<std::string> lines;
    if (!lf_lines(text, lines)) return bad("its last line does not end in a line feed");
    if (lines.size() < 4 || (lines[0] != kFractalVersion && lines[0] != kFractalVersion2))
        return bad(std::string("its first line is not ") + kFractalVersion + " or " + kFractalVersion2 + ", or it has no axiom and rule");
    Fractal f;
    f.version = lines[0] == kFractalVersion2 ? 2 : 1;
    if (lines[1] == "substitution")
    {
        if (f.version == 2) return bad("a substitution is written as sieve-fractal-v1 (v2 widens the L-system alone)");
        f.family = Fractal::Family::Substitution;
    }
    else if (lines[1].rfind("lsystem ", 0) == 0)
    {
        f.family = Fractal::Family::LSystem;
        std::string_view rest = std::string_view(lines[1]).substr(8);
        if (f.version == 1)
        {
            const auto a = decimal(rest, 1, 359);
            if (!a) return bad("an L-system's angle is whole degrees, 1 to 359");
            f.angle_tenths = *a * 10;
        }
        else
        {
            const size_t sp = rest.find(" scale ");
            const auto a = angle_tenths_of(rest.substr(0, sp));
            if (!a) return bad("a v2 L-system's angle is whole degrees (1 to 359) or degrees and a tenth (0.1 to 359.9, as 22.5)");
            f.angle_tenths = *a;
            if (sp != std::string_view::npos)
            {
                const auto s = scale_of(rest.substr(sp + 7));
                if (!s) return bad("a scale is 0.001 to 0.999, one to three decimals with no 0 at the end (as 0.5 or 0.707)");
                f.scale = *s;
            }
            if (f.angle_tenths % 10 == 0 && f.scale == 0) return bad("a v2 ruleset has an angle with a tenth or a scale (anything else is written as sieve-fractal-v1)");
        }
    }
    else return bad("its second line is neither \"substitution\" nor \"lsystem ANGLE\"");
    const bool sub = f.family == Fractal::Family::Substitution;
    auto word_ok = [&](std::string_view w) {
        if (w.empty() || w.size() > kFractalWordMost) return false;
        return std::all_of(w.begin(), w.end(), [&](char c) { return sub ? substitution_symbol(c) : f.scale ? scaled_symbol(c) : lsystem_symbol(c); });
    };
    if (lines[2].rfind("axiom ", 0) != 0 || !word_ok(std::string_view(lines[2]).substr(6))) return bad("its third line is not \"axiom WORD\" (1 to 64 of its symbols)");
    f.axiom = lines[2].substr(6);
    for (size_t i = 3; i < lines.size(); ++i)
    {
        const std::string& l = lines[i];
        if (l.size() < 8 || l.rfind("rule ", 0) != 0 || l[6] != ' ') return bad("line " + std::to_string(i + 1) + " is not \"rule SYMBOL WORD\"");
        const char s = l[5];
        if (!(sub ? substitution_symbol(s) : letter(s))) return bad("line " + std::to_string(i + 1) + " rewrites a symbol that cannot have a rule");
        if (!word_ok(std::string_view(l).substr(7))) return bad("line " + std::to_string(i + 1) + "'s word is not 1 to 64 of its symbols");
        if (!f.rules.empty() && !(f.rules.back().first < s)) return bad("line " + std::to_string(i + 1) + ": the rules are in the order of their symbols, each once");
        f.rules.push_back({s, l.substr(7)});
    }
    return f;
}

std::vector<std::array<double, 4>> fractal_lines(const Fractal& f, const std::string& word)
{
    std::vector<std::array<double, 4>> out;
    const double turn = double(f.angle_tenths) * 3.14159265358979323846 / 1800.0, ratio = double(f.scale) / 1000.0;
    double x = 0, y = 0, a = 0, step = 1;
    std::vector<std::array<double, 4>> kept;
    for (const char c : word)
    {
        if (c == 'F' || c == 'G' || c == 'f')
        {
            const double nx = x + step * std::cos(a), ny = y + step * std::sin(a);
            if (c != 'f') out.push_back({x, y, nx, ny});
            x = nx, y = ny;
        }
        else if (c == '+') a += turn;
        else if (c == '-') a -= turn;
        else if (c == '|') a += 3.14159265358979323846;
        // (held within 10^-60 and 10^60, so a word of a million ' still draws: a bound chosen)
        else if (c == '"' && ratio > 0) step = std::max(step * ratio, 1e-60);
        else if (c == '\'' && ratio > 0) step = std::min(step / ratio, 1e60);
        else if (c == '[') kept.push_back({x, y, a, step});
        else if (c == ']' && !kept.empty()) x = kept.back()[0], y = kept.back()[1], a = kept.back()[2], step = kept.back()[3], kept.pop_back();
    }
    return out;
}

std::optional<RgbaImage> draw_fractal(const Fractal& f, uint32_t depth, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > 8192 || height > 8192) return std::nullopt;
    RgbaImage img = blank(width, height);
    if (f.family == Fractal::Family::Substitution)
    {
        // Each depth a row, stretched to the width; each symbol its colour.
        std::string symbols;
        for (uint32_t d = 0; d <= depth; ++d)
        {
            const auto w = f.grow(d);
            if (!w) return std::nullopt;
            for (const char c : *w)
                if (symbols.find(c) == std::string::npos) symbols += c;
        }
        std::sort(symbols.begin(), symbols.end());
        const uint32_t rows = depth + 1, gap = rows > 1 && height / rows > 4 ? 1 : 0;
        for (uint32_t d = 0; d <= depth; ++d)
        {
            const std::string w = *f.grow(d);
            const uint32_t y0 = uint32_t(uint64_t(height) * d / rows), y1 = uint32_t(uint64_t(height) * (d + 1) / rows) - gap;
            for (uint32_t x = 0; x < width; ++x)
            {
                const char c = w[size_t(uint64_t(x) * w.size() / width)];
                const auto col = hue(symbols.find(c), symbols.size());
                for (uint32_t y = y0; y < y1; ++y) put(img, int(x), int(y), col);
            }
        }
        return img;
    }
    const auto word = f.grow(depth);
    if (!word) return std::nullopt;
    const auto lines = fractal_lines(f, *word);
    if (lines.empty()) return img;
    double lx = lines[0][0], hx = lx, ly = lines[0][1], hy = ly;
    for (const auto& l : lines)
        for (int k = 0; k < 2; ++k) lx = std::min(lx, l[size_t(2 * k)]), hx = std::max(hx, l[size_t(2 * k)]), ly = std::min(ly, l[size_t(2 * k + 1)]), hy = std::max(hy, l[size_t(2 * k + 1)]);
    const double margin = 0.05, span = std::max(hx - lx, hy - ly);
    const double s = (1 - 2 * margin) * double(std::min(width, height)) / std::max(span, 1e-9);
    const double ox = (double(width) - (hx - lx) * s) / 2, oy = (double(height) - (hy - ly) * s) / 2;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const auto& l = lines[i];
        // (y up, as a turtle's is; the colour along the path, from red through to violet)
        line(img, int(std::lround(ox + (l[0] - lx) * s)), int(std::lround(double(height) - oy - (l[1] - ly) * s)), int(std::lround(ox + (l[2] - lx) * s)),
             int(std::lround(double(height) - oy - (l[3] - ly) * s)), hue(i * 5, lines.size() * 6));
    }
    return img;
}

std::optional<FractalDimension> fractal_box_dimension(const Fractal& f)
{
    if (f.family != Fractal::Family::LSystem) return std::nullopt;
    const uint32_t depth = fractal_deepest_within(f, kFractalDimensionSymbols);
    const auto word = f.grow(depth);
    if (!word) return std::nullopt;
    const auto lines = fractal_lines(f, *word);
    if (lines.size() < 16) return std::nullopt;
    double lx = lines[0][0], hx = lx, ly = lines[0][1], hy = ly;
    std::vector<double> lengths; // (each line's, for their median)
    lengths.reserve(lines.size());
    for (const auto& l : lines)
    {
        for (int k = 0; k < 2; ++k) lx = std::min(lx, l[size_t(2 * k)]), hx = std::max(hx, l[size_t(2 * k)]), ly = std::min(ly, l[size_t(2 * k + 1)]), hy = std::max(hy, l[size_t(2 * k + 1)]);
        lengths.push_back(std::hypot(l[2] - l[0], l[3] - l[1]));
    }
    const double span = std::max(hx - lx, hy - ly) * (1 + 1e-9);
    if (!(span > 0) || !std::isfinite(span)) return std::nullopt;
    std::nth_element(lengths.begin(), lengths.begin() + std::ptrdiff_t(lengths.size() / 2), lengths.end());
    const double median = lengths[lengths.size() / 2];
    std::vector<double> xs, ys;
    std::vector<uint8_t> cells;
    for (uint32_t n = 4; n <= 1024 && span / n >= median; n *= 2)
    {
        const double cell = span / n;
        // (a grid whose points along the lines would pass 2^22 is not laid: a bound chosen, so a ruleset of
        // many long lines is measured in a moment, on the coarser grids)
        double points = 0;
        for (const auto& l : lines) points += std::ceil(std::hypot(l[2] - l[0], l[3] - l[1]) / (cell / 2)) + 1;
        if (points > double(1 << 22)) break;
        cells.assign(size_t(n) * n, 0);
        size_t count = 0;
        auto mark = [&](double x, double y) {
            const size_t cx = std::min<size_t>(n - 1, size_t(std::max(0.0, (x - lx) / cell))), cy = std::min<size_t>(n - 1, size_t(std::max(0.0, (y - ly) / cell)));
            uint8_t& c = cells[cy * n + cx];
            count += c == 0, c = 1;
        };
        for (const auto& l : lines)
        {
            // (points along it no further apart than half a cell, so every cell it crosses is met)
            const size_t steps = size_t(std::ceil(std::hypot(l[2] - l[0], l[3] - l[1]) / (cell / 2)));
            for (size_t s = 0; s <= steps; ++s)
            {
                const double t = steps ? double(s) / double(steps) : 0;
                mark(l[0] + (l[2] - l[0]) * t, l[1] + (l[3] - l[1]) * t);
            }
        }
        xs.push_back(std::log2(double(n))), ys.push_back(std::log2(double(count)));
    }
    if (xs.size() < 3) return std::nullopt;
    double mx = 0, my = 0;
    for (size_t k = 0; k < xs.size(); ++k) mx += xs[k], my += ys[k];
    mx /= double(xs.size()), my /= double(xs.size());
    double sxy = 0, sxx = 0;
    for (size_t k = 0; k < xs.size(); ++k) sxy += (xs[k] - mx) * (ys[k] - my), sxx += (xs[k] - mx) * (xs[k] - mx);
    return FractalDimension{sxy / sxx, depth, uint32_t(xs.size())};
}

std::vector<ChartedFractal> read_fractal_chart(const std::filesystem::path& folder)
{
    auto read = [](const std::filesystem::path& p) {
        std::ifstream in(p, std::ios::binary);
        if (!in) throw std::invalid_argument("cannot read " + p.string());
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    };
    std::vector<std::string> lines;
    if (!lf_lines(read(folder / "chart.txt"), lines) || lines.empty() || (lines[0] != kFractalChartVersion && lines[0] != kFractalChartVersion2))
        throw std::invalid_argument(std::string("the chart is not a ") + kFractalChartVersion + " or v2 with every line ended");
    const bool sourced = lines[0] == kFractalChartVersion2;
    std::vector<ChartedFractal> out;
    for (size_t i = 1; i < lines.size(); ++i)
    {
        const std::string& l = lines[i];
        if (sourced && l.rfind("source ", 0) == 0)
        {
            if (out.empty() || !out.back().source.empty() || l.size() == 7) throw std::invalid_argument("chart line " + std::to_string(i + 1) + ": a \"source TEXT\" line follows its fractal's, once");
            out.back().source = l.substr(7);
            continue;
        }
        if (sourced && !out.empty() && out.back().source.empty()) throw std::invalid_argument(out.back().id + " has no \"source TEXT\" line after it");
        const size_t sp = l.find(' ', 8);
        if (l.rfind("fractal ", 0) != 0 || sp == std::string::npos || sp + 1 >= l.size()) throw std::invalid_argument("chart line " + std::to_string(i + 1) + " is not \"fractal ID TITLE\"");
        ChartedFractal c;
        c.id = l.substr(8, sp - 8), c.title = l.substr(sp + 1);
        if (c.id.empty() || c.id.size() > 64 || !std::all_of(c.id.begin(), c.id.end(), [](char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-'; }))
            throw std::invalid_argument("chart line " + std::to_string(i + 1) + ": an id is 1 to 64 of a-z, 0-9 and '-'");
        for (const auto& o : out)
            if (o.id == c.id) throw std::invalid_argument("chart line " + std::to_string(i + 1) + ": " + c.id + " is charted twice");
        c.text = read(folder / (c.id + ".fractal"));
        std::string why;
        const auto f = read_fractal(c.text, &why);
        if (!f) throw std::invalid_argument(c.id + ".fractal: " + why);
        c.fractal = *f;
        out.push_back(std::move(c));
    }
    if (sourced && !out.empty() && out.back().source.empty()) throw std::invalid_argument(out.back().id + " has no \"source TEXT\" line after it");
    return out;
}

namespace {
std::mutex& chart_mutex()
{
    static std::mutex m;
    return m;
}
std::filesystem::path& chart_folder()
{
    static std::filesystem::path p;
    return p;
}
std::shared_ptr<const std::vector<ChartedFractal>>& chart_read()
{
    static std::shared_ptr<const std::vector<ChartedFractal>> c;
    return c;
}
} // namespace

void set_fractal_chart_folder(const std::filesystem::path& folder)
{
    std::lock_guard<std::mutex> lock(chart_mutex());
    chart_folder() = folder;
    chart_read().reset();
}

std::shared_ptr<const std::vector<ChartedFractal>> fractal_chart()
{
    std::lock_guard<std::mutex> lock(chart_mutex());
    if (!chart_read())
    {
        if (chart_folder().empty()) throw std::invalid_argument("no fractal chart is known here (the fractals folder)");
        chart_read() = std::make_shared<const std::vector<ChartedFractal>>(read_fractal_chart(chart_folder()));
    }
    return chart_read();
}

} // namespace sieve
