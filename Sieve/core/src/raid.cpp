// Sieve — the infinite RAID, stage 1 (see raid.hpp).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits. (A request, not a condition: whoever uses
// the RAID in a system run for profit is asked to give a share of that profit to charity, the share
// settled by open, democratic debate; IDEAS 19.177.)
#include "sieve/raid.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <system_error>

namespace sieve {

namespace fs = std::filesystem;

namespace {

bool is_hex64(const std::string& s)
{
    return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == std::string::npos;
}

std::optional<std::string> read_file(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return s;
}

// Written whole or not at all: to a file beside it, then renamed over it.
bool write_file(const fs::path& p, const std::string& bytes)
{
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    const fs::path tmp = p.string() + ".part";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), std::streamsize(bytes.size()));
        if (!out) return false;
    }
    fs::rename(tmp, p, ec);
    if (ec)
    {
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

fs::path chunk_path(const fs::path& folder, const std::string& h) { return folder / "chunks" / h.substr(0, 2) / h; }
fs::path manifest_path(const fs::path& folder, const std::string& id) { return folder / "manifests" / (id + ".manifest"); }

std::string sha_hex(const std::string& bytes) { return Sha256::hex(Sha256::hash(bytes)); }

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

std::string RaidManifest::id() const { return sha_hex(text()); }

RaidManifest read_raid_manifest(const std::string& text)
{
    std::vector<std::string> lines;
    for (size_t i = 0; i < text.size();)
    {
        const size_t e = text.find('\n', i);
        if (e == std::string::npos) throw std::invalid_argument("a manifest's last line has no line feed");
        lines.push_back(text.substr(i, e - i));
        i = e + 1;
    }
    if (lines.size() < 4 || lines[0] != kRaidManifestVersion) throw std::invalid_argument(std::string("not a manifest: it does not start with ") + kRaidManifestVersion);
    RaidManifest m;
    if (lines[1].rfind("name ", 0) != 0) throw std::invalid_argument("a manifest's second line is its name");
    m.name = lines[1].substr(5);
    auto number = [](const std::string& s) {
        if (s.empty() || s.size() > 19 || s.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument("not a number: " + s);
        return uint64_t(std::stoull(s));
    };
    if (lines[2].rfind("size ", 0) != 0) throw std::invalid_argument("a manifest's third line is its size");
    m.size = number(lines[2].substr(5));
    if (lines[3].rfind("previous ", 0) != 0) throw std::invalid_argument("a manifest's fourth line is its previous version");
    const std::string prev = lines[3].substr(9);
    if (prev != "none")
    {
        if (!is_hex64(prev)) throw std::invalid_argument("previous: not a SHA-256: " + prev);
        m.previous = prev;
    }
    uint64_t total = 0;
    for (size_t i = 4; i < lines.size(); ++i)
    {
        const std::string& l = lines[i];
        if (l.size() < 6 + 64 + 2 || l.rfind("chunk ", 0) != 0 || l[70] != ' ') throw std::invalid_argument("not a chunk's line: " + l);
        const std::string h = l.substr(6, 64);
        if (!is_hex64(h)) throw std::invalid_argument("not a SHA-256: " + h);
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
        std::error_code ec;
        fs::create_directories(f, ec);
        if (!fs::is_directory(f, ec)) throw std::invalid_argument("not a folder, and cannot be made one: " + f.string());
        const fs::path tag = f / "sieve-raid";
        std::string id;
        if (const auto t = read_file(tag))
        {
            const std::string want = std::string(kRaidVersion) + " id=";
            if (t->rfind(want, 0) != 0 || t->size() < want.size() + 32) throw std::invalid_argument(f.string() + "'s sieve-raid file is not " + kRaidVersion);
            id = t->substr(want.size(), 32);
            if (id.find_first_not_of("0123456789abcdef") != std::string::npos) throw std::invalid_argument(f.string() + "'s sieve-raid id is not hex");
        }
        else
        {
            std::random_device rd;
            static const char* kHex = "0123456789abcdef";
            for (int i = 0; i < 32; ++i) id += kHex[rd() & 15];
            if (!write_file(tag, std::string(kRaidVersion) + " id=" + id + "\n")) throw std::invalid_argument("cannot write " + tag.string());
        }
        if (std::find(ids_.begin(), ids_.end(), id) != ids_.end()) throw std::invalid_argument(f.string() + " is a folder given twice (the same sieve-raid id)");
        ids_.push_back(id);
    }
}

std::vector<size_t> Raid::homes(const std::string& chunk) const
{
    std::vector<std::pair<std::string, size_t>> rank;
    for (size_t i = 0; i < ids_.size(); ++i) rank.push_back({sha_hex(chunk + ids_[i]), i});
    std::sort(rank.begin(), rank.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<size_t> out;
    for (const auto& r : rank) out.push_back(r.second);
    return out;
}

std::string Raid::put(const std::string& name, const std::string& bytes)
{
    if (name.empty() || name.find('\n') != std::string::npos || name.find('\r') != std::string::npos)
        throw std::invalid_argument("a save's name is one line of text, not empty");
    RaidManifest m;
    m.name = name;
    m.size = bytes.size();
    const auto cuts = raid_cut(bytes);
    for (const auto& [at, n] : cuts) m.chunks.push_back({sha_hex(bytes.substr(at, n)), n});
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
    if (!is_hex64(version)) throw std::invalid_argument("not a version id (64 hex digits): " + version);
    std::optional<RaidManifest> m;
    for (const fs::path& f : folders_)
        if (const auto t = read_file(manifest_path(f, version)); t && sha_hex(*t) == version)
        {
            m = read_raid_manifest(*t);
            break;
        }
    if (!m) throw std::runtime_error("no good copy of version " + version + " in any folder");
    std::string out;
    out.reserve(size_t(m->size));
    for (const auto& [h, n] : m->chunks)
    {
        bool found = false;
        for (const size_t i : homes(h))
            if (const auto c = read_file(chunk_path(folders_[i], h)); c && c->size() == n && sha_hex(*c) == h)
            {
                out += *c;
                found = true;
                break;
            }
        if (!found) throw std::runtime_error("chunk " + h + " has no good copy in any folder (repair cannot help: it is lost)");
    }
    return out;
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
            if (all.count(id) || !is_hex64(id)) continue;
            const auto t = read_file(e.path());
            if (!t || sha_hex(*t) != id) continue;
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
        std::sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) {
            const size_t da = depth(a), db = depth(b);
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
    std::map<std::string, const RaidManifest*> by_id;
    for (const auto& m : all) by_id[m.id()] = &m;
    for (const auto& m : all)
    {
        if (m.name != name) continue;
        size_t d = 0;
        std::string at = m.previous;
        while (!at.empty() && by_id.count(at) && d <= all.size()) at = by_id.at(at)->previous, ++d;
        const std::string id = m.id();
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
            if (c->size() == n && sha_hex(*c) == h)
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
