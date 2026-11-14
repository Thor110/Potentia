// Sieve — the catalogue of formats, stage 4: figures measured as formats are used (see format_stats.hpp).
#include "sieve/format_stats.hpp"

#include "sieve/paths.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace sieve {

namespace fs = std::filesystem;

std::string grouped(uint64_t v)
{
    std::string d = std::to_string(v), out;
    for (size_t i = 0; i < d.size(); ++i)
    {
        if (i && (d.size() - i) % 3 == 0) out += ',';
        out += d[i];
    }
    return out;
}

namespace {

// A LABEL, WHAT, RESULT or KEY: an id of at most 64 characters.
bool is_word(std::string_view s) { return is_plain_id(s, 64); }

bool is_date(const std::string& s)
{
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

std::string env(const char* name)
{
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}

std::string percent(uint64_t part, uint64_t whole)
{
    if (!whole) return "-";
    char b[32];
    std::snprintf(b, sizeof b, "%.1f%%", 100.0 * double(part) / double(whole));
    return b;
}

uint64_t median(std::vector<uint64_t> v)
{
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v.size() % 2 ? v[v.size() / 2] : (v[v.size() / 2 - 1] + v[v.size() / 2]) / 2;
}

// A record's time (make-us, install-us) per MiB, so files of any size can be put together. Only for a
// record whose bytes are not 0: the caller checks, and leaves the rest out rather than counting 0.
uint64_t per_mib(const FormatStat& s, const char* key) { return uint64_t(double(s.get(key)) * 1048576.0 / double(s.get("bytes"))); }

// The machines and the dates a summary's records came from, said as "3 times on 2 machines, A to B".
struct Seen
{
    std::set<std::string> machines;
    std::string first, last;
    void add(const FormatStat& s)
    {
        machines.insert(s.machine);
        if (first.empty() || s.date < first) first = s.date;
        if (last.empty() || s.date > last) last = s.date;
    }
    std::string times(size_t n) const
    {
        return std::to_string(n) + (n == 1 ? " time" : " times") + " on " + std::to_string(machines.size()) + (machines.size() == 1 ? " machine" : " machines") + ", " +
               (first == last ? first : first + " to " + last);
    }
};

} // namespace

uint64_t FormatStat::get(const std::string& key) const
{
    for (const auto& [k, v] : figures)
        if (k == key) return v;
    return 0;
}

bool FormatStat::has(const std::string& key) const
{
    for (const auto& [k, v] : figures)
        if (k == key) return true;
    return false;
}

std::string FormatStat::text() const
{
    std::string t = std::string(kFormatStatVersion) + " date=" + date + " machine=" + machine + " what=" + what + " result=" + result;
    for (const auto& [k, v] : figures) t += " " + k + "=" + std::to_string(v);
    return t;
}

FormatStat read_format_stat(const std::string& line)
{
    const std::vector<std::string> f = split_fields(line, ' ');
    if (f.size() < 5 || f[0] != kFormatStatVersion) throw std::invalid_argument(std::string("not a record: it starts ") + kFormatStatVersion + " and date, machine, what and result");
    auto value = [&](size_t i, const char* key) {
        const std::string want = std::string(key) + "=";
        if (f[i].rfind(want, 0) != 0) throw std::invalid_argument("field " + std::to_string(i + 1) + " is not " + key);
        return f[i].substr(want.size());
    };
    FormatStat s;
    s.date = value(1, "date");
    s.machine = value(2, "machine");
    s.what = value(3, "what");
    s.result = value(4, "result");
    if (!is_date(s.date)) throw std::invalid_argument("its date is not YYYY-MM-DD");
    if (!is_word(s.machine) || !is_word(s.what) || !is_word(s.result)) throw std::invalid_argument("its machine, what or result is not a word of a-z, 0-9, '.', '_' and '-'");
    for (size_t i = 5; i < f.size(); ++i)
    {
        const size_t eq = f[i].find('=');
        if (eq == std::string::npos) throw std::invalid_argument("field " + std::to_string(i + 1) + " is not KEY=N");
        const std::string k = f[i].substr(0, eq), n = f[i].substr(eq + 1);
        const std::optional<uint64_t> v = decimal_u64(n);
        if (!is_word(k) || !v) throw std::invalid_argument("field " + std::to_string(i + 1) + " is not KEY=N");
        if (!s.figures.empty() && !(s.figures.back().first < k)) throw std::invalid_argument("its keys are not in order, each once");
        s.figures.push_back({k, *v});
    }
    return s;
}

fs::path format_stats_folder(const std::string& given)
{
    if (!given.empty()) return from_u8(given);
    if (const std::string s = env("SIEVE_STATS"); !s.empty()) return from_u8(s);
#ifdef _WIN32
    if (const std::string s = env("LOCALAPPDATA"); !s.empty()) return from_u8(s) / "Sieve" / "stats";
#endif
    if (const std::string s = env("XDG_DATA_HOME"); !s.empty()) return from_u8(s) / "sieve" / "stats";
    if (const std::string s = env("HOME"); !s.empty()) return from_u8(s) / ".local" / "share" / "sieve" / "stats";
    return fs::path("sieve-stats");
}

std::string format_stats_machine(const std::string& given)
{
    std::string m = given.empty() ? env("SIEVE_MACHINE") : given;
    std::string out;
    for (const char c : m)
    {
        const char l = char(std::tolower(static_cast<unsigned char>(c)));
        out += (l >= 'a' && l <= 'z') || (l >= '0' && l <= '9') || l == '.' || l == '_' || l == '-' ? l : '-';
        if (out.size() == 64) break;
    }
    return out.empty() ? "unnamed" : out;
}

std::string format_stats_today()
{
    const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm u{};
#ifdef _WIN32
    gmtime_s(&u, &t);
#else
    gmtime_r(&t, &u);
#endif
    char b[40]; // (room for any int, so no warning; a date is 10)
    std::snprintf(b, sizeof b, "%04d-%02d-%02d", u.tm_year + 1900, u.tm_mon + 1, u.tm_mday);
    return b;
}

void add_format_stat(const fs::path& folder, const std::string& entry_id, const FormatStat& s)
{
    (void)read_format_stat(s.text()); // a record that would not read back is never written
    if (!is_word(entry_id)) throw std::runtime_error("not an entry id: " + entry_id);
    std::error_code ec;
    fs::create_directories(folder, ec);
    std::ofstream out(folder / (entry_id + ".stats"), std::ios::binary | std::ios::app);
    out << s.text() << "\n";
    out.close();
    if (!out) throw std::runtime_error("cannot write to " + (folder / (entry_id + ".stats")).string());
}

std::vector<FormatStat> format_stats(const fs::path& folder, const std::string& entry_id, size_t* skipped)
{
    std::vector<FormatStat> out;
    if (skipped) *skipped = 0;
    if (!is_word(entry_id)) return out;
    std::ifstream in(folder / (entry_id + ".stats"), std::ios::binary);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        try
        {
            out.push_back(read_format_stat(line));
        }
        catch (const std::invalid_argument&)
        {
            if (skipped) ++*skipped;
        }
    }
    return out;
}

std::vector<std::string> measured_entries(const fs::path& folder)
{
    std::vector<std::string> ids;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(folder, ec))
        if (e.path().extension() == ".stats" && is_word(e.path().stem().string())) ids.push_back(e.path().stem().string());
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<std::string> format_stats_summary(const std::vector<FormatStat>& stats)
{
    std::vector<std::string> lines;
    {
        size_t n = 0, lossless = 0;
        uint64_t bytes = 0, sv = 0, lz = 0, df = 0;
        std::vector<uint64_t> make, install;
        Seen seen;
        for (const auto& s : stats)
        {
            if (s.what != "pack") continue;
            ++n;
            lossless += s.result == "lossless";
            bytes += s.get("bytes");
            sv += s.get("sieve");
            lz += s.get("lzma2");
            df += s.get("deflate");
            if (s.get("bytes"))
            {
                make.push_back(per_mib(s, "make-us"));
                install.push_back(per_mib(s, "install-us"));
            }
            seen.add(s);
        }
        if (n)
        {
            lines.push_back("measured   packed " + seen.times(n) + ": " + grouped(bytes) + " bytes to " + grouped(sv) + " (" + percent(sv, bytes) +
                            "); LZMA2 alone " + grouped(lz) + " (" + percent(lz, bytes) + "), deflate " + grouped(df) + " (" + percent(df, bytes) + ")");
            lines.push_back("           made in " + grouped(median(make) / 1000) + " ms a MiB, installed in " + grouped(median(install) / 1000) +
                            " ms a MiB (medians); round trips " + std::to_string(lossless) + " of " + std::to_string(n) + " lossless");
        }
    }
    {
        // Read in the File Locator: its .sieve made, not installed back; LZMA2 and deflate where Compare
        // was ticked (added up over those records alone).
        size_t n = 0, compared = 0;
        uint64_t bytes = 0, sv = 0, cbytes = 0, lz = 0, df = 0;
        std::vector<uint64_t> make;
        Seen seen;
        for (const auto& s : stats)
        {
            if (s.what != "read") continue;
            ++n;
            bytes += s.get("bytes");
            sv += s.get("sieve");
            if (s.has("lzma2"))
            {
                ++compared;
                cbytes += s.get("bytes");
                lz += s.get("lzma2");
                df += s.get("deflate");
            }
            if (s.get("bytes")) make.push_back(per_mib(s, "make-us"));
            seen.add(s);
        }
        if (n)
        {
            lines.push_back("read       in the File Locator " + seen.times(n) + ": " + grouped(bytes) + " bytes to " + grouped(sv) + " (" +
                            percent(sv, bytes) + "), made in " + grouped(median(make) / 1000) + " ms a MiB (median)");
            if (compared)
                lines.push_back("           of the " + std::to_string(compared) + " compared: LZMA2 alone " + percent(lz, cbytes) + ", deflate " + percent(df, cbytes));
        }
    }
    {
        size_t n = 0, whole = 0;
        uint64_t vi = 0, vk = 0, fi = 0, fk = 0, joints = 0, anims = 0, left = 0;
        Seen seen;
        for (const auto& s : stats)
        {
            if (s.what != "import") continue;
            ++n;
            whole += s.result == "whole";
            vi += s.get("vertices-in");
            vk += s.get("vertices-kept");
            fi += s.get("faces-in");
            fk += s.get("faces-kept");
            joints += s.get("joints");
            anims += s.get("animations");
            left += s.get("left-out");
            seen.add(s);
        }
        if (n)
        {
            lines.push_back("imported   " + seen.times(n) + ": " + grouped(vk) + " of " + grouped(vi) + " vertices kept, " + grouped(fk) + " of " +
                            grouped(fi) + " faces; " + grouped(joints) + " joints, " + grouped(anims) + " animations");
            lines.push_back("           " + std::to_string(whole) + " of " + std::to_string(n) + " whole; " + grouped(left) + " things left out in all");
        }
    }
    return lines;
}

std::vector<std::string> format_stats_short(const std::vector<FormatStat>& stats)
{
    std::vector<std::string> lines;
    size_t packs = 0, lossless = 0, imports = 0, reads = 0;
    uint64_t rbytes = 0, rsv = 0;
    uint64_t bytes = 0, sv = 0, lz = 0, vi = 0, vk = 0;
    std::vector<uint64_t> make;
    for (const auto& s : stats)
        if (s.what == "pack")
        {
            ++packs;
            lossless += s.result == "lossless";
            bytes += s.get("bytes");
            sv += s.get("sieve");
            lz += s.get("lzma2");
            if (s.get("bytes")) make.push_back(per_mib(s, "make-us"));
        }
        else if (s.what == "read")
        {
            ++reads;
            rbytes += s.get("bytes");
            rsv += s.get("sieve");
        }
        else if (s.what == "import")
        {
            ++imports;
            vi += s.get("vertices-in");
            vk += s.get("vertices-kept");
        }
    if (packs)
    {
        char b[48];
        std::snprintf(b, sizeof b, "%.1f s", double(median(make)) / 1e6);
        lines.push_back("packed " + std::to_string(packs) + "x: " + percent(sv, bytes) + " of its bytes (LZMA2 " + percent(lz, bytes) + ")");
        lines.push_back(std::to_string(lossless) + " of " + std::to_string(packs) + " lossless; " + b + " a MiB to make");
    }
    if (reads) lines.push_back("read " + std::to_string(reads) + "x in the File Locator: " + percent(rsv, rbytes) + " of its bytes");
    if (imports) lines.push_back("imported " + std::to_string(imports) + "x: " + grouped(vk) + " of " + grouped(vi) + " vertices kept");
    return lines;
}

} // namespace sieve
