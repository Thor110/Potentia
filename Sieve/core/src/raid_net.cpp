// Sieve — the infinite RAID, stage 3: between machines (see raid_net.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "sieve/raid_net.hpp"

#include "raid_files.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <system_error>

namespace sieve {

namespace fs = std::filesystem;
using raid_files::read_file;
using raid_files::write_file;

namespace {

bool is_hex64(std::string_view s)
{
    return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == std::string_view::npos;
}

std::string sha_hex(std::string_view bytes) { return Sha256::hex(Sha256::hash(bytes)); }

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

// A whole decimal number of at most 19 digits.
std::optional<uint64_t> number(std::string_view t)
{
    if (t.empty() || t.size() > 19 || t.find_first_not_of("0123456789") != std::string_view::npos) return std::nullopt;
    uint64_t v = 0;
    for (const char c : t) v = v * 10 + uint64_t(c - '0');
    return v;
}

} // namespace

// The held folder.

RaidHeld::RaidHeld(fs::path folder, HeldLimits limits) : folder_(std::move(folder)), limits_(limits)
{
    std::error_code ec;
    fs::create_directories(folder_, ec);
    if (!fs::is_directory(folder_, ec)) throw std::invalid_argument("not a folder, and cannot be made one: " + folder_.string());
    const fs::path tag = folder_ / "sieve-raid-held";
    const std::string want = std::string(kRaidHeldVersion) + " salt=";
    if (const auto t = read_file(tag))
    {
        if (t->rfind(want, 0) != 0 || t->size() < want.size() + 32) throw std::invalid_argument(folder_.string() + "'s sieve-raid-held file is not " + kRaidHeldVersion);
        salt_ = t->substr(want.size(), 32);
        if (salt_.find_first_not_of("0123456789abcdef") != std::string::npos) throw std::invalid_argument(folder_.string() + "'s salt is not hex");
    }
    else
    {
        static const char* kHex = "0123456789abcdef"; // from the system's random source, as a key is
        const SealKey r = new_seal_key();
        for (size_t i = 0; i < 16; ++i) salt_ += kHex[r[i] >> 4], salt_ += kHex[r[i] & 15];
        if (!write_file(tag, want + salt_ + "\n")) throw std::invalid_argument("cannot write " + tag.string());
    }
    if (const auto p = read_file(folder_ / "peers"))
        for (size_t i = 0; i < p->size();)
        {
            const size_t e = std::min(p->find('\n', i), p->size());
            const std::string l = p->substr(i, e - i);
            i = e + 1;
            const size_t sp = l.find(' ');
            if (sp != 64 || !is_hex64(l.substr(0, 64))) continue;
            if (const auto n = number(l.substr(65))) peers_.push_back({l.substr(0, 64), *n});
        }
    std::sort(peers_.begin(), peers_.end());
}

std::string RaidHeld::peer_key(const std::string& peer_address) const { return sha_hex(salt_ + peer_address); }

bool RaidHeld::has(const std::string& name) const
{
    std::error_code ec;
    return is_hex64(name) && fs::is_regular_file(blob_path(folder_, name), ec);
}

std::optional<std::string> RaidHeld::get(const std::string& name) const
{
    if (!is_hex64(name)) return std::nullopt;
    auto b = read_file(blob_path(folder_, name));
    if (!b || sha_hex(*b) != name) return std::nullopt;
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

void RaidHeld::save_peers() const
{
    std::string t;
    for (const auto& [k, n] : peers_) t += k + " " + std::to_string(n) + "\n";
    write_file(folder_ / "peers", t);
}

RaidHeld::Put RaidHeld::put(const std::string& name, const std::string& bytes, const std::string& peer_address)
{
    if (!is_hex64(name)) return {Put::Is::Refused, "not a name (64 hex digits)"};
    if (bytes.size() > kRaidNetMax) return {Put::Is::Refused, "too long"};
    if (sha_hex(bytes) != name) return {Put::Is::Refused, "its bytes are not its name"};
    const std::string k = peer_key(peer_address);
    std::lock_guard<std::mutex> lock(mx_); // (the same chunk, or the last of the space, asked for twice at once)
    if (get(name)) return {Put::Is::Had, ""};
    if (used_locked() + bytes.size() > limits_.space) return {Put::Is::Refused, "no space left that this machine gives"};
    if (used_by_locked(k) + bytes.size() > limits_.per_peer) return {Put::Is::Refused, "this address has had as much kept as one address may"};
    if (!write_file(blob_path(folder_, name), bytes)) return {Put::Is::Refused, "cannot write here"};
    const auto at = std::lower_bound(peers_.begin(), peers_.end(), std::make_pair(k, uint64_t(0)));
    if (at != peers_.end() && at->first == k)
        at->second += bytes.size();
    else
        peers_.insert(at, {k, uint64_t(bytes.size())});
    save_peers();
    return {Put::Is::Kept, ""};
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
        if (r.size() == 4 + 64 && r.rfind("has ", 0) == 0 && is_hex64(r.substr(4)))
        {
            if (!s.write(has(r.substr(4)) ? "yes\n" : "no\n")) return answered;
        }
        else if (r.size() == 4 + 64 && r.rfind("get ", 0) == 0 && is_hex64(r.substr(4)))
        {
            const auto b = get(r.substr(4));
            if (!s.write(b ? "blob " + std::to_string(b->size()) + "\n" + *b : std::string("no\n"))) return answered;
        }
        else if (r.size() > 4 + 64 + 1 && r.rfind("put ", 0) == 0 && is_hex64(r.substr(4, 64)) && r[68] == ' ')
        {
            const auto n = number(r.substr(69));
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

std::optional<std::string> RaidNetPeer::get(const std::string& name)
{
    if (!s_->write("get " + name + "\n")) throw std::runtime_error(address_ + " closed");
    const std::string a = line();
    if (a == "no") return std::nullopt;
    const auto n = a.rfind("blob ", 0) == 0 ? number(std::string_view(a).substr(5)) : std::nullopt;
    if (!n || *n > kRaidNetMax) throw std::runtime_error(address_ + " answered out of place: " + a);
    std::string b(size_t(*n), '\0');
    if (*n && !s_->read(b.data(), b.size())) throw std::runtime_error(address_ + " stopped part way");
    if (sha_hex(b) != name) return std::nullopt; // not what was asked for: as good as not had
    return b;
}

std::string RaidNetPeer::put(const std::string& name, const std::string& bytes)
{
    if (!s_->write("put " + name + " " + std::to_string(bytes.size()) + "\n") || !s_->write(bytes)) throw std::runtime_error(address_ + " closed");
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
    std::vector<std::pair<std::string, size_t>> rank;
    for (size_t i = 0; i < peers.size(); ++i) rank.push_back({sha_hex(name + peers[i]->address()), i});
    std::sort(rank.begin(), rank.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<size_t> out;
    for (const auto& r : rank) out.push_back(r.second);
    return out;
}

namespace {

// Puts one sealed piece on `want` of the peers, its homes first, going further down the ranking when
// one refuses or fails. Returns how many hold it.
size_t place(const std::string& name, const std::string& bytes, const std::vector<RaidPeer*>& peers, size_t want, RaidShareReport& r)
{
    size_t holding = 0;
    for (const size_t i : raid_peer_homes(name, peers))
    {
        if (holding >= want) break;
        RaidPeer& p = *peers[i];
        try
        {
            if (p.has(name))
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
    std::optional<RaidManifest> m;
    for (auto& x : raid.list())
        if (x.id() == version) m = std::move(x);
    if (!m) throw std::invalid_argument("no version " + version + " in the RAID");
    const std::string bytes = raid.get(version);
    const size_t want = std::min<size_t>(std::max<uint32_t>(copies, 1), peers.size());

    RaidShareReport r;
    r.chunks = m->chunks.size();
    std::string index = std::string(kRaidRemoteVersion) + "\n";
    const std::string sealed_manifest = seal(key, m->text());
    const std::string manifest_name = sha_hex(sealed_manifest);
    index += "manifest " + manifest_name + "\n";
    size_t at = 0;
    for (const auto& [h, n] : m->chunks)
    {
        const std::string sealed = seal(key, std::string_view(bytes).substr(at, size_t(n)));
        at += size_t(n);
        const std::string name = sha_hex(sealed);
        index += "blob " + name + "\n";
        if (place(name, sealed, peers, want, r) < want) ++r.short_of;
    }
    // The manifest and the index on every peer: small, and without them nothing else can be read.
    const std::string sealed_index = seal(key, index);
    r.remote = sha_hex(sealed_index);
    if (place(manifest_name, sealed_manifest, peers, peers.size(), r) < want) ++r.short_of;
    if (place(r.remote, sealed_index, peers, peers.size(), r) < want) ++r.short_of;
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
    if (!is_hex64(remote)) throw std::runtime_error("not a remote id (64 hex digits): " + remote);
    std::vector<std::string> tried;
    const std::string index = fetch_piece(remote, key, peers, tried);
    std::vector<std::string> lines;
    for (size_t i = 0; i < index.size();)
    {
        const size_t e = index.find('\n', i);
        if (e == std::string::npos) throw std::runtime_error("the index's last line has no line feed");
        lines.push_back(index.substr(i, e - i));
        i = e + 1;
    }
    if (lines.size() < 2 || lines[0] != kRaidRemoteVersion || lines[1].rfind("manifest ", 0) != 0 || !is_hex64(lines[1].substr(9)))
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
        if (l.rfind("blob ", 0) != 0 || !is_hex64(l.substr(5))) throw std::runtime_error("index line " + std::to_string(i + 3) + " is out of place");
        const std::string c = fetch_piece(l.substr(5), key, peers, tried);
        const auto& [h, n] = f.manifest.chunks[i];
        if (c.size() != n || sha_hex(c) != h) throw std::runtime_error("chunk " + std::to_string(i + 1) + " is not the one its manifest names");
        f.bytes += c;
    }
    if (f.bytes.size() != f.manifest.size) throw std::runtime_error("the chunks do not add up to the save's size");
    return f;
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
    const auto p = number(port);
    if (host.empty() || !p || *p == 0 || *p > 65535) throw std::invalid_argument("an address is HOST:PORT, the port 1 to 65535: " + a);
    return {host, port};
}

} // namespace sieve
