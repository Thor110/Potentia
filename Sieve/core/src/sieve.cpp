#include "sieve/sieve.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
#include <thread>
#include <utility>

namespace sieve {

namespace {
constexpr char kSymbols[] = " abcdefghijklmnopqrstuvwxyz";
constexpr uint32_t kBase = 27;

bool is_letter(char c) { return c >= 'a' && c <= 'z'; }

bool sorted_has_prefix(const std::vector<std::string>& v, std::string_view t)
{
    auto it = std::lower_bound(v.begin(), v.end(), t,
                               [](const std::string& a, std::string_view b) { return std::string_view(a) < b; });
    return it != v.end() && std::string_view(*it).substr(0, t.size()) == t;
}
} // namespace

const char* to_string(SieveFilter f)
{
    switch (f)
    {
    case SieveFilter::Clean: return "clean";
    case SieveFilter::Window: return "window";
    case SieveFilter::Words: return "words";
    }
    return "?";
}

// ---------------------------------------------------------------- Dictionary

namespace {

struct WordFile
{
    std::string bytes;
    std::vector<std::string> words; // valid, lowercased; may contain duplicates
    size_t skipped = 0;
};

WordFile read_word_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open dictionary '" + path + "'");
    WordFile f;
    in.seekg(0, std::ios::end);
    f.bytes.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0);
    in.read(f.bytes.data(), static_cast<std::streamsize>(f.bytes.size()));

    const std::string_view all(f.bytes);
    for (size_t pos = 0; pos < all.size();)
    {
        size_t end = all.find('\n', pos);
        if (end == std::string_view::npos) end = all.size();
        size_t a = pos, b = end;
        pos = end + 1;
        while (a < b && std::isspace(static_cast<unsigned char>(all[a]))) ++a;
        while (b > a && std::isspace(static_cast<unsigned char>(all[b - 1]))) --b;
        if (a == b) continue;
        std::string w(all.substr(a, b - a));
        bool ok = true;
        for (char& c : w)
        {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            if (!is_letter(c)) { ok = false; break; }
        }
        if (ok) f.words.push_back(std::move(w));
        else ++f.skipped;
    }
    return f;
}

} // namespace

DictionaryFileInfo Dictionary::inspect_file(const std::string& path)
{
    WordFile f = read_word_file(path);
    std::sort(f.words.begin(), f.words.end());
    DictionaryFileInfo info;
    info.word_count = static_cast<size_t>(std::unique(f.words.begin(), f.words.end()) - f.words.begin());
    info.skipped_lines = f.skipped;
    info.sha256 = Sha256::hex(Sha256::hash(f.bytes));
    return info;
}

Dictionary Dictionary::load_file(const std::string& path)
{
    WordFile f = read_word_file(path);
    Dictionary d;
    d.skipped_ = f.skipped;
    d.sha256_ = Sha256::hex(Sha256::hash(f.bytes));
    d.build(std::move(f.words));
    return d;
}

Dictionary Dictionary::from_words(const std::vector<std::string>& words)
{
    std::string joined;
    for (const auto& w : words) joined += w + "\n";
    Dictionary d;
    d.sha256_ = Sha256::hex(Sha256::hash(joined));
    d.build(words);
    return d;
}

void Dictionary::build(std::vector<std::string> words)
{
    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());
    if (!words.empty() && words.front().empty()) words.erase(words.begin()); // an empty "word" is not a word
    for (const auto& w : words)
        for (char c : w)
            if (!is_letter(c)) throw std::invalid_argument("dictionary words must be a-z only");
    sorted_words_ = words;
    words_.reserve(words.size());
    words_.insert(words.begin(), words.end());
    // All distinct suffixes. A word's suffixes are its reversal's prefixes, so with the reversed
    // words sorted, the new suffixes an entry adds are those longer than its common prefix with
    // the previous entry: deduplicated without hashing, then sorted once.
    std::vector<std::string> reversed(words.rbegin(), words.rend());
    for (auto& r : reversed) std::reverse(r.begin(), r.end());
    std::sort(reversed.begin(), reversed.end());
    const std::string* prev = nullptr;
    for (const auto& r : reversed)
    {
        size_t lcp = 0;
        if (prev)
            while (lcp < r.size() && lcp < prev->size() && r[lcp] == (*prev)[lcp]) ++lcp;
        for (size_t k = lcp + 1; k <= r.size(); ++k) sorted_suffixes_.emplace_back(r.rend() - static_cast<std::ptrdiff_t>(k), r.rend());
        prev = &r;
    }
    std::sort(sorted_suffixes_.begin(), sorted_suffixes_.end());
    suffixes_.reserve(sorted_suffixes_.size());
    suffixes_.insert(sorted_suffixes_.begin(), sorted_suffixes_.end());

    // Per-length histograms. In a sorted list, the distinct prefixes contributed by an entry
    // are those longer than its common prefix with the previous entry.
    auto bump = [](std::vector<uint32_t>& h, size_t k) {
        if (h.size() <= k) h.resize(k + 1, 0);
        ++h[k];
    };
    auto distinct_prefixes = [&](const std::vector<std::string>& sorted, std::vector<uint32_t>& h) {
        const std::string* prev = nullptr;
        for (const auto& s : sorted)
        {
            size_t lcp = 0;
            if (prev)
                while (lcp < s.size() && lcp < prev->size() && s[lcp] == (*prev)[lcp]) ++lcp;
            for (size_t k = lcp + 1; k <= s.size(); ++k) bump(h, k);
            prev = &s;
        }
    };
    for (const auto& w : sorted_words_) bump(words_by_len_, w.size());
    for (const auto& s : sorted_suffixes_) bump(suffixes_by_len_, s.size());
    distinct_prefixes(sorted_words_, prefixes_by_len_);       // prefixes of words
    distinct_prefixes(sorted_suffixes_, substrings_by_len_);  // prefixes of suffixes = substrings
}

bool Dictionary::is_word_prefix(std::string_view t) const { return sorted_has_prefix(sorted_words_, t); }
// A substring of some word is exactly a prefix of some suffix.
bool Dictionary::is_substring(std::string_view t) const { return sorted_has_prefix(sorted_suffixes_, t); }

// ---------------------------------------------------------------- direct test

bool unit_passes(std::string_view u, SieveFilter f, const Dictionary& dict)
{
    const size_t n = u.size();
    bool has_letter = false;
    for (size_t i = 0; i < n; ++i)
    {
        if (u[i] == ' ' && i + 1 < n && u[i + 1] == ' ') return false;
        if (is_letter(u[i])) has_letter = true;
        else if (u[i] != ' ') throw std::invalid_argument("sieve units must be lower27 text");
    }
    if (!has_letter) return false;
    if (f == SieveFilter::Clean) return true;

    size_t i = 0;
    while (i < n)
    {
        if (u[i] == ' ') { ++i; continue; }
        size_t j = i;
        while (j < n && u[j] != ' ') ++j;
        const std::string_view t = u.substr(i, j - i);
        const bool open_left = (i == 0), open_right = (j == n);
        bool ok;
        if (f == SieveFilter::Words) ok = dict.is_word(t);
        else if (open_left && open_right) ok = dict.is_substring(t);
        else if (open_left) ok = dict.is_suffix(t);
        else if (open_right) ok = dict.is_word_prefix(t);
        else ok = dict.is_word(t);
        if (!ok) return false;
        i = j;
    }
    return true;
}

// ---------------------------------------------------------------- brute force

SieveCount sieve_brute(uint32_t length, const Dictionary& dict, unsigned threads)
{
    if (length == 0) throw std::invalid_argument("length must be at least 1");
    threads = std::max(1u, threads);
    std::atomic<uint32_t> next{0};
    std::vector<std::array<uint64_t, 3>> partial(threads, {0, 0, 0});

    auto worker = [&](unsigned t) {
        std::string u(length, ' ');
        std::vector<uint32_t> d(length, 0);
        for (uint32_t first; (first = next.fetch_add(1)) < kBase;)
        {
            std::fill(d.begin(), d.end(), 0);
            d[0] = first;
            for (uint32_t i = 0; i < length; ++i) u[i] = kSymbols[d[i]];
            while (true)
            {
                if (unit_passes(u, SieveFilter::Clean, dict))
                {
                    ++partial[t][0];
                    if (unit_passes(u, SieveFilter::Window, dict))
                    {
                        ++partial[t][1];
                        if (unit_passes(u, SieveFilter::Words, dict)) ++partial[t][2];
                    }
                }
                // Odometer over digits 1..L-1.
                int64_t k = static_cast<int64_t>(length) - 1;
                while (k >= 1)
                {
                    if (++d[k] < kBase) { u[k] = kSymbols[d[k]]; break; }
                    d[k] = 0;
                    u[k] = ' ';
                    --k;
                }
                if (k < 1) break;
            }
        }
    };
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();

    uint64_t c = 0, w = 0, s = 0;
    for (const auto& p : partial) { c += p[0]; w += p[1]; s += p[2]; }
    return {BigUint(c), BigUint(w), BigUint(s)};
}

// ---------------------------------------------------------------- pruned walk

namespace {

struct WalkState
{
    std::string token;      // current token being built
    bool left_open = true;  // token started at position 0
    bool last_space = false;
    bool had_token = false;
};

// Advances the state by one symbol. Returns false if no completion can pass the filter.
bool walk_step(WalkState& s, char c, SieveFilter f, const Dictionary& d)
{
    if (c == ' ')
    {
        if (s.last_space) return false;
        if (!s.token.empty())
        {
            if (f == SieveFilter::Words && !d.is_word(s.token)) return false;
            if (f == SieveFilter::Window && !(s.left_open ? d.is_suffix(s.token) : d.is_word(s.token))) return false;
            s.token.clear();
            s.had_token = true;
        }
        s.left_open = false;
        s.last_space = true;
        return true;
    }
    s.token.push_back(c);
    s.last_space = false;
    if (f == SieveFilter::Words && !d.is_word_prefix(s.token)) return false;
    if (f == SieveFilter::Window && !(s.left_open ? d.is_substring(s.token) : d.is_word_prefix(s.token))) return false;
    return true;
}

bool walk_accept(const WalkState& s, SieveFilter f, const Dictionary& d)
{
    if (!s.token.empty())
    {
        // Right-open token. For Window the prefix/substring condition already holds.
        if (f == SieveFilter::Words && !d.is_word(s.token)) return false;
        return true;
    }
    return s.had_token;
}

void walk(const WalkState& s, uint32_t remaining, SieveFilter f, const Dictionary& d, PrunedResult& r)
{
    if (remaining == 0)
    {
        if (walk_accept(s, f, d)) ++r.survivors;
        return;
    }
    for (uint32_t k = 0; k < kBase; ++k)
    {
        WalkState next = s;
        if (!walk_step(next, kSymbols[k], f, d)) continue;
        ++r.states_explored;
        walk(next, remaining - 1, f, d, r);
    }
}

} // namespace

PrunedResult sieve_pruned(uint32_t length, SieveFilter f, const Dictionary& dict, unsigned threads)
{
    if (length == 0) throw std::invalid_argument("length must be at least 1");
    threads = std::max(1u, threads);
    std::atomic<uint32_t> next{0};
    std::vector<PrunedResult> partial(threads);
    auto worker = [&](unsigned t) {
        for (uint32_t first; (first = next.fetch_add(1)) < kBase;)
        {
            WalkState s;
            if (!walk_step(s, kSymbols[first], f, dict)) continue;
            ++partial[t].states_explored;
            walk(s, length - 1, f, dict, partial[t]);
        }
    };
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();

    PrunedResult r;
    r.states_explored = 1; // root
    for (const auto& p : partial)
    {
        r.survivors += p.survivors;
        r.states_explored += p.states_explored;
    }
    return r;
}

// ---------------------------------------------------------------- exact counting

namespace {

// Units of length L with no two consecutive spaces and at least one letter.
BigUint count_clean(uint32_t length)
{
    BigUint ends_letter(26), ends_space(1);
    for (uint32_t i = 1; i < length; ++i)
    {
        BigUint letter = ends_letter;
        letter += ends_space;
        letter.mul_small(26);
        ends_space = ends_letter;
        ends_letter = letter;
    }
    BigUint total = ends_letter;
    total += ends_space;
    if (length == 1) total = BigUint(26); // the unit " " has no letter
    return total;
}

// Exact count for Window or Words by dynamic programming over token lengths.
//
// Both filters depend on a token only through its length and which dictionary set it must
// belong to, so only the per-length set sizes matter:
//   W[k] words, P[k] word prefixes, X[k] word suffixes, S[k] word substrings of length k.
// F[p] = number of valid completions of positions p..L-1 when position p-1 is a space and a
// token has already been seen:
//   F[L] = 1
//   F[p] = sum_{k : p+k < L} W[k] * F[p+k+1]   (a word, then a space)
//        + (Window ? P[L-p] : W[L-p])          (a final right-open token)
// The unit either starts with a space (then F[1], if L > 1) or with a left-open token of length k:
//   k = L: Window ? S[L] : W[L];   k < L: (Window ? X[k] : W[k]) * F[k+1].
BigUint count_filter(uint32_t L, SieveFilter f, const Dictionary& d)
{
    const bool window = f == SieveFilter::Window;
    auto W = [&](uint32_t k) { return d.words_of_length(k); };
    std::vector<BigUint> F(L + 1);
    F[L] = BigUint(1);
    for (uint32_t p = L; p-- > 1;)
    {
        BigUint t;
        for (uint32_t k = 1; p + k < L; ++k)
        {
            const uint32_t w = W(k);
            if (!w || F[p + k + 1].is_zero()) continue;
            BigUint term = F[p + k + 1];
            term.mul_small(w);
            t += term;
        }
        const uint32_t k = L - p;
        t += BigUint(window ? d.prefixes_of_length(k) : W(k));
        F[p] = t;
    }
    BigUint total;
    if (L > 1) total += F[1];
    for (uint32_t k = 1; k <= L; ++k)
    {
        if (k == L)
        {
            total += BigUint(window ? d.substrings_of_length(L) : W(L));
            continue;
        }
        const uint32_t m = window ? d.suffixes_of_length(k) : W(k);
        if (!m || F[k + 1].is_zero()) continue;
        BigUint term = F[k + 1];
        term.mul_small(m);
        total += term;
    }
    return total;
}

} // namespace

SieveCount sieve_counted(uint32_t length, const Dictionary& dict)
{
    if (length == 0) throw std::invalid_argument("length must be at least 1");
    SieveCount r;
    r.clean = count_clean(length);
    r.window = count_filter(length, SieveFilter::Window, dict);
    r.words = count_filter(length, SieveFilter::Words, dict);
    return r;
}

BigUint prefix_tree_nodes(uint32_t length)
{
    BigUint total(1), level(1);
    for (uint32_t i = 0; i < length; ++i)
    {
        level.mul_small(kBase);
        total += level;
    }
    return total;
}

} // namespace sieve
