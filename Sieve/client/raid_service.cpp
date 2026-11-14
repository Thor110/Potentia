// Sieve — the infinite RAID between machines from the menus (see raid_service.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "raid_service.hpp"

#include "cli/raid_socket.hpp"
#include "sieve/paths.hpp"
#include "sieve/format_stats.hpp"
#include "sieve/raid.hpp"
#include "sieve/raid_net.hpp"
#include "sieve/seal.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
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
std::thread g_checker;
std::atomic<bool> g_checking{false};
std::optional<std::string> g_check_note;

// The settings' peers, each with the spaces at its ends taken off (a hand-written "a:1, b:2" names
// b:2), and those that were only spaces left out.
std::vector<std::string> comma_list(const std::string& s)
{
    std::vector<std::string> out;
    for (const std::string& one : sieve::split_nonempty(s, ','))
        if (std::string t = sieve::trim(one, " "); !t.empty()) out.push_back(std::move(t));
    return out;
}

using sieve::grouped;

// A share's or a check's note, left for the menus to take (one at a time with the server's status).
void put_note(std::optional<std::string>& note, std::string text)
{
    std::lock_guard<std::mutex> lock(g_mx);
    note = std::move(text);
}

std::optional<std::string> take_note(std::optional<std::string>& note)
{
    std::lock_guard<std::mutex> lock(g_mx);
    std::optional<std::string> n;
    n.swap(note);
    return n;
}

// The RAID key, read from its file; "no key at FILE" and the hint if there is none to read.
sieve::SealKey read_key(const fs::path& p, const std::string& hint)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("no key at " + sieve::u8(p) + hint);
    return sieve::read_seal_key(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
}

// Every machine named that answers, given 10 seconds each; a machine away is passed over quietly.
sieve_cli::RaidPeers connect_peers(const std::vector<std::string>& peers)
{
    sieve_cli::RaidPeers c = sieve_cli::raid_connect_all(peers, 10);
    if (c.owned.empty()) throw std::runtime_error("none of the " + std::to_string(peers.size()) + " machines answered");
    return c;
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
    const std::string config = s.online && s.raid_serve ? s.raid_listen + "|" + sieve::u8(held_path(s, settings_path)) + "|" + std::to_string(s.raid_space_mb) + "|" +
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
    // A moment for it to start listening (or say why it cannot), so what Settings > Infinite RAID shows next
    // is so: at most two seconds.
    for (int i = 0; i < 200 && serving_status().rfind("starting", 0) == 0; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void stop()
{
    stop_server();
    if (g_sharer.joinable()) g_sharer.join();
    if (g_checker.joinable()) g_checker.join();
}

std::string serving_status()
{
    std::lock_guard<std::mutex> lock(g_mx);
    return g_status;
}

void share(const AppSettings& s, const fs::path& settings_path, std::vector<fs::path> drives, std::vector<std::string> versions)
{
    auto note = [](std::string n) { put_note(g_share_note, std::move(n)); };
    if (!s.raid_share) return;
    if (!s.online)
    {
        note("not shared: Online is off (Settings > Online)");
        return;
    }
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
        key = read_key(key_path(s, settings_path), " (turning sharing on in Settings > Infinite RAID makes one)");
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
    const uint32_t copies = uint32_t(std::max(1, s.raid_copies));
    g_sharer = std::thread([peers, key, drives = std::move(drives), versions = std::move(versions), note, log, copies] {
        try
        {
            const sieve_cli::RaidPeers c = connect_peers(peers);
            const sieve::Raid raid(drives);
            uint64_t bytes = 0;
            size_t short_of = 0;
            std::string remote;
            for (const auto& v : versions)
            {
                const sieve::RaidShareReport r = sieve::raid_share(raid, v, key, c.peers, copies);
                bytes += r.bytes_sent;
                short_of += r.short_of;
                remote = r.remote; // (the last: your save, which names the rest)
                std::ofstream(log, std::ios::binary | std::ios::app) << v << " " << r.remote << "\n";
            }
            note("shared with " + std::to_string(c.owned.size()) + " of " + std::to_string(peers.size()) + " machines, " + grouped(bytes) + " bytes sent" +
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

std::optional<std::string> take_share_note() { return take_note(g_share_note); }

bool sharing() { return g_sharing; }

void check(const AppSettings& s, const fs::path& settings_path)
{
    auto note = [](std::string n) { put_note(g_check_note, std::move(n)); };
    if (g_checking) return;
    if (g_checker.joinable()) g_checker.join();
    if (!s.online)
    {
        note("nothing checked: Online is off (Settings > Online)");
        return;
    }
    const std::vector<std::string> peers = comma_list(s.raid_peers);
    if (peers.empty())
    {
        note("nothing checked: no machines named ([raid] peers)");
        return;
    }
    // Each save shared from here: the remote ids beside the key, each once.
    std::vector<std::string> remotes;
    {
        std::ifstream in(key_path(s, settings_path).parent_path() / "raid-shared.txt", std::ios::binary);
        std::string l;
        while (std::getline(in, l))
        {
            const size_t sp = l.find(' ');
            if (sp == std::string::npos) continue;
            std::string r = l.substr(sp + 1);
            while (!r.empty() && (r.back() == '\r' || r.back() == ' ')) r.pop_back();
            if (r.size() == 64 && std::find(remotes.begin(), remotes.end(), r) == remotes.end()) remotes.push_back(r);
        }
    }
    if (remotes.empty())
    {
        note("nothing checked: nothing shared from here yet");
        return;
    }
    sieve::SealKey key;
    try
    {
        key = read_key(key_path(s, settings_path), "");
    }
    catch (const std::exception& e)
    {
        note(std::string("nothing checked: ") + e.what());
        return;
    }
    g_checking = true;
    note("checking " + std::to_string(remotes.size()) + " shared versions...");
    const uint32_t copies = uint32_t(std::max(1, s.raid_copies));
    g_checker = std::thread([peers, key, remotes, copies, note] {
        try
        {
            const sieve_cli::RaidPeers on = connect_peers(peers);
            size_t pieces = 0, short_of = 0, repaired = 0, lost = 0, unreadable = 0;
            for (const auto& r : remotes)
                try
                {
                    const sieve::RaidCheckReport c = sieve::raid_check(r, key, on.peers, copies, true);
                    pieces += c.pieces;
                    short_of += c.short_of;
                    repaired += c.repaired;
                    lost += c.lost;
                }
                catch (const std::exception&)
                {
                    ++unreadable; // (its index had by none of them)
                }
            note("checked " + std::to_string(remotes.size()) + " versions, " + std::to_string(pieces) + " pieces on " + std::to_string(on.owned.size()) + " of " +
                 std::to_string(peers.size()) + " machines: " + std::to_string(short_of) + " short, " + std::to_string(repaired) + " put back, " +
                 std::to_string(lost) + " lost" + (unreadable ? ", " + std::to_string(unreadable) + " versions not found" : ""));
        }
        catch (const std::exception& e)
        {
            note(std::string("not checked: ") + e.what());
        }
        g_checking = false;
    });
}

std::optional<std::string> take_check_note() { return take_note(g_check_note); }

bool checking() { return g_checking; }

} // namespace raid_service
} // namespace hallway
