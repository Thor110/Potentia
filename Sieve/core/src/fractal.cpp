// Sieve — the fractal dimension's rulesets (sieve/fractal.hpp; IDEAS 19.191, 19.235).
#include "sieve/fractal.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
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
bool digit(char c) { return c >= '0' && c <= '9'; }
// v3's symbols: v2's with a scale, digits, ! (swap the turns) and & ^ \ / (pitch and roll).
bool v3_symbol(char c)
{
    return scaled_symbol(c) || digit(c) || c == '!' || c == '&' || c == '^' || c == '\\' || c == '/';
}

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

// A decimal written with no leading zero (a lone 0 before the point), at most `most` decimals, the
// last not 0, at most 3 digits before the point; its value in millionths.
std::optional<int64_t> millionths(std::string_view s, size_t most)
{
    const size_t dot = s.find('.');
    const std::string_view whole = s.substr(0, dot);
    if (whole.empty() || whole.size() > 3 || (whole.size() > 1 && whole[0] == '0') || !std::all_of(whole.begin(), whole.end(), digit)) return std::nullopt;
    int64_t v = 0;
    for (const char c : whole) v = v * 10 + (c - '0');
    v *= 1000000;
    if (dot == std::string_view::npos) return v;
    const std::string_view frac = s.substr(dot + 1);
    if (frac.empty() || frac.size() > most || frac.back() == '0' || !std::all_of(frac.begin(), frac.end(), digit)) return std::nullopt;
    int64_t f = 0;
    for (size_t k = 0; k < 6; ++k) f = f * 10 + (k < frac.size() ? frac[k] - '0' : 0);
    return v + f;
}

// A whole number with no leading zero, lo to hi.
std::optional<uint64_t> whole_number(std::string_view s, uint64_t lo, uint64_t hi)
{
    if (s.empty() || s.size() > 10 || (s.size() > 1 && s[0] == '0') || !std::all_of(s.begin(), s.end(), digit)) return std::nullopt;
    uint64_t v = 0;
    for (const char c : s) v = v * 10 + uint64_t(c - '0');
    if (v < lo || v > hi) return std::nullopt;
    return v;
}

// The lines after the version line of a v3 ruleset (sieve/fractal.hpp).
std::optional<Fractal> read_v3(const std::vector<std::string>& lines, const std::function<std::optional<Fractal>(const std::string&)>& bad)
{
    Fractal f;
    f.version = 3, f.family = Fractal::Family::LSystem;
    if (lines.size() < 4 || lines[1].rfind("lsystem ", 0) != 0) return bad("a v3 ruleset's second line is \"lsystem ANGLE\"");
    f.angle_text = lines[1].substr(8);
    if (f.angle_text.rfind("360/", 0) == 0)
    {
        const auto n = whole_number(std::string_view(f.angle_text).substr(4), 2, 9999);
        if (!n) return bad("an angle 360/N has N from 2 to 9999");
        f.angle_degrees = 360.0 / double(*n);
    }
    else
    {
        const auto a = millionths(f.angle_text, 6);
        if (!a || *a <= 0 || *a >= 360000000) return bad("a v3 angle is more than 0 and less than 360, at most 6 decimals with no 0 at the end, or 360/N");
        f.angle_degrees = double(*a) / 1e6;
    }
    size_t i = 2;
    if (lines[i].rfind("scale ", 0) == 0)
    {
        f.scale_text = lines[i].substr(6);
        const auto r = millionths(f.scale_text, 6);
        if (!r || *r <= 0 || *r >= 1000000) return bad("a v3 scale is 0.d, 1 to 6 decimals with no 0 at the end (as 0.618034)");
        f.scale_ratio = double(*r) / 1e6;
        ++i;
    }
    if (i < lines.size() && lines[i].rfind("seed ", 0) == 0)
    {
        const auto n = whole_number(std::string_view(lines[i]).substr(5), 1, 4294967295ull);
        if (!n) return bad("a seed is 1 to 4294967295, no leading zero");
        f.seed = uint32_t(*n);
        ++i;
    }
    if (i < lines.size() && lines[i].rfind("ignore ", 0) == 0)
    {
        f.ignore = lines[i].substr(7);
        if (f.ignore.empty() || !std::all_of(f.ignore.begin(), f.ignore.end(), v3_symbol)) return bad("ignore lists symbols of the ruleset's");
        for (size_t k = 1; k < f.ignore.size(); ++k)
            if (!(f.ignore[k - 1] < f.ignore[k])) return bad("ignore lists its symbols in the order of their codes, each once");
        ++i;
    }
    for (; i < lines.size() && lines[i].rfind("define ", 0) == 0; ++i)
    {
        const std::string& l = lines[i];
        Fractal::Define d;
        if (l.size() < 10 || !digit(l[7]) || l[8] != ' ') return bad("line " + std::to_string(i + 1) + " is not \"define DIGIT turn DEGREES\" or \"define DIGIT scale FACTOR\"");
        d.digit = l[7];
        if (!f.defines.empty() && !(f.defines.back().digit < d.digit)) return bad("line " + std::to_string(i + 1) + ": the digits are defined in order, each once");
        if (l.compare(9, 5, "turn ") == 0)
        {
            d.turn = true, d.text = l.substr(14);
            const bool negative = !d.text.empty() && d.text[0] == '-';
            const auto v = millionths(std::string_view(d.text).substr(negative ? 1 : 0), 6);
            if (!v || *v <= 0 || *v >= 360000000) return bad("line " + std::to_string(i + 1) + ": a turn is more than 0 and less than 360 degrees either way, at most 6 decimals with no 0 at the end");
            d.value = (negative ? -1.0 : 1.0) * double(*v) / 1e6;
        }
        else if (l.compare(9, 6, "scale ") == 0)
        {
            d.turn = false, d.text = l.substr(15);
            const auto v = millionths(d.text, 6);
            if (!v || *v <= 0 || *v >= 1000000000 || *v == 1000000) return bad("line " + std::to_string(i + 1) + ": a factor is more than 0 and less than 1000, not 1, at most 6 decimals with no 0 at the end");
            d.value = double(*v) / 1e6;
        }
        else return bad("line " + std::to_string(i + 1) + " is not \"define DIGIT turn DEGREES\" or \"define DIGIT scale FACTOR\"");
        f.defines.push_back(d);
    }
    auto word_ok = [](std::string_view w) { return !w.empty() && w.size() <= kFractalWordMostV3 && std::all_of(w.begin(), w.end(), v3_symbol); };
    if (i >= lines.size() || lines[i].rfind("axiom ", 0) != 0 || !word_ok(std::string_view(lines[i]).substr(6))) return bad("line " + std::to_string(i + 1) + " is not \"axiom WORD\" (1 to 255 of its symbols)");
    f.axiom = lines[i].substr(6);
    if (++i >= lines.size()) return bad("it has no rule");
    for (; i < lines.size(); ++i)
    {
        const std::string& l = lines[i];
        const size_t sp = l.find(' ', 5);
        if (l.rfind("rule ", 0) != 0 || sp == std::string::npos) return bad("line " + std::to_string(i + 1) + " is not \"rule [L<]SYMBOL[>R] WORD\"");
        std::string_view head = std::string_view(l).substr(5, sp - 5);
        char left = 0, right = 0;
        if (head.size() >= 3 && head[1] == '<') left = head[0], head.remove_prefix(2);
        if (head.size() >= 3 && head[1] == '>') right = head[2], head.remove_suffix(2);
        if (head.size() != 1 || !letter(head[0]) || (left && !letter(left)) || (right && !letter(right)))
            return bad("line " + std::to_string(i + 1) + " is not \"rule [L<]SYMBOL[>R] WORD\" (letters)");
        if (!word_ok(std::string_view(l).substr(sp + 1))) return bad("line " + std::to_string(i + 1) + "'s word is not 1 to 255 of its symbols");
        if (!f.rules.empty() && head[0] < f.rules.back().first) return bad("line " + std::to_string(i + 1) + ": the rules are in the order of their symbols");
        f.rules.push_back({head[0], l.substr(sp + 1)});
        f.contexts.push_back({left, right});
    }
    return f;
}

// A v4 number (sieve/fractal.hpp): an optional -, a whole part of 0 to 999, up to 6 decimals, the last
// not 0, never -0; 0 itself only where zero_ok.
std::optional<double> v4_number(std::string_view s, bool zero_ok)
{
    const bool negative = !s.empty() && s[0] == '-';
    const std::string_view body = s.substr(negative ? 1 : 0);
    const auto v = millionths(body, 6);
    if (!v) return std::nullopt;
    if (*v == 0) return zero_ok && !negative && body == "0" ? std::optional<double>(0.0) : std::nullopt;
    return (negative ? -1.0 : 1.0) * double(*v) / 1e6;
}

// A v4 formula's token: how many values it takes (2, 1 or 0); nothing for one that is none.
std::optional<int> v4_token(std::string_view t)
{
    if (t == "+" || t == "-" || t == "*" || t == "/" || t == "^") return 2;
    if (t == "abs" || t == "conj" || t == "cos" || t == "exp" || t == "log" || t == "sin") return 1;
    if (t == "z" || t == "c" || t == "i" || v4_number(t, false)) return 0;
    return std::nullopt;
}

// Splits "WORD WORD ..." at single spaces: nothing where a word is empty.
std::optional<std::vector<std::string>> words_of(std::string_view s)
{
    std::vector<std::string> out;
    size_t from = 0;
    for (;;)
    {
        const size_t sp = s.find(' ', from);
        const std::string_view w = s.substr(from, sp == std::string_view::npos ? std::string_view::npos : sp - from);
        if (w.empty()) return std::nullopt;
        out.emplace_back(w);
        if (sp == std::string_view::npos) return out;
        from = sp + 1;
    }
}

// The lines after the version line of a v4 ruleset (sieve/fractal.hpp).
std::optional<Fractal> read_v4(const std::vector<std::string>& lines, const std::function<std::optional<Fractal>(const std::string&)>& bad)
{
    Fractal f;
    f.version = 4;
    if (lines.size() < 2) return bad("a v4 ruleset's second line is \"ifs\", \"escape FORMULA\" or \"converge FORMULA\"");
    if (lines[1] == "ifs")
    {
        f.family = Fractal::Family::Ifs;
        if (lines.size() < 3) return bad("an IFS has one map at least");
        for (size_t i = 2; i < lines.size(); ++i)
        {
            const std::string& l = lines[i];
            const auto w = l.rfind("map ", 0) == 0 ? words_of(std::string_view(l).substr(4)) : std::nullopt;
            if (!w || w->size() != 6) return bad("line " + std::to_string(i + 1) + " is not \"map A B C D E F\"");
            Fractal::Map m;
            for (size_t k = 0; k < 6; ++k)
            {
                const auto v = v4_number((*w)[k], true);
                if (!v) return bad("line " + std::to_string(i + 1) + ": a number is an optional -, 0 to 999 and up to 6 decimals with no 0 at the end (not -0)");
                m.text[k] = (*w)[k], m.value[k] = *v;
            }
            f.maps.push_back(m);
        }
        return f;
    }
    const bool escape = lines[1].rfind("escape ", 0) == 0, converge = lines[1].rfind("converge ", 0) == 0;
    if (!escape && !converge) return bad("a v4 ruleset's second line is \"ifs\", \"escape FORMULA\" or \"converge FORMULA\"");
    f.family = escape ? Fractal::Family::Escape : Fractal::Family::Converge;
    const auto tokens = words_of(std::string_view(lines[1]).substr(escape ? 7 : 9));
    if (!tokens) return bad("a formula's tokens are one space apart");
    uint32_t awaited = 1;
    for (size_t k = 0; k < tokens->size(); ++k)
    {
        const auto takes = v4_token((*tokens)[k]);
        if (!takes) return bad("\"" + (*tokens)[k] + "\" is not a token of a formula (+ - * / ^, abs conj cos exp log sin, z c i, or a number not 0)");
        if (awaited == 0) return bad("the formula is whole before its last token");
        awaited = awaited - 1 + uint32_t(*takes);
        if (awaited > kFractalAwaitedMost) return bad("a formula awaits " + std::to_string(kFractalAwaitedMost) + " values at most at once");
    }
    if (awaited != 0) return bad("the formula awaits " + std::to_string(awaited) + " more value(s)");
    f.formula = *tokens;
    size_t i = 2;
    if (i < lines.size() && lines[i].rfind("julia ", 0) == 0)
    {
        const auto w = words_of(std::string_view(lines[i]).substr(6));
        if (!w || w->size() != 2) return bad("line 3 is not \"julia RE IM\"");
        for (size_t k = 0; k < 2; ++k)
        {
            const auto v = v4_number((*w)[k], true);
            if (!v) return bad("julia's numbers are each an optional -, 0 to 999 and up to 6 decimals with no 0 at the end (not -0)");
            f.julia_text[k] = (*w)[k], f.julia_value[k] = *v;
        }
        f.julia = true, ++i;
    }
    if (i < lines.size() && lines[i].rfind("view ", 0) == 0)
    {
        const auto w = words_of(std::string_view(lines[i]).substr(5));
        if (!w || w->size() != 3) return bad("line " + std::to_string(i + 1) + " is not \"view X Y SPAN\"");
        for (size_t k = 0; k < 3; ++k)
        {
            const auto v = v4_number((*w)[k], k < 2);
            if (!v || (k == 2 && *v <= 0)) return bad("view's numbers are each an optional -, 0 to 999 and up to 6 decimals with no 0 at the end (not -0), its span more than 0");
            f.view_text[k] = (*w)[k], f.view_value[k] = *v;
        }
        f.view = true, ++i;
    }
    if (i != lines.size()) return bad("line " + std::to_string(i + 1) + " is not \"julia RE IM\" or \"view X Y SPAN\" in that order, each once");
    return f;
}

// A v4 formula's arithmetic, Sieve's own (list X4): only + - * / and sqrt, which IEEE 754 rounds the same
// on every machine, and floor, ldexp, frexp and fmod, which are exact. The system's complex numbers and
// its exp, log, sin, cos, pow and atan2 differ in their last bit between libraries (Microsoft's and GNU's,
// measured 16 November 2026: 1,057 charted pictures at 10x10 on one, another count on the other), and a
// point followed 4,096 times grows that bit until it escapes on one machine and not on the other; so a
// picture, and every count made of pictures, would be the machine's and not the ruleset's.
struct Cx
{
    double re = 0, im = 0;
};
Cx operator+(Cx a, Cx b) { return {a.re + b.re, a.im + b.im}; }
Cx operator-(Cx a, Cx b) { return {a.re - b.re, a.im - b.im}; }
Cx operator*(Cx a, Cx b)
{
    const double rr = a.re * b.re, ii = a.im * b.im, ri = a.re * b.im, ir = a.im * b.re;
    return {rr - ii, ri + ir};
}
Cx operator/(Cx a, Cx b)
{
    // (Smith's way: the smaller part of b divided by the larger, so neither square overflows)
    if (std::fabs(b.re) >= std::fabs(b.im))
    {
        const double r = b.im / b.re, d = b.re + b.im * r;
        return {(a.re + a.im * r) / d, (a.im - a.re * r) / d};
    }
    const double r = b.re / b.im, d = b.re * r + b.im;
    return {(a.re * r + a.im) / d, (a.im * r - a.re) / d};
}
double norm(Cx a) { return a.re * a.re + a.im * a.im; }

constexpr double kLn2Hi = 6.93147180369123816490e-01, kLn2Lo = 1.90821492927058770002e-10; // (ln 2 in two parts)
constexpr double kHalfPi1 = 1.57079632673412561417e+00, kHalfPi2 = 6.07710050650619224932e-11, kHalfPi3 = 2.02226624879595063154e-21;

double own_exp(double x)
{
    if (x != x) return x;
    if (x > 709.78) return HUGE_VAL;
    if (x < -745.2) return 0;
    const double k = std::floor(x / (kLn2Hi + kLn2Lo) + 0.5);
    const double r = (x - k * kLn2Hi) - k * kLn2Lo; // (|r| at most about ln 2 / 2)
    double s = 1;
    for (int t = 17; t >= 1; --t) s = 1 + s * r / t; // (Taylor's series to r^17, from the far end)
    return std::ldexp(s, int(k));
}
double own_log(double x)
{
    if (x != x || x < 0) return std::numeric_limits<double>::quiet_NaN();
    if (x == 0) return -HUGE_VAL;
    if (x == HUGE_VAL) return x;
    int e = 0;
    double m = std::frexp(x, &e); // (x = m 2^e, m in [0.5, 1))
    if (m < 0.70710678118654752) m *= 2, --e;
    const double s = (m - 1) / (m + 1), s2 = s * s; // (|s| at most 0.172: ln m = 2 (s + s^3/3 + ...))
    double sum = 0;
    for (int t = 25; t >= 1; t -= 2) sum = 1.0 / t + s2 * sum;
    return (double(e) * kLn2Hi + 2 * s * sum) + double(e) * kLn2Lo;
}
// sin and cos of x at once: x less a whole number k of quarter turns (pi/2 in three parts), then each by
// Taylor's series on what is left (at most pi/4), turned by k.
void own_sin_cos(double x, double& sn, double& cs)
{
    if (!std::isfinite(x)) { sn = cs = std::numeric_limits<double>::quiet_NaN(); return; }
    const double k = std::floor(x / (kHalfPi1 + kHalfPi2) + 0.5);
    const double r = ((x - k * kHalfPi1) - k * kHalfPi2) - k * kHalfPi3, r2 = r * r;
    double s = 0, c = 0;
    for (int t = 21; t >= 3; t -= 2) s = (1 - s) * r2 / (double(t) * double(t - 1)); // (1 - r^2/3! (1 - r^2/(4 5) ...))
    for (int t = 22; t >= 2; t -= 2) c = (1 - c) * r2 / (double(t) * double(t - 1));
    s = r * (1 - s), c = 1 - c;
    double q = std::fmod(k, 4.0);
    if (q < 0) q += 4;
    if (q == 0) sn = s, cs = c;
    else if (q == 1) sn = c, cs = -s;
    else if (q == 2) sn = -s, cs = -c;
    else sn = -c, cs = s;
}
double own_atan2(double y, double x)
{
    if (x != x || y != y) return std::numeric_limits<double>::quiet_NaN();
    if (y == 0 && x == 0) return std::signbit(x) ? std::copysign(2 * (kHalfPi1 + kHalfPi2), y) : y;
    const double ax = std::fabs(x), ay = std::fabs(y);
    const bool swap = ay > ax;
    double t = swap ? ax / ay : ay / ax; // (at most 1; NaN for two infinities, as the point is lost there)
    // (atan t = 2 atan(t / (1 + sqrt(1 + t^2))), twice: t at most 0.199, then its series to t^41)
    t = t / (1 + std::sqrt(1 + t * t));
    t = t / (1 + std::sqrt(1 + t * t));
    const double t2 = t * t;
    double sum = 0;
    for (int n = 41; n >= 1; n -= 2) sum = 1.0 / n - t2 * sum;
    double a = 4 * t * sum;
    const double half_pi = kHalfPi1 + kHalfPi2, pi = 2 * half_pi;
    if (swap) a = half_pi - a;
    if (x < 0) a = pi - a;
    return std::signbit(y) ? -a : a;
}
double own_sinh(double y)
{
    if (std::fabs(y) < 0.5)
    {
        // (its series, as e^y - e^-y would lose the small y)
        const double y2 = y * y;
        double s = 0;
        for (int t = 17; t >= 3; t -= 2) s = (1 + s) * y2 / (double(t) * double(t - 1));
        return y * (1 + s);
    }
    const double e = own_exp(y);
    return (e - 1 / e) / 2;
}
double own_cosh(double y)
{
    const double e = own_exp(y);
    return (e + 1 / e) / 2;
}
Cx cx_exp(Cx a)
{
    const double m = own_exp(a.re);
    if (a.im == 0) return {m, a.im};
    double s, c;
    own_sin_cos(a.im, s, c);
    return {m * c, m * s};
}
Cx cx_log(Cx a) { return {own_log(norm(a)) / 2, own_atan2(a.im, a.re)}; }
Cx cx_sin(Cx a)
{
    double s, c;
    own_sin_cos(a.re, s, c);
    return {s * own_cosh(a.im), c * own_sinh(a.im)};
}
Cx cx_cos(Cx a)
{
    double s, c;
    own_sin_cos(a.re, s, c);
    return {c * own_cosh(a.im), -(s * own_sinh(a.im))};
}

// A v4 formula made ready to follow a point: its tokens as operations, read from the right.
struct Program
{
    struct Op
    {
        char kind; // + - * / ^, a (abs) j (conj) C (cos) e (exp) l (log) s (sin), z c, n (a number)
        Cx value;
    };
    std::vector<Op> ops; // (in reverse, so a stack takes them as they come)
    explicit Program(const std::vector<std::string>& tokens)
    {
        for (size_t k = tokens.size(); k-- > 0;)
        {
            const std::string& t = tokens[k];
            Op o{t[0], {}};
            if (t == "abs") o.kind = 'a';
            else if (t == "conj") o.kind = 'j';
            else if (t == "cos") o.kind = 'C';
            else if (t == "exp") o.kind = 'e';
            else if (t == "log") o.kind = 'l';
            else if (t == "sin") o.kind = 's';
            else if (t == "i") o.kind = 'n', o.value = {0, 1};
            else if (t.size() > 1 || (t[0] >= '0' && t[0] <= '9')) o.kind = 'n', o.value = {*v4_number(t, false), 0};
            ops.push_back(o);
        }
    }
    static Cx power(Cx a, Cx b)
    {
        // (a whole exponent by multiplying, so z^2 is z z exactly; any other as exp(b log a), 0 to any
        // other 0 where b's real part is above 0, and not a number where it is not)
        if (b.im == 0 && b.re == std::floor(b.re) && std::fabs(b.re) <= 64)
        {
            int n = int(b.re);
            const bool inverse = n < 0;
            n = std::abs(n);
            Cx r{1, 0}, x = a;
            while (n)
            {
                if (n & 1) r = r * x;
                x = x * x, n >>= 1;
            }
            return inverse ? Cx{1, 0} / r : r;
        }
        if (a.re == 0 && a.im == 0) return b.re > 0 ? Cx{0, 0} : Cx{std::numeric_limits<double>::quiet_NaN(), 0};
        return cx_exp(b * cx_log(a));
    }
    Cx operator()(Cx z, Cx c) const
    {
        std::array<Cx, kFractalAwaitedMost + 1> st;
        size_t n = 0;
        for (const Op& o : ops)
            switch (o.kind)
            {
            case 'z': st[n++] = z; break;
            case 'c': st[n++] = c; break;
            case 'n': st[n++] = o.value; break;
            case 'a': st[n - 1] = {std::fabs(st[n - 1].re), std::fabs(st[n - 1].im)}; break;
            case 'j': st[n - 1].im = -st[n - 1].im; break;
            case 'C': st[n - 1] = cx_cos(st[n - 1]); break;
            case 'e': st[n - 1] = cx_exp(st[n - 1]); break;
            case 'l': st[n - 1] = cx_log(st[n - 1]); break;
            case 's': st[n - 1] = cx_sin(st[n - 1]); break;
            default:
            {
                // (the top the first, left, value)
                const Cx a = st[n - 1], b = st[n - 2];
                --n;
                st[n - 1] = o.kind == '+' ? a + b : o.kind == '-' ? a - b : o.kind == '*' ? a * b : o.kind == '/' ? a / b : power(a, b);
            }
            }
        return st[0];
    }
};

uint64_t mix64(uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// A v3 ruleset grown a step: each place's rule chosen among those that apply (sieve/fractal.hpp).
std::optional<std::string> grow_v3(const Fractal& f, const std::string& word, uint32_t step)
{
    std::array<std::vector<size_t>, 256> by{};
    bool contexts = false;
    for (size_t r = 0; r < f.rules.size(); ++r)
    {
        by[uint8_t(f.rules[r].first)].push_back(r);
        contexts = contexts || f.contexts[r].first || f.contexts[r].second;
    }
    std::array<bool, 256> ignored{};
    for (const char c : f.ignore) ignored[uint8_t(c)] = true;
    auto skipped = [&](char c) { return ignored[uint8_t(c)] || c == '[' || c == ']'; };
    // Each place's context: the symbol before it on its path (whole branches passed over, a branch's
    // start leading back to its parent) and the one after it (whole branches passed over, a branch's end
    // ending it).
    std::vector<char> before, after;
    if (contexts)
    {
        before.assign(word.size(), 0), after.assign(word.size(), 0);
        std::vector<char> kept;
        char last = 0;
        for (size_t i = 0; i < word.size(); ++i)
        {
            const char c = word[i];
            before[i] = last;
            if (c == '[') kept.push_back(last);
            else if (c == ']')
            {
                last = kept.empty() ? 0 : kept.back();
                if (!kept.empty()) kept.pop_back();
            }
            else if (!skipped(c)) last = c;
        }
        kept.clear();
        char next = 0;
        for (size_t i = word.size(); i-- > 0;)
        {
            const char c = word[i];
            after[i] = next;
            if (c == ']') kept.push_back(next), next = 0;
            else if (c == '[')
            {
                next = kept.empty() ? 0 : kept.back();
                if (!kept.empty()) kept.pop_back();
            }
            else if (!skipped(c)) next = c;
        }
    }
    std::string out;
    std::vector<size_t> fit, plain;
    for (size_t i = 0; i < word.size(); ++i)
    {
        const char c = word[i];
        const auto& rs = by[uint8_t(c)];
        if (rs.empty())
        {
            out += c;
            continue;
        }
        fit.clear(), plain.clear();
        for (const size_t r : rs)
        {
            const auto [l, rr] = f.contexts[r];
            if (!l && !rr) plain.push_back(r);
            else if ((!l || before[i] == l) && (!rr || after[i] == rr)) fit.push_back(r);
        }
        const auto& from = fit.empty() ? plain : fit;
        if (from.empty()) out += c;
        else
        {
            const size_t k = from.size() == 1 ? 0 : size_t(mix64(mix64(uint64_t(f.seed) * 0x100000001ull + step) ^ uint64_t(i)) % from.size());
            out += f.rules[from[k]].second;
        }
        if (out.size() > kFractalMost) return std::nullopt;
    }
    return out;
}

} // namespace

std::string Fractal::angle_written() const { return version == 3 ? angle_text : fractal_angle_text(angle_tenths); }

std::string Fractal::scale_written() const
{
    if (version == 3) return scale_ratio > 0 ? scale_text : std::string();
    return scale ? fractal_scale_text(scale) : std::string();
}

std::string fractal_angle_text(uint32_t tenths) { return std::to_string(tenths / 10) + (tenths % 10 ? "." + std::to_string(tenths % 10) : ""); }

std::string fractal_scale_text(uint32_t thousandths)
{
    std::string t = std::to_string(1000 + thousandths % 1000).substr(1);
    while (!t.empty() && t.back() == '0') t.pop_back();
    return "0." + t;
}

std::string Fractal::text() const
{
    if (version == 4)
    {
        std::string t = std::string(kFractalVersion4) + "\n";
        if (family == Family::Ifs)
        {
            t += "ifs\n";
            for (const Map& m : maps)
            {
                t += "map";
                for (const std::string& n : m.text) t += " " + n;
                t += "\n";
            }
            return t;
        }
        t += family == Family::Escape ? "escape" : "converge";
        for (const std::string& k : formula) t += " " + k;
        t += "\n";
        if (julia) t += "julia " + julia_text[0] + " " + julia_text[1] + "\n";
        if (view) t += "view " + view_text[0] + " " + view_text[1] + " " + view_text[2] + "\n";
        return t;
    }
    if (version == 3)
    {
        std::string t = std::string(kFractalVersion3) + "\nlsystem " + angle_text + "\n";
        if (scale_ratio > 0) t += "scale " + scale_text + "\n";
        if (seed) t += "seed " + std::to_string(seed) + "\n";
        if (!ignore.empty()) t += "ignore " + ignore + "\n";
        for (const Define& d : defines) t += std::string("define ") + d.digit + (d.turn ? " turn " : " scale ") + d.text + "\n";
        t += "axiom " + axiom + "\n";
        for (size_t r = 0; r < rules.size(); ++r)
        {
            t += "rule ";
            if (contexts[r].first) t += std::string(1, contexts[r].first) + "<";
            t += rules[r].first;
            if (contexts[r].second) t += std::string(">") + contexts[r].second;
            t += " " + rules[r].second + "\n";
        }
        return t;
    }
    std::string t = std::string(version == 2 ? kFractalVersion2 : kFractalVersion) + "\n";
    t += family == Family::Substitution ? std::string("substitution\n")
                                        : "lsystem " + fractal_angle_text(angle_tenths) + (scale ? " scale " + fractal_scale_text(scale) : std::string()) + "\n";
    t += "axiom " + axiom + "\n";
    for (const auto& [s, w] : rules) t += std::string("rule ") + s + " " + w + "\n";
    return t;
}

std::optional<std::string> Fractal::grow(uint32_t depth) const
{
    if (!rewrites()) return std::nullopt;
    if (version == 3)
    {
        std::string word = axiom;
        for (uint32_t d = 0; d < depth; ++d)
        {
            auto next = grow_v3(*this, word, d);
            if (!next) return std::nullopt;
            word = std::move(*next);
        }
        return word;
    }
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
    if (f.family == Fractal::Family::Ifs)
    {
        // (1 + n + n^2 + ... + n^d points)
        uint32_t d = 0;
        for (uint64_t level = 1, points = 1; d < kFractalDepthMost && points + level * f.maps.size() <= limit; ++d) level *= f.maps.size(), points += level;
        return d;
    }
    if (!f.rewrites())
    {
        // (a picture's points followed for fractal_iterations steps each: counted as 256 symbols a step)
        uint32_t d = 0;
        while (d < kFractalEscapeDeepest && uint64_t(fractal_iterations(d + 1)) * 256 <= limit) ++d;
        return d;
    }
    const auto& rules = f.rules;
    const auto& axiom = f.axiom;
    std::array<uint64_t, 256> size{};
    for (int c = 0; c < 256; ++c) size[size_t(c)] = 1;
    uint32_t d = 0;
    for (; d < kFractalDepthMost; ++d)
    {
        // (a symbol with several rules, v3's, as long as the longest of them, or as itself where its
        // rules have contexts and none may apply)
        std::array<uint64_t, 256> next = size;
        std::array<bool, 256> met{};
        for (size_t r = 0; r < rules.size(); ++r)
        {
            const auto& [s, w] = rules[r];
            uint64_t n = 0;
            for (const char c : w) n = std::min<uint64_t>(n + size[uint8_t(c)], limit + 1);
            const bool context = r < f.contexts.size() && (f.contexts[r].first || f.contexts[r].second);
            uint64_t& to = next[uint8_t(s)];
            to = met[uint8_t(s)] ? std::max(to, n) : context ? std::max(size[uint8_t(s)], n) : n;
            met[uint8_t(s)] = true;
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
    if (!lines.empty() && lines[0] == kFractalVersion3) return read_v3(lines, bad);
    if (!lines.empty() && lines[0] == kFractalVersion4) return read_v4(lines, bad);
    if (lines.size() < 4 || (lines[0] != kFractalVersion && lines[0] != kFractalVersion2))
        return bad(std::string("its first line is not ") + kFractalVersion + ", " + kFractalVersion2 + ", " + kFractalVersion3 + " or " + kFractalVersion4 + ", or it has no axiom and rule");
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
    constexpr double kPi = 3.14159265358979323846;
    if (f.version != 3)
    {
        // v1 and v2: in the plane, the heading an angle (so their drawings are as they always were).
        const double turn = double(f.angle_tenths) * kPi / 1800.0, ratio = double(f.scale) / 1000.0;
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
            else if (c == '|') a += kPi;
            // (held within 10^-60 and 10^60, so a word of a million ' still draws: a bound chosen)
            else if (c == '"' && ratio > 0) step = std::max(step * ratio, 1e-60);
            else if (c == '\'' && ratio > 0) step = std::min(step / ratio, 1e60);
            else if (c == '[') kept.push_back({x, y, a, step});
            else if (c == ']' && !kept.empty()) x = kept.back()[0], y = kept.back()[1], a = kept.back()[2], step = kept.back()[3], kept.pop_back();
        }
        return out;
    }
    const double turn = f.turn_degrees() * kPi / 180.0, ratio = f.scale_ratio;
    // v3's digits, each a turn (radians) or a factor.
    std::array<double, 10> digit_value{};
    std::array<int, 10> digit_kind{}; // 0 none, 1 turn, 2 scale
    for (const auto& d : f.defines) digit_value[size_t(d.digit - '0')] = d.turn ? d.value * kPi / 180.0 : d.value, digit_kind[size_t(d.digit - '0')] = d.turn ? 1 : 2;
    // The turtle in three dimensions: its place, its heading H, left L and up U (v1 and v2 stay in the
    // plane of H and L, and are drawn as before); the step's length; whether + and - are swapped.
    using V = std::array<double, 3>;
    struct Turtle
    {
        V p{0, 0, 0}, h{1, 0, 0}, l{0, 1, 0}, u{0, 0, 1};
        double step = 1;
        bool swapped = false;
    } t;
    auto turn_in = [](V& a, V& b, double angle) { // a towards b by angle
        const double c = std::cos(angle), s = std::sin(angle);
        const V na{a[0] * c + b[0] * s, a[1] * c + b[1] * s, a[2] * c + b[2] * s}, nb{b[0] * c - a[0] * s, b[1] * c - a[1] * s, b[2] * c - a[2] * s};
        a = na, b = nb;
    };
    std::vector<Turtle> kept;
    std::vector<std::array<double, 6>> lines3;
    bool solid = false; // (whether it left the plane)
    for (const char c : word)
    {
        if (c == 'F' || c == 'G' || c == 'f')
        {
            const V np{t.p[0] + t.step * t.h[0], t.p[1] + t.step * t.h[1], t.p[2] + t.step * t.h[2]};
            if (c != 'f') lines3.push_back({t.p[0], t.p[1], t.p[2], np[0], np[1], np[2]});
            t.p = np;
        }
        else if (c == '+') turn_in(t.h, t.l, t.swapped ? -turn : turn);
        else if (c == '-') turn_in(t.h, t.l, t.swapped ? turn : -turn);
        else if (c == '|') turn_in(t.h, t.l, kPi);
        else if (c == '!') t.swapped = !t.swapped;
        else if (c == '&') turn_in(t.h, t.u, -turn), solid = true; // pitch down
        else if (c == '^') turn_in(t.h, t.u, turn), solid = true;  // pitch up
        else if (c == '\\') turn_in(t.l, t.u, turn), solid = true; // roll left
        else if (c == '/') turn_in(t.l, t.u, -turn), solid = true; // roll right
        // (held within 10^-60 and 10^60, so a word of a million ' still draws: a bound chosen)
        else if (c == '"' && ratio > 0) t.step = std::max(t.step * ratio, 1e-60);
        else if (c == '\'' && ratio > 0) t.step = std::min(t.step / ratio, 1e60);
        else if (c >= '0' && c <= '9')
        {
            const size_t k = size_t(c - '0');
            if (digit_kind[k] == 1) turn_in(t.h, t.l, digit_value[k]);
            else if (digit_kind[k] == 2) t.step = std::clamp(t.step * digit_value[k], 1e-60, 1e60);
        }
        else if (c == '[') kept.push_back(t);
        else if (c == ']' && !kept.empty()) t = kept.back(), kept.pop_back();
    }
    // Drawn in the plane as it is; out of it, seen from a fixed side (turned 30 degrees about the first
    // heading, then 20 about the first up), the same on every machine.
    const double cx = std::cos(30 * kPi / 180), sx = std::sin(30 * kPi / 180), cz = std::cos(20 * kPi / 180), sz = std::sin(20 * kPi / 180);
    auto seen = [&](double x, double y, double z) -> std::array<double, 2> {
        if (!solid) return {x, y};
        const double y1 = y * cx - z * sx, z1 = y * sx + z * cx;
        return {x * cz + z1 * sz, y1};
    };
    out.reserve(lines3.size());
    for (const auto& l : lines3)
    {
        const auto a = seen(l[0], l[1], l[2]), b = seen(l[3], l[4], l[5]);
        out.push_back({a[0], a[1], b[0], b[1]});
    }
    return out;
}

std::optional<std::vector<std::array<double, 3>>> fractal_points(const Fractal& f, uint32_t depth)
{
    if (f.family != Fractal::Family::Ifs || depth > fractal_deepest_within(f, kFractalMost)) return std::nullopt;
    // The first map's fixed point (on the figure, where the map shrinks), or (0, 0) where it has none;
    // then level by level, each map applied to every point of the level before (the last applied the
    // outermost), every level kept.
    const auto& m0 = f.maps[0].value;
    const double det = (1 - m0[0]) * (1 - m0[3]) - m0[1] * m0[2];
    std::array<double, 3> start{0, 0, 0};
    if (std::abs(det) > 1e-12) start = {((1 - m0[3]) * m0[4] + m0[1] * m0[5]) / det, (m0[2] * m0[4] + (1 - m0[0]) * m0[5]) / det, 0};
    std::vector<std::array<double, 3>> all{start}, level{start}, next;
    for (uint32_t d = 0; d < depth; ++d)
    {
        next.clear();
        next.reserve(level.size() * f.maps.size());
        for (size_t k = 0; k < f.maps.size(); ++k)
        {
            const auto& m = f.maps[k].value;
            for (const auto& q : level) next.push_back({m[0] * q[0] + m[1] * q[1] + m[4], m[2] * q[0] + m[3] * q[1] + m[5], double(k)});
        }
        level.swap(next);
        all.insert(all.end(), level.begin(), level.end());
    }
    return all;
}

namespace {

// A point's place on the picture of an escape or converge ruleset: centre and span from its view (y up).
Cx view_point(const Fractal& f, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    const double unit = f.view_value[2] / double(std::min(width, height));
    return {f.view_value[0] + (double(x) + 0.5 - double(width) / 2) * unit, f.view_value[1] - (double(y) + 0.5 - double(height) / 2) * unit};
}

// Follows one point: the steps it took to escape (or settle), and where z ended; steps == the most for
// one that did neither.
std::pair<uint32_t, Cx> follow(const Fractal& f, const Program& prog, Cx point, uint32_t most)
{
    Cx z = f.julia ? point : Cx{0, 0};
    const Cx c = f.julia ? Cx{f.julia_value[0], f.julia_value[1]} : point;
    const bool escape = f.family == Fractal::Family::Escape;
    for (uint32_t n = 0; n < most; ++n)
    {
        const Cx next = prog(z, c);
        if (!std::isfinite(next.re) || !std::isfinite(next.im)) return {escape ? n : most, z};
        if (escape ? norm(next) > kFractalEscapeRadius * kFractalEscapeRadius : norm(next - z) < kFractalSettled * kFractalSettled) return {n, next};
        z = next;
    }
    return {most, z};
}

} // namespace

std::optional<RgbaImage> draw_fractal(const Fractal& f, uint32_t depth, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > 8192 || height > 8192) return std::nullopt;
    RgbaImage img = blank(width, height);
    if (f.family == Fractal::Family::Escape || f.family == Fractal::Family::Converge)
    {
        // Each pixel's point followed: an escaped one coloured by how soon it escaped, round the wheel; a
        // settled one by where it settled (the angle of z, so each root its hue), darker the longer it took;
        // the rest the ground.
        if (depth > fractal_deepest_within(f, kFractalMost)) return std::nullopt;
        const Program prog(f.formula);
        const uint32_t most = fractal_iterations(depth);
        for (uint32_t y = 0; y < height; ++y)
            for (uint32_t x = 0; x < width; ++x)
            {
                const auto [n, z] = follow(f, prog, view_point(f, x, y, width, height), most);
                if (n >= most) continue;
                if (f.family == Fractal::Family::Escape) put(img, int(x), int(y), hue(n % 48, 48));
                else
                {
                    const double turn = own_atan2(z.im, z.re) / (2 * 3.141592653589793) + 0.5;
                    auto col = hue(size_t(std::min(0.999999, std::max(0.0, turn)) * 360), 360);
                    const double shade = 1.0 / (1.0 + double(n) / 16.0);
                    for (auto& ch : col) ch = uint8_t(double(kGround[0]) + (double(ch) - double(kGround[0])) * shade);
                    put(img, int(x), int(y), col);
                }
            }
        return img;
    }
    if (f.family == Fractal::Family::Ifs)
    {
        // The points fitted to the picture as a path is, each coloured by the map that placed it.
        const auto pts = fractal_points(f, depth);
        if (!pts) return std::nullopt;
        double lx = (*pts)[0][0], hx = lx, ly = (*pts)[0][1], hy = ly;
        for (const auto& q : *pts)
            if (std::isfinite(q[0]) && std::isfinite(q[1])) lx = std::min(lx, q[0]), hx = std::max(hx, q[0]), ly = std::min(ly, q[1]), hy = std::max(hy, q[1]);
        const double margin = 0.05, span = std::max(hx - lx, hy - ly);
        const double s = (1 - 2 * margin) * double(std::min(width, height)) / std::max(span, 1e-9);
        const double ox = (double(width) - (hx - lx) * s) / 2, oy = (double(height) - (hy - ly) * s) / 2;
        for (const auto& q : *pts)
            if (std::isfinite(q[0]) && std::isfinite(q[1]))
                put(img, int(std::lround(ox + (q[0] - lx) * s)), int(std::lround(double(height) - oy - (q[1] - ly) * s)), hue(size_t(q[2]), f.maps.size()));
        return img;
    }
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
    if (f.family == Fractal::Family::Ifs)
    {
        const uint32_t depth = fractal_deepest_within(f, kFractalDimensionPoints);
        const auto pts = fractal_points(f, depth);
        if (!pts || pts->size() < 16) return std::nullopt;
        double lx = (*pts)[0][0], hx = lx, ly = (*pts)[0][1], hy = ly;
        for (const auto& q : *pts) lx = std::min(lx, q[0]), hx = std::max(hx, q[0]), ly = std::min(ly, q[1]), hy = std::max(hy, q[1]);
        const double span = std::max(hx - lx, hy - ly) * (1 + 1e-9);
        if (!(span > 0) || !std::isfinite(span)) return std::nullopt;
        std::vector<double> xs, ys;
        std::vector<uint8_t> cells;
        for (uint32_t n = 4; n <= 1024; n *= 2)
        {
            const double cell = span / n;
            cells.assign(size_t(n) * n, 0);
            size_t count = 0;
            for (const auto& q : *pts)
            {
                const size_t cx = std::min<size_t>(n - 1, size_t(std::max(0.0, (q[0] - lx) / cell))), cy = std::min<size_t>(n - 1, size_t(std::max(0.0, (q[1] - ly) / cell)));
                uint8_t& c = cells[cy * n + cx];
                count += c == 0, c = 1;
            }
            // (past a quarter of the points, the cells count the points more than the figure)
            if (count * 4 > pts->size()) break;
            xs.push_back(std::log(double(n))), ys.push_back(std::log(double(count)));
        }
        if (xs.size() < 3) return std::nullopt;
        double mx = 0, my = 0;
        for (size_t k = 0; k < xs.size(); ++k) mx += xs[k], my += ys[k];
        mx /= double(xs.size()), my /= double(xs.size());
        double num = 0, den = 0;
        for (size_t k = 0; k < xs.size(); ++k) num += (xs[k] - mx) * (ys[k] - my), den += (xs[k] - mx) * (xs[k] - mx);
        return FractalDimension{num / den, depth, uint32_t(xs.size())};
    }
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
