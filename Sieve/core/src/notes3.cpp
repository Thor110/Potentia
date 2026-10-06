#include "sieve/notes3.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <stdexcept>

namespace sieve {

namespace {

constexpr const char* kSharp[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
constexpr uint32_t kCodeSixteenths[8] = {1, 2, 3, 4, 6, 8, 12, 16};                       // s e E q Q h H w
constexpr const char* kCodeNotation[8] = {"s", "e", "e.", "q", "q.", "h", "h.", "w"};

int letter_semitone(char c)
{
    switch (c)
    {
    case 'C': return 0;
    case 'D': return 2;
    case 'E': return 4;
    case 'F': return 5;
    case 'G': return 7;
    case 'A': return 9;
    case 'B': return 11;
    default: return -1;
    }
}

uint32_t parse_u32(const std::string& s, const char* what)
{
    if (s.empty() || s.size() > 9 || !std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; }))
        throw std::invalid_argument(std::string("bad ") + what + " in a notes3 id");
    return uint32_t(std::stoul(s));
}

// MIDI channel of voice v (0-based): 0..8, then 10..15 (channel 10, the drums, is left out).
uint32_t channel_of(uint32_t v) { return v < 9 ? v : v + 1; }

} // namespace

uint32_t notes3_midi_of(const std::string& name)
{
    auto bad = [&] { return std::invalid_argument("cannot read the note '" + name + "' (examples: C-1, F#5, G9)"); };
    if (name.size() < 2) throw bad();
    int semitone = letter_semitone(char(std::toupper(static_cast<unsigned char>(name[0]))));
    if (semitone < 0) throw bad();
    size_t k = 1;
    if (name[k] == '#') { ++semitone; ++k; }
    else if (name[k] == 'b') { --semitone; ++k; }
    bool minus = k < name.size() && name[k] == '-';
    if (minus) ++k;
    if (k + 1 != name.size() || !std::isdigit(static_cast<unsigned char>(name[k]))) throw bad();
    const int octave = minus ? -(name[k] - '0') : name[k] - '0';
    const int midi = (octave + 1) * 12 + semitone;
    if (midi < 0 || midi > 127) throw bad();
    return uint32_t(midi);
}

std::string notes3_name(uint32_t midi) { return std::string(kSharp[midi % 12]) + std::to_string(int(midi / 12) - 1); }

uint32_t Notes3Set::velocity(uint32_t k) const { return (127 * k + levels / 2) / levels; }

uint32_t Notes3Set::default_level() const
{
    uint32_t best = 1, gap = ~0u;
    for (uint32_t k = 1; k <= levels; ++k)
    {
        const uint32_t v = velocity(k), g = v > 96 ? v - 96 : 96 - v;
        if (g <= gap) // ties to the louder (the later)
        {
            best = k;
            gap = g;
        }
    }
    return best;
}

uint32_t Notes3Set::digit(bool is_rest, uint32_t m, uint32_t k, uint32_t t) const
{
    const uint32_t cls = is_rest ? 0 : 1 + (m - low) * levels + (k - 1);
    return (t - 1) + longest * cls;
}

std::string Notes3Set::id() const
{
    std::string ins;
    for (size_t i = 0; i < instruments.size(); ++i) ins += (i ? "," : "") + std::to_string(instruments[i]);
    return "notes3/" + notes3_name(low) + ".." + notes3_name(high) + "/q" + std::to_string(tpq) + "/d" + std::to_string(longest) + "/v" +
           std::to_string(levels) + "/V" + std::to_string(voices) + "/t" + std::to_string(tempo) + "/i" + ins;
}

Notes3Set make_notes3_set(uint32_t low, uint32_t high, uint32_t tpq, uint32_t longest, uint32_t levels, uint32_t voices, uint32_t tempo,
                          std::vector<uint32_t> instruments)
{
    if (low > 127 || high > 127) throw std::invalid_argument("notes3's range lies within MIDI 0..127 (C-1..G9)");
    if (high < low + 11) throw std::invalid_argument("notes3's range spans at least an octave (12 notes)");
    if (tpq < 1 || tpq > 960) throw std::invalid_argument("notes3 has 1 to 960 ticks a quarter note");
    if (longest < 1 || longest > 65535) throw std::invalid_argument("notes3's longest length is 1 to 65535 ticks");
    if (levels < 1 || levels > 127) throw std::invalid_argument("notes3 has 1 to 127 loudness levels");
    if (voices < 1 || voices > kNotes3MaxVoices) throw std::invalid_argument("notes3 has 1 to 15 voices (a MIDI channel each, the drums' left out)");
    if (tempo < 1 || tempo > 1000) throw std::invalid_argument("notes3's tempo is 1 to 1000 quarter notes a minute");
    if (instruments.empty()) instruments = {0};
    if (instruments.size() == 1) instruments.assign(voices, instruments[0]);
    if (instruments.size() != voices) throw std::invalid_argument("notes3 takes one instrument, or one a voice");
    for (uint32_t p : instruments)
        if (p > 127) throw std::invalid_argument("an instrument is a General MIDI program, 0 to 127");
    Notes3Set s{low, high, tpq, longest, levels, voices, tempo, instruments};
    if (uint64_t(longest) * (1 + uint64_t(s.pitches()) * levels) > 0xFFFFFFFFull)
        throw std::invalid_argument("too many symbols for one position (longest x (1 + pitches x levels) must stay under 2^32)");
    return s;
}

bool is_notes3_symbols(std::string_view id)
{
    if (id.substr(0, 7) != "notes3/") return false; // without parsing: this is asked while drawing
    try
    {
        (void)notes3_set_of(id);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

Notes3Set notes3_set_of(std::string_view id)
{
    // notes3/LOW..HIGH/qTPQ/dLONGEST/vLEVELS/VVOICES/tTEMPO/iI,I,...
    const std::string s(id);
    auto bad = [&] { return std::invalid_argument("'" + s + "' is not a notes3 set (e.g. notes3/C-1..G9/q4/d16/v8/V1/t120/i0)"); };
    if (s.rfind("notes3/", 0) != 0) throw bad();
    std::vector<std::string> f;
    for (size_t at = 7;;)
    {
        const size_t sl = s.find('/', at);
        f.push_back(s.substr(at, sl == std::string::npos ? std::string::npos : sl - at));
        if (sl == std::string::npos) break;
        at = sl + 1;
    }
    if (f.size() != 7 || f[1][0] != 'q' || f[2][0] != 'd' || f[3][0] != 'v' || f[4][0] != 'V' || f[5][0] != 't' || f[6][0] != 'i') throw bad();
    const size_t dots = f[0].find("..");
    if (dots == std::string::npos) throw bad();
    std::vector<uint32_t> ins;
    for (size_t at = 1;;)
    {
        const size_t c = f[6].find(',', at);
        ins.push_back(parse_u32(f[6].substr(at, c == std::string::npos ? std::string::npos : c - at), "instrument"));
        if (c == std::string::npos) break;
        at = c + 1;
    }
    const Notes3Set set = make_notes3_set(notes3_midi_of(f[0].substr(0, dots)), notes3_midi_of(f[0].substr(dots + 2)), parse_u32(f[1].substr(1), "ticks"),
                                          parse_u32(f[2].substr(1), "longest length"), parse_u32(f[3].substr(1), "levels"),
                                          parse_u32(f[4].substr(1), "voices"), parse_u32(f[5].substr(1), "tempo"), ins);
    if (set.id() != s) throw bad(); // one spelling only
    return set;
}

Notes3CanonResult canonicalise_notes3(std::string_view text, const Notes3Set& set, uint32_t length)
{
    if (length == 0) throw std::invalid_argument("unit length must be at least 1");
    Notes3CanonResult r;
    std::vector<std::string_view> parts;
    for (size_t at = 0;;)
    {
        const size_t sl = text.find("//", at);
        parts.push_back(text.substr(at, sl == std::string_view::npos ? std::string_view::npos : sl - at));
        if (sl == std::string_view::npos) break;
        at = sl + 2;
    }
    if (parts.size() > set.voices)
        throw std::invalid_argument("the notation has " + std::to_string(parts.size()) + " voices; the line has " + std::to_string(set.voices));
    std::vector<std::vector<uint32_t>> voices(set.voices);
    const uint32_t default_level = set.default_level();
    // An event longer than the longest becomes the event and rests (below); one asking for more
    // than this many of them is refused rather than filling memory.
    constexpr uint64_t kMostSplits = uint64_t(1) << 24;
    for (size_t v = 0; v < parts.size(); ++v)
    {
        const std::string_view part = parts[v];
        size_t i = 0;
        while (i < part.size())
        {
            const char c = part[i];
            if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ',') { ++i; continue; }
            size_t j = i;
            while (j < part.size() && !std::isspace(static_cast<unsigned char>(part[j])) && part[j] != '|' && part[j] != ',') ++j;
            const std::string tok(part.substr(i, j - i));
            i = j;
            auto bad = [&] { return std::invalid_argument("cannot read the event '" + tok + "' (examples: C#4:3!5  R:2  E4q  Bb3e.!2)"); };
            size_t k = 0;
            bool is_rest = false;
            uint32_t midi = 0;
            const char head = char(std::toupper(static_cast<unsigned char>(tok[k++])));
            if (head == 'R') is_rest = true;
            else
            {
                int semitone = letter_semitone(head);
                if (semitone < 0) throw bad();
                if (k < tok.size() && tok[k] == '#') { ++semitone; ++k; }
                else if (k < tok.size() && tok[k] == 'b') { --semitone; ++k; ++r.flats_rewritten; }
                const bool minus = k < tok.size() && tok[k] == '-';
                if (minus) ++k;
                if (k >= tok.size() || !std::isdigit(static_cast<unsigned char>(tok[k]))) throw bad();
                int m = ((minus ? -(tok[k] - '0') : tok[k] - '0') + 1) * 12 + semitone;
                ++k;
                bool shifted = false;
                while (m < int(set.low)) { m += 12; shifted = true; }
                while (m > int(set.high)) { m -= 12; shifted = true; }
                if (shifted) ++r.octave_shifted;
                midi = uint32_t(m);
            }
            // The length: ":TICKS", a notes2 code, or none (a quarter).
            uint64_t ticks = set.tpq;
            if (k < tok.size() && tok[k] == ':')
            {
                size_t e = ++k;
                while (e < tok.size() && std::isdigit(static_cast<unsigned char>(tok[e]))) ++e;
                if (e == k || e - k > 9) throw bad();
                ticks = std::stoull(tok.substr(k, e - k));
                if (ticks == 0) throw bad();
                k = e;
            }
            else if (k < tok.size() && std::isalpha(static_cast<unsigned char>(tok[k])))
            {
                const char l = char(std::tolower(static_cast<unsigned char>(tok[k++])));
                const bool dotted = k < tok.size() && tok[k] == '.';
                if (dotted) ++k;
                const std::string code = std::string(1, l) + (dotted ? "." : "");
                int found = -1;
                for (int n = 0; n < 8; ++n)
                    if (code == kCodeNotation[n]) found = n;
                if (found < 0) throw bad();
                // round(sixteenths x TPQ / 4), at least one tick.
                const uint64_t num = uint64_t(kCodeSixteenths[found]) * set.tpq;
                ticks = std::max<uint64_t>(1, (num * 2 + 4) / 8);
                if (num % 4 != 0) ++r.lengths_rounded;
            }
            else ++r.default_lengths;
            // The level: "!LEVEL", or the one nearest velocity 96.
            uint32_t level = default_level;
            if (k < tok.size() && tok[k] == '!')
            {
                if (is_rest) throw bad();
                size_t e = ++k;
                while (e < tok.size() && std::isdigit(static_cast<unsigned char>(tok[e]))) ++e;
                if (e == k || e - k > 9) throw bad();
                const uint64_t l = std::stoull(tok.substr(k, e - k));
                if (l == 0) throw bad();
                level = uint32_t(std::min<uint64_t>(l, set.levels));
                if (l > set.levels) ++r.levels_clamped;
                k = e;
            }
            else if (!is_rest) ++r.default_levels;
            if (k != tok.size()) throw bad();
            // Longer than the longest: the event, then rests for the rest of it.
            uint64_t left = ticks;
            bool first = true;
            if (left > set.longest) ++r.lengths_split;
            if ((left + set.longest - 1) / set.longest > kMostSplits)
                throw std::invalid_argument("the event '" + tok + "' is " + std::to_string(left) + " ticks: more than " + std::to_string(kMostSplits) +
                                            " of the longest length (" + std::to_string(set.longest) + " ticks)");
            while (left > 0)
            {
                const uint32_t t = uint32_t(std::min<uint64_t>(left, set.longest));
                voices[v].push_back(set.digit(is_rest || !first, midi, level, t));
                left -= t;
                first = false;
            }
            ++r.events;
        }
    }
    size_t longest = 0;
    for (const auto& v : voices) longest = std::max(longest, v.size());
    const size_t runs = std::max<size_t>(1, (longest + length - 1) / length);
    for (size_t k = 0; k < runs; ++k)
    {
        std::vector<uint32_t> unit;
        unit.reserve(size_t(length) * set.voices);
        for (const auto& v : voices)
            for (size_t e = 0; e < length; ++e)
            {
                const size_t at = k * length + e;
                if (at < v.size()) unit.push_back(v[at]);
                else
                {
                    unit.push_back(0);
                    ++r.padding;
                }
            }
        r.units.push_back(std::move(unit));
    }
    return r;
}

std::string notes3_token(const Notes3Set& set, uint32_t d)
{
    if (d >= set.base()) throw std::out_of_range("note symbol out of range");
    if (set.rest(d)) return "R:" + std::to_string(set.ticks(d));
    return notes3_name(set.midi(d)) + ":" + std::to_string(set.ticks(d)) + "!" + std::to_string(set.level(d));
}

std::string notes3_to_notation(const Notes3Set& set, const std::vector<uint32_t>& digits)
{
    const size_t per = digits.size() / set.voices;
    std::string s;
    for (size_t i = 0; i < digits.size(); ++i)
    {
        if (i) s += per && i % per == 0 ? " // " : " ";
        s += notes3_token(set, digits[i]);
    }
    return s;
}

std::string notes3_to_midi(const Notes3Set& set, const std::vector<uint32_t>& digits)
{
    std::string file = "MThd";
    auto u32be = [&](std::string& out, uint32_t v) { for (int s = 24; s >= 0; s -= 8) out.push_back(char(v >> s)); };
    auto u16be = [&](std::string& out, uint32_t v) { out.push_back(char(v >> 8)); out.push_back(char(v)); };
    auto vlq = [](std::string& out, uint64_t v) {
        char buf[10];
        int n = 0;
        buf[n++] = char(v & 0x7F);
        while (v >>= 7) buf[n++] = char(0x80 | (v & 0x7F));
        while (n--) out.push_back(buf[n]);
    };
    auto chunk = [&](const std::string& track) {
        file += "MTrk";
        u32be(file, uint32_t(track.size()));
        file += track;
    };
    u32be(file, 6);
    u16be(file, 1);
    u16be(file, 1 + set.voices);
    u16be(file, set.tpq);
    {
        // Microseconds a quarter note, rounded half up.
        const uint32_t us = uint32_t((uint64_t(60000000) * 2 + set.tempo) / (2 * uint64_t(set.tempo)));
        std::string t;
        vlq(t, 0);
        for (int b : {0xFF, 0x51, 0x03}) t.push_back(char(b));
        t.push_back(char(us >> 16));
        t.push_back(char(us >> 8));
        t.push_back(char(us));
        vlq(t, 0);
        for (int b : {0xFF, 0x2F, 0x00}) t.push_back(char(b));
        chunk(t);
    }
    const size_t per = digits.size() / set.voices;
    for (uint32_t v = 0; v < set.voices; ++v)
    {
        const uint32_t ch = channel_of(v);
        std::string t;
        vlq(t, 0);
        t.push_back(char(0xC0 | ch));
        t.push_back(char(set.instruments[v]));
        uint64_t pending = 0;
        for (size_t e = 0; e < per; ++e)
        {
            const uint32_t d = digits[v * per + e];
            if (d >= set.base()) throw std::out_of_range("note symbol out of range");
            const uint32_t ticks = set.ticks(d);
            if (set.rest(d))
            {
                pending += ticks;
                continue;
            }
            vlq(t, pending);
            for (int b : {int(0x90 | ch), int(set.midi(d)), int(set.velocity(set.level(d)))}) t.push_back(char(b));
            vlq(t, ticks);
            for (int b : {int(0x80 | ch), int(set.midi(d)), 0}) t.push_back(char(b));
            pending = 0;
        }
        vlq(t, pending);
        for (int b : {0xFF, 0x2F, 0x00}) t.push_back(char(b));
        chunk(t);
    }
    return file;
}

std::string midi_to_notes3(const std::vector<uint8_t>& m, const Notes3Set& set, std::vector<std::string>* report)
{
    size_t at = 0;
    auto need = [&](size_t n) {
        if (at + n > m.size()) throw std::invalid_argument("the MIDI file ends too soon");
    };
    auto u32 = [&] {
        need(4);
        const uint32_t v = uint32_t(m[at]) << 24 | uint32_t(m[at + 1]) << 16 | uint32_t(m[at + 2]) << 8 | m[at + 3];
        at += 4;
        return v;
    };
    auto u16 = [&] {
        need(2);
        const uint32_t v = uint32_t(m[at]) << 8 | m[at + 1];
        at += 2;
        return v;
    };
    need(4);
    if (!(m[0] == 'M' && m[1] == 'T' && m[2] == 'h' && m[3] == 'd')) throw std::invalid_argument("not a MIDI file (no MThd)");
    at = 4;
    const uint32_t header = u32();
    if (header < 6) throw std::invalid_argument("the MIDI header is too short");
    (void)u16();
    const uint32_t tracks = u16(), division = u16();
    if (division & 0x8000) throw std::invalid_argument("MIDI timed in SMPTE frames is not read");
    if (division == 0) throw std::invalid_argument("the MIDI file has no ticks per quarter note");
    at = 8 + header;
    struct Note
    {
        uint64_t start, end;
        uint32_t pitch, velocity;
    };
    std::vector<std::vector<Note>> voices;
    std::vector<uint64_t> ends;
    for (uint32_t t = 0; t < tracks && at < m.size(); ++t)
    {
        need(8);
        const bool track = m[at] == 'M' && m[at + 1] == 'T' && m[at + 2] == 'r' && m[at + 3] == 'k';
        at += 4;
        const uint32_t len = u32();
        need(len);
        const size_t stop = at + len;
        if (!track)
        {
            at = stop;
            continue;
        }
        std::vector<Note> notes;
        std::map<uint32_t, std::pair<uint64_t, uint32_t>> open; // channel << 8 | pitch -> start, velocity
        uint64_t now = 0;
        uint8_t status = 0;
        auto vlq = [&] {
            uint64_t v = 0;
            for (int i = 0; i < 4; ++i)
            {
                if (at >= stop) throw std::invalid_argument("a MIDI track ends inside an event");
                const uint8_t b = m[at++];
                v = v << 7 | (b & 0x7F);
                if (!(b & 0x80)) return v;
            }
            throw std::invalid_argument("a MIDI delta time is too long");
        };
        while (at < stop)
        {
            now += vlq();
            if (at >= stop) break;
            const uint8_t b = m[at];
            if (b == 0xFF)
            {
                at += 2;
                if (at > stop) throw std::invalid_argument("a MIDI track ends inside an event");
                const uint8_t type = m[at - 1];
                const uint64_t n = vlq();
                at += size_t(n);
                if (type == 0x2F) break;
                continue;
            }
            if (b == 0xF0 || b == 0xF7)
            {
                ++at;
                at += size_t(vlq());
                continue;
            }
            if (b & 0x80)
            {
                status = b;
                ++at;
            }
            else if (!status) throw std::invalid_argument("a MIDI event with no status");
            const uint32_t kind = status & 0xF0u, ch = status & 0x0Fu;
            const size_t data = kind == 0xC0 || kind == 0xD0 ? 1 : 2;
            if (at + data > stop) throw std::invalid_argument("a MIDI track ends inside an event");
            const uint32_t d1 = m[at], d2 = data == 2 ? uint32_t(m[at + 1]) : 0u;
            at += data;
            const uint32_t key = ch << 8 | d1;
            if (kind == 0x90 && d2 > 0)
            {
                if (auto it = open.find(key); it != open.end()) notes.push_back({it->second.first, now, d1, it->second.second});
                open[key] = {now, d2};
            }
            else if (kind == 0x80 || kind == 0x90)
            {
                if (auto it = open.find(key); it != open.end())
                {
                    notes.push_back({it->second.first, now, d1, it->second.second});
                    open.erase(it);
                }
            }
        }
        for (const auto& [key, sv] : open) notes.push_back({sv.first, now, key & 0xFF, sv.second});
        at = stop;
        if (notes.empty() && now == 0) continue;
        std::stable_sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) { return a.start < b.start; });
        voices.push_back(std::move(notes));
        ends.push_back(now);
    }
    if (voices.empty()) throw std::invalid_argument("the MIDI file has no notes, and no length");
    // The file's ticks as the set's: round(ticks x TPQ / division), half up.
    auto grid = [&](uint64_t ticks) { return (ticks * set.tpq * 2 + division) / (2 * uint64_t(division)); };
    auto level_of = [&](uint32_t vel) {
        uint32_t best = 1, gap = ~0u;
        for (uint32_t k = 1; k <= set.levels; ++k)
        {
            const uint32_t v = set.velocity(k), g = v > vel ? v - vel : vel - v;
            if (g <= gap)
            {
                best = k;
                gap = g;
            }
        }
        return best;
    };
    size_t overlapped = 0;
    const size_t used = std::min<size_t>(voices.size(), set.voices), dropped = voices.size() - used;
    std::string out;
    for (size_t vi = 0; vi < used; ++vi)
    {
        std::string v;
        auto put = [&](const std::string& token) {
            if (!v.empty()) v += ' ';
            v += token;
        };
        uint64_t cur = 0;
        for (const Note& n : voices[vi])
        {
            uint64_t s = grid(n.start), e = grid(n.end);
            if (s < cur)
            {
                ++overlapped;
                continue;
            }
            if (e <= s) e = s + 1;
            if (s > cur) put("R:" + std::to_string(s - cur));
            put(notes3_name(n.pitch) + ":" + std::to_string(e - s) + "!" + std::to_string(level_of(n.velocity)));
            cur = e;
        }
        const uint64_t end = grid(ends[vi]);
        if (end > cur) put("R:" + std::to_string(end - cur));
        if (!out.empty()) out += " // ";
        out += v;
    }
    if (report)
    {
        if (overlapped) report->push_back(std::to_string(overlapped) + " note(s) starting inside another left out (one note at a time)");
        if (dropped) report->push_back(std::to_string(dropped) + " track(s) beyond the set's voices left out");
        report->push_back("the file's tempo and instruments are not read: they are the line's");
    }
    return out;
}

} // namespace sieve
