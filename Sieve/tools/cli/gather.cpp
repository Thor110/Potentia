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

// Several files chosen at once (6 November 2026): Explorer starts one program for each, within
// moments of each other, all in one folder. Each asks for a lock kept in the temporary folder for
// that folder and that command (a directory made there, which only one can make); the one that
// makes it gathers: it waits until no path has come for half a second (five seconds at most) and
// returns every path, its own first. Each other writes its path beside the lock and waits for the
// first to say it took it (a file of its own name in the temporary folder); then it closes, the
// first making the one result. Not taken within eight seconds (the first had finished), it takes
// its path back and works alone. A lock older than ten seconds was left by one that stopped, and is
// taken. One file chosen: just its own path, half a second later.
// (The review of 6 November 2026: the key had no command in it, so a zip and a 7z of one folder at
// once ran as one; and a path that came as the first finished was lost. Now the first closes the
// lock's folder by renaming it before its last look, so a path either reaches it or is refused.)
std::optional<std::vector<std::string>> gather_chosen(const std::string& what, const std::string& kind)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path parent = fs::absolute(path_of_utf8(what), ec).parent_path();
    char key[32];
    std::snprintf(key, sizeof key, "%016llx", (unsigned long long)std::hash<std::string>{}(kind + "|" + utf8_of(parent)));
    const fs::path temp = fs::temp_directory_path(ec);
    const fs::path dir = temp / ("sieve-gather-" + std::string(key));
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
        if (ec)
        {
            fs::remove(dir / (name + ".part"), ec);
            return std::vector<std::string>{what}; // (could not hand it over: made alone)
        }
        const fs::path ack = temp / ("sieve-gather-" + name + ".taken");
        const auto asked = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - asked < std::chrono::seconds(8))
        {
            if (fs::exists(ack, ec))
            {
                fs::remove(ack, ec);
                return std::nullopt; // (taken: the first makes the result)
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        // Not taken: taken back, if it is still there to take, and made alone. (Gone without a
        // word, the first took it as it ended: it is in that result.)
        if (fs::remove(dir / (name + ".path"), ec)) return std::vector<std::string>{what};
        if (fs::exists(ack, ec))
        {
            fs::remove(ack, ec);
            return std::nullopt;
        }
        return std::vector<std::string>{what};
    }
    std::vector<std::string> got{what};
    std::set<std::string> seen;
    auto take = [&](const fs::path& folder) {
        bool any = false;
        for (const auto& e : fs::directory_iterator(folder, ec))
            if (e.path().extension() == ".path" && seen.insert(utf8_of(e.path().filename())).second)
            {
                std::string p;
                {
                    std::ifstream in(e.path(), std::ios::binary);
                    p.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                }
                // (only what is in the same folder: one result is a folder of them)
                if (!p.empty() && fs::absolute(path_of_utf8(p), ec).parent_path() == parent) got.push_back(p);
                std::ofstream(temp / ("sieve-gather-" + e.path().stem().string() + ".taken"), std::ios::binary).put('1');
                any = true;
            }
        return any;
    };
    const auto start = std::chrono::steady_clock::now();
    auto last = start;
    while (std::chrono::steady_clock::now() - last < std::chrono::milliseconds(500) && std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (take(dir)) last = std::chrono::steady_clock::now();
    }
    // Closed: the folder renamed away (no path can land in it after), then looked in once more.
    const fs::path closed = temp / ("sieve-gather-" + std::string(key) + "-closed-" + std::to_string(std::random_device{}()));
    fs::rename(dir, closed, ec);
    if (!ec)
    {
        take(closed);
        fs::remove_all(closed, ec);
    }
    else
        fs::remove_all(dir, ec);
    return got;
}

} // namespace sieve::cli
