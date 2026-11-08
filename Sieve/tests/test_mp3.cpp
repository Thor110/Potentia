// packMP3 in Sieve, tested (8 and 9 November 2026; results/mp3-pmp-2026-11-08.md).
//
//   sieve_mp3_check TESTS_FOLDER
//       tests/mp3/tone.mp3 coded by packMP3 and given back exactly; then its packMP3 form changed,
//       2,000 times (bytes flipped, bytes set to edges, cut short), each decoded: refused, or decoded
//       to some MP3, but never read outside its buffers (under AddressSanitizer this checks that; in
//       any build, that none ends the program). Prints "N damaged forms: R refused, D decoded, 0 failures".
//
//   sieve_mp3_check --folder FOLDER [--log FILE] [--damage N]
//       Every .mp3 under FOLDER (any depth, any case of the extension): coded by packMP3, decoded again
//       and compared byte for byte; with --damage N, its packMP3 form also changed N times and each
//       decoded, as above. One line a file, tab-separated (to FILE where given, else the screen): the
//       result (same, not coded, not an MP3's start, not given back, failed), the MP3's bytes, its
//       packMP3 form's bytes, seconds, the damaged forms refused and decoded, the reason, the path. Then
//       totals. "not given back" or "failed" is what matters: either is a fault to report.

#include "cli/mp3_glue.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> read_all(const std::filesystem::path& p)
{
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

std::string utf8(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

// packMP3's form changed n times, each decoded: (refused, decoded); none may end the program.
std::pair<int, int> damage(const std::vector<uint8_t>& pmp, int n, uint32_t seed)
{
    std::mt19937 rng(seed);
    int refused = 0, decoded = 0;
    for (int k = 0; k < n && pmp.size() > 4; ++k)
    {
        std::vector<uint8_t> c = pmp;
        const int changes = 1 + int(rng() % 8);
        for (int j = 0; j < changes; ++j)
        {
            const size_t at = 2 + (rng() % 4 == 0 ? rng() % std::min<size_t>(40, c.size() - 2) : rng() % (c.size() - 2));
            c[at] = rng() % 3 == 0 ? uint8_t(rng() % 2 ? 0xFF : 0x00) : uint8_t(c[at] ^ (1u + rng() % 255));
        }
        if (rng() % 6 == 0) c.resize(2 + rng() % (c.size() - 2));
        try
        {
            sieve::cli::pmp_to_mp3(c);
            ++decoded;
        }
        catch (const std::exception&)
        {
            ++refused;
        }
    }
    return {refused, decoded};
}

int folder_mode(const std::filesystem::path& root, const std::filesystem::path& log_path, int damaged)
{
    std::FILE* log = stdout;
    if (!log_path.empty())
    {
        std::ofstream make(log_path, std::ios::binary); // (created, so a path in any script opens)
        make.close();
#ifdef _WIN32
        log = _wfopen(log_path.wstring().c_str(), L"ab");
#else
        log = std::fopen(log_path.c_str(), "ab");
#endif
        if (!log)
        {
            std::fprintf(stderr, "cannot write %s\n", utf8(log_path).c_str());
            return 2;
        }
    }
    std::fprintf(log, "result\tmp3 bytes\tpmp bytes\tseconds\tdamaged refused\tdamaged decoded\treason\tpath\n");
    std::map<std::string, int> counts, reasons;
    uint64_t in_same = 0, out_same = 0, files = 0, damaged_refused = 0, damaged_decoded = 0;
    double seconds = 0;
    std::error_code ec;
    const auto opts = std::filesystem::directory_options::skip_permission_denied;
    for (auto it = std::filesystem::recursive_directory_iterator(root, opts, ec); !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
    {
        if (!it->is_regular_file(ec)) continue;
        std::string ext = utf8(it->path().extension());
        for (char& c : ext) c = char(std::tolower(uint8_t(c)));
        if (ext != ".mp3") continue;
        ++files;
        const auto t0 = std::chrono::steady_clock::now();
        std::string result, why;
        uint64_t mp3_bytes = 0, pmp_bytes = 0;
        std::pair<int, int> dmg{0, 0};
        try
        {
            const std::vector<uint8_t> b = read_all(it->path());
            mp3_bytes = b.size();
            const auto pmp = sieve::cli::mp3_to_pmp(b, &why);
            if (!pmp)
                result = why.rfind("not coded", 0) == 0 ? "not coded" : why.rfind("not given back", 0) == 0 ? "not given back" : why;
            else
            {
                pmp_bytes = pmp->size();
                // (given back once more here, apart from the check inside mp3_to_pmp)
                result = sieve::cli::pmp_to_mp3(*pmp) == b ? "same" : "not given back";
                if (result == "same")
                {
                    in_same += mp3_bytes;
                    out_same += pmp_bytes;
                    if (damaged > 0) dmg = damage(*pmp, damaged, uint32_t(files));
                }
            }
        }
        catch (const std::exception& e)
        {
            result = "failed";
            why = e.what();
        }
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        seconds += s;
        damaged_refused += uint64_t(dmg.first);
        damaged_decoded += uint64_t(dmg.second);
        ++counts[result];
        if (result != "same") ++reasons[why.empty() ? result : why];
        std::fprintf(log, "%s\t%llu\t%llu\t%.3f\t%d\t%d\t%s\t%s\n", result.c_str(), (unsigned long long)mp3_bytes, (unsigned long long)pmp_bytes, s, dmg.first,
                     dmg.second, why.c_str(), utf8(it->path()).c_str());
        std::fflush(log);
        if (log != stdout) std::printf("%llu  %s  %s\n", (unsigned long long)files, result.c_str(), utf8(it->path().filename()).c_str());
    }
    if (ec) std::fprintf(stderr, "stopped reading %s: %s\n", utf8(root).c_str(), ec.message().c_str());
    auto totals = [&](std::FILE* o) {
        std::fprintf(o, "\n%llu MP3 file(s), %.1f s\n", (unsigned long long)files, seconds);
        for (const auto& [k, v] : counts) std::fprintf(o, "  %-16s %d\n", k.c_str(), v);
        if (in_same)
            std::fprintf(o, "  given back exactly: %llu bytes as %llu (%.2f%% smaller)\n", (unsigned long long)in_same, (unsigned long long)out_same,
                         100.0 * double(in_same - out_same) / double(in_same));
        if (damaged > 0)
            std::fprintf(o, "  damaged forms: %llu refused, %llu decoded, 0 failures\n", (unsigned long long)damaged_refused, (unsigned long long)damaged_decoded);
        for (const auto& [k, v] : reasons) std::fprintf(o, "  why, %d: %s\n", v, k.c_str());
    };
    totals(stdout);
    if (log != stdout) totals(log);
    if (log != stdout) std::fclose(log);
    return counts.count("not given back") || counts.count("failed") ? 1 : 0;
}

int run(const std::vector<std::filesystem::path>& args)
{
    if (args.empty()) return 2;
    if (args[0] == "--folder")
    {
        if (args.size() < 2) return 2;
        std::filesystem::path log;
        int damaged = 0;
        for (size_t k = 2; k + 1 < args.size(); k += 2)
        {
            if (args[k] == "--log") log = args[k + 1];
            else if (args[k] == "--damage") damaged = std::max(0, std::stoi(utf8(args[k + 1])));
            else return 2;
        }
        return folder_mode(args[1], log, damaged);
    }
    const std::vector<uint8_t> mp3 = read_all(args[0] / "mp3" / "tone.mp3");
    const auto pmp = sieve::cli::mp3_to_pmp(mp3);
    if (!pmp || sieve::cli::pmp_to_mp3(*pmp) != mp3)
    {
        std::printf("tone.mp3 not given back by packMP3: 1 failure\n");
        return 1;
    }
    const int n = 2000;
    const auto [refused, decoded] = damage(*pmp, n, 34);
    std::printf("%d damaged forms: %d refused, %d decoded, 0 failures\n", n, refused, decoded);
    return 0;
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    std::vector<std::filesystem::path> args;
    for (int k = 1; k < argc; ++k) args.emplace_back(argv[k]);
    return run(args);
}
#else
int main(int argc, char** argv)
{
    std::vector<std::filesystem::path> args;
    for (int k = 1; k < argc; ++k) args.emplace_back(argv[k]);
    return run(args);
}
#endif
