// Sieve — the infinite RAID, stage 3: between machines (see raid_net.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "sieve/raid_net.hpp"

#include "raid_files.hpp"
#include "sieve/hex.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <optional>
#include <span>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace sieve {

namespace fs = std::filesystem;
using raid_files::read_file;
using raid_files::write_file;

namespace {

fs::path blob_path(const fs::path& folder, const std::string& h) { return folder / "blobs" / h.substr(0, 2) / h; }

// A line of at most kRaidNetLine bytes before its line feed; nothing if there is none.
std::optional<std::string> read_line(RaidStream& s)
{
    std::string l;
    for (;;)
    {
        char c;
        if (!s.read(&c, 1)) return std::nullopt;
        if (c == '\n') return l;
        if (l.size() >= kRaidNetLine) return std::nullopt;
        l += c;
    }
}

} // namespace

// The held folder.

RaidHeld::RaidHeld(fs::path folder, HeldLimits limits) : folder_(std::move(folder)), limits_(limits)
{
    salt_ = raid_files::folder_tag(folder_, "sieve-raid-held", kRaidHeldVersion, "salt", "salt", [] {
        const SealKey r = new_seal_key(); // from the system's random source, as a key is
        return hex_of(std::span(r.data(), 16));
    });
    if (const auto p = read_file(folder_ / "peers"))
        for (size_t i = 0; i < p->size();)
        {
            const size_t e = std::min(p->find('\n', i), p->size());
            const std::string l = p->substr(i, e - i);
            i = e + 1;
            const size_t sp = l.find(' ');
            if (sp != 64 || !is_sha256_hex(l.substr(0, 64))) continue;
            if (const auto n = decimal_u64(l.substr(65))) peers_.push_back({l.substr(0, 64), *n});
        }
    std::sort(peers_.begin(), peers_.end());
}

std::string RaidHeld::peer_key(const std::string& peer_address) const { return Sha256::hex_hash(salt_ + peer_address); }

bool RaidHeld::has(const std::string& name) const
{
    std::error_code ec;
    return is_sha256_hex(name) && fs::is_regular_file(blob_path(folder_, name), ec);
}

std::optional<std::string> RaidHeld::get(const std::string& name) const
{
    if (!is_sha256_hex(name)) return std::nullopt;
    auto b = read_file(blob_path(folder_, name));
    if (!b || Sha256::hex_hash(*b) != name) return std::nullopt;
    return b;
}

uint64_t RaidHeld::used_locked() const
{
    uint64_t n = 0;
    for (const auto& p : peers_) n += p.second;
    return n;
}

uint64_t RaidHeld::used_by_locked(const std::string& k) const
{
    const auto at = std::lower_bound(peers_.begin(), peers_.end(), std::make_pair(k, uint64_t(0)));
    return at != peers_.end() && at->first == k ? at->second : 0;
}

uint64_t RaidHeld::used() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return used_locked();
}

uint64_t RaidHeld::used_by(const std::string& peer_address) const
{
    const std::string k = peer_key(peer_address);
    std::lock_guard<std::mutex> lock(mx_);
    return used_by_locked(k);
}

std::string RaidHeld::peers_text_locked() const
{
    std::string t;
    for (const auto& [k, n] : peers_) t += k + " " + std::to_string(n) + "\n";
    return t;
}

void RaidHeld::save_peers(const std::string& text, uint64_t generation)
{
    std::lock_guard<std::mutex> lock(peers_file_mx_);
    if (generation <= peers_written_) return; // (a later view of the counts is already there)
    write_file(folder_ / "peers", text);
    peers_written_ = generation;
}

void RaidHeld::count_locked(const std::string& k, int64_t bytes)
{
    const auto at = std::lower_bound(peers_.begin(), peers_.end(), std::make_pair(k, uint64_t(0)));
    if (at != peers_.end() && at->first == k)
    {
        at->second = uint64_t(int64_t(at->second) + bytes);
        if (at->second == 0) peers_.erase(at); // (a reservation given back: as if it had never been)
    }
    else if (bytes > 0)
        peers_.insert(at, {k, uint64_t(bytes)});
}

RaidHeld::Put RaidHeld::put(const std::string& name, const std::string& bytes, const std::string& peer_address)
{
    if (!is_sha256_hex(name)) return {Put::Is::Refused, "not a name (64 hex digits)"};
    if (bytes.size() > kRaidNetMax) return {Put::Is::Refused, "too long"};
    if (Sha256::hex_hash(bytes) != name) return {Put::Is::Refused, "its bytes are not its name"};
    const std::string k = peer_key(peer_address);
    // The same piece asked for twice at once: the second waits for the first, then finds it had.
    {
        std::unique_lock<std::mutex> lock(mx_);
        writing_done_.wait(lock, [&] { return !writing_.count(name); });
        writing_.insert(name);
    }
    auto done = [&](Put p, int64_t give_back) {
        std::string text;
        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (give_back) count_locked(k, -give_back);
            writing_.erase(name);
            if (p.is == Put::Is::Kept || give_back)
            {
                text = peers_text_locked();
                generation = ++peers_made_;
            }
        }
        writing_done_.notify_all();
        if (generation) save_peers(text, generation);
        return p;
    };
    if (get(name)) return done({Put::Is::Had, ""}, 0);
    // The space counted before the write (the last of it, asked for twice at once, goes to one).
    {
        std::lock_guard<std::mutex> lock(mx_);
        const char* refused = used_locked() + bytes.size() > limits_.space ? "no space left that this machine gives"
                              : used_by_locked(k) + bytes.size() > limits_.per_peer ? "this address has had as much kept as one address may"
                                                                                     : nullptr;
        if (refused)
        {
            writing_.erase(name);
            writing_done_.notify_all();
            return {Put::Is::Refused, refused};
        }
        count_locked(k, int64_t(bytes.size()));
    }
    if (!write_file(blob_path(folder_, name), bytes, ".part" + std::to_string(++parts_))) return done({Put::Is::Refused, "cannot write here"}, int64_t(bytes.size()));
    return done({Put::Is::Kept, ""}, 0);
}

size_t RaidHeld::serve(RaidStream& s, const std::string& peer_address)
{
    const auto hello = read_line(s);
    if (!hello || *hello != kRaidNetVersion)
    {
        s.write("refused not sieve-raid-net-v1\n");
        return 0;
    }
    if (!s.write(std::string(kRaidNetVersion) + "\n")) return 0;
    size_t answered = 0;
    for (;;)
    {
        const auto l = read_line(s);
        if (!l || *l == "bye") return answered;
        const std::string& r = *l;
        if (r.size() == 4 + 64 && r.rfind("has ", 0) == 0 && is_sha256_hex(r.substr(4)))
        {
            if (!s.write(has(r.substr(4)) ? "yes\n" : "no\n")) return answered;
        }
        else if (r.size() == 4 + 64 && r.rfind("get ", 0) == 0 && is_sha256_hex(r.substr(4)))
        {
            const auto b = get(r.substr(4));
            if (!s.write(b ? "blob " + std::to_string(b->size()) + "\n" + *b : std::string("no\n"))) return answered;
        }
        else if (r.size() > 4 + 64 + 1 && r.rfind("put ", 0) == 0 && is_sha256_hex(r.substr(4, 64)) && r[68] == ' ')
        {
            const auto n = decimal_u64(r.substr(69));
            if (!n || *n > kRaidNetMax)
            {
                s.write("refused too long\n");
                return answered;
            }
            std::string bytes(size_t(*n), '\0');
            if (*n && !s.read(bytes.data(), bytes.size())) return answered;
            const Put p = put(r.substr(4, 64), bytes, peer_address);
            const std::string a = p.is == Put::Is::Kept ? "kept\n" : p.is == Put::Is::Had ? "had\n" : "refused " + p.why + "\n";
            if (!s.write(a)) return answered;
        }
        else
        {
            s.write("refused not a request\n");
            return answered;
        }
        ++answered;
    }
}

// Peers.

RaidNetPeer::RaidNetPeer(std::unique_ptr<RaidStream> stream, std::string address) : s_(std::move(stream)), address_(std::move(address))
{
    if (!s_->write(std::string(kRaidNetVersion) + "\n")) throw std::runtime_error(address_ + " did not take the greeting");
    const std::string a = line();
    if (a != kRaidNetVersion) throw std::runtime_error(address_ + " answered: " + a);
}

RaidNetPeer::~RaidNetPeer()
{
    if (s_) s_->write("bye\n");
}

std::string RaidNetPeer::line()
{
    const auto l = read_line(*s_);
    if (!l) throw std::runtime_error(address_ + " did not answer");
    return *l;
}

bool RaidNetPeer::has(const std::string& name)
{
    if (!s_->write("has " + name + "\n")) throw std::runtime_error(address_ + " closed");
    const std::string a = line();
    if (a != "yes" && a != "no") throw std::runtime_error(address_ + " answered out of place: " + a);
    return a == "yes";
}

std::vector<bool> RaidPeer::has_each(const std::vector<std::string>& names)
{
    std::vector<bool> out;
    out.reserve(names.size());
    for (const auto& n : names) out.push_back(has(n));
    return out;
}

std::vector<bool> RaidNetPeer::has_each(const std::vector<std::string>& names)
{
    std::vector<bool> out;
    out.reserve(names.size());
    for (size_t at = 0; at < names.size(); at += kRaidAskedTogether)
    {
        const size_t end = std::min(names.size(), at + kRaidAskedTogether);
        std::string asks;
        for (size_t i = at; i < end; ++i) asks += "has " + names[i] + "\n";
        if (!s_->write(asks)) throw std::runtime_error(address_ + " closed");
        for (size_t i = at; i < end; ++i)
        {
            const std::string a = line();
            if (a != "yes" && a != "no") throw std::runtime_error(address_ + " answered out of place: " + a);
            out.push_back(a == "yes");
        }
    }
    return out;
}

std::optional<std::string> RaidNetPeer::get(const std::string& name)
{
    if (!s_->write("get " + name + "\n")) throw std::runtime_error(address_ + " closed");
    const std::string a = line();
    if (a == "no") return std::nullopt;
    const auto n = a.rfind("blob ", 0) == 0 ? decimal_u64(std::string_view(a).substr(5)) : std::nullopt;
    if (!n || *n > kRaidNetMax) throw std::runtime_error(address_ + " answered out of place: " + a);
    std::string b(size_t(*n), '\0');
    if (*n && !s_->read(b.data(), b.size())) throw std::runtime_error(address_ + " stopped part way");
    if (Sha256::hex_hash(b) != name) return std::nullopt; // not what was asked for: as good as not had
    return b;
}

std::string RaidNetPeer::put(const std::string& name, const std::string& bytes)
{
    // (the request and its bytes in one write: two writes then a wait for the answer stalled on the
    // network's acknowledgements; review 1)
    std::string msg = "put " + name + " " + std::to_string(bytes.size()) + "\n";
    msg += bytes;
    if (!s_->write(msg)) throw std::runtime_error(address_ + " closed");
    const std::string a = line();
    if (a == "kept" || a == "had") return "";
    if (a.rfind("refused ", 0) == 0) return a.substr(8);
    throw std::runtime_error(address_ + " answered out of place: " + a);
}

std::string RaidLocalPeer::put(const std::string& name, const std::string& bytes)
{
    const auto p = held_.put(name, bytes, from_);
    return p.is == RaidHeld::Put::Is::Refused ? p.why : "";
}

// Sharing and fetching.

std::vector<size_t> raid_peer_homes(const std::string& name, const std::vector<RaidPeer*>& peers)
{
    std::vector<std::string> addresses;
    addresses.reserve(peers.size());
    for (const RaidPeer* p : peers) addresses.push_back(p->address());
    return raid_rank(name, addresses);
}

namespace {

// What each peer said it holds of a group of pieces, asked of it together (has_each: OPTIMISATIONS 2),
// so placing them asks no peer again one piece at a time. A peer that could not answer so is asked one
// piece at a time, as before, so a peer away gives the same notes as it always has. A piece put on a
// peer since counts as held there, as asking it again would have found.
class Asked
{
public:
    void ask(const std::vector<std::string>& names, const std::vector<RaidPeer*>& peers)
    {
        said_.assign(peers.size(), std::nullopt);
        for (size_t i = 0; i < peers.size(); ++i)
            try
            {
                const std::vector<bool> a = peers[i]->has_each(names);
                std::unordered_map<std::string, bool> m;
                for (size_t k = 0; k < names.size() && k < a.size(); ++k) m.emplace(names[k], a[k]);
                said_[i] = std::move(m);
            }
            catch (const std::exception&)
            {
                // (asked one at a time when it comes to it, and its trouble noted then, as before)
            }
    }
    // Throws as RaidPeer::has does, where the peer is asked.
    bool has(size_t i, RaidPeer& p, const std::string& name) const
    {
        if (put_.count({i, name})) return true;
        if (i < said_.size() && said_[i])
            if (const auto it = said_[i]->find(name); it != said_[i]->end()) return it->second;
        return p.has(name);
    }
    void kept(size_t i, const std::string& name) { put_.insert({i, name}); }

private:
    std::vector<std::optional<std::unordered_map<std::string, bool>>> said_;
    std::set<std::pair<size_t, std::string>> put_;
};

// Puts one sealed piece on `want` of the peers, its homes first, going further down the ranking when
// one refuses or fails. Returns how many hold it.
size_t place(const std::string& name, const std::string& bytes, const std::vector<RaidPeer*>& peers, size_t want, RaidShareReport& r, Asked& asked)
{
    size_t holding = 0;
    for (const size_t i : raid_peer_homes(name, peers))
    {
        if (holding >= want) break;
        RaidPeer& p = *peers[i];
        try
        {
            if (asked.has(i, p, name))
            {
                ++r.had;
                ++holding;
                continue;
            }
            const std::string why = p.put(name, bytes);
            if (why.empty())
            {
                ++r.sent;
                r.bytes_sent += bytes.size();
                ++holding;
                asked.kept(i, name);
            }
            else
                r.notes.push_back(p.address() + " refused " + name.substr(0, 12) + ": " + why);
        }
        catch (const std::exception& e)
        {
            r.notes.push_back(e.what());
        }
    }
    return holding;
}

} // namespace

RaidShareReport raid_share(const Raid& raid, const std::string& version, const SealKey& key, const std::vector<RaidPeer*>& peers, uint32_t copies)
{
    if (peers.empty()) throw std::invalid_argument("no peers to share with");
    // The one manifest asked for, not every one in every folder. A file named by its own SHA-256 that
    // is no manifest is not a version, as list() has always had it.
    std::optional<RaidManifest> m;
    try
    {
        m = raid.manifest(version);
    }
    catch (const std::invalid_argument&)
    {
    }
    if (!m) throw std::invalid_argument("no version " + version + " in the RAID");
    const std::string bytes = raid.get(version);
    const size_t want = std::min<size_t>(std::max<uint32_t>(copies, 1), peers.size());

    RaidShareReport r;
    r.chunks = m->chunks.size();
    std::string index = std::string(kRaidRemoteVersion) + "\n";
    const std::string sealed_manifest = seal(key, m->text());
    const std::string manifest_name = Sha256::hex_hash(sealed_manifest);
    index += "manifest " + manifest_name + "\n";
    // The chunks sealed a group at a time (kRaidAskedTogether of them, or 64 MiB), every peer asked
    // which of the group it holds in one go, then each placed in order as before.
    Asked asked;
    std::vector<std::string> names, sealed;
    auto place_group = [&] {
        asked.ask(names, peers);
        for (size_t k = 0; k < names.size(); ++k)
            if (place(names[k], sealed[k], peers, want, r, asked) < want) ++r.short_of;
        names.clear();
        sealed.clear();
    };
    size_t at = 0, group_bytes = 0;
    for (const auto& [h, n] : m->chunks)
    {
        sealed.push_back(seal(key, std::string_view(bytes).substr(at, size_t(n))));
        at += size_t(n);
        group_bytes += sealed.back().size();
        names.push_back(Sha256::hex_hash(sealed.back()));
        index += "blob " + names.back() + "\n";
        if (names.size() >= kRaidAskedTogether || group_bytes >= (size_t(64) << 20))
        {
            place_group();
            group_bytes = 0;
        }
    }
    place_group();
    // The manifest and the index as every chunk is: on `copies` of its homes (13 November 2026; on
    // every peer before, which does not grow with the network).
    const std::string sealed_index = seal(key, index);
    r.remote = Sha256::hex_hash(sealed_index);
    names = {manifest_name, r.remote};
    sealed = {sealed_manifest, sealed_index};
    place_group();
    return r;
}

namespace {

std::string fetch_piece(const std::string& name, const SealKey& key, const std::vector<RaidPeer*>& peers, std::vector<std::string>& tried)
{
    for (const size_t i : raid_peer_homes(name, peers))
    {
        try
        {
            if (const auto b = peers[i]->get(name))
            {
                if (auto plain = unseal(key, *b)) return *plain;
                tried.push_back(peers[i]->address() + ": " + name.substr(0, 12) + " does not open with this key");
            }
        }
        catch (const std::exception& e)
        {
            tried.push_back(e.what());
        }
    }
    std::string why = "no peer gave " + name;
    for (const auto& t : tried) why += "; " + t;
    throw std::runtime_error(why);
}

} // namespace

RaidFetched raid_fetch(const std::string& remote, const SealKey& key, const std::vector<RaidPeer*>& peers)
{
    if (!is_sha256_hex(remote)) throw std::runtime_error("not a remote id (64 hex digits): " + remote);
    std::vector<std::string> tried;
    const std::string index = fetch_piece(remote, key, peers, tried);
    std::vector<std::string> lines;
    if (!lf_lines(index, lines)) throw std::runtime_error("the index's last line has no line feed");
    if (lines.size() < 2 || lines[0] != kRaidRemoteVersion || lines[1].rfind("manifest ", 0) != 0 || !is_sha256_hex(lines[1].substr(9)))
        throw std::runtime_error(std::string("not an index: it does not start ") + kRaidRemoteVersion + " and a manifest");
    RaidFetched f;
    try
    {
        f.manifest = read_raid_manifest(fetch_piece(lines[1].substr(9), key, peers, tried));
    }
    catch (const std::invalid_argument& e)
    {
        throw std::runtime_error(std::string("its manifest: ") + e.what());
    }
    if (lines.size() - 2 != f.manifest.chunks.size()) throw std::runtime_error("the index and the manifest name different numbers of chunks");
    f.bytes.reserve(size_t(f.manifest.size));
    for (size_t i = 0; i < f.manifest.chunks.size(); ++i)
    {
        const std::string& l = lines[i + 2];
        if (l.rfind("blob ", 0) != 0 || !is_sha256_hex(l.substr(5))) throw std::runtime_error("index line " + std::to_string(i + 3) + " is out of place");
        const std::string c = fetch_piece(l.substr(5), key, peers, tried);
        const auto& [h, n] = f.manifest.chunks[i];
        if (c.size() != n || Sha256::hex_hash(c) != h) throw std::runtime_error("chunk " + std::to_string(i + 1) + " is not the one its manifest names");
        f.bytes += c;
    }
    if (f.bytes.size() != f.manifest.size) throw std::runtime_error("the chunks do not add up to the save's size");
    return f;
}

RaidCheckReport raid_check(const std::string& remote, const SealKey& key, const std::vector<RaidPeer*>& peers, uint32_t copies, bool repair)
{
    if (!is_sha256_hex(remote)) throw std::runtime_error("not a remote id (64 hex digits): " + remote);
    if (peers.empty()) throw std::runtime_error("no peers to check");
    const size_t want = std::min<size_t>(std::max<uint32_t>(copies, 1), peers.size());
    std::vector<std::string> tried;
    const std::string index = fetch_piece(remote, key, peers, tried); // (opened with the key: it is the owner's)
    std::vector<std::string> names{remote};
    for (size_t i = 0; i < index.size();)
    {
        const size_t e = index.find('\n', i);
        if (e == std::string::npos) break;
        const std::string l = index.substr(i, e - i);
        i = e + 1;
        if (l.rfind("manifest ", 0) == 0 && is_sha256_hex(l.substr(9))) names.push_back(l.substr(9));
        else if (l.rfind("blob ", 0) == 0 && is_sha256_hex(l.substr(5))) names.push_back(l.substr(5));
    }
    RaidCheckReport c;
    // Every peer asked which pieces it holds in one go (OPTIMISATIONS 2), then each piece counted in
    // order as before.
    Asked asked;
    asked.ask(names, peers);
    for (const std::string& name : names)
    {
        ++c.pieces;
        size_t holding = 0;
        for (size_t i = 0; i < peers.size(); ++i)
            try
            {
                holding += asked.has(i, *peers[i], name) ? 1 : 0;
            }
            catch (const std::exception& e)
            {
                c.notes.push_back(peers[i]->address() + ": " + e.what());
            }
        if (holding >= want)
        {
            ++c.whole;
            continue;
        }
        if (holding == 0)
        {
            ++c.lost;
            c.notes.push_back(name.substr(0, 12) + " is held by none of the peers given");
            continue;
        }
        ++c.short_of;
        if (!repair) continue;
        // Its sealed bytes from one that holds it, checked against its name, then on its next homes.
        std::optional<std::string> sealed;
        for (const size_t i : raid_peer_homes(name, peers))
            try
            {
                if (auto b = peers[i]->get(name); b && Sha256::hex_hash(*b) == name)
                {
                    sealed = std::move(b);
                    break;
                }
            }
            catch (const std::exception&)
            {
            }
        if (!sealed)
        {
            c.notes.push_back(name.substr(0, 12) + " could not be had whole from those that said they hold it");
            continue;
        }
        RaidShareReport r;
        if (place(name, *sealed, peers, want, r, asked) >= want) ++c.repaired;
        c.bytes_sent += r.bytes_sent;
        for (auto& n : r.notes) c.notes.push_back(std::move(n));
    }
    return c;
}

std::pair<std::string, std::string> raid_split_address(const std::string& a)
{
    std::string host, port;
    if (!a.empty() && a[0] == '[')
    {
        const size_t e = a.find("]:");
        if (e == std::string::npos) throw std::invalid_argument("an address is HOST:PORT or [IPv6]:PORT: " + a);
        host = a.substr(1, e - 1);
        port = a.substr(e + 2);
    }
    else
    {
        const size_t c = a.rfind(':');
        if (c == std::string::npos || a.find(':') != c) throw std::invalid_argument("an address is HOST:PORT or [IPv6]:PORT: " + a);
        host = a.substr(0, c);
        port = a.substr(c + 1);
    }
    const auto p = decimal_u64(port);
    if (host.empty() || !p || *p == 0 || *p > 65535) throw std::invalid_argument("an address is HOST:PORT, the port 1 to 65535: " + a);
    return {host, port};
}

} // namespace sieve
