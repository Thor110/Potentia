// Sieve — the catalogue of formats, stage 4: figures measured as formats are used (see format_stats.hpp).
#include "sieve/format_stats.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <system_error>

namespace sieve {

namespace fs = std::filesystem;

namespace {

bool is_word(const std::string& s)
{
    if (s.empty() || s.size() > 64) return false;
    for (const char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return false;
    return true;
}

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

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// A number with its thousands apart by commas, as Sieve writes figures.
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

} // namespace

uint64_t FormatStat::get(const std::string& key) const
{
    for (const auto& [k, v] : figures)
        if (k == key) return v;
    return 0;
}

std::string FormatStat::text() const
{
    std::string t = std::string(kFormatStatVersion) + " date=" + date + " machine=" + machine + " what=" + what + " result=" + result;
    for (const auto& [k, v] : figures) t += " " + k + "=" + std::to_string(v);
    return t;
}

FormatStat read_format_stat(const std::string& line)
{
    std::vector<std::string> f;
    for (size_t i = 0; i <= line.size();)
    {
        const size_t e = std::min(line.find(' ', i), line.size());
        f.push_back(line.substr(i, e - i));
        i = e + 1;
    }
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
        if (!is_word(k) || n.empty() || n.size() > 19 || n.find_first_not_of("0123456789") != std::string::npos)
            throw std::invalid_argument("field " + std::to_string(i + 1) + " is not KEY=N");
        if (!s.figures.empty() && !(s.figures.back().first < k)) throw std::invalid_argument("its keys are not in order, each once");
        s.figures.push_back({k, std::stoull(n)});
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
    auto from = [](size_t n, const std::set<std::string>& machines, const std::string& first, const std::string& last) {
        return std::to_string(n) + (n == 1 ? " time" : " times") + " on " + std::to_string(machines.size()) + (machines.size() == 1 ? " machine" : " machines") + ", " +
               (first == last ? first : first + " to " + last);
    };
    {
        size_t n = 0, lossless = 0;
        uint64_t bytes = 0, sv = 0, lz = 0, df = 0;
        std::vector<uint64_t> make, install;
        std::set<std::string> machines;
        std::string first, last;
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
                // per MiB, so files of any size can be put together
                make.push_back(uint64_t(double(s.get("make-us")) * 1048576.0 / double(s.get("bytes"))));
                install.push_back(uint64_t(double(s.get("install-us")) * 1048576.0 / double(s.get("bytes"))));
            }
            machines.insert(s.machine);
            if (first.empty() || s.date < first) first = s.date;
            if (last.empty() || s.date > last) last = s.date;
        }
        if (n)
        {
            lines.push_back("measured   packed " + from(n, machines, first, last) + ": " + grouped(bytes) + " bytes to " + grouped(sv) + " (" + percent(sv, bytes) +
                            "); LZMA2 alone " + grouped(lz) + " (" + percent(lz, bytes) + "), deflate " + grouped(df) + " (" + percent(df, bytes) + ")");
            lines.push_back("           made in " + grouped(median(make) / 1000) + " ms a MiB, installed in " + grouped(median(install) / 1000) +
                            " ms a MiB (medians); round trips " + std::to_string(lossless) + " of " + std::to_string(n) + " lossless");
        }
    }
    {
        size_t n = 0, whole = 0;
        uint64_t vi = 0, vk = 0, fi = 0, fk = 0, joints = 0, anims = 0, left = 0;
        std::set<std::string> machines;
        std::string first, last;
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
            machines.insert(s.machine);
            if (first.empty() || s.date < first) first = s.date;
            if (last.empty() || s.date > last) last = s.date;
        }
        if (n)
        {
            lines.push_back("imported   " + from(n, machines, first, last) + ": " + grouped(vk) + " of " + grouped(vi) + " vertices kept, " + grouped(fk) + " of " +
                            grouped(fi) + " faces; " + grouped(joints) + " joints, " + grouped(anims) + " animations");
            lines.push_back("           " + std::to_string(whole) + " of " + std::to_string(n) + " whole; " + grouped(left) + " things left out in all");
        }
    }
    return lines;
}

std::vector<std::string> format_stats_short(const std::vector<FormatStat>& stats)
{
    std::vector<std::string> lines;
    size_t packs = 0, lossless = 0, imports = 0;
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
            if (s.get("bytes")) make.push_back(uint64_t(double(s.get("make-us")) * 1048576.0 / double(s.get("bytes"))));
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
    if (imports) lines.push_back("imported " + std::to_string(imports) + "x: " + grouped(vk) + " of " + grouped(vi) + " vertices kept");
    return lines;
}

} // namespace sieve
