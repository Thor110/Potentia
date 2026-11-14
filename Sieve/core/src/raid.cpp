// Sieve — the infinite RAID, stage 1 (see raid.hpp).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits. (A request, not a condition: whoever uses
// the RAID in a system run for profit is asked to give a share of that profit to charity, the share
// settled by open, democratic debate; IDEAS 19.177.)
#include "sieve/raid.hpp"

#include "sieve/hex.hpp"
#include "sieve/seal.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"
#include "raid_files.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace sieve {

namespace fs = std::filesystem;

namespace {

using raid_files::read_file;
using raid_files::write_file;

fs::path chunk_path(const fs::path& folder, const std::string& h) { return folder / "chunks" / h.substr(0, 2) / h; }
fs::path manifest_path(const fs::path& folder, const std::string& id) { return folder / "manifests" / (id + ".manifest"); }

// A new folder's id: 32 hex digits, the start of the SHA-256 of everything that tells this folder from
// any other, so that no one of them failing can make two folders alike (Edward, 14 November 2026:
// "could you not combine it with the system id? ... yes, hash them"). Some MinGW builds'
// std::random_device gives a fixed sequence (seal.cpp), which alone would give every new folder the
// same id and have the RAID refuse the second as "a folder given twice". Mixed in: the system's
// random bytes (as a key is made, where it answers) and std::random_device's; on systems other than
// Windows, the machine id file (/etc/machine-id, else /var/lib/dbus/machine-id) and the host name
// (Windows' own machine id is not read); the drive the folder is on and the folder's own number on it
// (two folders never share both), its path, and the time to the clock's finest step. Hashed, so nothing of the machine can be read from the id,
// which travels with the folder; and still 32 hex digits, as every folder made before has.
std::string new_folder_id(const fs::path& folder)
{
    std::string mix = "sieve-raid folder id\n";
    try
    {
        const SealKey r = new_seal_key();
        mix.append(reinterpret_cast<const char*>(r.data()), r.size());
    }
    catch (const std::exception&)
    {
        mix += "(no system random bytes)\n";
    }
    {
        std::random_device rd; // (more, where it is good; harmless where it is not)
        for (int i = 0; i < 4; ++i) mix += std::to_string(rd()) + " ";
    }
#ifdef _WIN32
    const HANDLE h = CreateFileW(folder.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION info{};
        if (GetFileInformationByHandle(h, &info))
            mix += "drive " + std::to_string(info.dwVolumeSerialNumber) + " folder " + std::to_string((uint64_t(info.nFileIndexHigh) << 32) | info.nFileIndexLow) + "\n";
        CloseHandle(h);
    }
#else
    for (const char* f : {"/etc/machine-id", "/var/lib/dbus/machine-id"})
        if (const auto t = raid_files::read_file(f))
        {
            mix += *t;
            break;
        }
    char host[256] = {};
    if (gethostname(host, sizeof host - 1) == 0) mix += std::string("host ") + host + "\n";
    struct stat st{};
    if (stat(folder.c_str(), &st) == 0) mix += "drive " + std::to_string(uint64_t(st.st_dev)) + " folder " + std::to_string(uint64_t(st.st_ino)) + "\n";
#endif
    mix += "path " + folder.lexically_normal().generic_string() + "\n";
    mix += "time " + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " " +
           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "\n";
    return Sha256::hex_hash(mix).substr(0, 32);
}

} // namespace

std::vector<std::pair<size_t, size_t>> raid_cut(const std::string& bytes)
{
    // The gear table: entry i is the first 8 bytes, little-endian, of SHA-256("sieve-raid-gear-v1" | i).
    static const std::vector<uint64_t> gear = [] {
        std::vector<uint64_t> g(256);
        for (int i = 0; i < 256; ++i)
        {
            const auto d = Sha256::hash(std::string("sieve-raid-gear-v1") + char(i));
            uint64_t v = 0;
            for (int k = 7; k >= 0; --k) v = v << 8 | d[size_t(k)];
            g[size_t(i)] = v;
        }
        return g;
    }();
    std::vector<std::pair<size_t, size_t>> out;
    size_t start = 0;
    uint64_t h = 0;
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        h = (h << 1) + gear[uint8_t(bytes[i])];
        const size_t len = i + 1 - start;
        if ((len >= kRaidCutMin && (h & kRaidCutMask) == 0) || len >= kRaidCutMax)
        {
            out.push_back({start, len});
            start = i + 1;
            h = 0;
        }
    }
    if (start < bytes.size()) out.push_back({start, bytes.size() - start});
    return out;
}

std::string RaidManifest::text() const
{
    std::string t = std::string(kRaidManifestVersion) + "\nname " + name + "\nsize " + std::to_string(size) + "\nprevious " +
                    (previous.empty() ? std::string("none") : previous) + "\n";
    for (const auto& [h, n] : chunks) t += "chunk " + h + " " + std::to_string(n) + "\n";
    return t;
}

std::string RaidManifest::id() const { return Sha256::hex_hash(text()); }

RaidManifest read_raid_manifest(const std::string& text)
{
    std::vector<std::string> lines;
    if (!lf_lines(text, lines)) throw std::invalid_argument("a manifest's last line has no line feed");
    if (lines.size() < 4 || lines[0] != kRaidManifestVersion) throw std::invalid_argument(std::string("not a manifest: it does not start with ") + kRaidManifestVersion);
    RaidManifest m;
    if (lines[1].rfind("name ", 0) != 0) throw std::invalid_argument("a manifest's second line is its name");
    m.name = lines[1].substr(5);
    auto number = [](const std::string& s) {
        if (const auto v = decimal_u64(s)) return *v;
        throw std::invalid_argument("not a number: " + s);
    };
    if (lines[2].rfind("size ", 0) != 0) throw std::invalid_argument("a manifest's third line is its size");
    m.size = number(lines[2].substr(5));
    if (lines[3].rfind("previous ", 0) != 0) throw std::invalid_argument("a manifest's fourth line is its previous version");
    const std::string prev = lines[3].substr(9);
    if (prev != "none")
    {
        if (!is_sha256_hex(prev)) throw std::invalid_argument("previous: not a SHA-256: " + prev);
        m.previous = prev;
    }
    uint64_t total = 0;
    for (size_t i = 4; i < lines.size(); ++i)
    {
        const std::string& l = lines[i];
        if (l.size() < 6 + 64 + 2 || l.rfind("chunk ", 0) != 0 || l[70] != ' ') throw std::invalid_argument("not a chunk's line: " + l);
        const std::string h = l.substr(6, 64);
        if (!is_sha256_hex(h)) throw std::invalid_argument("not a SHA-256: " + h);
        const uint64_t n = number(l.substr(71));
        if (n == 0 || n > kRaidChunk) throw std::invalid_argument("a chunk's length is out of range: " + l);
        m.chunks.push_back({h, n});
        total += n;
    }
    if (total != m.size) throw std::invalid_argument("its chunks do not add up to its size");
    if (m.text() != text) throw std::invalid_argument("not in its canonical form");
    return m;
}

Raid::Raid(std::vector<fs::path> folders, uint32_t copies) : folders_(std::move(folders)), copies_(std::max<uint32_t>(1, copies))
{
    if (folders_.empty()) throw std::invalid_argument("a RAID needs at least one folder");
    for (const fs::path& f : folders_)
    {
        const std::string id = raid_files::folder_tag(f, "sieve-raid", kRaidVersion, "id", "sieve-raid id", [&f] { return new_folder_id(f); });
        if (std::find(ids_.begin(), ids_.end(), id) != ids_.end()) throw std::invalid_argument(f.string() + " is a folder given twice (the same sieve-raid id)");
        ids_.push_back(id);
    }
}

std::vector<size_t> raid_rank(const std::string& name, const std::vector<std::string>& keys)
{
    std::vector<std::pair<std::string, size_t>> rank;
    for (size_t i = 0; i < keys.size(); ++i) rank.push_back({Sha256::hex_hash(name + keys[i]), i});
    std::sort(rank.begin(), rank.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<size_t> out;
    for (const auto& r : rank) out.push_back(r.second);
    return out;
}

std::vector<size_t> Raid::homes(const std::string& chunk) const { return raid_rank(chunk, ids_); }

std::string Raid::put(const std::string& name, const std::string& bytes)
{
    if (name.empty() || name.find('\n') != std::string::npos || name.find('\r') != std::string::npos)
        throw std::invalid_argument("a save's name is one line of text, not empty");
    RaidManifest m;
    m.name = name;
    m.size = bytes.size();
    const auto cuts = raid_cut(bytes);
    for (const auto& [at, n] : cuts) m.chunks.push_back({Sha256::hex_hash(bytes.substr(at, n)), n});
    if (const auto last = latest(name))
    {
        bool same = false;
        for (const fs::path& f : folders_)
            if (const auto mt = read_file(manifest_path(f, *last)))
                try
                {
                    const RaidManifest prev = read_raid_manifest(*mt);
                    same = prev.size == m.size && prev.chunks == m.chunks;
                    // Cut another way (a store written before stage 2): the same bytes are still the same save.
                    if (!same && prev.size == m.size)
                        try
                        {
                            same = get(*last) == bytes;
                        }
                        catch (const std::exception&)
                        {
                        }
                    break;
                }
                catch (const std::exception&)
                {
                }
        if (same) return *last;
        m.previous = *last;
    }
    // The chunks first, then the manifest, so a manifest never names a chunk not yet written.
    const size_t want = std::min<size_t>(copies_, folders_.size());
    for (size_t k = 0; k < m.chunks.size(); ++k)
    {
        const std::string c = bytes.substr(cuts[k].first, cuts[k].second);
        const std::string& h = m.chunks[k].first;
        size_t written = 0;
        for (const size_t i : homes(h))
        {
            if (written >= want) break;
            const fs::path p = chunk_path(folders_[i], h);
            const auto have = read_file(p);
            if ((have && *have == c) || write_file(p, c)) ++written;
        }
        if (written == 0) throw std::runtime_error("no folder would take chunk " + h);
    }
    const std::string text = m.text(), id = m.id();
    size_t kept = 0;
    for (const fs::path& f : folders_)
        if (write_file(manifest_path(f, id), text)) ++kept;
    if (kept == 0) throw std::runtime_error("no folder would take the manifest");
    return id;
}

std::string Raid::get(const std::string& version) const
{
    if (!is_sha256_hex(version)) throw std::invalid_argument("not a version id (64 hex digits): " + version);
    const std::optional<RaidManifest> m = manifest(version);
    if (!m) throw std::runtime_error("no good copy of version " + version + " in any folder");
    std::string out;
    out.reserve(size_t(m->size));
    for (const auto& [h, n] : m->chunks)
    {
        bool found = false;
        for (const size_t i : homes(h))
            if (const auto c = read_file(chunk_path(folders_[i], h)); c && c->size() == n && Sha256::hex_hash(*c) == h)
            {
                out += *c;
                found = true;
                break;
            }
        if (!found) throw std::runtime_error("chunk " + h + " has no good copy in any folder (repair cannot help: it is lost)");
    }
    return out;
}

std::optional<RaidManifest> Raid::manifest(const std::string& version) const
{
    if (!is_sha256_hex(version)) return std::nullopt; // (never a path made of anything else)
    for (const fs::path& f : folders_)
        if (const auto t = read_file(manifest_path(f, version)); t && Sha256::hex_hash(*t) == version) return read_raid_manifest(*t);
    return std::nullopt;
}

std::vector<RaidManifest> Raid::list() const
{
    std::map<std::string, RaidManifest> all; // by id
    for (const fs::path& f : folders_)
    {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(f / "manifests", ec))
        {
            if (e.path().extension() != ".manifest") continue;
            const std::string id = e.path().stem().string();
            if (all.count(id) || !is_sha256_hex(id)) continue;
            const auto t = read_file(e.path());
            if (!t || Sha256::hex_hash(*t) != id) continue;
            try
            {
                all.emplace(id, read_raid_manifest(*t));
            }
            catch (const std::exception&)
            {
            }
        }
    }
    // By name; within a name, along each chain from its start.
    std::map<std::string, std::vector<std::string>> by_name;
    for (const auto& [id, m] : all) by_name[m.name].push_back(id);
    std::vector<RaidManifest> out;
    for (auto& [name, ids] : by_name)
    {
        auto depth = [&](std::string id) {
            size_t d = 0;
            while (all.count(id) && !all.at(id).previous.empty() && d <= all.size()) id = all.at(id).previous, ++d;
            return d;
        };
        // (each id's depth walked once, not twice in every comparison: a store of 684 versions took
        // 1.6 s to list and 3 s to put into; review 1, 14 November 2026)
        std::map<std::string, size_t> depths;
        for (const auto& id : ids) depths[id] = depth(id);
        std::sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) {
            const size_t da = depths.at(a), db = depths.at(b);
            return da != db ? da < db : a < b;
        });
        for (const auto& id : ids) out.push_back(all.at(id));
    }
    return out;
}

std::optional<std::string> Raid::latest(const std::string& name) const
{
    std::optional<std::string> best;
    size_t best_depth = 0;
    const auto all = list();
    // (each manifest's id worked out once: it is its whole text hashed)
    std::vector<std::string> ids;
    ids.reserve(all.size());
    for (const auto& m : all) ids.push_back(m.id());
    std::map<std::string, const RaidManifest*> by_id;
    for (size_t k = 0; k < all.size(); ++k) by_id[ids[k]] = &all[k];
    for (size_t k = 0; k < all.size(); ++k)
    {
        const RaidManifest& m = all[k];
        if (m.name != name) continue;
        size_t d = 0;
        std::string at = m.previous;
        while (!at.empty() && by_id.count(at) && d <= all.size()) at = by_id.at(at)->previous, ++d;
        const std::string& id = ids[k];
        if (!best || d > best_depth || (d == best_depth && id > *best)) best = id, best_depth = d;
    }
    return best;
}

RaidReport Raid::verify() const { return check(false); }
RaidReport Raid::repair() { return check(true); }

RaidReport Raid::check(bool fix) const
{
    RaidReport r;
    r.folders = folders_.size();
    const auto all = list();
    r.manifests = all.size();
    // Every manifest in every folder (they are small): a missing or damaged one written again.
    for (const auto& m : all)
    {
        const std::string id = m.id(), text = m.text();
        for (const fs::path& f : folders_)
        {
            const auto t = read_file(manifest_path(f, id));
            if (t && *t == text) continue;
            if (t) ++r.damaged, r.notes.push_back("manifest " + id.substr(0, 12) + " damaged in " + f.string());
            else r.notes.push_back("manifest " + id.substr(0, 12) + " missing from " + f.string());
            if (fix && write_file(manifest_path(f, id), text)) ++r.repaired;
        }
    }
    // Every chunk named by any manifest.
    std::map<std::string, uint64_t> chunks;
    for (const auto& m : all)
        for (const auto& [h, n] : m.chunks) chunks[h] = n;
    r.chunks = chunks.size();
    const size_t want = std::min<size_t>(copies_, folders_.size());
    for (const auto& [h, n] : chunks)
    {
        const std::vector<size_t> order = homes(h);
        std::optional<std::string> good;
        std::vector<size_t> have, bad;
        for (const size_t i : order)
        {
            const auto c = read_file(chunk_path(folders_[i], h));
            if (!c) continue;
            if (c->size() == n && Sha256::hex_hash(*c) == h)
            {
                have.push_back(i);
                if (!good) good = *c;
            }
            else bad.push_back(i);
        }
        r.good_copies += have.size();
        r.damaged += bad.size();
        for (const size_t i : bad) r.notes.push_back("chunk " + h.substr(0, 12) + " damaged in " + folders_[i].string());
        if (have.empty())
        {
            ++r.lost;
            r.notes.push_back("chunk " + h.substr(0, 12) + " lost: no good copy in any folder");
            continue;
        }
        if (have.size() < want)
        {
            ++r.missing;
            r.notes.push_back("chunk " + h.substr(0, 12) + " on " + std::to_string(have.size()) + " of the " + std::to_string(want) + " folders it should be on");
        }
        if (!fix) continue;
        // Damaged copies made good; then copies added, best homes first, until there are enough.
        for (const size_t i : bad)
            if (write_file(chunk_path(folders_[i], h), *good)) ++r.repaired, have.push_back(i);
        for (const size_t i : order)
        {
            if (have.size() >= want) break;
            if (std::find(have.begin(), have.end(), i) != have.end()) continue;
            if (write_file(chunk_path(folders_[i], h), *good)) ++r.repaired, have.push_back(i);
        }
    }
    return r;
}

} // namespace sieve
