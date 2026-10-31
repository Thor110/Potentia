// Sieve CLI — `sieve rope` (see rope.hpp).
#include "cli/rope.hpp"

#include "cli/compare.hpp"
#include "cli/items.hpp" // use_item_maker: mix-v3's word lists
#include "cli/locate.hpp"
#include "sieve/rope_wave.hpp"
#include "sieve/mix.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

fs::path path_of(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

void write_bytes(const std::string& to, const std::vector<uint8_t>& b)
{
    std::ofstream out(path_of(to), std::ios::binary);
    out.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
    out.close(); // (checked once closed: a small file is written as it closes)
    if (!out) throw std::runtime_error("could not write " + to);
}

double seconds_of(const Args& a, const char* key)
{
    const std::string v = a.get(key);
    size_t used = 0;
    double s = -1;
    try
    {
        s = std::stod(v, &used);
    }
    catch (const std::exception&)
    {
    }
    if (used != v.size() || !(s >= 0)) throw std::invalid_argument(std::string("--") + key + ": seconds, 0 or more (0: no limit): " + v);
    return s;
}

std::string pct(uint64_t v, uint64_t of)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.2f%%", of ? 100.0 * double(v) / double(of) : 100.0);
    return b;
}

} // namespace

int cmd_rope(const Args& a)
{
    if (a.has("decode"))
    {
        // The file back from its code, its parameters and its size.
        if (!a.has("params") || !a.has("size") || !a.has("out")) throw std::invalid_argument("rope --decode CODE needs --params, --size and --out");
        const std::vector<uint8_t> code = read_file_bytes(path_of(a.get("decode")));
        const RopeParams p = RopeParams::parse(a.get("params"));
        const std::vector<uint8_t> file = rope_file(code, p, std::stoull(a.get("size")));
        write_bytes(a.get("out"), file);
        std::cout << "file      " << a.get("out") << ": " << file.size() << " bytes, sha256 " << sha256_hex(file) << "\n";
        return 0;
    }
    if (a.positional.empty()) throw std::invalid_argument("rope needs a file (see: sieve help rope)");
    const std::string name = a.positional[0];
    const std::vector<uint8_t> file = read_file_bytes(path_of(name));
    std::cout << "file      " << name << ": " << file.size() << " bytes, sha256 " << sha256_hex(file) << "\n";
    RopeParams p;
    if (a.has("params")) p = RopeParams::parse(a.get("params"));
    else
    {
        // Pulling tight: the best wave for the file, within the time given.
        const double seconds = a.has("seconds") ? seconds_of(a, "seconds") : 0;
        const auto t0 = std::chrono::steady_clock::now();
        const RopeSearch s = rope_search(file, seconds);
        const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        char b[64];
        std::snprintf(b, sizeof b, "%.2f s", took);
        std::cout << "search    " << s.tried << " waves tried in " << b << (s.timed_out ? ", stopped at its time: the best found by then" : "")
                  << "\n";
        std::snprintf(b, sizeof b, "%.3f s", s.best_at);
        std::cout << "best at   wave " << s.best_trial << " of " << s.tried << ", " << b << " into the search\n";
        p = s.best;
    }
    const std::vector<uint8_t> code = rope_code(file, p);
    if (rope_file(code, p, file.size()) != file) throw std::logic_error("rope-wave-v1 did not give the file back (a fault in the coder)");
    const uint64_t lz = lzma2_size(file);
    std::cout << "wave      " << p.text() << " (" << (rope_v6(p) ? kRopeWaveTriangleVersion : p.above ? kRopeWaveAboveVersion : p.grouped ? kRopeWaveGroupsVersion : p.columns == 2 ? kRopeWaveLinesVersion : p.columns ? kRopeWaveColumnsVersion : kRopeWaveVersion) << ")\n"
              << "code      " << code.size() << " bytes, " << pct(code.size(), file.size()) << " of the file; read back and checked\n"
              << "LZMA2     " << lz << " bytes, " << pct(lz, file.size()) << " (7z's, one stream, no container)\n";
    if (a.has("out"))
    {
        write_bytes(a.get("out"), code);
        std::cout << "written   " << a.get("out") << " (decode it with: sieve rope --decode " << a.get("out") << " --params \"" << p.text()
                  << "\" --size " << file.size() << " --out FILE)\n";
    }
    return 0;
}

int cmd_mix(const Args& a)
{
    // mix-v4 unless --v1 or --v2 is given (--v2 --words: mix-v3). Parameters: "B22", a picture's
    // "B22 I949x1", or with a word list "B22 D60" (mix-v4's; mix-v2's and mix-v3's the same, but
    // mix-v1's "B22" only).
    if (a.has("v1") && a.has("v2")) throw std::invalid_argument("mix: --v1 or --v2, not both");
    if (a.has("picture") && a.has("words")) throw std::invalid_argument("mix: --picture or --words, not both (a word list is for text)");
    if (a.has("v1") && (a.has("picture") || a.has("words"))) throw std::invalid_argument("mix --v1 takes neither --picture nor --words");
    // The list asked for (--words, counted by use with --by-use), and the one the parameters given
    // name (decoding, or --params): where both are given they must agree, else the code would be made
    // or read with a list its parameters do not name.
    uint32_t asked = 0;
    if (a.has("words"))
    {
        const std::string w = a.get("words");
        if (w != "35" && w != "60" && w != "80") throw std::invalid_argument("mix --words is 35, 60 or 80 (scowl-en-35, 60 or 80): " + w);
        asked = uint32_t(std::stoul(w));
    }
    // v32 (in the making): --by-use, scowl-en-80 with each word counted by how common it is ("F80").
    if (a.has("by-use"))
    {
        if ((asked != 80 && a.has("words")) || a.has("v1") || a.has("v2")) throw std::invalid_argument("mix --by-use is for --words 80 with mix-v4 (\"B22 F80\")");
        asked = 80 | kMixWordsByUse;
    }
    uint32_t named = 0;
    if (a.has("params"))
    {
        const std::string& t = a.get("params");
        if (const size_t d = t.find(" D"); d != std::string::npos) named = uint32_t(std::stoul(t.substr(d + 2)));
        else if (const size_t f = t.find(" F"); f != std::string::npos) named = uint32_t(std::stoul(t.substr(f + 2))) | kMixWordsByUse;
        if (asked && named != asked)
            throw std::invalid_argument("mix: --params names " + (named ? std::string(named & kMixWordsByUse ? "F" : "D") + std::to_string(named & 0xFF) : std::string("no word list")) +
                                        ", not the list --words and --by-use ask for");
    }
    const uint32_t list = a.has("params") ? named : asked;
    const int version = a.has("v1") ? 1 : a.has("v2") ? (list ? 3 : 2) : 4;
    if (version == 3 && (list & kMixWordsByUse)) throw std::invalid_argument("mix-v3 has no word list counted by use (F80 is mix-v4's)");
    const char* name_of_version = version == 1 ? kMixVersion : version == 2 ? kMix2Version : version == 3 ? kMix3Version : kMix4Version;
    const MixDictionary* words = nullptr;
    if (list)
    {
        if (!mix_words()) use_item_maker();
        words = &mix_words()(list); // the registered list, its SHA-256 checked, made ready once
    }
    auto code_of = [&](std::span<const uint8_t> f, const std::string& params) {
        switch (version)
        {
        case 1: return mix_code(f, MixParams::parse(params));
        case 2: return mix2_code(f, Mix2Params::parse(params));
        case 3: return mix3_code(f, Mix3Params::parse(params), *words);
        default: return mix4_code(f, Mix4Params::parse(params), words);
        }
    };
    auto file_of = [&](std::span<const uint8_t> c, const std::string& params, uint64_t size) {
        switch (version)
        {
        case 1: return mix_file(c, MixParams::parse(params), size);
        case 2: return mix2_file(c, Mix2Params::parse(params), size);
        case 3: return mix3_file(c, Mix3Params::parse(params), *words, size);
        default: return mix4_file(c, Mix4Params::parse(params), words, size);
        }
    };
    if (a.has("decode"))
    {
        if (!a.has("params") || !a.has("size") || !a.has("out")) throw std::invalid_argument("mix --decode CODE needs --params, --size and --out");
        const std::vector<uint8_t> code = read_file_bytes(path_of(a.get("decode")));
        const std::vector<uint8_t> file = file_of(code, a.get("params"), std::stoull(a.get("size")));
        write_bytes(a.get("out"), file);
        std::cout << "file      " << a.get("out") << ": " << file.size() << " bytes, sha256 " << sha256_hex(file) << "\n";
        return 0;
    }
    if (a.positional.empty()) throw std::invalid_argument("mix needs a file (see: sieve help mix)");
    const std::string name = a.positional[0];
    const std::vector<uint8_t> file = read_file_bytes(path_of(name));
    std::cout << "file      " << name << ": " << file.size() << " bytes, sha256 " << sha256_hex(file) << "\n";
    std::string params;
    if (a.has("params")) params = a.get("params");
    else
    {
        unsigned w = 0, c = 1;
        char extra = 0;
        if (a.has("picture") && std::sscanf(a.get("picture").c_str(), "%ux%u%c", &w, &c, &extra) != 2)
            throw std::invalid_argument("mix --picture is written WIDTHxBYTES, the bytes a pixel (949x1, 1280x3): " + a.get("picture"));
        switch (version)
        {
        case 1: params = MixParams::for_size(file.size()).text(); break;
        case 2: params = Mix2Params::for_size(file.size(), w, c).text(); break;
        case 3: params = Mix3Params::for_size(file.size(), list).text(); break;
        default: params = Mix4Params::for_size(file.size(), w, c, list & ~kMixWordsByUse, (list & kMixWordsByUse) != 0).text(); break;
        }
    }
    using clk = std::chrono::steady_clock;
    const auto t0 = clk::now();
    const std::vector<uint8_t> code = code_of(file, params);
    const auto t1 = clk::now();
    if (file_of(code, params, file.size()) != file) throw std::logic_error(std::string(name_of_version) + " did not give the file back (a fault in the coder)");
    const auto t2 = clk::now();
    const uint64_t lz = lzma2_size(file);
    char sp[96];
    std::snprintf(sp, sizeof sp, "coded in %.2f s, read back in %.2f s", std::chrono::duration<double>(t1 - t0).count(),
                  std::chrono::duration<double>(t2 - t1).count());
    std::cout << "mix       " << params << " (" << name_of_version << ")\n"
              << "code      " << code.size() << " bytes, " << pct(code.size(), file.size()) << " of the file; read back and checked (" << sp << ")\n"
              << "LZMA2     " << lz << " bytes, " << pct(lz, file.size()) << " (7z's, one stream, no container)\n";
    if (a.has("out"))
    {
        write_bytes(a.get("out"), code);
        std::cout << "written   " << a.get("out") << " (decode it with: sieve mix --decode " << a.get("out")
                  << (version == 1 ? " --v1" : version <= 3 ? " --v2" : "") << " --params \"" << params << "\" --size " << file.size() << " --out FILE)\n";
    }
    return 0;
}

} // namespace sieve::cli
