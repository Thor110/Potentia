// Sieve — the infinite RAID's pieces (sieve/raid_piece.hpp, IDEAS 19.210).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits.
#include "sieve/raid_piece.hpp"
#include "sieve/hex.hpp"
#include "sieve/raid.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sieve {

namespace {

// GF(2^8) under 0x11D: logarithms and powers of the generator 2.
struct Gf
{
    std::array<uint8_t, 512> exp{};
    std::array<int, 256> log{};
    Gf()
    {
        int x = 1;
        for (int i = 0; i < 255; ++i)
        {
            exp[size_t(i)] = uint8_t(x);
            log[size_t(x)] = i;
            x <<= 1;
            if (x & 0x100) x ^= 0x11D;
        }
        for (int i = 255; i < 512; ++i) exp[size_t(i)] = exp[size_t(i - 255)];
    }
    uint8_t mul(uint8_t a, uint8_t b) const { return (a == 0 || b == 0) ? 0 : exp[size_t(log[a] + log[b])]; }
    uint8_t inv(uint8_t a) const { return exp[size_t(255 - log[a])]; } // (a is never 0 here)
};

const Gf& gf()
{
    static const Gf g;
    return g;
}

// The coefficient of data piece i in parity piece j.
uint8_t cauchy(uint32_t k, uint32_t j, uint32_t i) { return gf().inv(uint8_t((k + j) ^ i)); }

// to += c x from, byte by byte: c's products with every byte worked out first, so each byte is one look-up
// (review 4: about twice as fast as a log and an exp a byte, with the same bytes).
void add_scaled(std::string& to, const std::string& from, uint8_t c)
{
    if (c == 0) return;
    const Gf& g = gf();
    const int lc = g.log[c];
    uint8_t times[256];
    times[0] = 0;
    for (int v = 1; v < 256; ++v) times[v] = g.exp[size_t(g.log[v] + lc)];
    const unsigned char* f = reinterpret_cast<const unsigned char*>(from.data());
    unsigned char* o = reinterpret_cast<unsigned char*>(to.data());
    for (size_t b = 0; b < from.size(); ++b) o[b] ^= times[f[b]];
}

// A whole number as written: decimal, no sign, no leading zero, within 64 bits.
std::optional<uint64_t> whole_number(std::string_view s)
{
    if (s.empty() || s.size() > 20 || (s.size() > 1 && s[0] == '0')) return std::nullopt;
    uint64_t v = 0;
    for (const char c : s)
    {
        if (c < '0' || c > '9') return std::nullopt;
        const uint64_t d = uint64_t(c - '0');
        if (v > (UINT64_MAX - d) / 10) return std::nullopt;
        v = v * 10 + d;
    }
    return v;
}

// A line's words between single spaces, an empty one where two spaces meet (sieve/text.hpp's).
std::vector<std::string> words(const std::string& line) { return split_fields(line, ' '); }

// ceil(length / k), without the sum wrapping past 2^64 (a piece's LENGTH near it made the size 0, and such a
// piece with no data passed, to throw elsewhere than invalid_argument; review 4).
uint64_t piece_size(uint64_t length, uint32_t k) { return length / k + (length % k != 0 ? 1 : 0); }

} // namespace

std::string RaidPiece::bytes() const
{
    return std::string(kRaidPieceVersion) + "\nof " + whole + " " + std::to_string(length) + "\ncode " + std::to_string(k) + " " + std::to_string(m) + " " + std::to_string(index) + "\n" + data;
}

std::string RaidPiece::name() const { return Sha256::hex_hash(bytes()); }

RaidPiece read_raid_piece(const std::string& bytes)
{
    auto bad = [](const std::string& why) { return std::invalid_argument("not a " + std::string(kRaidPieceVersion) + ": " + why); };
    size_t at = 0;
    std::vector<std::string> lines;
    for (int i = 0; i < 3; ++i)
    {
        const size_t lf = bytes.find('\n', at);
        if (lf == std::string::npos || lf - at > 200) throw bad("its head is not three lines");
        lines.push_back(bytes.substr(at, lf - at));
        at = lf + 1;
    }
    if (lines[0] != kRaidPieceVersion) throw bad("its first line is not " + std::string(kRaidPieceVersion));
    const auto of = words(lines[1]), code = words(lines[2]);
    if (of.size() != 3 || of[0] != "of" || !is_sha256_hex(of[1])) throw bad("its second line is not \"of SHA-256 LENGTH\"");
    if (code.size() != 4 || code[0] != "code") throw bad("its third line is not \"code K M INDEX\"");
    const auto length = whole_number(of[2]), k = whole_number(code[1]), m = whole_number(code[2]), index = whole_number(code[3]);
    if (!length || *length == 0) throw bad("its length is not a whole number of bytes, 1 or more");
    if (!k || !m || !index || *k == 0 || *k > kRaidPiecesMost || *m > kRaidPiecesMost - *k) throw bad("K is 1 or more and K + M at most 256, written in decimal");
    if (*index >= *k + *m) throw bad("its index is not below K + M");
    if (uint64_t(bytes.size() - at) != piece_size(*length, uint32_t(*k))) throw bad("its data is not ceil(LENGTH / K) bytes");
    RaidPiece p;
    p.whole = of[1];
    p.length = *length;
    p.k = uint32_t(*k), p.m = uint32_t(*m), p.index = uint32_t(*index);
    p.data = bytes.substr(at);
    return p;
}

std::vector<RaidPiece> raid_pieces(const std::string& whole, uint32_t k, uint32_t m)
{
    if (whole.empty()) throw std::invalid_argument("an empty whole has no pieces");
    if (k == 0 || k > kRaidPiecesMost || m > kRaidPiecesMost - k) throw std::invalid_argument("K is 1 or more and K + M at most 256");
    const uint64_t s = piece_size(whole.size(), k);
    const std::string sha = Sha256::hex_hash(whole);
    std::vector<RaidPiece> out;
    for (uint32_t i = 0; i < k + m; ++i)
    {
        RaidPiece p;
        p.whole = sha, p.length = whole.size(), p.k = k, p.m = m, p.index = i;
        if (i < k)
        {
            const uint64_t from = uint64_t(i) * s;
            p.data = from < whole.size() ? whole.substr(size_t(from), size_t(s)) : std::string();
            p.data.resize(size_t(s), '\0');
        }
        else
        {
            p.data.assign(size_t(s), '\0');
            for (uint32_t d = 0; d < k; ++d) add_scaled(p.data, out[d].data, cauchy(k, i - k, d));
        }
        out.push_back(std::move(p));
    }
    return out;
}

std::string raid_join(const std::vector<RaidPiece>& pieces)
{
    if (pieces.empty()) throw std::invalid_argument("no pieces given");
    const RaidPiece& f = pieces[0];
    std::vector<const RaidPiece*> use;
    std::set<uint32_t> seen;
    for (const RaidPiece& p : pieces)
    {
        if (p.whole != f.whole || p.length != f.length || p.k != f.k || p.m != f.m) throw std::invalid_argument("the pieces are not all of one whole, cut one way");
        if (p.index >= p.k + p.m || p.data.size() != piece_size(p.length, p.k)) throw std::invalid_argument("piece " + std::to_string(p.index) + " is not as its head says");
        if (!seen.insert(p.index).second) throw std::invalid_argument("piece " + std::to_string(p.index) + " is given twice");
        if (use.size() < f.k) use.push_back(&p);
    }
    const uint32_t k = f.k;
    if (use.size() < k) throw std::invalid_argument("only " + std::to_string(use.size()) + " of the " + std::to_string(k) + " pieces needed");
    // The rows of the pieces used, inverted (Gauss-Jordan in GF(2^8)), give each data piece.
    const Gf& g = gf();
    std::vector<std::vector<uint8_t>> a(k, std::vector<uint8_t>(k)), inv(k, std::vector<uint8_t>(k));
    for (uint32_t r = 0; r < k; ++r)
    {
        for (uint32_t c = 0; c < k; ++c) a[r][c] = use[r]->index < k ? uint8_t(use[r]->index == c) : cauchy(k, use[r]->index - k, c);
        inv[r][r] = 1;
    }
    for (uint32_t c = 0; c < k; ++c)
    {
        uint32_t p = c;
        while (p < k && a[p][c] == 0) ++p;
        if (p == k) throw std::invalid_argument("the pieces given cannot be joined"); // (a Cauchy matrix never comes here)
        std::swap(a[p], a[c]);
        std::swap(inv[p], inv[c]);
        const uint8_t s = g.inv(a[c][c]);
        for (uint32_t j = 0; j < k; ++j) a[c][j] = g.mul(a[c][j], s), inv[c][j] = g.mul(inv[c][j], s);
        for (uint32_t r = 0; r < k; ++r)
            if (r != c && a[r][c])
            {
                const uint8_t t = a[r][c];
                for (uint32_t j = 0; j < k; ++j) a[r][j] ^= g.mul(t, a[c][j]), inv[r][j] ^= g.mul(t, inv[c][j]);
            }
    }
    const size_t s = size_t(piece_size(f.length, k));
    std::string whole;
    whole.reserve(size_t(s) * k);
    for (uint32_t d = 0; d < k; ++d)
    {
        std::string part(s, '\0');
        for (uint32_t r = 0; r < k; ++r) add_scaled(part, use[r]->data, inv[d][r]);
        whole += part;
    }
    whole.resize(size_t(f.length));
    if (Sha256::hex_hash(whole) != f.whole) throw std::invalid_argument("the pieces joined are not the whole they name (one is damaged)");
    return whole;
}

std::pair<uint32_t, uint32_t> raid_piece_shape(uint64_t length, const RaidPieceRule& rule)
{
    if (length <= rule.split_above) return {1, rule.small_m};
    return {rule.k, rule.m};
}

std::string RaidTally::text() const
{
    std::ostringstream o;
    o << kRaidTallyVersion << "\n";
    for (const RaidKeeper& k : keepers) o << "keeper " << k.address << " " << k.space << " " << k.largest << " " << k.used << "\n";
    return o.str();
}

RaidTally read_raid_tally(const std::string& text)
{
    auto bad = [](const std::string& why) { return std::invalid_argument("not a " + std::string(kRaidTallyVersion) + ": " + why); };
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line)) throw bad("its first line is not " + std::string(kRaidTallyVersion));
    if (line == std::string(kRaidTallyVersion) + "\r") throw bad("its lines end in CR LF (Windows line endings); a tally's lines end in LF alone");
    if (line != kRaidTallyVersion) throw bad("its first line is not " + std::string(kRaidTallyVersion));
    RaidTally t;
    size_t n = 1;
    while (std::getline(in, line))
    {
        ++n;
        const auto w = words(line);
        const auto space = w.size() == 5 ? whole_number(w[2]) : std::nullopt, largest = w.size() == 5 ? whole_number(w[3]) : std::nullopt,
                   used = w.size() == 5 ? whole_number(w[4]) : std::nullopt;
        if (w.size() != 5 || w[0] != "keeper" || w[1].empty() || !space || !largest || !used)
            throw bad("line " + std::to_string(n) + " is not \"keeper ADDRESS SPACE LARGEST USED\"");
        if (*used > *space) throw bad("line " + std::to_string(n) + " keeps more than the space it gives");
        if (!t.keepers.empty() && !(t.keepers.back().address < w[1])) throw bad("line " + std::to_string(n) + " is out of the order of addresses, or one given twice");
        t.keepers.push_back({w[1], *space, *largest, *used});
    }
    if (!text.empty() && text.back() != '\n') throw bad("its last line does not end");
    return t;
}

std::optional<std::vector<size_t>> raid_place(const std::string& whole, uint32_t n, uint64_t piece_bytes, RaidTally& tally, std::string* why)
{
    std::vector<std::string> addresses;
    for (const RaidKeeper& k : tally.keepers) addresses.push_back(k.address);
    std::vector<size_t> homes;
    std::vector<bool> taken(tally.keepers.size(), false);
    for (uint32_t i = 0; i < n; ++i)
    {
        std::optional<size_t> home;
        for (const size_t c : raid_rank(whole + "/" + std::to_string(i), addresses))
        {
            const RaidKeeper& k = tally.keepers[c];
            if (!taken[c] && piece_bytes <= k.largest && piece_bytes <= k.space - k.used)
            {
                home = c;
                break;
            }
        }
        if (!home)
        {
            // (what was counted for this whole's earlier pieces is taken back: nothing is placed)
            for (uint32_t j = 0; j < i; ++j) tally.keepers[homes[j]].used -= piece_bytes;
            if (why) *why = "piece " + std::to_string(i) + " of " + std::to_string(n) + " (" + std::to_string(piece_bytes) + " bytes): no machine keeping no other piece of it takes a piece so large or has room for it";
            return std::nullopt;
        }
        taken[*home] = true;
        tally.keepers[*home].used += piece_bytes;
        homes.push_back(*home);
    }
    return homes;
}

std::optional<std::vector<RaidMove>> raid_leave(const std::string& whole, uint32_t n, uint64_t piece_bytes, const RaidTally& tally, const std::vector<size_t>& homes,
                                                const std::string& leaving, std::string* why)
{
    if (homes.size() != n) throw std::invalid_argument("a home is given for each piece");
    // (a keeper that is not in the tally: a mistyped address moved nothing and said so, as if it could go; review 4)
    if (std::none_of(tally.keepers.begin(), tally.keepers.end(), [&](const RaidKeeper& k) { return k.address == leaving; }))
        throw std::invalid_argument("no keeper " + leaving + " in the tally");
    RaidTally after;
    std::vector<std::string> before(n);
    for (size_t i = 0; i < n; ++i) before[i] = tally.keepers.at(homes[i]).address;
    for (size_t c = 0; c < tally.keepers.size(); ++c)
    {
        RaidKeeper k = tally.keepers[c];
        if (k.address == leaving) continue;
        // (its share of this whole taken off, to be placed again)
        for (size_t i = 0; i < n; ++i)
            if (homes[i] == c) k.used -= std::min(k.used, piece_bytes);
        after.keepers.push_back(k);
    }
    const auto placed = raid_place(whole, n, piece_bytes, after, why);
    if (!placed) return std::nullopt;
    std::vector<RaidMove> moves;
    for (uint32_t i = 0; i < n; ++i)
        if (after.keepers[(*placed)[i]].address != before[i]) moves.push_back({i, before[i], after.keepers[(*placed)[i]].address});
    // The leaving keeper's own first: it hands them on before it goes.
    std::stable_partition(moves.begin(), moves.end(), [&](const RaidMove& m) { return m.from == leaving; });
    return moves;
}

} // namespace sieve
