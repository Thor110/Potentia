// Sieve CLI — `sieve rope` (see rope.hpp).
#include "cli/rope.hpp"

#include "cli/compare.hpp"
#include "cli/locate.hpp"
#include "sieve/rope_wave.hpp"

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
        p = s.best;
    }
    const std::vector<uint8_t> code = rope_code(file, p);
    if (rope_file(code, p, file.size()) != file) throw std::logic_error("rope-wave-v1 did not give the file back (a fault in the coder)");
    const uint64_t lz = lzma2_size(file);
    std::cout << "wave      " << p.text() << " (" << (p.columns == 2 ? kRopeWaveLinesVersion : p.columns ? kRopeWaveColumnsVersion : kRopeWaveVersion) << ")\n"
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

} // namespace sieve::cli
