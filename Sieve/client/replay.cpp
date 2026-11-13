// Sieve hallway — recording and replay (see replay.hpp).
#include "replay.hpp"

#include <cstdio>
#include <deque>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace hallway {

namespace {

std::string g(double v)
{
    char b[40];
    std::snprintf(b, sizeof b, "%.9g", v);
    return b;
}

std::vector<std::string> split(const std::string& s, char by)
{
    std::vector<std::string> out;
    size_t i = 0;
    for (;;)
    {
        const size_t e = s.find(by, i);
        out.push_back(s.substr(i, e == std::string::npos ? std::string::npos : e - i));
        if (e == std::string::npos) return out;
        i = e + 1;
    }
}

// A whole number of at most `most`, written as it is (no sign, no leading zero but 0 itself).
bool whole(const std::string& s, uint64_t most, uint64_t& v)
{
    if (s.empty() || s.size() > 20 || s.find_first_not_of("0123456789") != std::string::npos || (s.size() > 1 && s[0] == '0')) return false;
    v = std::strtoull(s.c_str(), nullptr, 10);
    return v <= most;
}

bool number(const std::string& s, float& v)
{
    if (s.empty() || s.size() > 40) return false;
    char* end = nullptr;
    v = std::strtof(s.c_str(), &end);
    return end && *end == '\0';
}

int hexval(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }

} // namespace

std::string replay_token(const SDL_Event& e)
{
    switch (e.type)
    {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return std::string(e.type == SDL_EVENT_KEY_DOWN ? "kd:" : "ku:") + std::to_string(int(e.key.scancode)) + ":" + std::to_string(uint32_t(e.key.key)) + ":" +
               std::to_string(unsigned(e.key.mod)) + ":" + (e.key.repeat ? "1" : "0");
    case SDL_EVENT_TEXT_INPUT:
    {
        static const char* kHex = "0123456789abcdef";
        std::string t = "tx:";
        for (const char* p = e.text.text; p && *p; ++p) t += kHex[(uint8_t(*p) >> 4) & 15], t += kHex[uint8_t(*p) & 15];
        return t == "tx:" ? std::string() : t;
    }
    case SDL_EVENT_MOUSE_MOTION: return "mm:" + g(e.motion.x) + ":" + g(e.motion.y) + ":" + g(e.motion.xrel) + ":" + g(e.motion.yrel);
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return std::string(e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? "bd:" : "bu:") + std::to_string(int(e.button.button)) + ":" + std::to_string(int(e.button.clicks)) + ":" +
               g(e.button.x) + ":" + g(e.button.y);
    case SDL_EVENT_MOUSE_WHEEL: return "mw:" + g(e.wheel.x) + ":" + g(e.wheel.y);
    default: return "";
    }
}

std::string Replay::text() const
{
    std::string t = std::string(kReplayVersion) + "\n";
    t += "start line=" + line + " tile=" + tile + " x=" + g(x) + " y=" + g(y) + " z=" + g(z) + " yaw=" + g(yaw) + " pitch=" + g(pitch) + "\n";
    for (const auto& f : frames) t += replay_frame_line(f);
    return t;
}

std::string replay_frame_line(const ReplayFrame& f)
{
    std::string t = "f " + std::to_string(f.dt_us) + " ";
    if (f.held.empty()) t += "-";
    for (size_t i = 0; i < f.held.size(); ++i) t += (i ? "," : "") + std::to_string(f.held[i]);
    for (const auto& e : f.events)
        if (const std::string k = replay_token(e); !k.empty()) t += " " + k;
    return t + "\n";
}

double Replay::seconds() const
{
    double s = 0;
    for (const auto& f : frames) s += double(f.dt_us) / 1e6;
    return s;
}

Replay read_replay(const std::string& text)
{
    std::vector<std::string> lines;
    for (size_t i = 0; i < text.size();)
    {
        const size_t e = text.find('\n', i);
        if (e == std::string::npos) throw std::invalid_argument("its last line has no line feed");
        std::string l = text.substr(i, e - i);
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines.push_back(l);
        i = e + 1;
    }
    if (lines.size() < 2 || lines[0] != kReplayVersion) throw std::invalid_argument(std::string("not a recording: it does not start with ") + kReplayVersion);
    Replay r;
    {
        const auto f = split(lines[1], ' ');
        const char* keys[] = {"start", "line=", "tile=", "x=", "y=", "z=", "yaw=", "pitch="};
        if (f.size() != 8 || f[0] != keys[0]) throw std::invalid_argument("line 2 is not its start (start line= tile= x= y= z= yaw= pitch=)");
        for (size_t k = 1; k < 8; ++k)
            if (f[k].rfind(keys[k], 0) != 0) throw std::invalid_argument(std::string("line 2: field ") + std::to_string(k + 1) + " is not " + keys[k]);
        r.line = f[1].substr(5);
        r.tile = f[2].substr(5);
        float* v[] = {&r.x, &r.y, &r.z, &r.yaw, &r.pitch};
        for (size_t k = 3; k < 8; ++k)
            if (!number(f[k].substr(std::string(keys[k]).size()), *v[k - 3])) throw std::invalid_argument(std::string("line 2: ") + keys[k] + " is not a number");
    }
    for (size_t li = 2; li < lines.size(); ++li)
    {
        const std::string where = "line " + std::to_string(li + 1) + ": ";
        const auto f = split(lines[li], ' ');
        if (f.size() < 3 || f[0] != "f") throw std::invalid_argument(where + "not a frame (f DT HELD ...)");
        ReplayFrame fr;
        uint64_t n = 0;
        if (!whole(f[1], kReplayMaxDtUs, n) || n == 0) throw std::invalid_argument(where + "its time is not 1 to 100000 microseconds");
        fr.dt_us = uint32_t(n);
        if (f[2] != "-")
            for (const auto& h : split(f[2], ','))
            {
                if (!whole(h, SDL_SCANCODE_COUNT - 1, n) || (!fr.held.empty() && n <= fr.held.back())) throw std::invalid_argument(where + "its keys held are not scancodes in increasing order");
                fr.held.push_back(uint16_t(n));
            }
        for (size_t k = 3; k < f.size(); ++k)
        {
            const auto p = split(f[k], ':');
            SDL_Event e{};
            const std::string& kind = p[0];
            auto bad = [&] { return std::invalid_argument(where + "event " + std::to_string(k - 2) + " is not one of kd ku tx mm bd bu mw, written as they are"); };
            uint64_t a = 0, b = 0, c = 0;
            if ((kind == "kd" || kind == "ku") && p.size() == 5)
            {
                if (!whole(p[1], SDL_SCANCODE_COUNT - 1, a) || !whole(p[2], 0xFFFFFFFFu, b) || !whole(p[3], 0xFFFF, c) || (p[4] != "0" && p[4] != "1")) throw bad();
                e.type = kind == "kd" ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
                e.key.scancode = SDL_Scancode(a);
                e.key.key = SDL_Keycode(b);
                e.key.mod = SDL_Keymod(c);
                e.key.repeat = p[4] == "1";
                e.key.down = kind == "kd";
            }
            else if (kind == "tx" && p.size() == 2)
            {
                // Its bytes kept with the recording (SDL's own text events point at text it keeps).
                static thread_local std::deque<std::string> kept; // (a deque: what it holds never moves)
                std::string t;
                if (p[1].empty() || p[1].size() % 2 || p[1].size() > 64) throw bad();
                for (size_t i = 0; i < p[1].size(); i += 2)
                {
                    const int hi = hexval(p[1][i]), lo = hexval(p[1][i + 1]);
                    if (hi < 0 || lo < 0) throw bad();
                    t += char(hi * 16 + lo);
                }
                kept.push_back(t);
                e.type = SDL_EVENT_TEXT_INPUT;
                e.text.text = kept.back().c_str();
            }
            else if (kind == "mm" && p.size() == 5)
            {
                e.type = SDL_EVENT_MOUSE_MOTION;
                if (!number(p[1], e.motion.x) || !number(p[2], e.motion.y) || !number(p[3], e.motion.xrel) || !number(p[4], e.motion.yrel)) throw bad();
            }
            else if ((kind == "bd" || kind == "bu") && p.size() == 5)
            {
                if (!whole(p[1], 255, a) || !whole(p[2], 255, b)) throw bad();
                e.type = kind == "bd" ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
                e.button.button = Uint8(a);
                e.button.clicks = Uint8(b);
                e.button.down = kind == "bd";
                if (!number(p[3], e.button.x) || !number(p[4], e.button.y)) throw bad();
            }
            else if (kind == "mw" && p.size() == 3)
            {
                e.type = SDL_EVENT_MOUSE_WHEEL;
                if (!number(p[1], e.wheel.x) || !number(p[2], e.wheel.y)) throw bad();
            }
            else throw bad();
            fr.events.push_back(e);
        }
        r.frames.push_back(std::move(fr));
    }
    if (r.text() != text) throw std::invalid_argument("not in its canonical form (numbers written as %.9g writes them, one SPACE between fields)");
    return r;
}

std::vector<uint16_t> held_keys(const bool* keys)
{
    std::vector<uint16_t> out;
    for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
        if (keys[i]) out.push_back(uint16_t(i));
    return out;
}

std::array<bool, SDL_SCANCODE_COUNT> keyboard_of(const std::vector<uint16_t>& held)
{
    std::array<bool, SDL_SCANCODE_COUNT> k{};
    for (const uint16_t s : held) k[s] = true;
    return k;
}

} // namespace hallway
