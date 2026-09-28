// Sieve — timings (timings.hpp): the table of phases, kept under one lock, and written at exit.
//
// The table lives in a function-local static made before the exit handler is registered, so it is
// still there when the handler runs (handlers run before the statics made ahead of them go).

#include "cli/timings.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

namespace sieve::cli::timings {

namespace {

struct Stat
{
    uint64_t runs = 0;
    double total = 0, slowest = 0;
};

struct Table
{
    std::mutex mx;
    std::map<std::string, Stat> phases;
    std::filesystem::path file;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    bool registered = false;
};

std::atomic<bool> g_on{false};

Table& table()
{
    static Table t;
    return t;
}

std::string ms_text(double ms)
{
    char b[32];
    std::snprintf(b, sizeof b, ms < 10 ? "%.3f" : ms < 1000 ? "%.1f" : "%.0f", ms);
    return b;
}

void write_at_exit()
{
    const std::string text = report();
    std::cerr << text;
    Table& t = table();
    std::filesystem::path file;
    {
        std::lock_guard<std::mutex> lock(t.mx);
        file = t.file;
    }
    if (file.empty()) return;
    std::ofstream out(file, std::ios::binary);
    if (out) out << text;
    const std::u8string u = file.u8string();
    std::cerr << "timings written to " << std::string(u.begin(), u.end()) << "\n";
}

} // namespace

void enable(const std::filesystem::path& file)
{
    Table& t = table();
    {
        std::lock_guard<std::mutex> lock(t.mx);
        t.file = file;
        if (t.registered) return;
        t.registered = true;
    }
    std::atexit(write_at_exit);
    g_on.store(true, std::memory_order_relaxed);
}

bool enabled() { return g_on.load(std::memory_order_relaxed); }

void add(const std::string& phase, double ms)
{
    if (!enabled()) return;
    Table& t = table();
    std::lock_guard<std::mutex> lock(t.mx);
    Stat& s = t.phases[phase];
    ++s.runs;
    s.total += ms;
    s.slowest = std::max(s.slowest, ms);
}

std::string report()
{
    Table& t = table();
    std::vector<std::pair<std::string, Stat>> rows;
    double wall = 0;
    {
        std::lock_guard<std::mutex> lock(t.mx);
        rows.assign(t.phases.begin(), t.phases.end());
        wall = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t.started).count();
    }
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second.total > b.second.total; });
    size_t w = 5;
    for (const auto& r : rows) w = std::max(w, r.first.size());
    std::ostringstream o;
    o << "sieve timings (ms): " << ms_text(wall) << " ms in all; phases nest, so their totals overlap\n";
    auto cell = [](const std::string& s, size_t n) { return std::string(n > s.size() ? n - s.size() : 0, ' ') + s; };
    o << "phase" << std::string(w - 5, ' ') << cell("runs", 10) << cell("total", 12) << cell("mean", 11) << cell("slowest", 11) << "\n";
    for (const auto& [name, s] : rows)
        o << name << std::string(w - name.size(), ' ') << cell(std::to_string(s.runs), 10) << cell(ms_text(s.total), 12)
          << cell(ms_text(s.total / double(std::max<uint64_t>(s.runs, 1))), 11) << cell(ms_text(s.slowest), 11) << "\n";
    return o.str();
}

} // namespace sieve::cli::timings
