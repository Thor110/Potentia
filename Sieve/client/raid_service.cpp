// Sieve — the infinite RAID between machines from the menus (see raid_service.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "raid_service.hpp"

#include "cli/raid_socket.hpp"
#include "sieve/paths.hpp"
#include "sieve/raid.hpp"
#include "sieve/raid_net.hpp"
#include "sieve/seal.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

namespace hallway {
namespace raid_service {

namespace fs = std::filesystem;

namespace {

std::mutex g_mx;
std::thread g_server;
std::atomic<bool> g_stop{false};
std::string g_config; // what the running server was started with ("" when none)
std::string g_status = "off";
std::thread g_sharer;
std::atomic<bool> g_sharing{false};
std::optional<std::string> g_share_note;

std::vector<std::string> comma_list(const std::string& s)
{
    std::vector<std::string> out;
    for (size_t i = 0; i <= s.size();)
    {
        const size_t c = std::min(s.find(',', i), s.size());
        std::string one = s.substr(i, c - i);
        while (!one.empty() && one.front() == ' ') one.erase(one.begin());
        while (!one.empty() && one.back() == ' ') one.pop_back();
        if (!one.empty()) out.push_back(one);
        i = c + 1;
    }
    return out;
}

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

void set_status(std::string s)
{
    std::lock_guard<std::mutex> lock(g_mx);
    g_status = std::move(s);
}

void stop_server()
{
    g_stop = true;
    if (g_server.joinable()) g_server.join();
    g_stop = false;
    g_config.clear();
    set_status("off");
}

} // namespace

fs::path key_path(const AppSettings& s, const fs::path& settings_path)
{
    return s.raid_key.empty() ? settings_path.parent_path() / "raid.key" : sieve::from_u8(s.raid_key);
}

fs::path held_path(const AppSettings& s, const fs::path& settings_path)
{
    return s.raid_held.empty() ? settings_path.parent_path() / "raid-held" : sieve::from_u8(s.raid_held);
}

std::string ensure_key(const AppSettings& s, const fs::path& settings_path, bool* made)
{
    if (made) *made = false;
    const fs::path p = key_path(s, settings_path);
    std::error_code ec;
    if (fs::exists(p, ec)) return "";
    if (!p.parent_path().empty()) fs::create_directories(p.parent_path(), ec);
    std::ofstream out(p, std::ios::binary);
    out << sieve::seal_key_text(sieve::new_seal_key());
    out.close();
    if (!out) return "cannot write " + sieve::u8(p);
    fs::permissions(p, fs::perms::owner_read | fs::perms::owner_write, ec);
    if (made) *made = true;
    return "";
}

void apply(const AppSettings& s, const fs::path& settings_path)
{
    const std::string config = s.raid_serve ? s.raid_listen + "|" + sieve::u8(held_path(s, settings_path)) + "|" + std::to_string(s.raid_space_mb) + "|" +
                                                  std::to_string(s.raid_per_peer_mb)
                                            : std::string();
    if (config == g_config && (config.empty() || g_server.joinable())) return;
    stop_server();
    if (config.empty()) return;
    g_config = config;
    set_status("starting on " + s.raid_listen);
    sieve::HeldLimits lim;
    lim.space = uint64_t(s.raid_space_mb) << 20;
    lim.per_peer = uint64_t(s.raid_per_peer_mb) << 20;
    g_server = std::thread([listen = s.raid_listen, held_dir = held_path(s, settings_path), lim] {
        try
        {
            sieve::RaidHeld held(held_dir, lim);
            auto say = [&] { set_status("on " + listen + ", " + grouped(held.used()) + " bytes kept for others"); };
            sieve_cli::raid_listen(
                listen, 0, 30, say,
                [&](sieve::RaidStream& st, const std::string& from) {
                    held.serve(st, from); // (several at once: RaidHeld keeps its counts one at a time)
                    say();
                },
                &g_stop);
        }
        catch (const std::exception& e)
        {
            set_status(std::string("not serving: ") + e.what());
        }
    });
    // A moment for it to start listening (or say why it cannot), so what Settings > RAID shows next
    // is so: at most two seconds.
    for (int i = 0; i < 200 && serving_status().rfind("starting", 0) == 0; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void stop()
{
    stop_server();
    if (g_sharer.joinable()) g_sharer.join();
}

std::string serving_status()
{
    std::lock_guard<std::mutex> lock(g_mx);
    return g_status;
}

void share(const AppSettings& s, const fs::path& settings_path, std::vector<fs::path> drives, std::vector<std::string> versions)
{
    auto note = [](std::string n) {
        std::lock_guard<std::mutex> lock(g_mx);
        g_share_note = std::move(n);
    };
    if (!s.raid_share) return;
    const std::vector<std::string> peers = comma_list(s.raid_peers);
    if (peers.empty())
    {
        note("not shared: no machines named ([raid] peers)");
        return;
    }
    if (g_sharing)
    {
        note("not shared: the last share is still being sent");
        return;
    }
    if (g_sharer.joinable()) g_sharer.join();
    sieve::SealKey key;
    try
    {
        std::ifstream in(key_path(s, settings_path), std::ios::binary);
        if (!in) throw std::runtime_error("no key at " + sieve::u8(key_path(s, settings_path)) + " (turning sharing on in Settings > RAID makes one)");
        key = sieve::read_seal_key(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }
    catch (const std::exception& e)
    {
        note(std::string("not shared: ") + e.what());
        return;
    }
    g_sharing = true;
    // Each version's remote id, kept beside the key: with the key, all that is needed to fetch it
    // back on any machine (sieve raid fetch ID --key FILE --peers ...), and it cannot be worked out again.
    const fs::path log = key_path(s, settings_path).parent_path() / "raid-shared.txt";
    g_sharer = std::thread([peers, key, drives = std::move(drives), versions = std::move(versions), note, log] {
        try
        {
            std::vector<std::unique_ptr<sieve::RaidPeer>> owned;
            for (const auto& p : peers)
                try
                {
                    owned.push_back(std::make_unique<sieve::RaidNetPeer>(sieve_cli::raid_connect(p, 10), p));
                }
                catch (const std::exception&)
                {
                    // (a peer away: the others are used)
                }
            if (owned.empty()) throw std::runtime_error("none of the " + std::to_string(peers.size()) + " machines answered");
            std::vector<sieve::RaidPeer*> ptrs;
            for (auto& p : owned) ptrs.push_back(p.get());
            const sieve::Raid raid(drives);
            uint64_t bytes = 0;
            size_t short_of = 0;
            std::string remote;
            for (const auto& v : versions)
            {
                const sieve::RaidShareReport r = sieve::raid_share(raid, v, key, ptrs, uint32_t(std::min<size_t>(2, ptrs.size())));
                bytes += r.bytes_sent;
                short_of += r.short_of;
                remote = r.remote; // (the last: your save, which names the rest)
                std::ofstream(log, std::ios::binary | std::ios::app) << v << " " << r.remote << "\n";
            }
            note("shared with " + std::to_string(owned.size()) + " of " + std::to_string(peers.size()) + " machines, " + grouped(bytes) + " bytes sent" +
                 (short_of ? ", " + std::to_string(short_of) + " pieces short of their copies" : "") + "; remote id " + remote.substr(0, 12) +
                 "... (each in " + sieve::u8(log.filename()) + " beside your key)");
        }
        catch (const std::exception& e)
        {
            note(std::string("not shared: ") + e.what());
        }
        g_sharing = false;
    });
}

std::optional<std::string> take_share_note()
{
    std::lock_guard<std::mutex> lock(g_mx);
    std::optional<std::string> n;
    n.swap(g_share_note);
    return n;
}

bool sharing() { return g_sharing; }

} // namespace raid_service
} // namespace hallway
