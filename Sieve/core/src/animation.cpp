// Sieve — the animation line (see animation.hpp).
#include "sieve/animation.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve {

namespace {

constexpr double kPi = 3.14159265358979323846;

uint8_t step_of(double c)
{
    const double d = std::round(c * 128.0 / kPi) + 128.0;
    return uint8_t(std::clamp(d, 0.0, 255.0));
}

double angle_of(uint8_t d) { return (double(d) - 128.0) * kPi / 128.0; }

int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

} // namespace

BoneTurn turn_of_quaternion(double x, double y, double z, double w)
{
    const double n = std::sqrt(x * x + y * y + z * z + w * w);
    if (n == 0) return {};
    x /= n, y /= n, z /= n, w /= n;
    if (w < 0) x = -x, y = -y, z = -z, w = -w; // the shorter way round
    const double s = std::sqrt(x * x + y * y + z * z);
    if (s < 1e-12) return {};
    const double angle = 2.0 * std::atan2(s, w); // 0 to pi
    return {step_of(x / s * angle), step_of(y / s * angle), step_of(z / s * angle)};
}

void turn_matrix(const BoneTurn& t, double m[9])
{
    const double vx = angle_of(t.x), vy = angle_of(t.y), vz = angle_of(t.z);
    const double angle = std::sqrt(vx * vx + vy * vy + vz * vz);
    if (angle < 1e-12)
    {
        for (int i = 0; i < 9; ++i) m[i] = i % 4 == 0 ? 1 : 0;
        return;
    }
    const double x = vx / angle, y = vy / angle, z = vz / angle, c = std::cos(angle), s = std::sin(angle), k = 1 - c;
    m[0] = c + x * x * k, m[1] = x * y * k - z * s, m[2] = x * z * k + y * s;
    m[3] = y * x * k + z * s, m[4] = c + y * y * k, m[5] = y * z * k - x * s;
    m[6] = z * x * k - y * s, m[7] = z * y * k + x * s, m[8] = c + z * z * k;
}

std::string Animation::text() const
{
    std::string t = std::string(kAnimationVersion) + " bones=" + std::to_string(bones) + " frames=" + std::to_string(frames) + " rate=" +
                    std::to_string(kAnimationRate) + "\n";
    static const char* kHex = "0123456789abcdef";
    for (uint32_t f = 0; f < frames; ++f)
    {
        t += "f " + std::to_string(f);
        for (uint32_t b = 0; b < bones; ++b)
        {
            const BoneTurn& u = at(f, b);
            t += ' ';
            for (const uint8_t d : {u.x, u.y, u.z}) t += kHex[d >> 4], t += kHex[d & 15];
        }
        t += '\n';
    }
    return t;
}

std::string Animation::id() const { return Sha256::hex(Sha256::hash(text())); }

std::string Animation::address() const
{
    static const char* kHex = "0123456789abcdef";
    std::string a;
    a.reserve(turns.size() * 6);
    for (const BoneTurn& u : turns)
        for (const uint8_t d : {u.x, u.y, u.z}) a += kHex[d >> 4], a += kHex[d & 15];
    return a;
}

Animation read_animation(const std::string& text)
{
    std::vector<std::string> lines;
    for (size_t i = 0; i < text.size();)
    {
        size_t e = text.find('\n', i);
        if (e == std::string::npos) e = text.size();
        std::string l = text.substr(i, e - i);
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (!l.empty()) lines.push_back(l);
        i = e + 1;
    }
    if (lines.empty() || lines[0].rfind(std::string(kAnimationVersion) + " ", 0) != 0)
        throw std::invalid_argument(std::string("not an animation: it does not start with ") + kAnimationVersion);
    Animation a;
    bool seen[3] = {};
    {
        std::string rest = lines[0].substr(std::string(kAnimationVersion).size());
        size_t i = 0;
        while (i < rest.size())
        {
            while (i < rest.size() && rest[i] == ' ') ++i;
            if (i >= rest.size()) break;
            size_t e = rest.find(' ', i);
            if (e == std::string::npos) e = rest.size();
            const std::string w = rest.substr(i, e - i);
            i = e;
            const size_t eq = w.find('=');
            if (eq == std::string::npos) throw std::invalid_argument("not a setting: " + w);
            const std::string k = w.substr(0, eq), v = w.substr(eq + 1);
            if (v.empty() || v.size() > 6 || v.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument(k + ": not a number: " + v);
            const uint32_t n = uint32_t(std::stoul(v));
            if (k == "bones") a.bones = n, seen[0] = true;
            else if (k == "frames") a.frames = n, seen[1] = true;
            else if (k == "rate")
            {
                if (n != kAnimationRate) throw std::invalid_argument("rate: " + v + " (this version is " + std::to_string(kAnimationRate) + " frames a second)");
                seen[2] = true;
            }
            else throw std::invalid_argument("not a setting of " + std::string(kAnimationVersion) + ": " + k);
        }
    }
    if (!seen[0] || !seen[1] || !seen[2]) throw std::invalid_argument("missing: bones, frames or rate");
    if (a.bones < 1 || a.bones > kAnimationMaxBones) throw std::invalid_argument("bones: " + std::to_string(a.bones) + " (1 to " + std::to_string(kAnimationMaxBones) + ")");
    if (a.frames < 1 || a.frames > kAnimationMaxFrames) throw std::invalid_argument("frames: " + std::to_string(a.frames) + " (1 to " + std::to_string(kAnimationMaxFrames) + ")");
    if (lines.size() != size_t(a.frames) + 1) throw std::invalid_argument("it has " + std::to_string(lines.size() - 1) + " frames' lines, not " + std::to_string(a.frames));
    a.turns.reserve(size_t(a.frames) * a.bones);
    for (uint32_t f = 0; f < a.frames; ++f)
    {
        const std::string& l = lines[f + 1];
        const std::string head = "f " + std::to_string(f);
        if (l.rfind(head, 0) != 0) throw std::invalid_argument("frame " + std::to_string(f) + "'s line is not where it should be");
        const std::string groups = l.substr(head.size());
        if (groups.size() != size_t(a.bones) * 7) throw std::invalid_argument("frame " + std::to_string(f) + " does not hold " + std::to_string(a.bones) + " turns");
        for (uint32_t b = 0; b < a.bones; ++b)
        {
            const size_t at = size_t(b) * 7;
            if (groups[at] != ' ') throw std::invalid_argument("frame " + std::to_string(f) + ": turns are separated by one space");
            uint8_t d[3];
            for (int k = 0; k < 3; ++k)
            {
                const int hi = hexval(groups[at + 1 + 2 * size_t(k)]), lo = hexval(groups[at + 2 + 2 * size_t(k)]);
                if (hi < 0 || lo < 0) throw std::invalid_argument("frame " + std::to_string(f) + ": a turn is not six lower-case hex digits");
                d[k] = uint8_t(hi * 16 + lo);
            }
            a.turns.push_back({d[0], d[1], d[2]});
        }
    }
    return a;
}

std::vector<std::array<double, 3>> pose(const Character& c, const Animation& a, uint32_t frame)
{
    std::vector<std::array<double, 3>> p(c.spheres.size());
    if (c.spheres.empty()) return p;
    std::vector<std::array<double, 9>> turn(c.spheres.size());
    const uint32_t f = a.frames ? frame % a.frames : 0;
    for (size_t i = 0; i < c.spheres.size(); ++i)
    {
        const Sphere& s = c.spheres[i];
        if (i < a.bones && a.frames) turn_matrix(a.at(f, uint32_t(i)), turn[i].data());
        else if (s.parent >= 0) turn[i] = turn[size_t(s.parent)];
        else
        {
            turn[i] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        }
        if (s.parent < 0 || size_t(s.parent) >= i)
        {
            p[i] = {double(s.x), double(s.y), double(s.z)};
            continue;
        }
        const Sphere& q = c.spheres[size_t(s.parent)];
        const double ox = s.x - q.x, oy = s.y - q.y, oz = s.z - q.z;
        const auto& m = turn[size_t(s.parent)];
        const auto& pp = p[size_t(s.parent)];
        p[i] = {pp[0] + m[0] * ox + m[1] * oy + m[2] * oz, pp[1] + m[3] * ox + m[4] * oy + m[5] * oz, pp[2] + m[6] * ox + m[7] * oy + m[8] * oz};
    }
    return p;
}

} // namespace sieve
