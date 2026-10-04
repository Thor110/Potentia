// Sieve — file kinds (filekind.hpp): the signature table ("file-kinds-v1"), a file's kind from its
// first bytes, and the binary line's filter by kind, counted and ranked through a small automaton
// over the first 16 bytes (every longer file is a head of 16 and any bytes after it).

#include "sieve/filekind.hpp"
#include "sieve/plugin.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>

namespace sieve {

namespace {

// The table, in the order it is read: the first entry whose bytes all lie within the file wins.
// Each entry is its kind and its fixed bytes (offset, bytes), the bytes as a string with its length.
struct RawSignature
{
    const char* kind;
    std::vector<std::pair<uint32_t, std::string>> parts;
};

const std::vector<RawSignature>& raw_table()
{
    using namespace std::string_literals;
    static const std::vector<RawSignature> t = {
        {"PNG", {{0, "\x89PNG\r\n\x1a\n"s}}},
        {"7Z", {{0, "7z\xBC\xAF\x27\x1C"s}}},
        {"XZ", {{0, "\xFD" "7zXZ\0"s}}},
        {"MANIFEST", {{0, "sieve-manifest"s}}},
        {"BOOK", {{0, "sieve-book"s}}},
        {"GIF", {{0, "GIF87a"s}}},
        {"GIF", {{0, "GIF89a"s}}},
        {"WAV", {{0, "RIFF"s}, {8, "WAVE"s}}},
        {"AVI", {{0, "RIFF"s}, {8, "AVI "s}}},
        {"WEBP", {{0, "RIFF"s}, {8, "WEBP"s}}},
        {"MP4", {{4, "ftyp"s}}},
        {"ZIP", {{0, "PK\x03\x04"s}}},
        {"ZIP", {{0, "PK\x05\x06"s}}},
        {"ELF", {{0, "\x7F" "ELF"s}}},
        {"PDF", {{0, "%PDF"s}}},
        {"RAR", {{0, "Rar!"s}}},
        {"OGG", {{0, "OggS"s}}},
        {"FLAC", {{0, "fLaC"s}}},
        {"MID", {{0, "MThd"s}}},
        {"JPG", {{0, "\xFF\xD8\xFF"s}}},
        {"MP3", {{0, "ID3"s}}},
        {"BZ2", {{0, "BZh"s}}},
        {"GZ", {{0, "\x1F\x8B"s}}},
        {"EXE", {{0, "MZ"s}}},
        {"BMP", {{0, "BM"s}}},
    };
    return t;
}

bool readable(uint8_t b) { return (b >= 0x20 || b == '\t' || b == '\n' || b == '\r') && b != 0x7F; }

size_t kind_index(std::string_view kind)
{
    const auto& all = file_kinds();
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i] == kind) return i;
    throw std::logic_error("unknown file kind");
}

std::string lower(std::string s)
{
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return s;
}

// "01" written n times, then shifted up a byte: 256 + 256^2 + ... + 256^n.
// (Built from its bytes, n ones and a zero, rather than from a hex string twice as long: on a line of
// files of megabytes these run several times for every file found by rank.)
BigUint powers_from_one(uint64_t n)
{
    if (n == 0) return {};
    std::vector<uint8_t> b(size_t(n + 1), 1);
    b.back() = 0;
    return BigUint::from_bytes(b);
}

BigUint bytes_value(const std::vector<uint8_t>& b, size_t from)
{
    if (from >= b.size()) return {};
    return BigUint::from_bytes(std::span<const uint8_t>(b).subspan(from));
}

} // namespace

const std::vector<FileSignature>& file_signatures()
{
    static const std::vector<FileSignature> sigs = [] {
        std::vector<FileSignature> out;
        for (const RawSignature& r : raw_table())
        {
            FileSignature s;
            s.kind = r.kind;
            s.fixed.fill(-1);
            for (const auto& [at, bytes] : r.parts)
                for (size_t i = 0; i < bytes.size(); ++i)
                {
                    s.fixed[at + i] = int16_t(uint8_t(bytes[i]));
                    s.end = std::max<uint32_t>(s.end, uint32_t(at + i + 1));
                }
            out.push_back(s);
        }
        return out;
    }();
    return sigs;
}

const std::vector<std::string>& file_kinds()
{
    static const std::vector<std::string> kinds = [] {
        std::vector<std::string> k{"EMPTY"};
        for (const FileSignature& s : file_signatures())
            if (std::find(k.begin(), k.end(), s.kind) == k.end()) k.push_back(s.kind);
        k.push_back("TXT");
        k.push_back("?");
        return k;
    }();
    return kinds;
}

bool is_signed_kind(std::string_view kind) { return kind != "EMPTY" && kind != "TXT" && kind != "?"; }

std::string file_kind(std::span<const uint8_t> head, uint64_t size)
{
    if (size == 0) return "EMPTY";
    const size_t h = size_t(std::min<uint64_t>(size, kKindHead));
    if (head.size() < h) throw std::invalid_argument("file_kind needs the file's first bytes");
    for (const FileSignature& s : file_signatures())
    {
        if (s.end > h) continue;
        bool ok = true;
        for (size_t p = 0; p < s.end && ok; ++p) ok = s.fixed[p] < 0 || head[p] == uint8_t(s.fixed[p]);
        if (ok) return s.kind;
    }
    for (size_t p = 0; p < h; ++p)
        if (!readable(head[p])) return "?";
    return "TXT";
}

const std::vector<std::string>& kind_set_names()
{
    static const std::vector<std::string> names = [] {
        std::vector<std::string> n{"signed", "text", "signed-or-text", "unknown", "empty", "any"};
        for (const std::string& k : file_kinds())
            if (is_signed_kind(k)) n.push_back(lower(k));
        return n;
    }();
    return names;
}

KindSet kind_set_of(const std::string& name)
{
    const auto& all = file_kinds();
    KindSet s(all.size(), 0);
    for (size_t i = 0; i < all.size(); ++i)
    {
        const std::string& k = all[i];
        bool in = false;
        if (name == "signed") in = is_signed_kind(k);
        else if (name == "text") in = k == "TXT";
        else if (name == "signed-or-text") in = is_signed_kind(k) || k == "TXT";
        else if (name == "unknown") in = k == "?";
        else if (name == "empty") in = k == "EMPTY";
        else if (name == "any") in = true;
        else in = is_signed_kind(k) && lower(k) == name;
        s[i] = in ? 1 : 0;
    }
    if (std::find(kind_set_names().begin(), kind_set_names().end(), name) == kind_set_names().end())
        throw std::invalid_argument("binary-kind-v1: unknown kinds '" + name + "'");
    return s;
}

// ---------------------------------------------------------------- KindCounter

KindCounter::KindCounter(uint64_t max_bytes, KindSet kinds) : kinds_(std::move(kinds)), max_bytes_(max_bytes)
{
    const auto& sigs = file_signatures();
    if (kinds_.size() != file_kinds().size()) throw std::invalid_argument("a kind set has one flag per kind");
    if (sigs.size() > 32) throw std::logic_error("too many signatures for the head automaton");
    // Which signatures a byte at position p is consistent with.
    std::array<std::array<uint32_t, 256>, kKindHead> ok{};
    for (size_t p = 0; p < kKindHead; ++p)
        for (uint32_t b = 0; b < 256; ++b)
            for (size_t i = 0; i < sigs.size(); ++i)
                if (sigs[i].fixed[p] < 0 || sigs[i].fixed[p] == int16_t(b)) ok[p][b] |= 1u << i;
    const uint32_t all = sigs.size() == 32 ? ~0u : (1u << sigs.size()) - 1;
    states_[0] = {HeadState{all, true}};
    for (size_t p = 0; p < kKindHead; ++p)
    {
        std::map<std::pair<uint32_t, bool>, uint32_t> id;
        next_[p].resize(states_[p].size() * 256);
        for (size_t s = 0; s < states_[p].size(); ++s)
            for (uint32_t b = 0; b < 256; ++b)
            {
                const HeadState t{states_[p][s].mask & ok[p][b], states_[p][s].text && readable(uint8_t(b))};
                const auto [it, added] = id.emplace(std::pair{t.mask, t.text}, uint32_t(states_[p + 1].size()));
                if (added) states_[p + 1].push_back(t);
                next_[p][s * 256 + b] = it->second;
            }
    }
    // Completions, backwards from each head length.
    for (uint32_t h = 0; h <= kKindHead; ++h)
    {
        comp_[h][h].resize(states_[h].size());
        for (uint32_t s = 0; s < states_[h].size(); ++s) comp_[h][h][s] = BigUint(kinds_[size_t(kind_of(h, s))] ? 1 : 0);
        for (uint32_t p = h; p-- > 0;)
        {
            comp_[h][p].resize(states_[p].size());
            for (uint32_t s = 0; s < states_[p].size(); ++s)
            {
                // Many bytes lead to the same state: count each target once.
                std::map<uint32_t, uint32_t> times;
                for (uint32_t b = 0; b < 256; ++b) ++times[next_[p][s * 256 + b]];
                BigUint& sum = comp_[h][p][s]; // summed in place
                for (const auto& [t, n] : times) sum.add_mul_small(comp_[h][p + 1][t], n);
            }
        }
    }
    count_ = shorter_than(max_bytes_ + 1);
}

int KindCounter::kind_of(uint32_t h, uint32_t s) const
{
    static const size_t empty = kind_index("EMPTY"), txt = kind_index("TXT"), unknown = kind_index("?");
    if (h == 0) return int(empty);
    const auto& sigs = file_signatures();
    const HeadState& st = states_[h][s];
    for (size_t i = 0; i < sigs.size(); ++i)
        if ((st.mask >> i & 1) && sigs[i].end <= h) return int(kind_index(sigs[i].kind));
    return int(st.text ? txt : unknown);
}

BigUint KindCounter::count_of_length(uint64_t length) const
{
    if (length > max_bytes_) return {};
    if (length <= kKindHead) return comp_[length][0][0];
    BigUint c = comp_[kKindHead][0][0];
    c <<= size_t(8 * (length - kKindHead));
    return c;
}

BigUint KindCounter::shorter_than(uint64_t length) const
{
    BigUint total;
    for (uint64_t l = 0; l < length && l <= kKindHead; ++l) total += comp_[size_t(l)][0][0];
    if (length > kKindHead + 1) total += BigUint::mul(comp_[kKindHead][0][0], powers_from_one(length - kKindHead - 1));
    return total;
}

bool KindCounter::passes(std::span<const uint8_t> head, uint64_t size) const
{
    if (size > max_bytes_) return false;
    return kinds_[kind_index(file_kind(head, size))] != 0;
}

BigUint KindCounter::rank(const std::vector<uint8_t>& file) const
{
    const uint64_t length = file.size();
    if (!passes(file, length)) throw std::invalid_argument("the file is not one of the chosen kinds");
    const uint32_t h = uint32_t(std::min<uint64_t>(length, kKindHead));
    BigUint head;
    uint32_t s = 0;
    for (uint32_t p = 0; p < h; ++p)
    {
        std::map<uint32_t, uint32_t> times;
        for (uint32_t b = 0; b < file[p]; ++b) ++times[next_[p][s * 256 + b]];
        for (const auto& [t, n] : times)
        {
            BigUint c = comp_[h][p + 1][t];
            c.mul_small(n);
            head += c;
        }
        s = next_[p][s * 256 + file[p]];
    }
    BigUint r = shorter_than(length);
    if (length > kKindHead)
    {
        head <<= size_t(8 * (length - kKindHead));
        head += bytes_value(file, kKindHead);
    }
    r += head;
    return r;
}

std::vector<uint8_t> KindCounter::unrank(const BigUint& k) const
{
    if (k >= count_) throw std::out_of_range("survivor number beyond the survivors");
    // Its length: the first whose survivors reach past k.
    uint64_t length = 0;
    BigUint r = k;
    for (;;)
    {
        if (length > kKindHead) break;
        const BigUint& c = comp_[size_t(length)][0][0];
        if (r < c) break;
        r -= c;
        ++length;
    }
    uint64_t tail_bytes = 0;
    if (length > kKindHead)
    {
        // Past 16 bytes: length 16 + j holds C * 256^j survivors (C = heads of 16 bytes), so
        // those of 17 .. 16 + j hold C * (256 + ... + 256^j). Find the first j reaching past r.
        const BigUint& c16 = comp_[kKindHead][0][0];
        uint64_t j = std::max<uint64_t>(1, r.bit_length() > c16.bit_length() ? (r.bit_length() - c16.bit_length()) / 8 : 1);
        auto below = [&](uint64_t n) { return BigUint::mul(c16, powers_from_one(n)); }; // lengths 17 .. 16 + n
        while (j > 1 && below(j - 1) > r) --j;
        while (!(below(j) > r)) ++j;
        r -= below(j - 1);
        length = kKindHead + j;
        tail_bytes = j;
    }
    const uint32_t h = uint32_t(std::min<uint64_t>(length, kKindHead));
    BigUint head_rank = r, tail;
    if (tail_bytes)
    {
        head_rank >>= size_t(8 * tail_bytes);
        BigUint top = head_rank;
        top <<= size_t(8 * tail_bytes);
        tail = r;
        tail -= top;
    }
    std::vector<uint8_t> out(size_t(length), 0);
    uint32_t s = 0;
    for (uint32_t p = 0; p < h; ++p)
    {
        uint32_t b = 0;
        for (; b < 256; ++b)
        {
            const BigUint& c = comp_[h][p + 1][next_[p][s * 256 + b]];
            if (head_rank < c) break;
            head_rank -= c;
        }
        if (b == 256) throw std::logic_error("unrank ran past the head's survivors");
        out[p] = uint8_t(b);
        s = next_[p][s * 256 + b];
    }
    if (tail_bytes)
    {
        const std::vector<uint8_t> t = tail.to_bytes(size_t(tail_bytes));
        std::copy(t.begin(), t.end(), out.begin() + std::ptrdiff_t(kKindHead));
    }
    return out;
}

namespace {

// The file at binary-v1 positional index v: its length, and it less the files shorter than it
// (its bytes read as one number).
uint64_t length_at(const BigUint& v)
{
    BigUint t = v;
    t.mul_small(255);
    t.add_small(1);
    return uint64_t((t.bit_length() - 1) / 8);
}

BigUint ones_of(uint64_t n)
{
    if (n == 0) return {};
    return BigUint::from_bytes(std::vector<uint8_t>(size_t(n), 1));
}

// Its first min(length, 16) bytes, and the rest as a number.
std::vector<uint8_t> head_at(const BigUint& v, uint64_t length, BigUint* tail)
{
    BigUint w = v;
    w -= ones_of(length);
    const uint64_t h = std::min<uint64_t>(length, kKindHead);
    const size_t shift = size_t(8 * (length - h));
    BigUint head = w;
    head >>= shift;
    if (tail)
    {
        BigUint top = head;
        top <<= shift;
        *tail = w;
        *tail -= top;
    }
    if (h == 0) return {};
    return head.to_bytes(static_cast<size_t>(h));
}

} // namespace

std::string file_kind_at(const BigUint& index)
{
    const uint64_t length = length_at(index);
    return file_kind(head_at(index, length, nullptr), length);
}

BigUint KindCounter::count_before(const BigUint& index) const
{
    if (index.is_zero()) return {};
    const uint64_t length = length_at(index);
    if (length > max_bytes_) return count_;
    BigUint tail;
    const std::vector<uint8_t> head = head_at(index, length, &tail);
    const uint32_t h = uint32_t(head.size());
    // The heads of h bytes below this one, each followed by any bytes after the head.
    BigUint below;
    uint32_t s = 0;
    for (uint32_t p = 0; p < h; ++p)
    {
        std::map<uint32_t, uint32_t> times;
        for (uint32_t b = 0; b < head[p]; ++b) ++times[next_[p][s * 256 + b]];
        for (const auto& [t, n] : times)
        {
            BigUint c = comp_[h][p + 1][t];
            c.mul_small(n);
            below += c;
        }
        s = next_[p][s * 256 + head[p]];
    }
    below <<= size_t(8 * (length - h));
    BigUint r = shorter_than(length);
    r += below;
    if (kinds_[size_t(kind_of(h, s))]) r += tail; // its own head, with the tails below its own
    return r;
}

KindCounter::PatternTable KindCounter::pattern_table(const Pattern& pat) const
{
    const size_t length = pat.allowed.size();
    const uint32_t h = uint32_t(std::min<size_t>(length, kKindHead));
    // Beyond the head, any allowed bytes: the product of how many each position allows.
    BigUint rest(1);
    for (size_t q = h; q < length; ++q)
    {
        uint32_t n = 0;
        for (bool x : pat.allowed[q]) n += x ? 1u : 0u;
        rest.mul_small(n);
    }
    std::vector<std::vector<BigUint>> t(h + 1);
    t[h].resize(states_[h].size());
    for (uint32_t s = 0; s < states_[h].size(); ++s) t[h][s] = kinds_[size_t(kind_of(h, s))] ? rest : BigUint();
    for (uint32_t p = h; p-- > 0;)
    {
        t[p].resize(states_[p].size());
        for (uint32_t s = 0; s < states_[p].size(); ++s)
        {
            BigUint sum;
            for (uint32_t b = 0; b < 256; ++b)
                if (pat.allowed[p][b]) sum += t[p + 1][next_[p][s * 256 + b]];
            t[p][s] = sum;
        }
    }
    return t;
}

BigUint KindCounter::pattern_count(const Pattern& pat) const
{
    if (pat.allowed.size() > max_bytes_) return {};
    return pattern_table(pat)[0][0];
}

bool KindCounter::pattern_has(const Pattern& pat, const std::vector<uint8_t>& file)
{
    if (file.size() != pat.allowed.size()) return false;
    for (size_t p = 0; p < file.size(); ++p)
        if (!pat.allowed[p][file[p]]) return false;
    return true;
}

BigUint KindCounter::pattern_below(const Pattern& pat, const std::vector<uint8_t>& file) const
{
    if (pat.allowed.size() > max_bytes_ || file.size() < pat.allowed.size()) return {};
    return pattern_below(pat, pattern_table(pat), file);
}

BigUint KindCounter::pattern_below(const Pattern& pat, const PatternTable& t, const std::vector<uint8_t>& file) const
{
    const size_t length = pat.allowed.size();
    if (length > max_bytes_ || file.size() < length) return {};
    if (file.size() > length) return t[0][0];
    const uint32_t h = uint32_t(std::min<size_t>(length, kKindHead));
    BigUint below;
    uint32_t s = 0;
    for (size_t p = 0; p < length; ++p)
    {
        if (p < h)
        {
            for (uint32_t b = 0; b < file[p]; ++b)
                if (pat.allowed[p][b]) below += t[p + 1][next_[p][s * 256 + b]];
            if (!pat.allowed[p][file[p]]) return below;
            s = next_[p][s * 256 + file[p]];
            continue;
        }
        // Past the head the kind is decided: each allowed byte below, times the ways to finish.
        if (!kinds_[size_t(kind_of(h, s))]) return below;
        BigUint after(1);
        for (size_t q = p + 1; q < length; ++q)
        {
            uint32_t n = 0;
            for (bool x : pat.allowed[q]) n += x ? 1u : 0u;
            after.mul_small(n);
        }
        uint32_t smaller = 0;
        for (uint32_t b = 0; b < file[p]; ++b) smaller += pat.allowed[p][b] ? 1u : 0u;
        after.mul_small(smaller);
        below += after;
        if (!pat.allowed[p][file[p]]) return below;
    }
    return below; // the file itself, if a page, is not below itself
}

// ---------------------------------------------------------------- BinarySieve

// ---------------------------------------------------------------- utf8-valid-v1

// The automaton (RFC 3629, table 3-7 of the Unicode standard): state 0 between characters; 1, 2
// and 5 waiting for 1, 2 and 3 more continuation bytes (80..BF); 3 after E0 (A0..BF next, no
// overlong forms), 4 after ED (80..9F next, no surrogates), 6 after F0 (90..BF next), 7 after F4
// (80..8F next, nothing past U+10FFFF), and 8 after C2 when only text is allowed (A0..BF next: no
// C1 controls). C0, C1 and F5..FF never begin a character.
uint8_t Utf8Counter::step(uint8_t s, uint8_t b, bool text_only)
{
    auto in = [&](int lo, int hi) { return b >= lo && b <= hi; };
    switch (s)
    {
    case 0:
        if (b < 0x80)
        {
            if (text_only && ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) || b == 0x7F)) return kDead;
            return 0;
        }
        if (b == 0xC2 && text_only) return 8;
        if (in(0xC2, 0xDF)) return 1;
        if (b == 0xE0) return 3;
        if (b == 0xED) return 4;
        if (in(0xE1, 0xEF)) return 2;
        if (b == 0xF0) return 6;
        if (in(0xF1, 0xF3)) return 5;
        if (b == 0xF4) return 7;
        return kDead;
    case 1: return in(0x80, 0xBF) ? 0 : kDead;
    case 2: return in(0x80, 0xBF) ? 1 : kDead;
    case 3: return in(0xA0, 0xBF) ? 1 : kDead;
    case 4: return in(0x80, 0x9F) ? 1 : kDead;
    case 5: return in(0x80, 0xBF) ? 2 : kDead;
    case 6: return in(0x90, 0xBF) ? 2 : kDead;
    case 7: return in(0x80, 0x8F) ? 2 : kDead;
    case 8: return in(0xA0, 0xBF) ? 0 : kDead;
    default: return kDead;
    }
}

bool Utf8Counter::valid(std::span<const uint8_t> file, bool text_only)
{
    uint8_t s = 0;
    for (uint8_t b : file)
        if ((s = step(s, b, text_only)) == kDead) return false;
    return s == 0;
}

Utf8Counter::Utf8Counter(uint64_t max_bytes, bool text_only) : max_bytes_(max_bytes), text_only_(text_only)
{
    // About kStates x (n + 1) numbers of up to 8n bits: past the filter memory, it judges only.
    if (table_bytes(max_bytes) > filter_memory())
    {
        can_rank_ = false;
        return;
    }
    below_.assign(kStates, {});
    for (uint8_t s = 0; s < kStates; ++s)
        for (uint32_t x = 1; x <= 256; ++x)
        {
            below_[s][x] = below_[s][x - 1];
            if (const uint8_t t = step(s, uint8_t(x - 1), text_only); t != kDead) ++below_[s][x][t];
        }
    ways_.assign(max_bytes + 1, std::vector<BigUint>(kStates));
    ways_[0][0] = BigUint(1);
    for (uint64_t r = 1; r <= max_bytes; ++r)
        for (uint8_t s = 0; s < kStates; ++s)
            for (uint8_t t = 0; t < kStates; ++t)
                if (const uint16_t n = below_[s][256][t]) ways_[r][s].add_mul_small(ways_[r - 1][t], n);
    before_.assign(max_bytes + 2, BigUint());
    for (uint64_t l = 0; l <= max_bytes; ++l) before_[l + 1] = BigUint(before_[l]) += ways_[l][0];
    count_ = before_[max_bytes + 1];
}

BigUint Utf8Counter::rank(const std::vector<uint8_t>& file) const
{
    if (!can_rank_) throw std::logic_error("utf8-valid-v1 cannot rank at this length");
    if (file.size() > max_bytes_ || !valid(file, text_only_)) throw std::invalid_argument("the file is not a survivor");
    BigUint r = before_[file.size()];
    uint8_t s = 0;
    for (size_t i = 0; i < file.size(); ++i)
    {
        const uint64_t left = file.size() - i - 1;
        for (uint8_t t = 0; t < kStates; ++t) // every byte under this one, by where it leads
            if (const uint16_t n = below_[s][file[i]][t]) r.add_mul_small(ways_[left][t], n);
        s = step(s, file[i], text_only_);
    }
    return r;
}

BigUint Utf8Counter::below_value(uint8_t s, uint32_t x, uint64_t left) const
{
    BigUint v;
    for (uint8_t t = 0; t < kStates; ++t)
        if (const uint16_t n = below_[s][x][t]) v.add_mul_small(ways_[left][t], n);
    return v;
}

std::vector<uint8_t> Utf8Counter::unrank(const BigUint& k) const
{
    if (!can_rank_) throw std::logic_error("utf8-valid-v1 cannot rank at this length");
    if (!(k < count_)) throw std::out_of_range("survivor number beyond the survivors");
    uint64_t l = 0;
    while (!(k < before_[l + 1])) ++l;
    BigUint rest = k;
    rest -= before_[l];
    std::vector<uint8_t> out;
    uint8_t s = 0;
    for (uint64_t i = 0; i < l; ++i)
    {
        const uint64_t left = l - i - 1;
        // The byte: the largest b whose bytes below skip no more than `rest` (below_value grows
        // with b, and rest is under the value at 256), found by halving, not byte by byte.
        uint32_t lo = 0, hi = 255;
        while (lo < hi)
        {
            const uint32_t mid = (lo + hi + 1) / 2;
            if (below_value(s, mid, left) <= rest) lo = mid;
            else hi = mid - 1;
        }
        const uint8_t t = step(s, uint8_t(lo), text_only_);
        if (t == kDead) throw std::logic_error("utf8 unrank landed on a byte that cannot follow");
        rest -= below_value(s, lo, left);
        out.push_back(uint8_t(lo));
        s = t;
    }
    return out;
}

// ---------------------------------------------------------------- counting utf8-valid-v1 with the kinds

Scaled::Scaled(double v)
{
    if (v <= 0) return;
    int x = 0;
    m = std::frexp(v, &x);
    e = x;
}

Scaled Scaled::of(const BigUint& v)
{
    const size_t bits = v.bit_length();
    if (bits == 0) return {};
    // Its top 64 bits as a double (rounded there: 53 bits are kept), times 2^(the rest).
    BigUint x = v;
    const size_t drop = bits > 64 ? bits - 64 : 0;
    x >>= drop;
    const uint64_t lo = x.low_bits(32);
    x >>= 32;
    const uint64_t hi = x.low_bits(32);
    Scaled out(double(hi) * 4294967296.0 + double(lo));
    out.e += int64_t(drop);
    return out;
}

Scaled& Scaled::operator+=(const Scaled& o)
{
    if (o.m == 0) return *this;
    if (m == 0) return *this = o;
    // The smaller is lost entirely past 64 bits below the larger, as a double would lose it.
    const int64_t d = e - o.e;
    if (d >= 64) return *this;
    if (d <= -64) return *this = o;
    const int64_t top = std::max(e, o.e);
    const double sum = std::ldexp(m, int(e - top)) + std::ldexp(o.m, int(o.e - top));
    *this = Scaled(sum);
    e += top;
    return *this;
}

Scaled& Scaled::operator-=(const Scaled& o)
{
    if (o.m == 0) return *this;
    const int64_t d = e - o.e;
    if (d >= 64) return *this;
    const double diff = m - std::ldexp(o.m, int(-d));
    const int64_t top = e;
    *this = Scaled(std::max(0.0, diff));
    if (m != 0) e += top;
    return *this;
}

Scaled& Scaled::operator*=(const Scaled& o)
{
    if (m == 0 || o.m == 0) return *this = Scaled();
    const int64_t top = e + o.e;
    *this = Scaled(m * o.m);
    e += top;
    return *this;
}

double Scaled::log10() const
{
    if (m == 0) return -std::numeric_limits<double>::infinity();
    return std::log10(m) + double(e) * std::log10(2.0);
}

namespace {
void times(BigUint& a, const BigUint& b) { a = BigUint::mul(a, b); }
void times(Scaled& a, const Scaled& b) { a *= b; }
} // namespace

template <class T>
T KindCounter::utf8_joint(bool text_only, const std::vector<T>& tail, const Pattern* pat) const
{
    constexpr uint32_t K = Utf8Counter::kStates;
    const uint64_t last = pat ? pat->allowed.size() : max_bytes_; // the longest file counted
    if (pat && last > max_bytes_) return T();
    const size_t head = size_t(std::min<uint64_t>(kKindHead, last));
    // cur[s * K + u]: heads of p bytes through kind state s and UTF-8 state u.
    std::vector<T> cur(states_[0].size() * K);
    cur[0] = T(1);
    T total;
    auto files_of = [&](size_t h) { // the files of exactly h <= 16 bytes: whole heads
        if (pat && h != last) return;
        for (uint32_t s = 0; s < states_[h].size(); ++s)
            if (kinds_[size_t(kind_of(uint32_t(h), s))]) total += cur[s * K];
    };
    files_of(0);
    for (size_t p = 0; p < head; ++p)
    {
        std::vector<T> nxt(states_[p + 1].size() * K);
        for (uint32_t s = 0; s < states_[p].size(); ++s)
            for (uint32_t u = 0; u < K; ++u)
            {
                const T& c = cur[s * K + u];
                if (c.is_zero()) continue;
                // Many bytes lead to the same pair of states: count each pair once.
                std::map<uint64_t, uint32_t> pairs;
                for (uint32_t b = 0; b < 256; ++b)
                {
                    if (pat && !pat->allowed[p][b]) continue;
                    const uint8_t t = Utf8Counter::step(uint8_t(u), uint8_t(b), text_only);
                    if (t == Utf8Counter::kDead) continue;
                    ++pairs[uint64_t(next_[p][s * 256 + b]) * K + t];
                }
                for (const auto& [key, n] : pairs)
                {
                    T add = c;
                    add.mul_small(n);
                    nxt[size_t(key)] += add;
                }
            }
        cur = std::move(nxt);
        files_of(p + 1);
    }
    if (last <= kKindHead) return total;
    // Longer files: the kind is decided by the head; the rest need only finish on a whole character.
    std::vector<T> finish = tail;
    if (pat)
    {
        // The pattern's one length: backwards over its places after the head, ending between characters.
        std::vector<T> w(K);
        w[0] = T(1);
        for (uint64_t q = last; q-- > kKindHead;)
        {
            std::vector<T> before(K);
            for (uint32_t u = 0; u < K; ++u)
            {
                std::array<uint32_t, K> to{};
                for (uint32_t b = 0; b < 256; ++b)
                    if (pat->allowed[size_t(q)][b])
                        if (const uint8_t t = Utf8Counter::step(uint8_t(u), uint8_t(b), text_only); t != Utf8Counter::kDead) ++to[t];
                for (uint32_t t = 0; t < K; ++t)
                    if (to[t])
                    {
                        T add = w[t];
                        add.mul_small(to[t]);
                        before[u] += add;
                    }
            }
            w = std::move(before);
        }
        finish = std::move(w);
    }
    if (finish.size() != K) throw std::logic_error("utf8 tail: one sum per state");
    for (uint32_t s = 0; s < states_[kKindHead].size(); ++s)
        if (kinds_[size_t(kind_of(kKindHead, s))])
            for (uint32_t u = 0; u < K; ++u)
            {
                if (cur[s * K + u].is_zero() || finish[u].is_zero()) continue;
                T add = cur[s * K + u];
                times(add, finish[u]);
                total += add;
            }
    return total;
}

BigUint KindCounter::utf8_count(bool text_only, const std::vector<BigUint>& tail, const Pattern* pat) const
{
    return utf8_joint<BigUint>(text_only, tail, pat);
}

Scaled KindCounter::utf8_estimate(bool text_only, const std::vector<Scaled>& tail, const Pattern* pat) const
{
    return utf8_joint<Scaled>(text_only, tail, pat);
}

std::vector<BigUint> Utf8Counter::tail_sums(uint64_t most) const
{
    if (!can_rank_ || most > max_bytes_) throw std::logic_error("utf8-valid-v1: no table for these sums");
    std::vector<BigUint> out(kStates);
    for (uint64_t r = 1; r <= most; ++r)
        for (uint32_t u = 0; u < kStates; ++u) out[u] += ways_[r][u];
    return out;
}

std::vector<Scaled> Utf8Counter::tail_estimate(bool text_only, uint64_t most)
{
    // x(r) = (W(r), S(r)): W(r)[u] the ways to finish from u in exactly r bytes, S(r) their sum
    // over 1..r. W(r) = M W(r - 1) and S(r) = S(r - 1) + M W(r - 1), with M[u][t] the bytes that
    // take u to t; so x(most) = A^most x(0), A = [[M, 0], [M, I]], by squaring. Every entry is a
    // count, so a matrix is kept as doubles over one power of two, rescaled after each product.
    constexpr uint32_t K = kStates, N = 2 * K;
    struct Mat
    {
        std::array<double, N * N> a{};
        int64_t e = 0;
    };
    auto rescale = [](double* v, size_t n, int64_t& e) {
        double top = 0;
        for (size_t i = 0; i < n; ++i) top = std::max(top, v[i]);
        if (top == 0) return;
        int x = 0;
        std::frexp(top, &x);
        for (size_t i = 0; i < n; ++i) v[i] = std::ldexp(v[i], -x);
        e += x;
    };
    Mat a;
    for (uint32_t u = 0; u < K; ++u)
        for (uint32_t b = 0; b < 256; ++b)
            if (const uint8_t t = step(uint8_t(u), uint8_t(b), text_only); t != kDead)
            {
                a.a[u * N + t] += 1;      // W' = M W
                a.a[(K + u) * N + t] += 1; // S' = S + M W
            }
    for (uint32_t u = 0; u < K; ++u) a.a[(K + u) * N + K + u] = 1;
    std::array<double, N> x{};
    int64_t xe = 0;
    x[0] = 1; // W(0): only state 0 is already between characters
    for (uint64_t r = most; r; r >>= 1)
    {
        if (r & 1)
        {
            std::array<double, N> y{};
            for (uint32_t i = 0; i < N; ++i)
                for (uint32_t j = 0; j < N; ++j) y[i] += a.a[i * N + j] * x[j];
            x = y;
            xe += a.e;
            rescale(x.data(), N, xe);
        }
        if (r > 1)
        {
            Mat sq;
            for (uint32_t i = 0; i < N; ++i)
                for (uint32_t k = 0; k < N; ++k)
                    if (const double v = a.a[i * N + k]; v != 0)
                        for (uint32_t j = 0; j < N; ++j) sq.a[i * N + j] += v * a.a[k * N + j];
            sq.e = 2 * a.e;
            rescale(sq.a.data(), N * N, sq.e);
            a = sq;
        }
    }
    std::vector<Scaled> out(K);
    for (uint32_t u = 0; u < K; ++u)
    {
        out[u] = Scaled(x[K + u]);
        if (!out[u].is_zero()) out[u].e += xe;
    }
    return out;
}

BinarySieve::BinarySieve(const BinarySpace& space, const std::vector<FilterStack::Entry>& entries, const BinaryItems* items)
{
    provenance_ = space.shape();
    if (items) items_ = *items;
    KindSet all(file_kinds().size(), 1);
    for (const auto& e : entries)
    {
        names_.push_back(e.spec->name());
        if (e.spec->id == "not-an-item")
        {
            const std::string forms = param_value(*e.spec, e.values, "items");
            item_forms_ = forms == "all" ? std::vector<std::string>{"pages", "melodies", "pictures", "models"} : std::vector<std::string>{forms};
            item_name_ = e.spec->name();
            sets_.push_back(KindSet(file_kinds().size(), 1));
            item_filter_.push_back(0);
            provenance_ += "; " + e.spec->name() + "{items=" + forms + "}";
            continue;
        }
        if (e.spec->id == "utf8-valid")
        {
            const std::string controls = param_value(*e.spec, e.values, "controls");
            utf8_text_only_ = controls == "text";
            utf8_filter_ = int(names_.size()) - 1;
            sets_.push_back(KindSet(file_kinds().size(), 1));
            item_filter_.push_back(-1);
            provenance_ += "; " + e.spec->name() + "{controls=" + controls + "}";
            continue;
        }
        if (e.spec->id != "binary-kind") throw std::invalid_argument(e.spec->name() + " does not apply to the binary line");
        const std::string kinds = param_value(*e.spec, e.values, "kinds"), keep = param_value(*e.spec, e.values, "keep");
        KindSet s = kind_set_of(kinds);
        if (keep == "exclude")
            for (auto& f : s) f = f ? 0 : 1;
        for (size_t i = 0; i < all.size(); ++i) all[i] = all[i] && s[i];
        sets_.push_back(std::move(s));
        item_filter_.push_back(-1);
        provenance_ += "; " + e.spec->name() + "{" + std::string(kFileKindsVersion) + " kinds=" + kinds + " keep=" + keep + "}";
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));
    counter_ = std::make_unique<KindCounter>(space.max_bytes(), all);
    count_ = counter_->count();
    // not-an-item-v1: pages as a pattern count exactly; any other form asked for is judged.
    for (const std::string& f : item_forms_)
    {
        if (f == "pages" && items_.pages)
        {
            exclude_pages_ = true;
            pages_in_ = counter_->pattern_count(*items_.pages);
            if (items_.pages->allowed.size() <= counter_->max_bytes()) pages_table_ = counter_->pattern_table(*items_.pages);
            count_ -= pages_in_;
            continue;
        }
        bool judged = false;
        for (const auto& j : items_.judges) judged = judged || j.first == f;
        can_rank_ = false;
        blocker_ = judged ? "not-an-item-v1 judges " + f + " file by file, so the survivors cannot be counted"
                          : "not-an-item-v1 needs the other lines' shapes to recognise " + f;
    }
    // utf8-valid-v1 ranks on its own (its table); with the kind filters or not-an-item-v1's pages
    // it is counted with them (KindCounter::utf8_count), and past its table estimated.
    if (!can_rank_) can_count_ = false; // a form judged file by file: nothing to count
    if (utf8_filter_ >= 0)
    {
        utf8_ = std::make_unique<Utf8Counter>(space.max_bytes(), utf8_text_only_);
        table_bytes_ = Utf8Counter::table_bytes(space.max_bytes());
        const bool alone = names_.size() == 1;
        const uint64_t rest = space.max_bytes() > kKindHead ? space.max_bytes() - kKindHead : 0;
        if (alone && utf8_->can_rank()) count_ = utf8_->count();
        else if (can_count_ && utf8_->can_rank())
        {
            survivors_ = counter_->utf8_count(utf8_text_only_, utf8_->tail_sums(rest));
            if (exclude_pages_) survivors_ -= counter_->utf8_count(utf8_text_only_, {}, &*items_.pages);
            survivors_log10_ = survivors_.is_zero() ? -std::numeric_limits<double>::infinity() : survivors_.log10_approx();
            can_rank_ = false;
            blocker_ = "utf8-valid-v1 is counted with binary-kind-v1 and not-an-item-v1, but ranked only on its own";
        }
        else if (can_count_)
        {
            Scaled n = counter_->utf8_estimate(utf8_text_only_, Utf8Counter::tail_estimate(utf8_text_only_, rest));
            if (exclude_pages_) n -= Scaled::of(counter_->utf8_count(utf8_text_only_, {}, &*items_.pages));
            survivors_log10_ = n.log10();
            count_exact_ = false;
            can_rank_ = false;
            blocker_ = over_table_limit("utf8-valid-v1's table") + ": its survivors are estimated, and it judges only";
        }
        else count_exact_ = false;
    }
    if (can_rank_) survivors_log10_ = count_.is_zero() ? -std::numeric_limits<double>::infinity() : count_.log10_approx();
    shuffle_ = std::make_unique<Shuffle>(count_.is_zero() ? BigUint(1) : count_, space.key(), id_);
    BigUint top = count_;
    if (!top.is_zero()) top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::string BinarySieve::first_failure(std::span<const uint8_t> head, uint64_t size) const
{
    if (names_.empty()) return {};
    const size_t i = kind_index(file_kind(head, size));
    for (size_t f = 0; f < sets_.size(); ++f)
        if (!sets_[f][i]) return names_[f];
    return {};
}

std::string BinarySieve::first_failure_of(const std::vector<uint8_t>& file) const
{
    if (names_.empty()) return {};
    const size_t i = kind_index(file_kind(file, file.size()));
    for (size_t f = 0; f < sets_.size(); ++f)
    {
        if (!sets_[f][i]) return names_[f];
        if (int(f) == utf8_filter_ && !Utf8Counter::valid(file, utf8_text_only_)) return names_[f];
        if (item_filter_[f] < 0) continue;
        for (const std::string& form : item_forms_)
        {
            if (form == "pages" && items_.pages && KindCounter::pattern_has(*items_.pages, file)) return names_[f];
            for (const auto& [name, judge] : items_.judges)
                if (name == form && judge(file)) return names_[f];
        }
    }
    return {};
}

BigUint BinarySieve::rank(const std::vector<uint8_t>& file) const
{
    if (utf8_) return utf8_->rank(file);
    BigUint r = counter_->rank(file);
    if (exclude_pages_ && !pages_table_.empty()) r -= counter_->pattern_below(*items_.pages, pages_table_, file);
    return r;
}

std::vector<uint8_t> BinarySieve::unrank(const BigUint& k) const
{
    if (utf8_) return utf8_->unrank(k);
    if (!exclude_pages_ || pages_table_.empty()) return counter_->unrank(k);
    // Files come shortest first, and every page is a file of exactly the page's length, so a
    // survivor past that length has all the pages below it: it is the kind survivor k + (the
    // pages there are), found directly. Only survivors of up to the page's length need the search
    // below (which on a line of files of megabytes would otherwise unrank one about 150 times).
    {
        const uint64_t page_bytes = items_.pages->allowed.size();
        BigUint kept_up_to = counter_->shorter_than(page_bytes + 1);
        kept_up_to -= pages_in_;
        if (k >= kept_up_to)
        {
            BigUint at = k;
            at += pages_in_;
            return counter_->unrank(at);
        }
    }
    // The smallest j (a place among the kind survivors) with more than k survivors in 0..j, the
    // pages among them taken away: between k and k + (the pages there are).
    auto kept_through = [&](const BigUint& j, std::vector<uint8_t>& f) {
        f = counter_->unrank(j);
        BigUint n = j;
        n.add_small(1);
        if (!pages_table_.empty()) n -= counter_->pattern_below(*items_.pages, pages_table_, f);
        if (KindCounter::pattern_has(*items_.pages, f)) n -= BigUint(1);
        return n;
    };
    BigUint lo = k, hi = k;
    hi += pages_in_;
    std::vector<uint8_t> f;
    while (lo < hi)
    {
        BigUint mid = lo;
        mid += hi;
        mid >>= 1;
        if (kept_through(mid, f) > k) hi = mid;
        else
        {
            lo = mid;
            lo.add_small(1);
        }
    }
    (void)kept_through(lo, f);
    return f;
}

BigUint BinarySieve::index_of(const std::vector<uint8_t>& file, AddressMode m) const
{
    if (!can_rank_) throw std::logic_error("this binary sieve cannot rank: " + blocker_);
    if (!first_failure_of(file).empty() || file.size() > counter_->max_bytes()) throw std::invalid_argument("the file is not a survivor");
    const BigUint r = rank(file);
    return m == AddressMode::Scrambled ? shuffle_->forward(r) : r;
}

BigUint BinarySieve::rank_of_index(const BigUint& index, AddressMode m) const
{
    return m == AddressMode::Scrambled ? shuffle_->inverse(index) : index;
}

std::vector<uint8_t> BinarySieve::file_at(const BigUint& index, AddressMode m) const
{
    if (!can_rank_) throw std::logic_error("this binary sieve cannot rank: " + blocker_);
    if (index >= count_) throw std::out_of_range("compact address beyond the survivors");
    return unrank(rank_of_index(index, m));
}

std::string BinarySieve::hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }

BigUint BinarySieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= count_) throw std::out_of_range("compact address beyond the survivors");
    return v;
}

} // namespace sieve
