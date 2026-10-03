// Sieve — text that is a file written out (written.hpp): the readings decoded outright, for
// judging, and the same readings as small machines walked over an alphabet's symbols, for the
// automaton that counts and ranks what not-written-v1 keeps.
//
// Every machine carries the signature matcher's state (Head below): after p decoded bytes, which
// signatures of file-kinds-v1 those bytes still fit. It is decided -- signed, or not -- once a
// signature's last fixed byte arrives or none fits any more, after at most 14 bytes; a reading
// whose matcher has failed is dead, so the machines stay small.

#include "sieve/written.hpp"

#include "sieve/filekind.hpp"
#include "sieve/plugin.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace sieve {

namespace {

enum Reading : uint32_t { kText, kHex, kBase64, kBase32, kDecimal, kNibbles, kSpelled, kBinary, kReadings };

bool space(uint32_t c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v'; }

int hex_value(uint32_t c)
{
    if (c >= '0' && c <= '9') return int(c - '0');
    if (c >= 'a' && c <= 'f') return int(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return int(c - 'A' + 10);
    return -1;
}

uint32_t lower(uint32_t c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }

// base64's value of a symbol, and which alphabet it belongs to (1 standard, 2 URL-safe, 0 both).
int base64_value(uint32_t c, int& alphabet)
{
    alphabet = 0;
    if (c >= 'A' && c <= 'Z') return int(c - 'A');
    if (c >= 'a' && c <= 'z') return int(c - 'a' + 26);
    if (c >= '0' && c <= '9') return int(c - '0' + 52);
    if (c == '+' || c == '/') { alphabet = 1; return c == '+' ? 62 : 63; }
    if (c == '-' || c == '_') { alphabet = 2; return c == '-' ? 62 : 63; }
    return -1;
}

int base32_value(uint32_t c)
{
    if (c >= 'A' && c <= 'Z') return int(c - 'A');
    if (c >= 'a' && c <= 'z') return int(c - 'a');
    if (c >= '2' && c <= '7') return int(c - '2' + 26);
    return -1;
}

bool decimal_separator(uint32_t c) { return space(c) || c == ',' || c == ';' || c == '[' || c == ']' || c == '{' || c == '}' || c == '(' || c == ')'; }
bool spelled_separator(uint32_t c) { return space(c) || c == ',' || c == '.' || c == '-'; }
bool hex_separator(uint32_t c) { return space(c) || c == ',' || c == ':' || c == ';' || c == '-' || c == '_'; }

const char* const kWords[16] = {"zero", "one", "two", "three", "four", "five", "six", "seven",
                                "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen"};

// ---------------------------------------------------------------- decoding outright

using Bytes = std::vector<uint8_t>;

std::optional<Bytes> pairs(const std::vector<int>& n)
{
    if (n.empty() || n.size() % 2) return std::nullopt;
    Bytes b(n.size() / 2);
    for (size_t i = 0; i < b.size(); ++i) b[i] = uint8_t(n[2 * i] << 4 | n[2 * i + 1]);
    return b;
}

std::string without_space(const std::string& s)
{
    std::string t;
    for (char c : s)
        if (!space(uint8_t(c))) t += c;
    return t;
}

std::vector<Bytes> read_text(const std::u32string& text)
{
    Bytes b;
    bool latin1 = true;
    for (char32_t c : text)
        if (c >= 0x100) latin1 = false;
    if (latin1)
        for (char32_t c : text) b.push_back(uint8_t(c));
    else
    {
        const std::string u = utf8_encode(text);
        b.assign(u.begin(), u.end());
    }
    Bytes trimmed = b;
    while (!trimmed.empty() && space(trimmed.back())) trimmed.pop_back();
    Bytes lf = trimmed;
    lf.push_back('\n');
    return {b, trimmed, lf};
}

std::optional<Bytes> read_hex(const std::string& s)
{
    std::vector<int> n;
    for (size_t i = 0; i < s.size(); ++i)
    {
        const uint8_t c = uint8_t(s[i]);
        if ((c == '0' || c == '\\') && i + 1 < s.size() && (s[i + 1] == 'x' || s[i + 1] == 'X'))
        {
            ++i;
            continue;
        }
        if (hex_separator(c)) continue;
        const int v = hex_value(c);
        if (v < 0) return std::nullopt;
        n.push_back(v);
    }
    return pairs(n);
}

std::optional<Bytes> read_base64(const std::string& s)
{
    std::string t = without_space(s);
    const size_t at = t.find("base64,");
    if (t.rfind("data:", 0) == 0 && at != std::string::npos) t = t.substr(at + 7);
    while (!t.empty() && t.back() == '=') t.pop_back();
    if (t.empty() || t.size() % 4 == 1) return std::nullopt;
    Bytes out;
    uint32_t acc = 0;
    int bits = 0, used = 0;
    for (char ch : t)
    {
        int alphabet = 0;
        const int v = base64_value(uint8_t(ch), alphabet);
        if (v < 0) return std::nullopt;
        used |= alphabet;
        if (used == 3) return std::nullopt;
        acc = acc << 6 | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    return out;
}

std::optional<Bytes> read_base32(const std::string& s)
{
    std::string t = without_space(s);
    while (!t.empty() && t.back() == '=') t.pop_back();
    if (t.empty()) return std::nullopt;
    Bytes out;
    uint32_t acc = 0;
    int bits = 0;
    for (char ch : t)
    {
        const int v = base32_value(uint8_t(ch));
        if (v < 0) return std::nullopt;
        acc = acc << 5 | uint32_t(v);
        bits += 5;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    return out;
}

std::optional<Bytes> read_decimal(const std::string& s)
{
    Bytes out;
    uint32_t v = 0;
    int digits = 0;
    auto end = [&] {
        if (digits == 0) return true;
        if (v > 255) return false;
        out.push_back(uint8_t(v));
        v = 0;
        digits = 0;
        return true;
    };
    for (char ch : s)
    {
        const uint8_t c = uint8_t(ch);
        if (c >= '0' && c <= '9')
        {
            if (++digits > 3) return std::nullopt;
            v = v * 10 + (c - '0');
        }
        else if (!decimal_separator(c) || !end()) return std::nullopt;
    }
    if (!end() || out.empty()) return std::nullopt;
    return out;
}

std::optional<Bytes> read_nibbles(const std::string& s)
{
    std::vector<int> n;
    for (char ch : s)
    {
        const uint32_t c = lower(uint8_t(ch));
        if (space(c)) continue;
        if (c < 'a' || c > 'p') return std::nullopt;
        n.push_back(int(c - 'a'));
    }
    return pairs(n);
}

std::optional<Bytes> read_spelled(const std::string& s)
{
    std::vector<int> n;
    std::string w;
    auto end = [&] {
        if (w.empty()) return true;
        int v = -1;
        if (w.size() == 1 && w[0] >= 'a' && w[0] <= 'f') v = w[0] - 'a' + 10;
        for (int i = 0; i < 16 && v < 0; ++i)
            if (w == kWords[i]) v = i;
        w.clear();
        if (v < 0) return false;
        n.push_back(v);
        return true;
    };
    for (char ch : s)
    {
        const uint32_t c = lower(uint8_t(ch));
        if (c >= 'a' && c <= 'z') w += char(c);
        else if (!spelled_separator(c) || !end()) return std::nullopt;
    }
    if (!end()) return std::nullopt;
    return pairs(n);
}

std::vector<Bytes> read_binary(const std::string& s)
{
    const std::string t = without_space(s);
    if (t.empty() || t.size() % 8) return {};
    const char a = t[0];
    int b = -1;
    for (char c : t)
    {
        if (c == a) continue;
        if (b < 0) b = uint8_t(c);
        else if (uint8_t(c) != b) return {};
    }
    if (b < 0) return {};
    std::vector<Bytes> out;
    for (int way = 0; way < 2; ++way)
    {
        Bytes bytes(t.size() / 8, 0);
        for (size_t i = 0; i < t.size(); ++i)
            if ((uint8_t(t[i]) == b) != (way == 1)) bytes[i / 8] = uint8_t(bytes[i / 8] | (0x80u >> (i % 8)));
        out.push_back(std::move(bytes));
    }
    return out;
}

std::optional<std::string> signed_kind(const Bytes& b)
{
    if (b.empty()) return std::nullopt;
    const std::string k = file_kind(b, b.size());
    if (!is_signed_kind(k)) return std::nullopt;
    return k;
}

// ---------------------------------------------------------------- the machines

constexpr int32_t kDeadHead = -1, kSigned = -2;
constexpr uint32_t kMaskBits = 25;

// The signature matcher: kSigned, kDeadHead, or (bytes seen) << 25 | (signatures still possible).
class Head
{
public:
    Head()
    {
        const auto& sigs = file_signatures();
        if (sigs.size() > kMaskBits) throw std::logic_error("too many signatures for the matcher");
        all_ = (1u << sigs.size()) - 1;
        for (size_t p = 0; p < kKindHead; ++p)
            for (uint32_t b = 0; b < 256; ++b)
                for (size_t i = 0; i < sigs.size(); ++i)
                    if (sigs[i].fixed[p] < 0 || sigs[i].fixed[p] == int16_t(b)) ok_[p][b] |= 1u << i;
        for (size_t p = 0; p <= kKindHead; ++p)
            for (size_t i = 0; i < sigs.size(); ++i)
                if (sigs[i].end <= p) done_[p] |= 1u << i;
    }
    int32_t start() const { return int32_t(all_); }
    int32_t feed(int32_t h, uint32_t byte) const
    {
        if (h < 0) return h;
        uint32_t pos = uint32_t(h) >> kMaskBits, mask = uint32_t(h) & ((1u << kMaskBits) - 1);
        mask &= ok_[pos][byte & 0xFF];
        ++pos;
        if (mask == 0) return kDeadHead;
        if (mask & done_[pos]) return kSigned;
        if (pos >= kKindHead) return kDeadHead;
        return int32_t(pos << kMaskBits | mask);
    }
    static bool decided(int32_t h) { return h < 0; }

private:
    uint32_t all_ = 0;
    std::array<std::array<uint32_t, 256>, kKindHead> ok_{};
    std::array<uint32_t, kKindHead + 1> done_{};
};

const Head& head()
{
    static const Head h;
    return h;
}

using St = std::array<int32_t, 16>;

struct StHash
{
    size_t operator()(const St& s) const
    {
        uint64_t h = 1469598103934665603ull;
        for (int32_t v : s) h = (h ^ uint32_t(v)) * 1099511628211ull;
        return size_t(h ^ (h >> 29));
    }
};

class Machine
{
public:
    virtual ~Machine() = default;
    virtual St start() const = 0;
    virtual bool step(St& s, uint32_t in) const = 0; // false: no way on to a signed file
    virtual bool accept(const St& s) const = 0;
    virtual bool codepoints() const { return false; } // reads code points, not UTF-8 bytes
};

// Four bits at a time into bytes: slots p (parity), p+1 (the high half), p+2 (the matcher).
bool nibble(St& s, size_t p, int v)
{
    if (s[p] == 0)
    {
        s[p + 1] = Head::decided(s[p + 2]) ? 0 : v; // once decided, the byte no longer matters
        s[p] = 1;
        return true;
    }
    s[p + 2] = head().feed(s[p + 2], uint32_t(s[p + 1] << 4 | v));
    s[p] = 0;
    s[p + 1] = 0;
    return s[p + 2] != kDeadHead;
}

// text: slots 0 (a code point past U+00FF seen), 1-2 the Latin-1 bytes' matcher as they are and
// at the last byte that was not whitespace, 3-4 the same for the UTF-8 bytes.
class TextMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[1] = s[2] = s[3] = s[4] = head().start();
        return s;
    }
    bool codepoints() const override { return true; }
    bool step(St& s, uint32_t cp) const override
    {
        if (cp < 0x100)
        {
            s[1] = head().feed(s[1], cp);
            if (!space(cp)) s[2] = s[1];
        }
        else
        {
            s[0] = 1;
            s[1] = s[2] = kDeadHead;
        }
        for (char c : utf8_encode(char32_t(cp)))
        {
            s[3] = head().feed(s[3], uint8_t(c));
            if (!space(uint8_t(c))) s[4] = s[3];
        }
        if (s[0]) return s[3] != kDeadHead || s[4] != kDeadHead;
        return s[1] != kDeadHead || s[2] != kDeadHead || s[3] != kDeadHead || s[4] != kDeadHead;
    }
    bool accept(const St& s) const override
    {
        const int32_t as_is = s[0] ? s[3] : s[1], trimmed = s[0] ? s[4] : s[2];
        return as_is == kSigned || trimmed == kSigned || head().feed(trimmed, '\n') == kSigned;
    }
};

// hex: slots 0 (a 0 or \ held back, 1 or 2, to see whether an x follows), 1-3 the nibbles.
class HexMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[3] = head().start();
        return s;
    }
    bool step(St& s, uint32_t c) const override
    {
        if (s[0])
        {
            const int32_t held = s[0];
            s[0] = 0;
            if (c == 'x' || c == 'X') return true; // a prefix: both dropped
            if (held == 2 || !nibble(s, 1, 0)) return false;
        }
        if (c == '0' || c == '\\')
        {
            s[0] = c == '0' ? 1 : 2;
            return true;
        }
        if (hex_separator(c)) return true;
        const int v = hex_value(c);
        return v >= 0 && nibble(s, 1, v);
    }
    bool accept(const St& s) const override
    {
        St t = s;
        if (t[0] == 2) return false;
        if (t[0] == 1 && !nibble(t, 1, 0)) return false;
        return t[1] == 0 && t[3] == kSigned;
    }
};

// base64: slots 0-7 the text read as it is (0 alive, 1 symbols mod 4, 2 in the = padding, 3 the
// alphabets used, 4 bits waiting, 5 their value, 6 the matcher, 7 any symbol at all); slot 8 the
// data: URL (0-4: its prefix matched so far, 5-11: looking for "base64," with 0-6 of it matched,
// 12: found, the rest read in slots 9-15 as slots 1-7 are; -1: not a data URL).
class Base64Machine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[0] = 1;
        s[6] = head().start();
        s[14] = head().start();
        return s;
    }
    static bool read(St& s, size_t p, uint32_t c) // slots p .. p+6 as 1-7 above
    {
        if (s[p + 1]) return c == '=';
        if (c == '=')
        {
            s[p + 1] = 1;
            return true;
        }
        int alphabet = 0;
        const int v = base64_value(c, alphabet);
        if (v < 0) return false;
        s[p + 2] |= alphabet;
        if (s[p + 2] == 3) return false;
        s[p] = (s[p] + 1) % 4;
        s[p + 6] = 1;
        s[p + 3] += 6;
        if (Head::decided(s[p + 5]))
        {
            s[p + 3] %= 8;
            s[p + 4] = 0;
            return true;
        }
        s[p + 4] = s[p + 4] << 6 | v;
        if (s[p + 3] >= 8)
        {
            s[p + 3] -= 8;
            s[p + 5] = head().feed(s[p + 5], uint32_t(s[p + 4] >> s[p + 3]));
            s[p + 4] &= (1 << s[p + 3]) - 1;
            if (s[p + 5] == kDeadHead) return false;
            if (s[p + 5] == kSigned) s[p + 4] = 0;
        }
        return true;
    }
    static bool done(const St& s, size_t p) { return s[p + 6] && s[p] != 1 && s[p + 5] == kSigned; }
    bool step(St& s, uint32_t c) const override
    {
        if (space(c)) return true;
        if (s[0] && !read(s, 1, c)) s[0] = 0;
        static const char* const prefix = "data:";
        static const char* const mark = "base64,";
        int32_t& d = s[8];
        if (d >= 0 && d < 5) d = c == uint8_t(prefix[d]) ? d + 1 : -1;
        else if (d >= 5 && d < 12)
        {
            int k = d - 5; // the longest prefix of "base64," ending here (it has no repeated prefix)
            k = c == uint8_t(mark[k]) ? k + 1 : c == uint8_t(mark[0]) ? 1 : 0;
            d = k == 7 ? 12 : 5 + k;
        }
        else if (d == 12 && !read(s, 9, c)) d = -1;
        return s[0] || d >= 0;
    }
    bool accept(const St& s) const override { return (s[0] && done(s, 1)) || (s[8] == 12 && done(s, 9)); }
};

// base32: slots 0 in the padding, 1 any symbol, 2 bits waiting, 3 their value, 4 the matcher.
class Base32Machine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[4] = head().start();
        return s;
    }
    bool step(St& s, uint32_t c) const override
    {
        if (space(c)) return true;
        if (s[0]) return c == '=';
        if (c == '=')
        {
            s[0] = 1;
            return true;
        }
        const int v = base32_value(c);
        if (v < 0) return false;
        s[1] = 1;
        s[2] += 5;
        if (Head::decided(s[4]))
        {
            s[2] %= 8;
            s[3] = 0;
            return true;
        }
        s[3] = s[3] << 5 | v;
        if (s[2] >= 8)
        {
            s[2] -= 8;
            s[4] = head().feed(s[4], uint32_t(s[3] >> s[2]));
            s[3] &= (1 << s[2]) - 1;
            if (s[4] == kDeadHead) return false;
            if (s[4] == kSigned) s[3] = 0;
        }
        return true;
    }
    bool accept(const St& s) const override { return s[1] && s[4] == kSigned; }
};

// decimal: slots 0 digits so far (0-3), 1 their value (256 for anything above 255), 2 the matcher.
class DecimalMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[2] = head().start();
        return s;
    }
    static bool end(St& s)
    {
        if (s[0] == 0) return true;
        if (s[1] > 255) return false;
        s[2] = head().feed(s[2], uint32_t(s[1]));
        s[0] = s[1] = 0;
        return s[2] != kDeadHead;
    }
    bool step(St& s, uint32_t c) const override
    {
        if (c >= '0' && c <= '9')
        {
            if (++s[0] > 3) return false;
            s[1] = std::min(256, s[1] * 10 + int32_t(c - '0'));
            return true;
        }
        return decimal_separator(c) && end(s);
    }
    bool accept(const St& s) const override
    {
        St t = s;
        return end(t) && t[2] == kSigned;
    }
};

// nibbles: slots 0-2 the nibbles.
class NibblesMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[2] = head().start();
        return s;
    }
    bool step(St& s, uint32_t c) const override
    {
        c = lower(c);
        if (space(c)) return true;
        return c >= 'a' && c <= 'p' && nibble(s, 0, int(c - 'a'));
    }
    bool accept(const St& s) const override { return s[0] == 0 && s[2] == kSigned; }
};

// spelled: slot 0 where the word so far is in a trie of the words, 1-3 the nibbles.
class SpelledMachine : public Machine
{
public:
    SpelledMachine()
    {
        nodes_.push_back({});
        auto add = [&](const std::string& w, int v) {
            int n = 0;
            for (char ch : w)
            {
                int& child = nodes_[size_t(n)].child[size_t(ch - 'a')];
                if (child == 0)
                {
                    child = int(nodes_.size());
                    nodes_.push_back({});
                }
                n = nodes_[size_t(n)].child[size_t(ch - 'a')];
            }
            nodes_[size_t(n)].value = v;
        };
        for (int i = 0; i < 16; ++i) add(kWords[i], i);
        for (int i = 0; i < 6; ++i) add(std::string(1, char('a' + i)), 10 + i);
    }
    St start() const override
    {
        St s{};
        s[3] = head().start();
        return s;
    }
    bool end(St& s) const
    {
        if (s[0] == 0) return true;
        const int v = nodes_[size_t(s[0])].value;
        s[0] = 0;
        return v >= 0 && nibble(s, 1, v);
    }
    bool step(St& s, uint32_t c) const override
    {
        c = lower(c);
        if (c >= 'a' && c <= 'z')
        {
            s[0] = nodes_[size_t(s[0])].child[c - 'a'];
            return s[0] != 0; // no word goes on this way
        }
        return spelled_separator(c) && end(s);
    }
    bool accept(const St& s) const override
    {
        St t = s;
        return end(t) && t[1] == 0 && t[3] == kSigned;
    }

private:
    struct Node
    {
        std::array<int, 26> child{};
        int value = -1;
    };
    std::vector<Node> nodes_;
};

// binary: slots 0 the first symbol, 1 the other (-1 before it appears), 2 bits mod 8, 3 the byte
// so far read with the first symbol as 0, 4-5 the matchers of the two ways round, 6 any symbol.
class BinaryMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[0] = s[1] = -1;
        s[4] = s[5] = head().start();
        return s;
    }
    bool step(St& s, uint32_t c) const override
    {
        if (space(c)) return true;
        int bit = 0;
        if (s[0] < 0) s[0] = int32_t(c);
        else if (int32_t(c) == s[0]) bit = 0;
        else if (s[1] < 0)
        {
            s[1] = int32_t(c);
            bit = 1;
        }
        else if (int32_t(c) == s[1]) bit = 1;
        else return false;
        s[6] = 1;
        s[2] = (s[2] + 1) % 8;
        const bool decided = Head::decided(s[4]) && Head::decided(s[5]);
        s[3] = decided ? 0 : s[3] << 1 | bit;
        if (s[2] == 0)
        {
            s[4] = head().feed(s[4], uint32_t(s[3]));
            s[5] = head().feed(s[5], uint32_t(~s[3] & 0xFF));
            s[3] = 0;
            if (s[4] == kDeadHead && s[5] == kDeadHead) return false;
        }
        return true;
    }
    bool accept(const St& s) const override { return s[6] && s[2] == 0 && s[1] >= 0 && (s[4] == kSigned || s[5] == kSigned); }
};

// A line of every byte: a unit is a file, its digits its bytes.
class BytesMachine : public Machine
{
public:
    St start() const override
    {
        St s{};
        s[0] = head().start();
        return s;
    }
    bool step(St& s, uint32_t d) const override
    {
        s[0] = head().feed(s[0], d);
        return s[0] != kDeadHead;
    }
    bool accept(const St& s) const override { return s[0] == kSigned; }
};

// ---- other lines' content in text (not-other-line-v1)

bool note_sep(uint32_t c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '|' || c == ','; }

// Melody notation: tokens separated by whitespace, | or ,; a token a note (A-G, # or b, an octave
// digit, a duration), a rest (R, a duration) or // between voices; durations s e e. q q. h h. w or
// none; at least one note or rest. Slots: 0 where in a token (0 between tokens, 1 after a letter,
// 2 after its accidental, 3 after its octave, 4 after e q or h, 5 after . s or w, 6 after R, 7 after
// one /, 8 after //), 1 a note or rest seen.
class NotesMachine : public Machine
{
public:
    St start() const override { return St{}; }
    bool step(St& s, uint32_t c) const override
    {
        const int32_t at = s[0];
        if (note_sep(c))
        {
            if (at == 1 || at == 2 || at == 7) return false;
            s[0] = 0;
            return true;
        }
        auto duration = [&]() {
            if (c == 'e' || c == 'q' || c == 'h') { s[0] = 4; return true; }
            if (c == 's' || c == 'w') { s[0] = 5; return true; }
            return false;
        };
        switch (at)
        {
        case 0:
            if (c >= 'A' && c <= 'G') { s[0] = 1; return true; }
            if (c == 'R') { s[0] = 6; s[1] = 1; return true; }
            if (c == '/') { s[0] = 7; return true; }
            return false;
        case 1:
            if (c == '#' || c == 'b') { s[0] = 2; return true; }
            [[fallthrough]];
        case 2:
            if (c >= '0' && c <= '9') { s[0] = 3; s[1] = 1; return true; }
            return false;
        case 3:
        case 6: return duration();
        case 4:
            if (c == '.') { s[0] = 5; return true; }
            return false;
        case 7:
            if (c == '/') { s[0] = 8; return true; }
            return false;
        default: return false;
        }
    }
    bool accept(const St& s) const override { return s[1] && s[0] != 1 && s[0] != 2 && s[0] != 7; }
};

// A model's .obj text: lines apart by line feeds (empty lines allowed), each "v" and three numbers
// ([+-]digits[.digits]) or "f" and three indices (no leading zero), one space before each; at
// least one v line and one f line; spaces allowed only at the very end (a page's padding). Slots:
// 0 the line's kind (0 none yet, 1 v, 2 f), 1 fields done, 2 where in a field (0 wanting a space,
// 1 wanting a number, 2 after a sign, 3 in its digits, 4 after a point, 5 in its decimals), 3 a v
// line seen, 4 an f line seen, 5 in the padding.
class ObjMachine : public Machine
{
public:
    St start() const override { return St{}; }
    static bool ended(const St& s) { return s[2] == 3 || (s[0] == 1 && s[2] == 5); } // a number just read
    static bool complete(const St& s) { return s[0] != 0 && s[1] == 2 && ended(s); }
    static void commit(St& s)
    {
        (s[0] == 1 ? s[3] : s[4]) = 1;
        s[0] = s[1] = s[2] = 0;
    }
    bool step(St& s, uint32_t c) const override
    {
        if (s[5]) return c == ' ';
        if (c == '\n')
        {
            if (s[0] == 0) return true;
            if (!complete(s)) return false;
            commit(s);
            return true;
        }
        if (c == ' ')
        {
            if (s[0] == 0) // padding after the last line
            {
                s[5] = 1;
                return true;
            }
            if (complete(s))
            {
                commit(s);
                s[5] = 1;
                return true;
            }
            if (s[2] == 0) { s[2] = 1; return true; }
            if (ended(s)) { ++s[1]; s[2] = 1; return true; }
            return false;
        }
        if (s[0] == 0)
        {
            if (c == 'v' || c == 'f') { s[0] = c == 'v' ? 1 : 2; s[1] = 0; s[2] = 0; return true; }
            return false;
        }
        const bool digit = c >= '0' && c <= '9';
        switch (s[2])
        {
        case 1:
            if (s[0] == 2) { if (c >= '1' && c <= '9') { s[2] = 3; return true; } return false; }
            if (c == '+' || c == '-') { s[2] = 2; return true; }
            if (digit) { s[2] = 3; return true; }
            return false;
        case 2: if (digit) { s[2] = 3; return true; } return false;
        case 3:
            if (digit) return true;
            if (c == '.' && s[0] == 1) { s[2] = 4; return true; }
            return false;
        case 4: if (digit) { s[2] = 5; return true; } return false;
        case 5: return digit;
        default: return false;
        }
    }
    bool accept(const St& s) const override
    {
        St t = s;
        if (!t[5] && t[0] != 0)
        {
            if (!complete(t)) return false;
            commit(t);
        }
        return t[3] && t[4];
    }
};

// A unit's symbols packed as bits (b each, most significant first) into bytes: a file with a
// signature? Slots: 0 bits waiting, 1 their value, 2 the matcher.
class BitsMachine : public Machine
{
public:
    explicit BitsMachine(uint32_t bits) : bits_(bits) {}
    St start() const override
    {
        St s{};
        s[2] = head().start();
        return s;
    }
    bool step(St& s, uint32_t d) const override
    {
        s[0] += int32_t(bits_);
        if (Head::decided(s[2]))
        {
            s[0] %= 8;
            s[1] = 0;
            return s[2] != kDeadHead;
        }
        s[1] = int32_t(uint32_t(s[1]) << bits_ | d);
        while (s[0] >= 8)
        {
            s[0] -= 8;
            s[2] = head().feed(s[2], uint32_t(s[1]) >> s[0]);
            s[1] &= (1 << s[0]) - 1;
            if (s[2] == kDeadHead) return false;
        }
        if (Head::decided(s[2])) s[1] = 0;
        return true;
    }
    bool accept(const St& s) const override { return s[2] == kSigned; }

private:
    uint32_t bits_;
};

std::unique_ptr<Machine> machine(uint32_t r)
{
    switch (r)
    {
    case kText: return std::make_unique<TextMachine>();
    case kHex: return std::make_unique<HexMachine>();
    case kBase64: return std::make_unique<Base64Machine>();
    case kBase32: return std::make_unique<Base32Machine>();
    case kDecimal: return std::make_unique<DecimalMachine>();
    case kNibbles: return std::make_unique<NibblesMachine>();
    case kSpelled: return std::make_unique<SpelledMachine>();
    default: return std::make_unique<BinaryMachine>();
    }
}

// The machine's states over the alphabet's symbols, walked from the start: a Dfa (minimised), or
// nullopt past max_states.
std::optional<Dfa> walk(const Machine& m, const std::vector<std::vector<uint32_t>>& inputs, size_t max_states)
{
    const uint32_t B = uint32_t(inputs.size());
    std::unordered_map<St, int32_t, StHash> id;
    std::vector<St> states;
    auto get = [&](const St& s) {
        const auto [it, added] = id.emplace(s, int32_t(states.size()));
        if (added) states.push_back(s);
        return it->second;
    };
    Dfa d;
    d.base = B;
    d.start = get(m.start());
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (states.size() > max_states) return std::nullopt;
        const St s = states[i];
        d.accept.push_back(m.accept(s) ? 1 : 0);
        for (uint32_t c = 0; c < B; ++c)
        {
            St t = s;
            bool alive = true;
            for (uint32_t in : inputs[c])
                if (!m.step(t, in))
                {
                    alive = false;
                    break;
                }
            d.next.push_back(alive ? get(t) : Dfa::kDead);
        }
    }
    return minimise(d);
}

} // namespace

const std::vector<std::string>& written_decoders()
{
    static const std::vector<std::string> names = {"text", "hex", "base64", "base32", "decimal", "nibbles", "spelled", "binary"};
    return names;
}

uint32_t written_mask_of(const std::string& name)
{
    if (name == "all") return (1u << kReadings) - 1;
    const auto& n = written_decoders();
    const auto it = std::find(n.begin(), n.end(), name);
    if (it == n.end()) throw std::invalid_argument("not-written-v1: unknown reading '" + name + "'");
    return 1u << uint32_t(it - n.begin());
}

std::optional<WrittenReading> written_as(const std::u32string& text, uint32_t mask)
{
    const std::string s = utf8_encode(text);
    auto check = [](const std::optional<Bytes>& b) { return b ? signed_kind(*b) : std::nullopt; };
    for (uint32_t r = 0; r < kReadings; ++r)
    {
        if (!(mask >> r & 1)) continue;
        std::optional<std::string> k;
        switch (r)
        {
        case kText:
            for (const Bytes& b : read_text(text))
                if (!k) k = signed_kind(b);
            break;
        case kHex: k = check(read_hex(s)); break;
        case kBase64: k = check(read_base64(s)); break;
        case kBase32: k = check(read_base32(s)); break;
        case kDecimal: k = check(read_decimal(s)); break;
        case kNibbles: k = check(read_nibbles(s)); break;
        case kSpelled: k = check(read_spelled(s)); break;
        default:
            for (const Bytes& b : read_binary(s))
                if (!k) k = signed_kind(b);
            break;
        }
        if (k) return WrittenReading{written_decoders()[r], *k};
    }
    return std::nullopt;
}

std::optional<WrittenReading> written_as_bytes(std::span<const uint32_t> digits)
{
    Bytes b;
    b.reserve(digits.size());
    for (uint32_t d : digits) b.push_back(uint8_t(d));
    if (auto k = signed_kind(b)) return WrittenReading{"bytes", *k};
    return std::nullopt;
}

std::optional<Dfa> written_dfa(const Alphabet& a, uint32_t mask, size_t max_states)
{
    const uint32_t B = a.size();
    if (holds_all_bytes(a))
    {
        std::vector<std::vector<uint32_t>> inputs(B);
        for (uint32_t c = 0; c < B; ++c) inputs[c] = {c};
        return walk(BytesMachine(), inputs, max_states);
    }
    std::vector<std::vector<uint32_t>> cps(B), bytes(B);
    for (uint32_t c = 0; c < B; ++c)
    {
        cps[c] = {uint32_t(a.symbol(c))};
        for (char ch : utf8_encode(a.symbol(c))) bytes[c].push_back(uint8_t(ch));
    }
    std::optional<Dfa> all;
    for (uint32_t r = 0; r < kReadings; ++r)
    {
        if (!(mask >> r & 1) || r == kBinary) continue;
        const auto m = machine(r);
        auto d = walk(*m, m->codepoints() ? cps : bytes, max_states);
        if (!d) return std::nullopt;
        all = all ? unite(*all, *d) : std::move(*d);
        if (all->states() > max_states) return std::nullopt;
    }
    if (!all)
    {
        Dfa none;
        none.base = B;
        return none;
    }
    return all;
}

// ---------------------------------------------------------------- the rule and its ranker

class WrittenRule
{
public:
    const Alphabet* alphabet = nullptr;
    uint32_t mask = 0, base = 0;
    bool bytes = false;           // a line of every byte: read as its own bytes only
    std::optional<Dfa> others;    // O: every chosen reading but binary (nullopt: too large)
    bool binary = false;          // the binary reading chosen, and walked as Bn
    std::string blocker;          // why it cannot count
    // Bn, over what a symbol is to it: 0 the first symbol, 1 the other. Its states' phase: 0
    // before any symbol, 1 with only the first seen, 2 with both.
    std::vector<std::array<int32_t, 2>> bnext;
    std::vector<uint8_t> baccept, bphase;
    int32_t bstart = -1;
    std::vector<uint8_t> space;   // per symbol: whitespace (binary skips it)
    uint32_t spaces = 0, others_count = 0; // whitespace symbols, and the rest

    int32_t bstep(int32_t beta, int32_t& a, int32_t& b, uint32_t c) const
    {
        if (beta < 0 || space[c]) return beta;
        switch (bphase[size_t(beta)])
        {
        case 0: a = int32_t(c); return bnext[size_t(beta)][0];
        case 1:
            if (int32_t(c) == a) return bnext[size_t(beta)][0];
            b = int32_t(c);
            return bnext[size_t(beta)][1];
        default: return int32_t(c) == a ? bnext[size_t(beta)][0] : int32_t(c) == b ? bnext[size_t(beta)][1] : -1;
        }
    }
};

std::shared_ptr<const WrittenRule> written_rule(const Alphabet& a, uint32_t mask)
{
    static std::mutex mx;
    static std::map<std::pair<std::string, uint32_t>, std::shared_ptr<const WrittenRule>> cache;
    const auto key = std::pair{a.id(), mask};
    {
        std::lock_guard<std::mutex> lock(mx);
        if (auto it = cache.find(key); it != cache.end()) return it->second;
    }
    auto r = std::make_shared<WrittenRule>();
    r->alphabet = &a;
    r->mask = mask;
    r->base = a.size();
    r->bytes = holds_all_bytes(a);
    r->others = written_dfa(a, mask);
    if (!r->others) r->blocker = "the readings' automaton is too large for this alphabet: it judges only";
    r->space.resize(r->base);
    bool ascii = true;
    for (uint32_t c = 0; c < r->base; ++c)
    {
        const char32_t cp = a.symbol(c);
        if (cp >= 0x80) ascii = false;
        r->space[c] = space(uint32_t(cp)) ? 1 : 0;
        (r->space[c] ? r->spaces : r->others_count) += 1;
    }
    if (!r->bytes && (mask >> kBinary & 1))
    {
        if (!ascii) r->blocker = "the binary reading of symbols of more than one byte cannot be counted: it judges only";
        else
        {
            // Bn: the binary machine fed 1 for the first symbol and 2 for the other.
            const BinaryMachine m;
            std::unordered_map<St, int32_t, StHash> id;
            std::vector<St> states;
            auto get = [&](const St& st) {
                const auto [it, added] = id.emplace(st, int32_t(states.size()));
                if (added) states.push_back(st);
                return it->second;
            };
            r->bstart = get(m.start());
            for (size_t i = 0; i < states.size(); ++i)
            {
                const St st = states[i];
                const uint8_t phase = st[0] < 0 ? 0 : st[1] < 0 ? 1 : 2;
                std::array<int32_t, 2> nx{-1, -1};
                for (int k = 0; k < 2; ++k)
                {
                    if (phase == 0 && k == 1) continue; // the first symbol is always the first
                    St t = st;
                    if (m.step(t, uint32_t(k + 1))) nx[size_t(k)] = get(t);
                }
                r->bnext.push_back(nx);
                r->baccept.push_back(m.accept(st) ? 1 : 0);
                r->bphase.push_back(phase);
            }
            // Minimised, keeping each state's phase and acceptance (Moore's refinement): states
            // that differ only in bits no signature can still use are merged.
            const size_t n = r->bnext.size();
            std::vector<int32_t> cls(n);
            {
                std::map<std::pair<int, int>, int32_t> first;
                for (size_t i = 0; i < n; ++i)
                    cls[i] = first.emplace(std::pair{int(r->baccept[i]), int(r->bphase[i])}, int32_t(first.size())).first->second;
            }
            for (size_t classes = 0;;)
            {
                std::map<std::array<int32_t, 3>, int32_t> by;
                std::vector<int32_t> next(n);
                for (size_t i = 0; i < n; ++i)
                {
                    const auto& nx = r->bnext[i];
                    const std::array<int32_t, 3> k{cls[i], nx[0] < 0 ? -1 : cls[size_t(nx[0])], nx[1] < 0 ? -1 : cls[size_t(nx[1])]};
                    next[i] = by.emplace(k, int32_t(by.size())).first->second;
                }
                cls = std::move(next);
                if (by.size() == classes) break;
                classes = by.size();
            }
            // Renumbered from the start, breadth first.
            std::vector<int32_t> rep; // a state of each new number
            std::map<int32_t, int32_t> of_class;
            auto visit = [&](int32_t s) {
                const auto [it, added] = of_class.emplace(cls[size_t(s)], int32_t(rep.size()));
                if (added) rep.push_back(s);
                return it->second;
            };
            visit(r->bstart);
            std::vector<std::array<int32_t, 2>> bnext;
            std::vector<uint8_t> baccept, bphase;
            for (size_t i = 0; i < rep.size(); ++i)
            {
                const int32_t s = rep[i];
                std::array<int32_t, 2> nx{-1, -1};
                for (int k = 0; k < 2; ++k)
                    if (r->bnext[size_t(s)][size_t(k)] >= 0) nx[size_t(k)] = visit(r->bnext[size_t(s)][size_t(k)]);
                bnext.push_back(nx);
                baccept.push_back(r->baccept[size_t(s)]);
                bphase.push_back(r->bphase[size_t(s)]);
            }
            r->bnext = std::move(bnext);
            r->baccept = std::move(baccept);
            r->bphase = std::move(bphase);
            r->bstart = 0;
            r->binary = true;
        }
    }
    std::lock_guard<std::mutex> lock(mx);
    return cache.emplace(key, std::shared_ptr<const WrittenRule>(r)).first->second;
}

std::string written_blocker(const WrittenRule& rule) { return rule.blocker; }

std::string written_summary(const WrittenRule& rule)
{
    if (!rule.others) return rule.blocker;
    return std::to_string(rule.others->states()) + " states (" + (rule.bytes ? "its own bytes" : "readings but binary") + ")" +
           (rule.binary ? ", binary " + std::to_string(rule.bnext.size()) + " states" : std::string());
}

bool written_by_rule(const WrittenRule& rule, std::span<const uint32_t> unit)
{
    if (!rule.blocker.empty())
    {
        if (rule.bytes) return written_as_bytes(unit).has_value();
        std::u32string t;
        for (uint32_t d : unit) t += rule.alphabet->symbol(d);
        return written_as(t, rule.mask).has_value();
    }
    int32_t o = rule.others->start, beta = rule.binary ? rule.bstart : -1, a = -1, b = -1;
    for (uint32_t c : unit)
    {
        o = rule.others->step(o, c);
        beta = rule.bstep(beta, a, b, c);
    }
    return (o >= 0 && rule.others->accept[size_t(o)]) || (beta >= 0 && rule.baccept[size_t(beta)]);
}

namespace {

// The walk of two-symbol units keeps a count for each (state, length) it has been through: about
// this many bytes each (the key, the number, the hash table's own), so as many as the filter memory
// holds (3,000,000 in 512 MB).
constexpr double kJointEntryBytes = 179;
size_t joint_budget() { return size_t(filter_memory() / kJointEntryBytes); }

// kept = |P| - |P and O| - |P and Bn| + |P and O and Bn| (see the header), from a state of all
// three at once: p in P (none: every unit), q in P-and-O, and Bn's state with its two symbols.
class WrittenRanker : public Ranker
{
public:
    // `p`: the plugins' automaton, minimal (none: every unit); `q`: the units it keeps that a
    // reading writes as a file (p and others, or others alone), minimal: written_ranker builds both.
    WrittenRanker(std::shared_ptr<const WrittenRule> rule, std::optional<Dfa> p, Dfa q, uint32_t length) : rule_(std::move(rule)), length_(length)
    {
        const WrittenRule& r = *rule_;
        if (p)
        {
            p_ = std::move(p);
            pr_ = std::make_unique<DfaRanker>(*p_, length);
        }
        if (q.start >= 0)
        {
            q_ = std::move(q);
            qr_ = std::make_unique<DfaRanker>(*q_, length);
        }
        powers_.push_back(BigUint(1));
        for (uint32_t i = 0; i < length; ++i)
        {
            BigUint n = powers_.back();
            n.mul_small(r.base);
            powers_.push_back(n);
        }
        if (r.binary)
        {
            // |Bn| from each of its states: by what a symbol is to it, times how many symbols are that.
            bn_.assign(length + 1, std::vector<BigUint>(r.bnext.size()));
            for (size_t s = 0; s < r.bnext.size(); ++s) bn_[0][s] = BigUint(r.baccept[s]);
            for (uint32_t k = 1; k <= length; ++k)
                for (size_t s = 0; s < r.bnext.size(); ++s)
                {
                    BigUint sum = bn_[k - 1][s];
                    sum.mul_small(r.spaces);
                    auto add = [&](int32_t t, uint32_t times) {
                        if (t < 0 || times == 0) return;
                        BigUint c = bn_[k - 1][size_t(t)];
                        c.mul_small(times);
                        sum += c;
                    };
                    switch (r.bphase[s])
                    {
                    case 0: add(r.bnext[s][0], r.others_count); break;
                    case 1: add(r.bnext[s][0], 1); add(r.bnext[s][1], r.others_count - 1); break;
                    default: add(r.bnext[s][0], 1); add(r.bnext[s][1], 1); break;
                    }
                    bn_[k][s] = sum;
                }
        }
        start_ = intern({p_ ? p_->start : 0, q_ ? q_->start : -1, -1, -1, r.binary ? r.bstart : -1});
        set_count();
    }
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return rule_->base; }
    State start() const override { return start_; }
    State next(State s, uint32_t c) const override
    {
        if (s == kDead || c >= rule_->base) return kDead;
        Joint j;
        {
            std::lock_guard<std::mutex> lock(mx_);
            j = joints_[size_t(s)];
        }
        Joint n;
        n.p = p_ ? p_->step(j.p, c) : 0;
        if (n.p < 0) return kDead;
        n.q = j.q >= 0 ? q_->step(j.q, c) : -1;
        n.a = j.a;
        n.b = j.b;
        n.beta = rule_->bstep(j.beta, n.a, n.b, c);
        if (n.beta < 0) n.a = n.b = -1;
        return intern(n);
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        std::lock_guard<std::mutex> lock(mx_);
        const Joint j = joints_[size_t(s)];
        // Added before anything is taken away, so no step goes below zero.
        BigUint kept = p_ ? pr_->completions(State(j.p), remaining) : powers_[remaining];
        if (j.beta >= 0 && j.q >= 0) kept += joint(1, j.q, j.a, j.b, j.beta, remaining);
        if (j.q >= 0) kept -= qr_->completions(State(j.q), remaining);
        if (j.beta >= 0) kept -= p_ ? joint(0, j.p, j.a, j.b, j.beta, remaining) : bn_[remaining][size_t(j.beta)];
        return kept;
    }

private:
    struct Joint
    {
        int32_t p = 0, q = -1, a = -1, b = -1, beta = -1;
        bool operator==(const Joint&) const = default;
    };
    struct JointHash
    {
        size_t operator()(const Joint& j) const
        {
            uint64_t h = uint64_t(uint32_t(j.p)) * 0x9E3779B97F4A7C15ull ^ uint64_t(uint32_t(j.q)) * 0xC2B2AE3D27D4EB4Full;
            h ^= (uint64_t(uint32_t(j.a)) << 40) ^ (uint64_t(uint32_t(j.b)) << 20) ^ uint64_t(uint32_t(j.beta));
            return size_t(h ^ (h >> 31));
        }
    };
    struct Key
    {
        uint64_t hi, lo;
        bool operator==(const Key&) const = default;
    };
    struct KeyHash
    {
        size_t operator()(const Key& k) const { return size_t((k.hi * 0x9E3779B97F4A7C15ull) ^ k.lo ^ (k.lo >> 29)); }
    };

    State intern(const Joint& j) const
    {
        std::lock_guard<std::mutex> lock(mx_);
        const auto [it, added] = ids_.emplace(j, State(joints_.size()));
        if (added) joints_.push_back(j);
        return it->second;
    }

    // From a state where Bn knows both its symbols, only those two and whitespace can follow; the
    // automaton from x over just them, renamed (0 the first symbol, 1 the other, 2.. whitespace)
    // and minimised, is all that the count depends on. Pairs that give the same small automaton
    // share their counts.
    struct Small
    {
        Dfa dfa;
        std::vector<uint8_t> universal; // per state: accepts everything from there
    };
    std::pair<int32_t, int32_t> small_of(int d, int32_t x, int32_t a, int32_t b) const
    {
        const Key where{uint64_t(uint32_t(d)) << 63 | uint64_t(uint32_t(x)), uint64_t(uint32_t(a)) << 32 | uint32_t(b)};
        if (auto it = small_at_.find(where); it != small_at_.end()) return it->second;
        const WrittenRule& w = *rule_;
        const Dfa& D = d == 0 ? *p_ : *q_;
        std::vector<uint32_t> sym{uint32_t(a), uint32_t(b)};
        for (uint32_t c = 0; c < w.base; ++c)
            if (w.space[c]) sym.push_back(c);
        Dfa s;
        s.base = uint32_t(sym.size());
        std::unordered_map<int32_t, int32_t> id;
        std::vector<int32_t> order{x};
        id.emplace(x, 0);
        s.start = 0;
        for (size_t i = 0; i < order.size(); ++i)
        {
            s.accept.push_back(D.accept[size_t(order[i])]);
            for (uint32_t c : sym)
            {
                const int32_t t = D.step(order[i], c);
                if (t < 0)
                {
                    s.next.push_back(Dfa::kDead);
                    continue;
                }
                const auto [it, added] = id.emplace(t, int32_t(order.size()));
                if (added) order.push_back(t);
                s.next.push_back(it->second);
            }
        }
        s = minimise(s);
        std::pair<int32_t, int32_t> found{-1, -1};
        if (s.start >= 0)
        {
            std::string key(reinterpret_cast<const char*>(s.next.data()), s.next.size() * sizeof(int32_t));
            key.append(reinterpret_cast<const char*>(s.accept.data()), s.accept.size());
            const auto [it, added] = small_id_.emplace(key, int32_t(smalls_.size()));
            if (added)
            {
                Small sm;
                sm.universal.resize(s.states());
                for (size_t st = 0; st < s.states(); ++st)
                {
                    bool u = s.accept[st] != 0;
                    for (uint32_t c = 0; c < s.base && u; ++c) u = s.next[st * s.base + c] == int32_t(st);
                    sm.universal[st] = u ? 1 : 0;
                }
                sm.dfa = std::move(s);
                smalls_.push_back(std::move(sm));
            }
            found = {it->second, smalls_[size_t(it->second)].dfa.start};
        }
        return small_at_.emplace(where, found).first->second;
    }

    // Continuations of `r` symbols (0 the first symbol, 1 the other, 2.. whitespace) that small
    // automaton k accepts from s and Bn accepts from beta.
    BigUint small_joint(int32_t k, int32_t s, int32_t beta, uint32_t r) const
    {
        const Small& sm = smalls_[size_t(k)];
        if (r == 0) return BigUint(sm.dfa.accept[size_t(s)] && rule_->baccept[size_t(beta)] ? 1 : 0);
        if (sm.universal[size_t(s)]) return bn_[r][size_t(beta)];
        const Key key{uint64_t(1) << 62 | uint64_t(uint32_t(k)) << 24 | uint64_t(uint32_t(beta)), uint64_t(uint32_t(s)) << 32 | r};
        if (auto it = memo_.find(key); it != memo_.end()) return it->second;
        BigUint sum;
        for (uint32_t c = 0; c < sm.dfa.base; ++c)
        {
            const int32_t t = sm.dfa.next[size_t(s) * sm.dfa.base + c];
            if (t < 0) continue;
            const int32_t nbeta = c < 2 ? rule_->bnext[size_t(beta)][c] : beta;
            if (nbeta < 0) continue;
            sum += small_joint(k, t, nbeta, r - 1);
        }
        if (memo_.size() >= joint_budget()) throw std::length_error("the walk of two-symbol units is over its budget");
        return memo_.emplace(key, sum).first->second;
    }

    // Continuations of `r` symbols that automaton d (0: P, 1: P-and-O) accepts from x and that Bn
    // accepts from beta with its symbols a and b: before both are known, walked with the symbols
    // named; after, in the small automaton of the pair (above).
    BigUint joint(int d, int32_t x, int32_t a, int32_t b, int32_t beta, uint32_t r) const
    {
        const WrittenRule& w = *rule_;
        const Dfa& D = d == 0 ? *p_ : *q_;
        if (r == 0) return BigUint(D.accept[size_t(x)] && w.baccept[size_t(beta)] ? 1 : 0);
        if (w.bphase[size_t(beta)] == 2)
        {
            const auto [k, s] = small_of(d, x, a, b);
            return k < 0 ? BigUint() : small_joint(k, s, beta, r);
        }
        const Key key{uint64_t(uint32_t(d)) << 63 | uint64_t(uint32_t(x)) << 24 | uint64_t(uint32_t(beta)),
                      uint64_t(uint32_t(a + 1)) << 48 | uint64_t(uint32_t(b + 1)) << 32 | r};
        if (auto it = memo_.find(key); it != memo_.end()) return it->second;
        BigUint sum;
        for (uint32_t c = 0; c < w.base; ++c)
        {
            const int32_t y = D.step(x, c);
            if (y < 0) continue;
            int32_t na = a, nb = b;
            const int32_t nbeta = w.bstep(beta, na, nb, c);
            if (nbeta < 0) continue;
            sum += joint(d, y, na, nb, nbeta, r - 1);
        }
        if (memo_.size() >= joint_budget()) throw std::length_error("the walk of two-symbol units is over its budget");
        return memo_.emplace(key, sum).first->second;
    }

    std::shared_ptr<const WrittenRule> rule_;
    uint32_t length_;
    std::optional<Dfa> p_, q_;
    std::unique_ptr<DfaRanker> pr_, qr_;
    std::vector<BigUint> powers_;
    std::vector<std::vector<BigUint>> bn_;
    State start_ = kDead;
    mutable std::mutex mx_;
    mutable std::vector<Joint> joints_;
    mutable std::unordered_map<Joint, State, JointHash> ids_;
    mutable std::unordered_map<Key, BigUint, KeyHash> memo_;
    mutable std::unordered_map<Key, std::pair<int32_t, int32_t>, KeyHash> small_at_;
    mutable std::unordered_map<std::string, int32_t> small_id_;
    mutable std::vector<Small> smalls_;
};

} // namespace

std::unique_ptr<Ranker> written_ranker(std::shared_ptr<const WrittenRule> rule, const Dfa* keep, uint32_t length, std::string& why, double* need)
{
    if (!rule->blocker.empty())
    {
        why = rule->blocker;
        return nullptr;
    }
    // The tables: P's (the plugins' automaton), P-and-O's (the units it keeps that a reading writes
    // as a file), and Bn's. P-and-O is built and measured, not guessed: most readings die within a
    // few symbols of a page, so it is usually far smaller than P (1,559 states against 236,034 for
    // every text plugin at 32 characters, where a bound of P times O's states had it at 13 GB).
    const auto bytes_of = [&](size_t states) { return DfaRanker::table_bytes(states, rule->base, length); };
    const double per_state = bytes_of(1);
    std::optional<Dfa> p;
    if (keep) p = minimise(*keep);
    const double fixed = (p ? bytes_of(p->states()) : 0.0) + bytes_of(rule->bnext.size());
    if (need) *need = fixed;
    if (fixed > filter_memory())
    {
        why = keep ? over_table_limit("the plugins' combined table", fixed) + ": they judge only"
                   : over_table_limit("not-written-v1's tables", fixed) + ": it judges only";
        return nullptr;
    }
    Dfa q;
    try
    {
        // As many pairs as could still fit, with room for the minimising to shrink them.
        const size_t room = size_t(std::max(1.0, (filter_memory() - fixed) / per_state)) * 4;
        q = p ? intersect(*p, *rule->others, room) : minimise(*rule->others);
    }
    catch (const std::length_error&)
    {
        why = over_table_limit("not-written-v1's tables") + ": it judges only";
        return nullptr;
    }
    if (need) *need = fixed + bytes_of(q.states());
    if (fixed + bytes_of(q.states()) > filter_memory())
    {
        why = over_table_limit("not-written-v1's tables", fixed + bytes_of(q.states())) + ": it judges only";
        return nullptr;
    }
    try
    {
        return std::make_unique<WrittenRanker>(std::move(rule), std::move(p), std::move(q), length);
    }
    catch (const std::length_error&)
    {
        why = "not-written-v1 with these filters walks too many two-symbol units to count: it judges only";
        return nullptr;
    }
}

// ---------------------------------------------------------------- other lines in text, and packed bits

const std::vector<std::string>& other_line_forms() { static const std::vector<std::string> f = {"notes", "obj"}; return f; }

uint32_t other_line_mask_of(const std::string& name)
{
    if (name == "all") return 3;
    if (name == "notes") return 1;
    if (name == "obj") return 2;
    throw std::invalid_argument("not-other-line-v1: unknown form '" + name + "'");
}

std::optional<std::string> other_line_as(const std::u32string& text, uint32_t mask)
{
    // Decided outright, from the rules in written.hpp, not by walking the machines.
    std::string t;
    for (char32_t c : text)
    {
        if (c >= 0x80) return std::nullopt; // every form is ASCII
        t += char(c);
    }
    if (mask & 1)
    {
        bool ok = true, any = false;
        size_t i = 0;
        while (ok && i < t.size())
        {
            if (note_sep(uint8_t(t[i]))) { ++i; continue; }
            size_t j = i;
            while (j < t.size() && !note_sep(uint8_t(t[j]))) ++j;
            const std::string tok = t.substr(i, j - i);
            i = j;
            if (tok == "//") continue;
            size_t k = 0;
            if (tok[0] == 'R') k = 1;
            else if (tok[0] >= 'A' && tok[0] <= 'G')
            {
                k = 1;
                if (k < tok.size() && (tok[k] == '#' || tok[k] == 'b')) ++k;
                if (k < tok.size() && tok[k] >= '0' && tok[k] <= '9') ++k;
                else { ok = false; break; }
            }
            else { ok = false; break; }
            const std::string dur = tok.substr(k);
            static const char* const durs[] = {"", "s", "e", "e.", "q", "q.", "h", "h.", "w"};
            bool known = false;
            for (const char* d : durs) known = known || dur == d;
            ok = known;
            any = true;
        }
        if (ok && any) return std::string("notes");
    }
    if (mask & 2)
    {
        std::string u = t;
        const size_t end = u.find_last_not_of(' ');
        const bool padded = end != std::string::npos && end + 1 < u.size();
        if (end == std::string::npos) u.clear();
        else u.resize(end + 1);
        // Spaces are padding only after the last line: none may stand before a line feed there.
        bool ok = !u.empty(), v = false, f = false;
        (void)padded;
        size_t i = 0;
        while (ok && i <= u.size())
        {
            size_t j = u.find('\n', i);
            if (j == std::string::npos) j = u.size();
            const std::string line = u.substr(i, j - i);
            i = j + 1;
            if (line.empty()) { if (j == u.size()) break; continue; }
            if (line.size() < 2 || (line[0] != 'v' && line[0] != 'f') || line[1] != ' ') { ok = false; break; }
            const bool vert = line[0] == 'v';
            size_t k = 1, fields = 0;
            while (ok && k < line.size())
            {
                if (line[k] != ' ') { ok = false; break; }
                ++k;
                size_t start = k;
                if (vert && k < line.size() && (line[k] == '+' || line[k] == '-')) ++k;
                size_t digits = k;
                while (k < line.size() && line[k] >= '0' && line[k] <= '9') ++k;
                if (k == digits) { ok = false; break; }
                if (!vert && line[start] == '0') { ok = false; break; }
                if (vert && k < line.size() && line[k] == '.')
                {
                    ++k;
                    const size_t dec = k;
                    while (k < line.size() && line[k] >= '0' && line[k] <= '9') ++k;
                    if (k == dec) { ok = false; break; }
                }
                ++fields;
            }
            if (fields != 3) ok = false;
            (vert ? v : f) = true;
            if (j == u.size()) break;
        }
        if (ok && v && f) return std::string("obj");
    }
    return std::nullopt;
}

std::optional<Dfa> other_line_dfa(const Alphabet& a, uint32_t mask, size_t max_states)
{
    const uint32_t B = a.size();
    std::vector<std::vector<uint32_t>> bytes(B);
    for (uint32_t c = 0; c < B; ++c)
        for (char ch : utf8_encode(a.symbol(c))) bytes[c].push_back(uint8_t(ch));
    std::optional<Dfa> all;
    if (mask & 1)
    {
        auto d = walk(NotesMachine(), bytes, max_states);
        if (!d) return std::nullopt;
        all = std::move(*d);
    }
    if (mask & 2)
    {
        auto d = walk(ObjMachine(), bytes, max_states);
        if (!d) return std::nullopt;
        all = all ? unite(*all, *d) : std::move(*d);
    }
    if (!all)
    {
        Dfa none;
        none.base = B;
        return none;
    }
    return all;
}

std::optional<std::string> packed_as(std::span<const uint32_t> digits, uint32_t bits)
{
    Bytes b;
    uint32_t acc = 0, n = 0;
    for (uint32_t d : digits)
    {
        acc = acc << bits | d;
        n += bits;
        while (n >= 8)
        {
            n -= 8;
            b.push_back(uint8_t(acc >> n));
            acc &= (1u << n) - 1;
        }
    }
    return signed_kind(b);
}

Dfa packed_dfa(uint32_t base, uint32_t bits)
{
    std::vector<std::vector<uint32_t>> inputs(base);
    for (uint32_t c = 0; c < base; ++c) inputs[c] = {c};
    return *walk(BitsMachine(bits), inputs, SIZE_MAX);
}

} // namespace sieve
