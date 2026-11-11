#include "cli/voice.hpp"

#include "cli/dictionaries.hpp"
#include "cli/flac_wav.hpp"
#include "sieve/paths.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace fs = std::filesystem;

namespace sieve::cli {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr size_t kEdge = 512;         // samples a join's spectrum is taken over
constexpr size_t kFade = 64;          // a join's crossfade (3 ms)
constexpr size_t kSlide = 110;        // how far a join may move to line up the waveforms (5 ms)
constexpr size_t kPause = 2646;       // a pause at a comma or full stop (0.12 s)
constexpr size_t kSilenceEdge = 1102; // the silence kept before a clip's first phone and after its last (50 ms)
constexpr size_t kShortest = 200;     // a unit shorter than this is not kept
constexpr double kLengthWeight = 20;  // a take's distance from its usual length, against a join's cost
constexpr int kBins = 33;             // the spectrum's bins at a join: 5, 11, ... 197 of 257

using Edge = std::array<double, kBins>;

// The log power spectrum of 512 samples (fewer: the rest as silence), Hamming-windowed, at every
// sixth bin from the fifth: what a join is weighed by.
Edge edge_of(const int16_t* x, size_t n)
{
    static const std::vector<double> window = [] {
        std::vector<double> w(kEdge);
        for (size_t i = 0; i < kEdge; ++i) w[i] = 0.54 - 0.46 * std::cos(2 * kPi * double(i) / double(kEdge - 1));
        return w;
    }();
    std::vector<std::complex<double>> a(kEdge);
    for (size_t i = 0; i < kEdge; ++i) a[i] = i < n ? double(x[i]) * window[i] : 0.0;
    // An iterative radix-2 FFT.
    for (size_t i = 1, j = 0; i < kEdge; ++i)
    {
        size_t bit = kEdge >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= kEdge; len <<= 1)
    {
        const std::complex<double> w1 = std::polar(1.0, -2 * kPi / double(len));
        for (size_t i = 0; i < kEdge; i += len)
        {
            std::complex<double> w = 1;
            for (size_t k = 0; k < len / 2; ++k, w *= w1)
            {
                const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
            }
        }
    }
    Edge e{};
    for (int b = 0; b < kBins; ++b) e[size_t(b)] = std::log(std::norm(a[size_t(5 + 6 * b)]) + 1e-3);
    return e;
}

double join_cost(const Edge& end, const Edge& start)
{
    double s = 0;
    for (int b = 0; b < kBins; ++b) s += (end[size_t(b)] - start[size_t(b)]) * (end[size_t(b)] - start[size_t(b)]);
    return s;
}

std::string plain(std::string p)
{
    while (!p.empty() && std::isdigit(static_cast<unsigned char>(p.back()))) p.pop_back();
    return p;
}

const char* const kOnes[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
                             "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
const char* const kTens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

// A whole number as words, up to 999,999; longer ones digit by digit.
void number_words(const std::string& digits, std::vector<std::string>& out)
{
    if (digits.size() > 6)
    {
        for (const char c : digits) out.push_back(kOnes[c - '0']);
        return;
    }
    const unsigned long n = std::stoul(digits);
    auto below_thousand = [&](unsigned long v) {
        if (v >= 100)
        {
            out.push_back(kOnes[v / 100]);
            out.push_back("hundred");
            v %= 100;
            if (v) out.push_back("and");
            else return;
        }
        if (v >= 20)
        {
            out.push_back(kTens[v / 10]);
            if (v % 10) out.push_back(kOnes[v % 10]);
        }
        else out.push_back(kOnes[v]);
    };
    if (n >= 1000)
    {
        below_thousand(n / 1000);
        out.push_back("thousand");
        if (n % 1000) below_thousand(n % 1000);
        return;
    }
    below_thousand(n);
}

std::vector<uint8_t> read_file(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read " + u8(p));
    return {std::istreambuf_iterator<char>(f), {}};
}

} // namespace

struct Voice::Impl
{
    struct Take
    {
        uint32_t clip = 0, at = 0, n = 0; // its samples: clips[clip][at, at + n)
        uint32_t k = 0;                    // its place in its clip (k-th of the clip's units)
        double off = 0;                    // its length's distance from its diphone's usual, as a share
        Edge start{}, end{};
    };
    std::vector<std::vector<int16_t>> clips;
    std::map<std::pair<std::string, std::string>, std::vector<Take>> units;
    std::unordered_map<std::string, std::vector<std::string>> dict;
    std::unordered_map<std::string, std::vector<std::string>> rules; // letter to sound (lts.tsv)
    size_t takes = 0;

    // A word CMUdict does not have: a word it has and its ending (printers: printer, -s), else the
    // letter-to-sound rules, else (no rules here) its letters spelt out.
    std::vector<std::string> unknown(const std::string& word) const;

    const std::vector<Take>* find(const std::string& a, const std::string& b) const
    {
        auto it = units.find({a, b});
        return it == units.end() || it->second.empty() ? nullptr : &it->second;
    }
};

Voice::Voice() : impl_(std::make_unique<Impl>()) {}
Voice::~Voice() = default;

size_t Voice::diphones() const { return impl_->units.size(); }
size_t Voice::takes() const { return impl_->takes; }

std::unique_ptr<Voice> Voice::load(const fs::path& folder, const fs::path& clips)
{
    std::unique_ptr<Voice> v(new Voice());
    Impl& I = *v->impl_;
    using Take = Impl::Take;
    // CMUdict: the first pronunciation of each word, without stress.
    {
        std::ifstream in(folder / "cmudict.dict");
        if (!in) throw std::runtime_error("cannot read " + u8(folder / "cmudict.dict"));
        std::string line;
        while (std::getline(in, line))
        {
            if (const size_t hash = line.find('#'); hash != std::string::npos) line.resize(hash);
            std::istringstream ss(line);
            std::string word, p;
            if (!(ss >> word) || word.find('(') != std::string::npos) continue;
            std::vector<std::string> ph;
            while (ss >> p) ph.push_back(plain(p));
            if (!ph.empty()) I.dict.emplace(word, std::move(ph));
        }
    }
    // The letter-to-sound rules (tools/voice/learn_lts.py), where they are: "abc[d]efg<TAB>phones".
    {
        std::ifstream in(folder / "lts.tsv");
        std::string line;
        while (in && std::getline(in, line))
        {
            if (line.empty() || line[0] == '#') continue;
            const size_t tab = line.find('\t');
            if (tab == std::string::npos) continue;
            std::istringstream ss(line.substr(tab + 1));
            std::vector<std::string> ph;
            std::string p;
            while (ss >> p)
                if (p != "-") ph.push_back(p);
            I.rules.emplace(line.substr(0, tab), std::move(ph));
        }
    }
    // The alignment: each clip's phones, in order, with their starts and ends.
    struct Phone
    {
        std::string p;
        uint32_t s, e;
    };
    std::vector<std::pair<std::string, std::vector<Phone>>> aligned;
    {
        std::ifstream in(folder / "alignment.tsv");
        if (!in) throw std::runtime_error("cannot read " + u8(folder / "alignment.tsv"));
        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            std::string clip, p;
            uint32_t s = 0, e = 0;
            if (!(std::getline(ss, clip, '\t') && std::getline(ss, p, '\t') && ss >> s >> e))
                throw std::runtime_error("alignment.tsv: a line that is not clip, phone, start, end");
            if (aligned.empty() || aligned.back().first != clip) aligned.push_back({clip, {}});
            aligned.back().second.push_back({plain(p), s, e});
        }
    }
    // Every clip's units: from the middle of each phone to the middle of the next, with a little of
    // the silence before the first and after the last.
    std::map<std::pair<std::string, std::string>, std::vector<Take>> all;
    for (const auto& [name, phones] : aligned)
    {
        const auto x = flac_mono16(read_file(clips / (name + ".flac")));
        if (!x) throw std::runtime_error(u8(clips / (name + ".flac")) + ": not a FLAC of one channel of 16-bit samples");
        const uint32_t clip = uint32_t(I.clips.size());
        I.clips.push_back(std::move(*x));
        const auto& samples = I.clips.back();
        std::vector<Phone> seq;
        seq.push_back({"SIL", 0, phones.front().s});
        seq.insert(seq.end(), phones.begin(), phones.end());
        seq.push_back({"SIL", phones.back().e, uint32_t(samples.size())});
        std::vector<int64_t> mids;
        for (const Phone& p : seq) mids.push_back((int64_t(p.s) + int64_t(p.e)) / 2);
        mids.front() = std::max<int64_t>(0, int64_t(seq[1].s) - int64_t(kSilenceEdge));
        mids.back() = std::min<int64_t>(int64_t(samples.size()), int64_t(seq[seq.size() - 2].e) + int64_t(kSilenceEdge));
        for (size_t k = 0; k + 1 < seq.size(); ++k)
        {
            const int64_t a = mids[k], b = mids[k + 1];
            if (b - a < int64_t(kShortest) || b > int64_t(samples.size())) continue;
            Take t;
            t.clip = clip;
            t.at = uint32_t(a);
            t.n = uint32_t(b - a);
            t.k = uint32_t(k);
            all[{seq[k].p, seq[k + 1].p}].push_back(t);
        }
    }
    for (auto& [key, list] : all)
    {
        std::vector<uint32_t> lengths;
        for (const Take& t : list) lengths.push_back(t.n);
        std::sort(lengths.begin(), lengths.end());
        const size_t m = lengths.size();
        const double median = m % 2 ? double(lengths[m / 2]) : (double(lengths[m / 2 - 1]) + double(lengths[m / 2])) / 2;
        // (in order of their distance from the usual length, the nearest first: where joins tie, the
        // more usual take is chosen)
        std::stable_sort(list.begin(), list.end(), [&](const Take& a, const Take& b) { return std::abs(double(a.n) - median) < std::abs(double(b.n) - median); });
        for (Take& t : list)
        {
            const int16_t* s = I.clips[t.clip].data() + t.at;
            t.off = std::abs(double(t.n) - median) / median;
            t.start = edge_of(s, t.n);
            t.end = t.n >= kEdge ? edge_of(s + t.n - kEdge, kEdge) : edge_of(s, t.n);
        }
        I.takes += list.size();
    }
    I.units = std::move(all);
    return v;
}

std::optional<std::pair<fs::path, fs::path>> Voice::where(const fs::path& given)
{
    std::vector<fs::path> folders;
    if (!given.empty()) folders.push_back(given);
    else
    {
        // data/voice/lj-speech here or above (a build inside the repository), or voice/lj-speech beside the program
        std::vector<fs::path> starts = {fs::current_path()};
        if (const fs::path exe = install_dir(); !exe.empty()) starts.push_back(exe);
        for (fs::path s : starts)
            for (int up = 0; up < 6 && !s.empty(); ++up, s = s.parent_path())
            {
                folders.push_back(s / "data" / "voice" / "lj-speech");
                folders.push_back(s / "voice" / "lj-speech");
                if (s == s.parent_path()) break;
            }
    }
    std::error_code ec;
    for (const fs::path& f : folders)
    {
        if (!fs::exists(f / "alignment.tsv", ec) || !fs::exists(f / "cmudict.dict", ec)) continue;
        // Its clips beside it, or in the anchors (the repository's own layout).
        for (const fs::path& c : {f / "clips", f.parent_path().parent_path() / "anchors" / "public-domain" / "voice" / "lj-speech" / "clips"})
            if (fs::exists(c, ec)) return std::make_pair(f, c);
    }
    return std::nullopt;
}

std::unique_ptr<Voice> Voice::find(const fs::path& given)
{
    const auto w = where(given);
    if (!w) throw std::runtime_error("cannot find Sieve's voice (data/voice/lj-speech, with its clips)");
    return load(w->first, w->second);
}

std::vector<std::string> Voice::Impl::unknown(const std::string& word) const
{
    std::string w; // (its letters alone)
    for (const char c : word)
        if (c >= 'a' && c <= 'z') w += c;
    if (w.empty()) return {};
    if (auto it = dict.find(w); it != dict.end()) return it->second;
    // A word CMUdict has, with an ending (measured: 58.6% of 2,000 words held out said exactly as
    // CMUdict says them, against 55.0% by the rules alone; results/voice-units-2026-11-09.md).
    static const std::vector<std::string> kVoiced = {"B", "D", "G", "V", "DH", "Z", "ZH", "JH", "M", "N", "NG", "L", "R", "W", "Y",
                                                     "AA", "AE", "AH", "AO", "AW", "AY", "EH", "ER", "EY", "IH", "IY", "OW", "OY", "UH", "UW"};
    auto voiced = [&](const std::string& p) { return std::find(kVoiced.begin(), kVoiced.end(), p) != kVoiced.end(); };
    auto s_end = [&](std::vector<std::string> p) {
        const std::string& l = p.back();
        if (l == "S" || l == "Z" || l == "SH" || l == "ZH" || l == "CH" || l == "JH") p.insert(p.end(), {"IH", "Z"});
        else p.push_back(voiced(l) ? "Z" : "S");
        return p;
    };
    auto ed_end = [&](std::vector<std::string> p) {
        const std::string& l = p.back();
        if (l == "T" || l == "D") p.insert(p.end(), {"IH", "D"});
        else p.push_back(voiced(l) ? "D" : "T");
        return p;
    };
    const std::pair<const char*, int> kEnds[] = {{"s", 0}, {"es", 0}, {"ed", 1}, {"d", 1}, {"ing", 2}, {"er", 3}, {"ers", 4}, {"ly", 5}};
    for (const auto& [suffix, kind] : kEnds)
    {
        const std::string suf = suffix;
        if (w.size() <= suf.size() + 2 || w.compare(w.size() - suf.size(), suf.size(), suf) != 0) continue;
        const std::string stem = w.substr(0, w.size() - suf.size());
        const std::vector<std::string>* base = nullptr;
        for (const std::string& st : {stem, stem + "e"})
            if (!base)
                if (auto it = dict.find(st); it != dict.end()) base = &it->second;
        if (!base && stem.size() > 1 && stem[stem.size() - 1] == stem[stem.size() - 2])
            if (auto it = dict.find(stem.substr(0, stem.size() - 1)); it != dict.end()) base = &it->second;
        if (!base && stem.back() == 'i')
            if (auto it = dict.find(stem.substr(0, stem.size() - 1) + "y"); it != dict.end()) base = &it->second;
        if (!base) continue;
        std::vector<std::string> p = *base;
        switch (kind)
        {
        case 0: return s_end(p);
        case 1: return ed_end(p);
        case 2: p.insert(p.end(), {"IH", "NG"}); return p;
        case 3: p.push_back("ER"); return p;
        case 4: p.insert(p.end(), {"ER", "Z"}); return p;
        default: p.insert(p.end(), {"L", "IY"}); return p;
        }
    }
    std::vector<std::string> out;
    if (!rules.empty())
    {
        // Each letter by its longest context written: three letters each side, then two and three,
        // two and two, one and two, one and one, none and one, the letter alone (each inside the last).
        static const std::pair<int, int> kContexts[] = {{3, 3}, {2, 3}, {2, 2}, {1, 2}, {1, 1}, {0, 1}, {0, 0}};
        const std::string pw = "###" + w + "###";
        for (size_t i = 0; i < w.size(); ++i)
            for (const auto& [l, r] : kContexts)
            {
                const std::string key = pw.substr(3 + i - size_t(l), size_t(l)) + "[" + w[i] + "]" + pw.substr(4 + i, size_t(r));
                if (auto it = rules.find(key); it != rules.end())
                {
                    out.insert(out.end(), it->second.begin(), it->second.end());
                    break;
                }
            }
        return out;
    }
    for (const char c : w) // (no rules here: its letters)
        if (auto it = dict.find(std::string(1, c)); it != dict.end()) out.insert(out.end(), it->second.begin(), it->second.end());
    return out;
}

std::vector<std::string> Voice::phones(const std::string& text) const
{
    const Impl& I = *impl_;
    std::vector<std::string> out = {"SIL"};
    auto add_word = [&](const std::string& w) {
        if (auto it = I.dict.find(w); it != I.dict.end())
        {
            out.insert(out.end(), it->second.begin(), it->second.end());
            return;
        }
        const std::vector<std::string> said = I.unknown(w); // (not in the dictionary)
        out.insert(out.end(), said.begin(), said.end());
    };
    std::string word;
    auto flush = [&] {
        while (!word.empty() && word.front() == '\'') word.erase(word.begin());
        while (!word.empty() && word.back() == '\'') word.pop_back();
        if (!word.empty()) add_word(word);
        word.clear();
    };
    for (size_t i = 0; i < text.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (std::isalpha(c) || (c == '\'' && !word.empty())) word += char(std::tolower(c));
        else if (std::isdigit(c))
        {
            flush();
            std::string digits;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) digits += text[i++];
            --i;
            std::vector<std::string> words;
            number_words(digits, words);
            for (const std::string& w : words) add_word(w);
        }
        else
        {
            flush();
            if (c && std::strchr(",.;:!?", int(c))) out.push_back("SIL");
        }
    }
    flush();
    out.push_back("SIL");
    std::vector<std::string> merged; // (pauses together as one)
    for (const std::string& p : out)
        if (!(p == "SIL" && !merged.empty() && merged.back() == "SIL")) merged.push_back(p);
    return merged;
}

std::vector<int16_t> Voice::speak(const std::string& text) const
{
    const Impl& I = *impl_;
    using Take = Impl::Take;
    const std::vector<std::string> ph = phones(text);
    if (ph.size() < 3) return {};
    // Each pair's takes (a pair never recorded: the first phone before a pause, or the second after one).
    std::vector<const std::vector<Take>*> cands;
    std::vector<bool> pause_before;
    static const std::vector<Take> kNone = {[] {
        Take t;
        t.n = kSilenceEdge;
        t.clip = std::numeric_limits<uint32_t>::max();
        return t;
    }()};
    for (size_t i = 0; i + 1 < ph.size(); ++i)
    {
        const std::vector<Take>* c = I.find(ph[i], ph[i + 1]);
        if (!c) c = I.find(ph[i], "SIL");
        if (!c) c = I.find("SIL", ph[i + 1]);
        cands.push_back(c ? c : &kNone);
        pause_before.push_back(ph[i] == "SIL" && i > 0);
    }
    // The takes chosen all at once: the least joins' cost and lengths' distance from the usual.
    const size_t n = cands.size();
    std::vector<std::vector<double>> cost(n);
    std::vector<std::vector<uint32_t>> back(n);
    cost[0].resize(cands[0]->size());
    for (size_t j = 0; j < cands[0]->size(); ++j) cost[0][j] = kLengthWeight * (*cands[0])[j].off;
    for (size_t i = 1; i < n; ++i)
    {
        const auto& prev = *cands[i - 1];
        const auto& cur = *cands[i];
        cost[i].assign(cur.size(), 0);
        back[i].assign(cur.size(), 0);
        for (size_t c = 0; c < cur.size(); ++c)
        {
            double best = std::numeric_limits<double>::infinity();
            uint32_t arg = 0;
            for (size_t p = 0; p < prev.size(); ++p)
            {
                const bool next_to = prev[p].clip == cur[c].clip && cur[c].clip != std::numeric_limits<uint32_t>::max() && prev[p].k + 1 == cur[c].k;
                const double v = (next_to ? 0.0 : join_cost(prev[p].end, cur[c].start)) + cost[i - 1][p];
                if (v < best)
                {
                    best = v;
                    arg = uint32_t(p);
                }
            }
            cost[i][c] = best + kLengthWeight * cur[c].off;
            back[i][c] = arg;
        }
    }
    std::vector<uint32_t> pick(n);
    pick[n - 1] = uint32_t(std::min_element(cost[n - 1].begin(), cost[n - 1].end()) - cost[n - 1].begin());
    for (size_t i = n - 1; i > 0; --i) pick[i - 1] = back[i][pick[i]];
    // Joined: each lined up on the waveform before it and crossfaded into it.
    std::vector<double> out;
    for (size_t i = 0; i < n; ++i)
    {
        const Take& t = (*cands[i])[pick[i]];
        std::vector<double> seg(t.n, 0.0);
        if (t.clip != std::numeric_limits<uint32_t>::max())
            for (uint32_t k = 0; k < t.n; ++k) seg[k] = I.clips[t.clip][t.at + k];
        if (pause_before[i]) out.insert(out.end(), kPause, 0.0);
        size_t from = 0;
        if (out.size() >= kFade && seg.size() >= kFade + kSlide)
        {
            const double* tail = out.data() + out.size() - kFade;
            double best = -std::numeric_limits<double>::infinity();
            for (size_t o = 0; o < kSlide; ++o)
            {
                double d = 0;
                for (size_t k = 0; k < kFade; ++k) d += tail[k] * seg[o + k];
                if (d > best)
                {
                    best = d;
                    from = o;
                }
            }
        }
        if (out.size() >= kFade && seg.size() - from >= kFade)
        {
            double* tail = out.data() + out.size() - kFade;
            for (size_t k = 0; k < kFade; ++k)
            {
                const double r = double(k) / double(kFade - 1);
                tail[k] = tail[k] * (1 - r) + seg[from + k] * r;
            }
            from += kFade;
        }
        out.insert(out.end(), seg.begin() + std::ptrdiff_t(from), seg.end());
    }
    std::vector<int16_t> pcm(out.size());
    for (size_t i = 0; i < out.size(); ++i) pcm[i] = int16_t(std::clamp(std::lround(out[i]), -32768L, 32767L));
    return pcm;
}

std::vector<uint8_t> wav_of(const std::vector<int16_t>& samples, uint32_t rate)
{
    std::vector<uint8_t> w;
    auto u32 = [&](uint32_t v) {
        for (int i = 0; i < 4; ++i) w.push_back(uint8_t(v >> (8 * i)));
    };
    auto u16 = [&](uint16_t v) {
        w.push_back(uint8_t(v));
        w.push_back(uint8_t(v >> 8));
    };
    const uint32_t bytes = uint32_t(samples.size() * 2);
    w.insert(w.end(), {'R', 'I', 'F', 'F'});
    u32(36 + bytes);
    w.insert(w.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    u32(16);
    u16(1);
    u16(1);
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    w.insert(w.end(), {'d', 'a', 't', 'a'});
    u32(bytes);
    for (const int16_t s : samples) u16(uint16_t(s));
    return w;
}

} // namespace sieve::cli
