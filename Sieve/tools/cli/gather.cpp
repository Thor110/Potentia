// Several files or folders chosen at once, gathered into one run (gather.hpp).
#include "gather.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <random>
#include <set>
#include <system_error>
#include <thread>

namespace sieve::cli
{

namespace
{

std::filesystem::path path_of_utf8(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }
std::string utf8_of(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

} // namespace

// Create .sieve on several files chosen at once (6 November 2026): Explorer starts one program for
// each, within moments of each other, all in one folder. Each asks for a lock kept in the temporary
// folder for that folder (a directory made there, which only one can make); the one that makes it
// gathers: it waits until no path has come for half a second (five seconds at most) and returns
// every path, its own first. Each other writes its path beside the lock and gets nothing back: it
// closes, the first making the .sieve. A lock older than ten seconds was left by one that stopped,
// and is taken. One file chosen: just its own path, half a second later.
std::optional<std::vector<std::string>> gather_chosen(const std::string& what)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path parent = fs::absolute(path_of_utf8(what), ec).parent_path();
    char key[32];
    std::snprintf(key, sizeof key, "%016llx", (unsigned long long)std::hash<std::string>{}(utf8_of(parent)));
    const fs::path dir = fs::temp_directory_path(ec) / ("sieve-create-" + std::string(key));
    const fs::path lock = dir / "lock";
    fs::create_directories(dir, ec);
    bool first = fs::create_directory(lock, ec);
    if (!first)
    {
        const auto age = fs::file_time_type::clock::now() - fs::last_write_time(lock, ec);
        if (!ec && age > std::chrono::seconds(10))
        {
            fs::remove_all(dir, ec);
            fs::create_directories(dir, ec);
            first = fs::create_directory(lock, ec);
        }
    }
    if (!first)
    {
        // (written whole, then named, so the first never reads half a path)
        const std::string name = "p-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                                 std::to_string(std::random_device{}());
        {
            std::ofstream o(dir / (name + ".part"), std::ios::binary);
            o << what;
        }
        fs::rename(dir / (name + ".part"), dir / (name + ".path"), ec);
        if (!ec) return std::nullopt;
        return std::vector<std::string>{what}; // (could not hand it over: made alone)
    }
    std::vector<std::string> got{what};
    std::set<fs::path> seen;
    const auto start = std::chrono::steady_clock::now();
    auto last = start;
    while (std::chrono::steady_clock::now() - last < std::chrono::milliseconds(500) && std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (const auto& e : fs::directory_iterator(dir, ec))
            if (e.path().extension() == ".path" && seen.insert(e.path()).second)
            {
                std::ifstream in(e.path(), std::ios::binary);
                std::string p((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                // (only what is in the same folder: one .sieve is a folder of them)
                if (!p.empty() && fs::absolute(path_of_utf8(p), ec).parent_path() == parent) got.push_back(p);
                last = std::chrono::steady_clock::now();
            }
    }
    fs::remove_all(dir, ec);
    return got;
}

} // namespace sieve::cli
