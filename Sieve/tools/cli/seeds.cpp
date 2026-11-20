// sieve seeds: a study of seeds on the binary line (REVIEW-ROUND list O, O8). Nothing here writes
// or reads an address anyone keeps; it measures what a seed would buy if one were named.
//
// Each file is a unit of the binary line of its own length (binary-v1): its positional place P is
// its bytes as a number plus the files shorter than it. A seed is the line's key, so the scrambled
// order under seed s puts the file at shuffle-sha256-v1 of P, keyed with s. n seeds are taken
// evenly through the 64-bit seeds (seed i is floor(i 2^64 / n), written in hex). For each seed and
// file: where it lands (its address as a share of the line) and the bits it saves, the bit length
// of P less the bit length of its address (an address written without its leading zeros). A place
// already leads by its first byte's zero bits; an even landing leads by about 2 bits on average, as the
// line's size is just above a power of two. Naming
// one of n seeds costs log2 n bits, so a seed pays only where it saves more than that.
//
// Then the movement: whether a seed that does well for one file does well for another of its kind
// (the correlation of their landings over the seeds), whether a seed's neighbour (its value plus
// one) moves a file near where the seed did, and whether the landings are even (a chi-square over
// sixteen bins). And the same for an affine family, x -> (x + b) mod M with b evenly spaced, whose
// landings are as even but whose neighbours move a file by one place; and whose b solved for a
// landing at 0 (b = M - P) holds the file's own bits.
#include "cli/args.hpp"
#include "cli/locate.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/biguint.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

std::string seed_key(uint64_t v)
{
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
    return buf;
}

// A share of the line: value / size, as a double (for display and the statistics only).
double share(const BigUint& v, const BigUint& size)
{
    const size_t b = size.bit_length();
    return v.ratio_to_power_of_two(b) / size.ratio_to_power_of_two(b);
}

// A file's kind, from its name: what the study groups by.
std::string kind_of(const fs::path& p)
{
    std::string e = p.extension().string();
    for (auto& c : e) c = char(std::tolower(static_cast<unsigned char>(c)));
    if (e == ".jpeg") e = ".jpg";
    if (e == ".midi") e = ".mid";
    if (e == ".txt" || e == ".md" || e == ".html" || e == ".py" || e == ".cpp" || e == ".hpp") return "text";
    if (e.empty()) return "program";
    return e.substr(1);
}

double pearson(const std::vector<double>& x, const std::vector<double>& y)
{
    const size_t n = x.size();
    double mx = 0, my = 0;
    for (size_t i = 0; i < n; ++i) mx += x[i], my += y[i];
    mx /= double(n), my /= double(n);
    double sxy = 0, sxx = 0, syy = 0;
    for (size_t i = 0; i < n; ++i) sxy += (x[i] - mx) * (y[i] - my), sxx += (x[i] - mx) * (x[i] - mx), syy += (y[i] - my) * (y[i] - my);
    return sxx > 0 && syy > 0 ? sxy / std::sqrt(sxx * syy) : 0.0;
}

struct FileRun
{
    std::string name, kind;
    uint64_t bytes = 0;
    size_t place_bits = 0;                 // bit length of P
    double size_share = 0;                 // M / 2^line_bits
    size_t line_bits = 0;                  // bit length of M, the line's size: an address's width in bits
    std::vector<int> address_lead;         // per seed: the address's leading zero bits within that width
    std::vector<int> saved;                // per seed: bits(P) - bits(address)
    std::vector<double> landing, neighbour; // per seed: the address as a share of the line; the neighbour seed's
    std::vector<int> affine_saved;         // per b: the same for (P + b) mod M
    size_t solved_bits = 0;                // bits of b = M - P, the b that lands the file at 0
    double ms_per_seed = 0;
};

void print_table_row(const std::string& label, const std::vector<std::string>& cells)
{
    std::printf("  %-28s", label.c_str());
    for (const auto& c : cells) std::printf(" %10s", c.c_str());
    std::printf("\n");
}

} // namespace

int cmd_seeds(const Args& a)
{
    if (a.positional.empty()) throw std::invalid_argument("name the files to study (sieve seeds FILE...)");
    const uint32_t n = a.get_positive("seeds", 16);
    if (n < 2) throw std::invalid_argument("--seeds takes 2 or more");
    const uint64_t max_bytes = a.has("max-bytes") ? parse_whole(a.get("max-bytes"), "--max-bytes", uint64_t(1) << 32) : 16384;
    const double name_bits = std::log2(double(n));

    // Seed i: floor(i 2^64 / n), evenly through the 64-bit seeds; its neighbour is one more.
    std::vector<uint64_t> seeds(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        BigUint v(1);
        v <<= 64;
        v.mul_small(i);
        v.divmod_small(n);
        seeds[i] = v.limbs().empty() ? 0 : v.limbs()[0];
    }

    std::vector<FileRun> runs;
    for (const auto& arg : a.positional)
    {
        const fs::path p(arg);
        std::vector<uint8_t> bytes = read_file_bytes(p);
        if (bytes.empty()) continue;
        if (bytes.size() > max_bytes) bytes.resize(max_bytes);
        FileRun r;
        r.name = p.filename().string();
        r.kind = kind_of(p);
        r.bytes = bytes.size();
        const BinarySpace plain(bytes.size(), "sieve");
        const BigUint place = plain.index_of(bytes, AddressMode::Positional);
        const BigUint& size = plain.size();
        r.place_bits = place.bit_length();
        r.line_bits = size.bit_length();
        r.size_share = size.ratio_to_power_of_two(r.line_bits);
        const auto t0 = std::chrono::steady_clock::now();
        for (uint32_t i = 0; i < n; ++i)
        {
            const BinarySpace line(bytes.size(), seed_key(seeds[i]));
            const BigUint at = line.index_of(bytes, AddressMode::Scrambled);
            r.saved.push_back(int(r.place_bits) - int(at.bit_length()));
            r.address_lead.push_back(int(r.line_bits) - int(at.bit_length()));
            r.landing.push_back(share(at, size));
            const BinarySpace next(bytes.size(), seed_key(seeds[i] + 1));
            r.neighbour.push_back(share(next.index_of(bytes, AddressMode::Scrambled), size));
        }
        r.ms_per_seed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / (2.0 * n);
        // The affine family: b_i = floor(i M / n), the landing (P + b_i) mod M.
        for (uint32_t i = 0; i < n; ++i)
        {
            BigUint b = size;
            b.mul_small(i);
            b.divmod_small(n);
            BigUint at = place;
            at += b;
            if (at >= size) at -= size;
            r.affine_saved.push_back(int(r.place_bits) - int(at.bit_length()));
        }
        BigUint solved = size;
        solved -= place;
        r.solved_bits = BigUint::mod(solved, size).bit_length();
        runs.push_back(std::move(r));
    }
    if (runs.empty()) throw std::invalid_argument("no file with any bytes to study");

    std::printf("seeds: %u, evenly through the 64-bit seeds (seed i = floor(i 2^64 / %u)); naming one costs %.2f bits\n", n, n, name_bits);
    std::printf("files: %zu, each read to its first %llu bytes at most, each on the binary line of its own length\n\n", runs.size(),
                static_cast<unsigned long long>(max_bytes));

    // Each file: its place, the best seed, and what it saves net of naming it.
    std::printf("each file (bits saved = bit length of its place less that of its address; net = the best less %.2f)\n", name_bits);
    std::printf("  %-28s %-8s %8s %10s %5s %9s %9s %9s %9s %10s\n", "file", "kind", "bytes", "place bits", "lead", "best", "net", "affine", "solved b", "ms a seed");
    double total_net = 0, total_affine_net = 0;
    for (const auto& r : runs)
    {
        const int best = *std::max_element(r.saved.begin(), r.saved.end());
        const int abest = *std::max_element(r.affine_saved.begin(), r.affine_saved.end());
        total_net += best - name_bits;
        total_affine_net += abest - name_bits;
        std::printf("  %-28s %-8s %8llu %10zu %5d %9d %9.2f %9.2f %9zu %10.2f\n", r.name.substr(0, 28).c_str(), r.kind.c_str(),
                    static_cast<unsigned long long>(r.bytes), r.place_bits, int(r.line_bits - r.place_bits), best, best - name_bits, abest - name_bits,
                    r.solved_bits, r.ms_per_seed);
    }
    std::printf("  net over every file: shuffle %.2f bits, affine %.2f bits (a positive figure would be a saving)\n\n", total_net, total_affine_net);

    // The seeds sorted by efficiency: the bits each saves over every file, and by kind.
    std::map<std::string, std::vector<size_t>> by_kind;
    for (size_t f = 0; f < runs.size(); ++f) by_kind[runs[f].kind].push_back(f);
    std::vector<std::pair<int, uint32_t>> order;
    for (uint32_t i = 0; i < n; ++i)
    {
        int s = 0;
        for (const auto& r : runs) s += r.saved[i];
        order.push_back({s, i});
    }
    std::stable_sort(order.begin(), order.end(), [](const auto& x, const auto& y) { return x.first > y.first; });
    std::printf("the seeds by the bits they save over every file (the best %u of %u; a seed that saves 0 lands every file as far out as its place)\n",
                std::min<uint32_t>(n, 8), n);
    std::vector<std::string> head;
    head.push_back("all");
    for (const auto& [k, fs_] : by_kind) head.push_back(k);
    print_table_row("seed", head);
    for (size_t j = 0; j < order.size() && j < 8; ++j)
    {
        const uint32_t i = order[j].second;
        std::vector<std::string> cells{std::to_string(order[j].first)};
        for (const auto& [k, files] : by_kind)
        {
            int s = 0;
            for (size_t f : files) s += runs[f].saved[i];
            cells.push_back(std::to_string(s));
        }
        print_table_row(seed_key(seeds[i]), cells);
    }
    {
        // An address's leading zero bits within the line's width, on average, against the places' own:
        // a seed saves on a file only what its address leads by more than the place already did.
        // An even landing in [0, M) leads by the sum over k of P(address < 2^(b - k)), min(1, 2^(b - k) / M):
        // about 2 here, as M is just above 2^(b - 1), so its top bit is almost never set.
        double mean = 0, place = 0, even = 0;
        for (const auto& r : runs)
        {
            for (int l : r.address_lead) mean += l;
            place += double(r.line_bits - r.place_bits);
            const double top = 1.0 / r.size_share; // 2^b / M
            for (int k = 1; k < 64; ++k) even += std::min(1.0, top / std::ldexp(1.0, k));
        }
        mean /= double(runs.size()) * n;
        place /= double(runs.size());
        even /= double(runs.size());
        std::printf("  leading zero bits of an address, on average: %.3f (an even landing: %.3f); of the places themselves: %.3f\n", mean, even, place);
        std::printf("  (a place leads by its first byte's own zero bits: text's first bytes are below 0x80, so a seed starts behind)\n\n");
    }

    // Movement 1: does a seed good for one file of a kind do well for another? The correlation of
    // two files' landings over the seeds, and where the best seed for one stands for the other.
    std::printf("a seed for one file and another of its kind: the correlation of their landings over the %u seeds\n", n);
    std::printf("  (independent landings give about 0, give or take %.2f; 1 would be a seed that moves both alike)\n", 2.0 / std::sqrt(double(n)));
    bool any_pair = false;
    for (const auto& [k, files] : by_kind)
    {
        if (files.size() < 2) continue;
        double sum = 0, worst = 0;
        int pairs = 0;
        double rank_sum = 0;
        for (size_t x = 0; x < files.size(); ++x)
            for (size_t y = x + 1; y < files.size(); ++y)
            {
                const double c = pearson(runs[files[x]].landing, runs[files[y]].landing);
                sum += c, ++pairs;
                worst = std::max(worst, std::fabs(c));
                // The seed that lands x nearest 0, and how many seeds land y nearer than it does.
                const auto& lx = runs[files[x]].landing;
                const auto& ly = runs[files[y]].landing;
                const size_t bi = size_t(std::min_element(lx.begin(), lx.end()) - lx.begin());
                rank_sum += double(std::count_if(ly.begin(), ly.end(), [&](double v) { return v < ly[bi]; }));
            }
        any_pair = true;
        std::printf("  %-8s %zu files, %d %s: mean correlation %+.3f, largest %.3f; the best seed for one stands at %.1f of %u for the other (by chance %.1f)\n",
                    k.c_str(), files.size(), pairs, pairs == 1 ? "pair" : "pairs", sum / pairs, worst, rank_sum / pairs, n, (n - 1) / 2.0);
    }
    if (!any_pair) std::printf("  (no kind has two files)\n");

    // Movement 2: neighbouring seeds. The mean distance round the loop between where seed s and
    // seed s + 1 put a file (an independent pair: 1/4 on a loop), and the affine family's (1/M).
    {
        double d = 0;
        size_t c = 0;
        for (const auto& r : runs)
            for (uint32_t i = 0; i < n; ++i)
            {
                double x = std::fabs(r.landing[i] - r.neighbour[i]);
                d += std::min(x, 1.0 - x), ++c;
            }
        std::printf("\nneighbouring seeds (s and s + 1): the mean distance round the loop between where they put a file\n");
        std::printf("  shuffle %.4f of the loop (independent places: 0.2500); affine, b and b + 1: one place, 2^-%zu of the loop or less\n",
                    d / double(c), runs.front().line_bits - 1);
    }

    // Movement 3: are the landings even? Sixteen bins over every seed and file.
    {
        std::vector<int> bins(16, 0);
        size_t c = 0;
        double mean = 0;
        for (const auto& r : runs)
            for (double v : r.landing) ++bins[std::min<size_t>(15, size_t(v * 16))], ++c, mean += v;
        const double e = double(c) / 16;
        double chi = 0;
        for (int b : bins) chi += (b - e) * (b - e) / e;
        std::printf("\nlandings: %zu, their mean %.4f of the line (even: 0.5000); chi-square over 16 bins %.2f on 15 degrees of freedom\n", c,
                    mean / double(c), chi);
        std::printf("  (even landings give about 15; above 30.6 would be uneven at the 1%% level)\n  bins:");
        for (int b : bins) std::printf(" %d", b);
        std::printf("\n");
    }

    // The solved affine seed: it lands the file at 0, and holds as many bits as the place did.
    std::printf("\nthe affine seed solved for a landing at 0 (b = M - P): its bits against the place's\n");
    for (const auto& r : runs)
        std::printf("  %-28s place %zu bits, b %zu bits: the seed carries the file\n", r.name.substr(0, 28).c_str(), r.place_bits, r.solved_bits);
    return 0;
}

} // namespace sieve::cli
