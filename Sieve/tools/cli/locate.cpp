// The file locator and the manifest creator (locate.hpp).

#include "cli/locate.hpp"

#include <functional>
#include <iterator>
#include "cli/vault.hpp"

#include "sieve/rope_wave.hpp"
#include "sieve/sha256.hpp"

#include "Bra.h"
#include "Lzma2Dec.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <span>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

// ---- v4: the two streams unpacked, and each file's way followed

void* lzma_alloc(ISzAllocPtr, size_t size) { return size ? std::malloc(size) : nullptr; }
void lzma_free(ISzAllocPtr, void* address) { std::free(address); }
const ISzAlloc kLzmaAlloc = {lzma_alloc, lzma_free};

// An LZMA2 dictionary size as its property byte: the smallest p whose size, (2 | (p & 1)) <<
// (p / 2 + 11), holds it (40: 4 GiB less one byte).
uint8_t lzma2_property(uint32_t dictionary)
{
    for (uint8_t p = 0; p < 40; ++p)
        if (uint64_t(dictionary) <= (uint64_t(2 | (p & 1)) << (p / 2 + 11))) return p;
    return 40;
}

// One stream: raw LZMA2 of `bytes` bytes, then (x86) the x86 branch filter undone, as 7-Zip's BCJ
// and xz's x86 filter both define it, from position 0.
std::vector<uint8_t> unpack_stream(std::span<const uint8_t> packed, const PackedStream& s, bool x86)
{
    std::vector<uint8_t> out(size_t(s.bytes));
    if (s.bytes == 0)
    {
        if (!packed.empty() && !(packed.size() == 1 && packed[0] == 0)) throw std::runtime_error("the installer's packed files are damaged");
        return out;
    }
    if (s.dictionary == 0)
    {
        // v12: carried as it is (only the lzma2 stream; the reader checks that).
        if (x86 || packed.size() != out.size()) throw std::runtime_error("the installer's packed files are damaged");
        std::copy(packed.begin(), packed.end(), out.begin());
        return out;
    }
    SizeT out_len = out.size(), in_len = packed.size();
    ELzmaStatus status;
    const SRes r = Lzma2Decode(out.data(), &out_len, packed.data(), &in_len, lzma2_property(s.dictionary), LZMA_FINISH_END, &status, &kLzmaAlloc);
    if (r != SZ_OK || out_len != out.size() || in_len != packed.size()) throw std::runtime_error("the installer's packed files are damaged");
    if (x86)
    {
        UInt32 state = Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL;
        z7_BranchConvSt_X86_Dec(out.data(), out.size(), 0, &state);
    }
    return out;
}

// A text's lines: its bytes split at each line feed (n line feeds make n + 1 lines).
std::vector<std::span<const uint8_t>> lines_of(const std::vector<uint8_t>& t)
{
    std::vector<std::span<const uint8_t>> out;
    size_t at = 0;
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i] == '\n')
        {
            out.emplace_back(t.data() + at, i - at);
            at = i + 1;
        }
    out.emplace_back(t.data() + at, t.size() - at);
    return out;
}

// Every file's bytes from a v4 manifest's streams, in manifest order.
ItemMaker& item_maker()
{
    static ItemMaker f;
    return f;
}

Remaker& remaker()
{
    static Remaker f;
    return f;
}

std::vector<uint8_t> unpack_v4(Manifest& m)
{
    if (m.streams.size() != m.x86.packed + m.lzma2.packed) throw std::runtime_error("manifest: its streams are not the size its lines say");
    const std::span<const uint8_t> all(m.streams);
    const std::vector<uint8_t> x86 = unpack_stream(all.subspan(0, size_t(m.x86.packed)), m.x86, true);
    const std::vector<uint8_t> plain = unpack_stream(all.subspan(size_t(m.x86.packed)), m.lzma2, false);
    std::vector<ManifestEntry*> files;
    for (ManifestEntry& e : m.entries)
        if (!e.dir) files.push_back(&e);
    std::vector<std::vector<uint8_t>> bytes(files.size());
    std::vector<std::pair<size_t, std::vector<uint8_t>>> masks(files.size()); // derived: its base, its mask
    size_t xa = 0, pa = 0, ra = 0;
    auto take = [](const std::vector<uint8_t>& from, size_t& at, uint64_t n, const std::string& path) {
        if (from.size() - at < n) throw std::runtime_error("the installer ends before " + path);
        std::vector<uint8_t> out(from.begin() + std::ptrdiff_t(at), from.begin() + std::ptrdiff_t(at + n));
        at += size_t(n);
        return out;
    };
    // First what is carried as it is, and each mask; then each derived file, from its base.
    std::vector<size_t> base_lines(files.size(), 0);
    for (size_t i = 0; i < files.size(); ++i)
    {
        // v14: a file remade from what it holds is that first (its inner size), then remade below.
        ManifestEntry e = *files[i];
        if (!e.remake.empty())
        {
            const std::string inner = e.remake.substr(e.remake.rfind(' ') + 1);
            if (inner.empty() || inner.size() > 19 || inner.find_first_not_of("0123456789") != std::string::npos ||
                (e.way != "raw" && e.way.rfind("rope ", 0) != 0 && e.way.rfind("pieces ", 0) != 0))
                throw std::runtime_error("manifest: " + e.path + " is remade from what no way gives");
            e.size = std::stoull(inner);
        }
        if (e.way == "x86") bytes[i] = take(x86, xa, e.size, e.path);
        else if (e.way == "raw") bytes[i] = take(plain, pa, e.size, e.path);
        else if (m.items && e.way.rfind("item ", 0) == 0)
        {
            // An item of one of Sieve's lines: made again from its line and its place there.
            const std::string rest = e.way.substr(5);
            const size_t sp = rest.find(' ');
            const size_t k = sp == std::string::npos ? m.lines.size() : size_t(std::stoull(rest.substr(0, sp)));
            if (k >= m.lines.size()) throw std::runtime_error("manifest: " + e.path + " names a line it does not list");
            if (!item_maker())
                throw std::runtime_error("these Sieve instructions carry items of Sieve's lines (" + e.path +
                                         "): open them with Sieve (sieve install, or the hallway's File Locator), which makes them again");
            bytes[i] = item_maker()(m.lines[k], rest.substr(sp + 1));
        }
        else if (m.pieces && m.roped && e.way.rfind("pieces ", 0) == 0)
        {
            // v13: "pieces z17733 r4121949 rope <parameters>": its z pieces next in the lzma2 stream,
            // its r pieces joined and coded by the rope, next in the rope stream; laid out in order.
            const std::string w = e.way.substr(7);
            const size_t rp_at = w.find(" rope ");
            if (rp_at == std::string::npos) throw std::runtime_error("manifest: " + e.path + "'s pieces name no rope");
            std::vector<std::pair<char, uint64_t>> list;
            uint64_t zsum = 0, rsum = 0;
            for (size_t a = 0; a < rp_at;)
            {
                size_t sp = w.find(' ', a);
                if (sp == std::string::npos || sp > rp_at) sp = rp_at;
                const std::string t = w.substr(a, sp - a);
                if (t.size() < 2 || (t[0] != 'z' && t[0] != 'r') || t.find_first_not_of("0123456789", 1) != std::string::npos || t[1] == '0' ||
                    t.size() > 17 || (!list.empty() && list.back().first == t[0]))
                    throw std::runtime_error("manifest: " + e.path + "'s pieces are not written as Sieve writes them");
                const uint64_t n = std::stoull(t.substr(1));
                list.emplace_back(t[0], n);
                (t[0] == 'z' ? zsum : rsum) += n;
                a = sp + 1;
            }
            if (list.size() < 2 || zsum + rsum != e.size) throw std::runtime_error("manifest: " + e.path + "'s pieces are not its size");
            const std::vector<uint8_t> z = take(plain, pa, zsum, e.path);
            uint64_t len = 0;
            for (int shift = 0;; shift += 7)
            {
                if (ra >= m.rope_carried.size() || shift > 56) throw std::runtime_error("the installer's rope stream ends before " + e.path);
                const uint8_t c = m.rope_carried[ra++];
                len |= uint64_t(c & 0x7F) << shift;
                if (!(c & 0x80)) break;
            }
            if (m.rope_carried.size() - ra < len) throw std::runtime_error("the installer's rope stream ends before " + e.path);
            std::vector<uint8_t> rbytes;
            try
            {
                const RopeParams prp = RopeParams::parse(w.substr(rp_at + 6));
                if (prp.above && !m.rope_above)
                    throw std::invalid_argument("a rope-wave-v5 way (the byte above) in " + std::string(m.version()) + ", which has none");
                rbytes = rope_file(std::span<const uint8_t>(m.rope_carried.data() + ra, size_t(len)), prp, rsum);
            }
            catch (const std::invalid_argument& x)
            {
                throw std::runtime_error("manifest: " + e.path + ": " + x.what());
            }
            ra += size_t(len);
            bytes[i].reserve(size_t(e.size));
            size_t za = 0, rb = 0;
            for (const auto& [k, n] : list)
            {
                const std::vector<uint8_t>& from = k == 'z' ? z : rbytes;
                size_t& at = k == 'z' ? za : rb;
                bytes[i].insert(bytes[i].end(), from.begin() + std::ptrdiff_t(at), from.begin() + std::ptrdiff_t(at + n));
                at += size_t(n);
            }
        }
        else if (m.roped && e.way.rfind("rope ", 0) == 0)
        {
            // v8: made again from its code, next in the rope stream (a varint of its length first).
            uint64_t len = 0;
            for (int shift = 0;; shift += 7)
            {
                if (ra >= m.rope_carried.size() || shift > 56) throw std::runtime_error("the installer's rope stream ends before " + e.path);
                const uint8_t b = m.rope_carried[ra++];
                len |= uint64_t(b & 0x7F) << shift;
                if (!(b & 0x80)) break;
            }
            if (m.rope_carried.size() - ra < len) throw std::runtime_error("the installer's rope stream ends before " + e.path);
            try
            {
                const RopeParams rp = RopeParams::parse(e.way.substr(5));
                if (rp.columns && !m.rope_columns)
                    throw std::invalid_argument("a rope-wave-v2 way (columns) in " + std::string(kRopeManifestVersion) + ", which has none");
                if (rp.columns == 2 && !m.rope_lines)
                    throw std::invalid_argument("a rope-wave-v3 way (columns by line) in " + std::string(m.version()) + ", which has none");
                if (rp.grouped && !m.rope_groups)
                    throw std::invalid_argument("a rope-wave-v4 way (columns sharing tables) in " + std::string(m.version()) + ", which has none");
                if (rp.above && !m.rope_above)
                    throw std::invalid_argument("a rope-wave-v5 way (the byte above) in " + std::string(m.version()) + ", which has none");
                bytes[i] = rope_file(std::span<const uint8_t>(m.rope_carried.data() + ra, size_t(len)), rp, e.size);
            }
            catch (const std::invalid_argument& x)
            {
                throw std::runtime_error("manifest: " + e.path + ": " + x.what());
            }
            ra += size_t(len);
        }
        else if (e.way.rfind("lines ", 0) != 0) throw std::runtime_error("manifest: " + e.path + " has no way to be carried");
        if (!e.remake.empty())
        {
            if (!remaker())
                throw std::runtime_error("these Sieve instructions remake an archive from what it holds (" + e.path +
                                         "): open them with Sieve (sieve install, or the hallway's File Locator), which remakes it");
            bytes[i] = remaker()(e.remake, bytes[i]);
            if (bytes[i].size() != files[i]->size)
                throw std::runtime_error(e.path + " was not remade as it was (" + std::to_string(bytes[i].size()) + " bytes, not " +
                                         std::to_string(files[i]->size) + "): this Sieve's compressor writes differently from the one that made the instructions");
        }
    }
    if (ra != m.rope_carried.size()) throw std::runtime_error("manifest: its rope stream holds more than its files");
    for (size_t i = 0; i < files.size(); ++i)
    {
        const ManifestEntry& e = *files[i];
        if (e.way.rfind("lines ", 0) != 0) continue;
        const size_t k = size_t(std::stoull(e.way.substr(6)));
        if (k >= files.size() || (files[k]->way != "x86" && files[k]->way != "raw" && files[k]->way.rfind("item ", 0) != 0 &&
                                  files[k]->way.rfind("rope ", 0) != 0 && files[k]->way.rfind("pieces ", 0) != 0))
            throw std::runtime_error("manifest: " + e.path + " is derived from a file that is not carried");
        const auto lines = lines_of(bytes[k]);
        masks[i] = {k, take(plain, pa, (lines.size() + 7) / 8, e.path)};
    }
    if (xa != x86.size() || pa != plain.size()) throw std::runtime_error("manifest: its streams hold more than its files");
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (files[i]->way.rfind("lines ", 0) != 0) continue;
        const auto lines = lines_of(bytes[masks[i].first]);
        const std::vector<uint8_t>& mask = masks[i].second;
        bool first = true;
        for (size_t l = 0; l < lines.size(); ++l)
            if (mask[l / 8] >> (l % 8) & 1)
            {
                if (!first) bytes[i].push_back('\n');
                bytes[i].insert(bytes[i].end(), lines[l].begin(), lines[l].end());
                first = false;
            }
    }
    std::vector<uint8_t> contents;
    contents.reserve(size_t(m.bytes));
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (files[i]->size == kSizeAsMade) files[i]->size = bytes[i].size(); // v7: an item is the size it is made
        if (bytes[i].size() != files[i]->size) throw std::runtime_error("the installer does not give " + files[i]->path + " (its size is wrong)");
        contents.insert(contents.end(), bytes[i].begin(), bytes[i].end());
    }
    return contents;
}

} // namespace

std::vector<uint8_t> read_file_bytes(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + file.generic_string());
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

BigUint binary_address(const std::vector<uint8_t>& bytes)
{
    // binary-v1: the bytes as one big-endian number, plus how many files are shorter, which is
    // "01" written once for each byte (binaryspace.hpp). Two hex conversions and an addition.
    static constexpr char kHex[] = "0123456789abcdef";
    std::string h, ones;
    h.reserve(bytes.size() * 2);
    ones.reserve(bytes.size() * 2);
    for (uint8_t b : bytes)
    {
        h += kHex[b >> 4];
        h += kHex[b & 15];
        ones += "01";
    }
    if (bytes.empty()) return BigUint();
    BigUint v = BigUint::from_hex(h);
    v += BigUint::from_hex(ones);
    return v;
}

std::vector<uint8_t> file_at(const BigUint& address)
{
    // Its length: the largest L with (256^L - 1) / 255 <= address, i.e. 256^L <= 255 a + 1.
    BigUint t = address;
    t.mul_small(255);
    t.add_small(1);
    const size_t length = (t.bit_length() - 1) / 8;
    std::vector<uint8_t> out(length);
    if (length == 0) return out;
    std::string ones;
    ones.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) ones += "01";
    BigUint v = address;
    v -= BigUint::from_hex(ones);
    const std::string h = v.to_hex(length * 2);
    auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
    for (size_t i = 0; i < length; ++i) out[i] = uint8_t(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
    return out;
}

std::string sha256_hex(const std::vector<uint8_t>& bytes)
{
    Sha256 s;
    s.update(std::span<const uint8_t>(bytes.data(), bytes.size()));
    return Sha256::hex(s.finish());
}

std::string sha256_file_hex(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + file.generic_string());
    Sha256 s;
    std::vector<char> buf(1 << 20);
    while (in)
    {
        in.read(buf.data(), std::streamsize(buf.size()));
        const std::streamsize got = in.gcount();
        if (got > 0) s.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf.data()), size_t(got)));
    }
    return Sha256::hex(s.finish());
}

std::string manifest_path(const fs::path& p, const fs::path& root)
{
    const std::u8string u = p.lexically_relative(root).generic_u8string();
    std::string out(u.begin(), u.end());
    if (out.find_first_of("\t\r\n") != std::string::npos)
        throw std::runtime_error("a path with a tab or a line break in it cannot be listed: " + out);
    return out;
}

const char* Manifest::version() const
{
    return v20 ? kNewestArchivesManifestVersion : more_archives ? kMoreArchivesManifestVersion : !graph.empty() ? kGraphManifestVersion : remake_gz ? kGzipManifestVersion : binary ? kBinaryManifestVersion : rope_above ? kRopeAboveManifestVersion : archives ? kArchivesManifestVersion : pieces ? kPiecesManifestVersion : trimmed ? kTrimmedManifestVersion : rope_groups ? kRopeGroupsManifestVersion : rope_lines ? kRopeLinesManifestVersion : rope_columns ? kRopeColumnsManifestVersion : roped ? kRopeManifestVersion : compact ? kCompactManifestVersion : minimal ? kMinimalManifestVersion : items ? kItemsManifestVersion : packed ? kPackedManifestVersion
         : with_contents ? kInstallManifestVersion : with_addresses ? kAddressManifestVersion : kManifestVersion;
}

bool Manifest::one_file_root() const { return entries.size() == 1 && !entries[0].dir && entries[0].path == root; }

namespace {

// v16: a dictionary as LZMA2's one-byte property (0xFF: carried as it is, dictionary 0), and back.
uint8_t dictionary_byte(uint32_t d)
{
    if (d == 0) return 0xFF;
    for (uint32_t p = 0; p <= 40; ++p)
        if ((p == 40 ? 0xFFFFFFFFu : (2u | (p & 1u)) << (p / 2 + 11)) == d) return uint8_t(p);
    throw std::logic_error("a dictionary LZMA2's property cannot name");
}

uint32_t dictionary_of_byte(uint8_t p)
{
    if (p == 0xFF) return 0;
    if (p > 40) throw std::runtime_error("manifest: a dictionary byte out of range");
    return p == 40 ? 0xFFFFFFFFu : (2u | (p & 1u)) << (p / 2 + 11);
}

void put_varint_to(std::vector<uint8_t>& out, uint64_t v)
{
    while (v >= 0x80)
    {
        out.push_back(uint8_t(v | 0x80));
        v >>= 7;
    }
    out.push_back(uint8_t(v));
}

uint64_t get_varint_from(std::string_view in, size_t& at)
{
    uint64_t v = 0;
    for (int shift = 0;; shift += 7)
    {
        if (at >= in.size() || shift > 63) throw std::runtime_error("manifest: its head ends inside a number");
        const uint8_t b = uint8_t(in[at++]);
        v |= uint64_t(b & 0x7F) << shift;
        if (!(b & 0x80)) return v;
    }
}

std::string stream_line(const char* name, const PackedStream& s)
{
    return std::string("stream ") + name + " " + std::to_string(s.packed) + " " + std::to_string(s.bytes) + " " + std::to_string(s.dictionary) + "\n";
}

// v7: an item's code (hex, after the "h" of its way) as bytes, and a varint of its length before it.
std::vector<uint8_t> code_bytes(std::string_view hex, const std::string& path)
{
    if (hex.size() % 2) throw std::runtime_error("manifest: " + path + "'s code is not whole bytes");
    std::vector<uint8_t> out;
    auto digit = [&](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw std::runtime_error("manifest: " + path + "'s code is not hex");
    };
    for (size_t i = 0; i < hex.size(); i += 2) out.push_back(uint8_t(digit(hex[i]) * 16 + digit(hex[i + 1])));
    return out;
}

void put_varint(std::vector<uint8_t>& out, uint64_t v)
{
    while (v >= 0x80)
    {
        out.push_back(uint8_t(v | 0x80));
        v >>= 7;
    }
    out.push_back(uint8_t(v));
}

} // namespace

std::string Manifest::text() const
{
    if (compact)
    {
        // v7: what the rest does not already give.
        std::string t = std::string(version()) + "\n";
        if (!one_file_root()) t += "root " + root + "\n";
        if (x86.packed) t += stream_line("x86", x86);
        if (lzma2.packed) t += stream_line("lzma2", lzma2);
        if (!trimmed || item_stream.packed || item_stream.bytes) t += stream_line("items", item_stream); // v12: none where empty
        if (roped) t += stream_line("rope", rope_stream);
        for (size_t k = 0; k < lines.size(); ++k)
            t += "line " + std::to_string(k) + " " + (line_codings.at(k).empty() ? std::to_string(line_widths.at(k)) : line_codings[k]) + " " + lines[k] + "\n";
        t += "tree " + tree + "\n";
        for (const UnpackedArchive& u : unpacked) // v14
            t += "u\t" + u.format + "\t" + std::to_string(u.size) + "\t" + u.archive + "\t" + u.folder + "\n";
        for (const ManifestEntry& e0 : entries)
        {
            ManifestEntry e = e0;
            if (!e.remake.empty()) e.way = e.remake + " " + e.way; // v14
            if (e.dir) t += "d\t" + e.path + "\n";
            else if (e.way.rfind("item ", 0) == 0)
            {
                const std::string way = e.way.substr(0, e.way.find(' ', 5));
                if (e.sha256 == "-") t += "i\t" + way.substr(5) + "\t" + e.path + "\n";
                else t += "f\t" + std::to_string(e.size) + "\t" + e.sha256 + "\t" + way + "\t" + e.path + "\n"; // --item-hashes
            }
            else if (e.sha256 == "-") t += "f\t" + std::to_string(e.size) + "\t" + e.way + "\t" + e.path + "\n";
            else t += "f\t" + std::to_string(e.size) + "\t" + e.sha256 + "\t" + e.way + "\t" + e.path + "\n"; // --file-hashes
        }
        // v18: the map's text, each of its lines after "g<TAB>" (so its own "end" is not the listing's).
        for (size_t at = 0; at < graph.size();)
        {
            const size_t nl = graph.find('\n', at);
            if (nl == std::string::npos) throw std::logic_error("a graph part whose last line has no line feed");
            t += "g\t" + graph.substr(at, nl - at) + "\n";
            at = nl + 1;
        }
        t += "end\n";
        return t;
    }
    std::string t = std::string(minimal ? kMinimalManifestVersion : items ? kItemsManifestVersion : packed ? kPackedManifestVersion : with_contents ? kInstallManifestVersion : with_addresses ? kAddressManifestVersion : kManifestVersion) + "\n";
    t += "root " + root + "\n";
    t += "files " + std::to_string(files) + "\n";
    t += "bytes " + std::to_string(bytes) + "\n";
    if (packed)
        for (const auto& [name, s] : {std::pair<const char*, const PackedStream*>{"x86", &x86}, {"lzma2", &lzma2}})
            t += std::string("stream ") + name + " " + std::to_string(s->packed) + " " + std::to_string(s->bytes) + " " + std::to_string(s->dictionary) + "\n";
    if (minimal) t += "stream items " + std::to_string(item_stream.packed) + " " + std::to_string(item_stream.bytes) + " " + std::to_string(item_stream.dictionary) + "\n";
    if (items)
    {
        for (size_t k = 0; k < lines.size(); ++k)
            t += "line " + std::to_string(k) + " " + (minimal ? std::to_string(line_widths.at(k)) + " " : std::string()) + lines[k] + "\n";
        t += "tree " + tree + "\n";
    }
    for (const ManifestEntry& e : entries)
    {
        if (e.dir) t += "d\t" + e.path + "\n";
        else
        {
            // v6: an item's way is "item K", its place in the items stream.
            std::string way = e.way;
            if (minimal && way.rfind("item ", 0) == 0) way = way.substr(0, way.find(' ', 5));
            t += "f\t" + std::to_string(e.size) + "\t" + e.sha256 + "\t" + (packed ? way + "\t" : with_addresses ? e.address + "\t" : std::string()) +
                 e.path + "\n";
        }
    }
    t += "end\n";
    return t;
}

namespace {

// A place (hex) as `width` raw bytes, most significant first, and back: v6's items stream. (Here
// rather than with the lines, so that sieve-install, which has none of them, can read v6 too.)
std::vector<uint8_t> raw_place(std::string_view hex, uint32_t width, const std::string& path)
{
    std::string h(hex.substr(std::min(hex.find_first_not_of('0'), hex.size())));
    if (h.size() > size_t(width) * 2) throw std::runtime_error("manifest: " + path + "'s place is wider than its line's places");
    h.insert(0, size_t(width) * 2 - h.size(), '0');
    std::vector<uint8_t> out(width);
    auto digit = [&](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw std::runtime_error("manifest: " + path + "'s place is not hex");
    };
    for (uint32_t i = 0; i < width; ++i) out[i] = uint8_t(digit(h[2 * i]) * 16 + digit(h[2 * i + 1]));
    return out;
}

std::string hex_place(const uint8_t* b, uint32_t width)
{
    static const char k[] = "0123456789abcdef";
    std::string h;
    for (uint32_t i = 0; i < width; ++i) h += {k[b[i] >> 4], k[b[i] & 15]};
    return h.empty() ? std::string("0") : h;
}

// The line an item's way ("item K ...") names.
size_t item_line_of(const ManifestEntry& e, size_t lines)
{
    const size_t sp = e.way.find(' ', 5);
    const size_t k = size_t(std::stoull(e.way.substr(5, sp == std::string::npos ? std::string::npos : sp - 5)));
    if (k >= lines) throw std::runtime_error("manifest: " + e.path + " names a line it does not list");
    return k;
}

} // namespace

std::vector<uint8_t> Manifest::items_stream() const
{
    std::vector<uint8_t> out;
    for (const ManifestEntry& e : entries)
    {
        if (e.dir || e.way.rfind("item ", 0) != 0) continue;
        const size_t k = item_line_of(e, lines.size());
        const size_t sp = e.way.find(' ', 5);
        if (sp == std::string::npos) throw std::runtime_error("manifest: " + e.path + " has no place to carry");
        if (compact && !line_codings.at(k).empty())
        {
            // v7: a coded place, its length first.
            if (e.way.size() < sp + 2 || e.way[sp + 1] != 'h') throw std::runtime_error("manifest: " + e.path + " has no code for its line");
            const std::vector<uint8_t> b = code_bytes(std::string_view(e.way).substr(sp + 2), e.path);
            put_varint(out, b.size());
            out.insert(out.end(), b.begin(), b.end());
            continue;
        }
        const std::vector<uint8_t> b = raw_place(std::string_view(e.way).substr(sp + 1), line_widths.at(k), e.path);
        out.insert(out.end(), b.begin(), b.end());
    }
    return out;
}

std::string listing_body(const Manifest& m)
{
    // The text's lines after its first, its root's and its streams', the tree's left out, up to "end".
    const std::string t = m.text();
    std::string body;
    size_t at = t.find('\n') + 1;
    while (at < t.size())
    {
        const size_t nl = t.find('\n', at);
        const std::string_view line(t.data() + at, nl - at);
        at = nl + 1;
        if (line == "end") break;
        if (line.rfind("root ", 0) == 0 || line.rfind("stream ", 0) == 0 || line.rfind("tree ", 0) == 0) continue;
        body.append(line);
        body += '\n';
    }
    return body;
}

// ---- v20: the listing's body in bytes (IDEAS §19.90). Each of its text lines is one record, read
// back into that same line, so everything after the reading is the text's as before.
//   1  a folder:  its path (as below)
//   2  a file:    its size (a varint), its way (as below), its path
//   3  any other line as it is: a varint of its length, then it ("line" lines, a map's "g" lines)
//   4  an archive carried as its contents ("u"): its format (as below), its size, the archive's path
//      and the folder's, each a varint of its length and then it
//   5  a "line" line: what follows "line ", then a line feed
// A path: a varint of how much of it is the last path's (a folder's or a file's) and of the rest's
// length, then the rest. A way: a byte, 1 "raw"; 3 "rope", 4 "pieces", each then a varint of the
// rest's length and the rest; 2 an archive remade on installing, its format, then the rest of its
// recipe as text (a varint of its length, then it) and its inner way; 0 any other, as text. A format:
// a varint of its number in kArchiveFormats, 0 for one not there, then its name as text.

static const char* const kArchiveFormats[] = {nullptr, "xz", "gz", "bz2", "zst", "lz4", "br", "lzma", "lz", "7z", "zip", "rar", "cab", "iso", "lha", "cpio", "ar", "warc"};

int archive_format_number(const std::string& name)
{
    for (int i = 1; i < int(std::size(kArchiveFormats)); ++i)
        if (name == kArchiveFormats[i]) return i;
    return 0;
}

std::string archive_format_name(int number)
{
    return number > 0 && number < int(std::size(kArchiveFormats)) ? std::string(kArchiveFormats[number]) : std::string();
}

// A remade archive's recipe: its words before the inner way (with the kind), by kind; 0 for none.
static int recipe_words(std::string_view way)
{
    const std::string_view kind = way.substr(0, way.find(' '));
    return kind == "xz" ? 4 : kind == "gz" ? 5 : kind == "bz2" ? 3 : kind == "zst" ? 6 : kind == "lz4" ? 8 : kind == "br" ? 4 : kind == "lzma" ? 3 : 0;
}

static void put_text(std::vector<uint8_t>& o, std::string_view t)
{
    put_varint_to(o, t.size());
    o.insert(o.end(), t.begin(), t.end());
}

static void put_way(std::vector<uint8_t>& o, std::string_view way)
{
    if (way == "raw") return o.push_back(1);
    if (const int w = recipe_words(way); w > 0)
    {
        size_t at = 0;
        for (int k = 0; k < w && at != std::string_view::npos; ++k) at = way.find(' ', at + 1);
        const std::string kind(way.substr(0, way.find(' ')));
        if (at != std::string_view::npos && archive_format_number(kind) > 0)
        {
            o.push_back(2);
            put_varint_to(o, uint64_t(archive_format_number(kind)));
            put_text(o, way.substr(kind.size() + 1, at - kind.size() - 1));
            return put_way(o, way.substr(at + 1));
        }
    }
    for (const auto& [code, word] : {std::pair<uint8_t, std::string_view>{3, "rope "}, {4, "pieces "}})
        if (way.substr(0, word.size()) == word)
        {
            o.push_back(code);
            return put_text(o, way.substr(word.size()));
        }
    o.push_back(0);
    put_text(o, way);
}

static bool decimal(std::string_view t) { return !t.empty() && t.size() <= 19 && t.find_first_not_of("0123456789") == std::string_view::npos && (t.size() == 1 || t[0] != '0'); }

std::vector<uint8_t> listing_to_bytes(const std::string& body)
{
    std::vector<uint8_t> o;
    std::string last; // the last path written
    auto put_path = [&](std::string_view p) {
        size_t same = 0;
        while (same < p.size() && same < last.size() && p[same] == last[same]) ++same;
        put_varint_to(o, same);
        put_text(o, p.substr(same));
        last.assign(p);
    };
    for (size_t at = 0; at < body.size();)
    {
        const size_t nl = body.find('\n', at);
        if (nl == std::string::npos) throw std::logic_error("a listing whose last line has no line feed");
        const std::string_view line(body.data() + at, nl - at);
        at = nl + 1;
        std::vector<std::string_view> f;
        for (size_t p = 0;;)
        {
            const size_t tab = line.find('\t', p);
            f.push_back(line.substr(p, tab == std::string_view::npos ? std::string_view::npos : tab - p));
            if (tab == std::string_view::npos) break;
            p = tab + 1;
        }
        if (f.size() == 2 && f[0] == "d")
        {
            o.push_back(1);
            put_path(f[1]);
        }
        else if (f.size() == 4 && f[0] == "f" && decimal(f[1]))
        {
            o.push_back(2);
            put_varint_to(o, std::stoull(std::string(f[1])));
            put_way(o, f[2]);
            put_path(f[3]);
        }
        else if (f.size() == 5 && f[0] == "u" && decimal(f[2]))
        {
            o.push_back(4);
            const int n = archive_format_number(std::string(f[1]));
            put_varint_to(o, uint64_t(n));
            if (!n) put_text(o, f[1]);
            put_varint_to(o, std::stoull(std::string(f[2])));
            put_text(o, f[3]);
            put_text(o, f[4]);
        }
        else if (line.substr(0, 5) == "line " && line.find('\n') == std::string_view::npos)
        {
            o.push_back(5);
            o.insert(o.end(), line.begin() + 5, line.end());
            o.push_back('\n');
        }
        else
        {
            o.push_back(3);
            put_text(o, line);
        }
    }
    if (listing_from_bytes(std::string_view(reinterpret_cast<const char*>(o.data()), o.size())) != body)
        throw std::logic_error("a listing in bytes that does not read back as its text");
    return o;
}

std::string listing_from_bytes(std::string_view b)
{
    size_t at = 0;
    auto byte = [&]() {
        if (at >= b.size()) throw std::runtime_error("manifest: its listing ends early");
        return uint8_t(b[at++]);
    };
    // (A line kept as it is may hold tabs, a "g" line's or a "line" line's; anything else, neither.)
    auto text = [&](bool tabs = false) {
        const uint64_t n = get_varint_from(b, at);
        if (n > b.size() - at) throw std::runtime_error("manifest: its listing ends early");
        const std::string_view t = b.substr(at, size_t(n));
        at += size_t(n);
        if (t.find_first_of(tabs ? "\n" : "\t\n") != std::string_view::npos)
            throw std::runtime_error("manifest: its listing has a tab or a line feed where it may not");
        return std::string(t);
    };
    std::string last, out;
    auto path = [&]() {
        const uint64_t same = get_varint_from(b, at);
        if (same > last.size()) throw std::runtime_error("manifest: its listing shares more of a path than the last had");
        last = last.substr(0, size_t(same)) + text();
        return last;
    };
    std::function<std::string(int)> way = [&](int depth) -> std::string {
        if (depth > 4) throw std::runtime_error("manifest: its listing's way is nested too deeply");
        switch (byte())
        {
        case 0: return text();
        case 1: return "raw";
        case 2:
        {
            const std::string kind = archive_format_name(int(get_varint_from(b, at)));
            if (kind.empty() || !recipe_words(kind + " ")) throw std::runtime_error("manifest: its listing names an archive's kind this Sieve does not know");
            const std::string rest = text();
            return kind + " " + rest + " " + way(depth + 1);
        }
        case 3: return "rope " + text();
        case 4: return "pieces " + text();
        default: throw std::runtime_error("manifest: its listing has a way this Sieve does not know");
        }
    };
    while (at < b.size())
    {
        switch (byte())
        {
        case 1: out += "d\t" + path() + "\n"; break;
        case 2:
        {
            const uint64_t size = get_varint_from(b, at);
            const std::string w = way(0);
            out += "f\t" + std::to_string(size) + "\t" + w + "\t" + path() + "\n";
            break;
        }
        case 3: out += text(true) + "\n"; break;
        case 5:
        {
            const size_t nl = b.find('\n', at);
            if (nl == std::string_view::npos) throw std::runtime_error("manifest: its listing ends early");
            out += "line " + std::string(b.substr(at, nl - at)) + "\n";
            at = nl + 1;
            break;
        }
        case 4:
        {
            const int n = int(get_varint_from(b, at));
            const std::string name = n ? archive_format_name(n) : text();
            if (name.empty()) throw std::runtime_error("manifest: its listing names an archive's format this Sieve does not know");
            const uint64_t size = get_varint_from(b, at);
            const std::string archive = text(), folder = text();
            out += "u\t" + name + "\t" + std::to_string(size) + "\t" + archive + "\t" + folder + "\n";
            break;
        }
        default: throw std::runtime_error("manifest: its listing has a record this Sieve does not know");
        }
    }
    return out;
}

// v16: "sieve" and 16 (17, 18: v17, v18); flags (1 root, 2 x86, 4 lzma2, 8 items, 16 rope, 32 the body packed,
// 64 a graph part, v18 only); the
// root (a varint of its length, then it); each stream present, its packed size and size as varints
// and its dictionary's byte (the rope's, its size alone: it is carried as it is); the tree's SHA-256
// as 32 bytes; the body: packed, its packed size, size and dictionary's byte, then it as LZMA2; else
// its size, then it.
static std::vector<uint8_t> binary_head(const Manifest& m)
{
    std::vector<uint8_t> o = {'s', 'i', 'e', 'v', 'e', uint8_t(m.v20 ? 20 : m.more_archives ? 19 : !m.graph.empty() ? 18 : m.remake_gz ? 17 : 16)};
    const bool root = !m.one_file_root(), items = !m.trimmed || m.item_stream.packed || m.item_stream.bytes;
    const bool body_packed = m.listing_stream.packed != 0;
    o.push_back(uint8_t((root ? 1 : 0) | (m.x86.packed ? 2 : 0) | (m.lzma2.packed ? 4 : 0) | (items ? 8 : 0) | (m.roped ? 16 : 0) |
                        (body_packed ? 32 : 0) | (m.graph.empty() ? 0 : 64) | (m.v20 && m.body_text ? 128 : 0))); // v18: 64, a graph part; v20: 128, the body as text
    if (root)
    {
        put_varint_to(o, m.root.size());
        o.insert(o.end(), m.root.begin(), m.root.end());
    }
    auto stream = [&](const PackedStream& s) {
        put_varint_to(o, s.packed);
        put_varint_to(o, s.bytes);
        o.push_back(dictionary_byte(s.dictionary));
    };
    if (m.x86.packed) stream(m.x86);
    if (m.lzma2.packed) stream(m.lzma2);
    if (items) stream(m.item_stream);
    if (m.roped) put_varint_to(o, m.rope_stream.packed);
    if (m.tree.size() != 64) throw std::logic_error("a tree's SHA-256 that is not 64 hex digits");
    for (size_t i = 0; i < 64; i += 2) o.push_back(uint8_t(std::stoul(m.tree.substr(i, 2), nullptr, 16)));
    if (body_packed)
    {
        if (m.listing_of != listing_body(m)) throw std::logic_error("Sieve instructions changed after their listing was packed");
        stream(m.listing_stream);
        o.insert(o.end(), m.listing_carried.begin(), m.listing_carried.end());
    }
    else
    {
        // (v20: the body in bytes; before it, as text.)
        const std::string text = listing_body(m);
        const std::vector<uint8_t> body = m.v20 && !m.body_text ? listing_to_bytes(text) : std::vector<uint8_t>(text.begin(), text.end());
        put_varint_to(o, body.size());
        o.insert(o.end(), body.begin(), body.end());
    }
    return o;
}

std::vector<uint8_t> Manifest::file() const
{
    const std::string t = text();
    std::vector<uint8_t> out;
    out.reserve(t.size() + (packed ? streams.size() : contents.size()));
    if (binary)
    {
        // v16: the head in bytes, then the body (its listing's lines after the streams', the tree's
        // left out, "end" too), as carried.
        out = binary_head(*this);
        out.insert(out.end(), streams.begin(), streams.end());
        out.insert(out.end(), items_carried.begin(), items_carried.end());
        if (roped) out.insert(out.end(), rope_carried.begin(), rope_carried.end());
        return out;
    }
    if (compact && listing_stream.packed)
    {
        // v7: its first line, the listing's line, then the listing as carried.
        if (listing_of != t) throw std::logic_error("Sieve instructions changed after their listing was packed");
        const std::string head = t.substr(0, t.find('\n') + 1) + stream_line("listing", listing_stream).substr(7);
        out.insert(out.end(), head.begin(), head.end());
        out.insert(out.end(), listing_carried.begin(), listing_carried.end());
    }
    else out.insert(out.end(), t.begin(), t.end());
    if (minimal)
    {
        // v6: the two streams, then the items stream as carried.
        out.insert(out.end(), streams.begin(), streams.end());
        out.insert(out.end(), items_carried.begin(), items_carried.end());
        if (roped) out.insert(out.end(), rope_carried.begin(), rope_carried.end());
    }
    else if (packed) out.insert(out.end(), streams.begin(), streams.end());
    else if (with_contents) out.insert(out.end(), contents.begin(), contents.end());
    return out;
}

Manifest manifest_of_file(const fs::path& file)
{
    if (!fs::is_regular_file(file)) throw std::runtime_error(file.generic_string() + " is not a file");
    Manifest m;
    const std::u8string name = fs::absolute(file).filename().u8string();
    m.root = std::string(name.begin(), name.end());
    ManifestEntry e;
    e.path = m.root;
    if (e.path.find_first_of("\t\r\n") != std::string::npos) throw std::runtime_error("a name with a tab or a line break in it cannot be listed: " + e.path);
    e.size = uint64_t(fs::file_size(file));
    e.sha256 = vault::hash_checked_file(file, e.path); // read once: its hash, and the vault's check
    m.entries.push_back(e);
    m.files = 1;
    m.bytes = e.size;
    return m;
}

Manifest walk_folder(const fs::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    if (!fs::is_directory(root)) throw std::runtime_error(root_in.generic_string() + " is not a folder");
    Manifest m;
    {
        // The folder's own name; for a path ending in a separator, the part before it.
        fs::path r = root;
        if (!r.has_filename()) r = r.parent_path();
        const std::u8string u = r.filename().generic_u8string();
        m.root = std::string(u.begin(), u.end());
    }
    for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::none); it != fs::recursive_directory_iterator(); ++it)
    {
        const fs::directory_entry& d = *it;
        if (d.is_symlink())
        {
            ++m.skipped;
            if (d.is_directory()) it.disable_recursion_pending();
            continue;
        }
        ManifestEntry e;
        if (d.is_directory())
        {
            e.dir = true;
            e.path = manifest_path(d.path(), root) + "/";
        }
        else if (d.is_regular_file())
        {
            e.path = manifest_path(d.path(), root);
            e.size = uint64_t(d.file_size());
            e.sha256 = vault::hash_checked_file(d.path(), e.path); // read once: its hash, and the vault's check
            ++m.files;
            m.bytes += e.size;
        }
        else
        {
            ++m.skipped; // devices, sockets and the like: not files anyone can install
            continue;
        }
        m.entries.push_back(std::move(e));
    }
    // By the paths' bytes, as unsigned bytes, so the order is the same on every system (char is
    // signed on some, which would put every non-ASCII name first).
    std::sort(m.entries.begin(), m.entries.end(), [](const ManifestEntry& a, const ManifestEntry& b) {
        return std::lexicographical_compare(a.path.begin(), a.path.end(), b.path.begin(), b.path.end(),
                                            [](char x, char y) { return uint8_t(x) < uint8_t(y); });
    });
    return m;
}

void add_addresses(Manifest& m, const std::filesystem::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    for (ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        const BigUint a = binary_address(read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end()))));
        e.address = a.is_zero() ? "0" : a.to_hex();
    }
    m.with_addresses = true;
}

void add_contents(Manifest& m, const std::filesystem::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    m.contents.clear();
    m.contents.reserve(size_t(m.bytes));
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        const auto b = read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end())));
        if (b.size() != e.size || sha256_hex(b) != e.sha256) throw std::runtime_error(e.path + " changed while the folder was being read");
        m.contents.insert(m.contents.end(), b.begin(), b.end());
    }
    m.with_contents = true;
    m.with_addresses = false;
}

Manifest Manifest::parse(std::string_view whole)
{
    // v16: the head in bytes, read back into v15's text (its root's and streams' lines, its lines,
    // the tree's, the rest of its body, "end"), followed by what follows it.
    if (whole.size() >= 6 && whole.substr(0, 5) == "sieve" && uint8_t(whole[5]) >= 16 && uint8_t(whole[5]) <= 20)
    {
        const uint8_t v = uint8_t(whole[5]);
        size_t at = 6;
        if (at >= whole.size()) throw std::runtime_error("manifest: its head ends early");
        const uint8_t flags = uint8_t(whole[at++]);
        if (((flags & 0x80) && v < 20) || ((flags & 64) && v < 18)) throw std::runtime_error("manifest: its head has flags this Sieve does not know");
        if (v == 18 && !(flags & 64)) throw std::runtime_error("manifest: v18 without its graph part");
        // (v17's text is v15's with gzip remade allowed, v18's with a graph part too, v19's with bzip2
        // and zstd as well: its first line says so.)
        std::string text =
            std::string(v == 20 ? kNewestArchivesManifestVersion : v == 19 ? kMoreArchivesManifestVersion : v == 18 ? kGraphManifestVersion : v == 17 ? kGzipManifestVersion : kRopeAboveManifestVersion) + "\n";
        auto take = [&](uint64_t n) {
            if (n > whole.size() - at) throw std::runtime_error("manifest: its head ends early");
            const std::string_view v = whole.substr(at, size_t(n));
            at += size_t(n);
            return v;
        };
        if (flags & 1)
        {
            const uint64_t n = get_varint_from(whole, at);
            text += "root " + std::string(take(n)) + "\n";
        }
        auto stream = [&]() {
            const uint64_t packed = get_varint_from(whole, at), bytes = get_varint_from(whole, at);
            const uint32_t d = dictionary_of_byte(uint8_t(take(1)[0]));
            return PackedStream{packed, bytes, d};
        };
        if (flags & 2) text += stream_line("x86", stream());
        if (flags & 4) text += stream_line("lzma2", stream());
        if (flags & 8) text += stream_line("items", stream());
        if (flags & 16)
        {
            const uint64_t n = get_varint_from(whole, at);
            text += stream_line("rope", PackedStream{n, n, 0});
        }
        const std::string_view tree = take(32);
        static const char* hex = "0123456789abcdef";
        std::string tree_hex;
        for (const char c : tree)
        {
            tree_hex += hex[uint8_t(c) >> 4];
            tree_hex += hex[uint8_t(c) & 15];
        }
        std::string body;
        PackedStream ls{};
        std::vector<uint8_t> carried;
        if (flags & 32)
        {
            ls = stream();
            if (ls.dictionary == 0 || ls.bytes > (uint64_t(1) << 32)) throw std::runtime_error("manifest: its listing's sizes do not fit");
            const std::string_view c = take(ls.packed);
            carried.assign(c.begin(), c.end());
            const std::vector<uint8_t> b = unpack_stream(carried, ls, false);
            body.assign(b.begin(), b.end());
        }
        else body = std::string(take(get_varint_from(whole, at)));
        if (v == 20 && !(flags & 0x80)) body = listing_from_bytes(body); // v20: the body in bytes (unless flag 128), read back as its text
        // The lines first, then the tree's, then the rest.
        size_t split = 0;
        while (split < body.size() && body.compare(split, 5, "line ") == 0)
        {
            const size_t nl = body.find('\n', split);
            if (nl == std::string::npos) throw std::runtime_error("manifest: its body's last line has no line feed");
            split = nl + 1;
        }
        if (!body.empty() && body.back() != '\n') throw std::runtime_error("manifest: its body's last line has no line feed");
        text += body.substr(0, split) + "tree " + tree_hex + "\n" + body.substr(split) + "end\n";
        text.append(whole.substr(at));
        Manifest m = parse(text);
        m.binary = true;
        m.body_text = v == 20 && (flags & 0x80);
        if (flags & 32)
        {
            m.listing_stream = ls;
            m.listing_carried = std::move(carried);
            m.listing_of = listing_body(m);
        }
        return m;
    }
    // v7 with its listing carried as LZMA2: unpacked, and read as the text it is.
    // (v7, v8 and v9 alike: their first line, then "listing ...")
    std::string packed_version;
    for (const char* v : {kCompactManifestVersion, kRopeManifestVersion, kRopeColumnsManifestVersion, kRopeLinesManifestVersion, kRopeGroupsManifestVersion, kTrimmedManifestVersion, kPiecesManifestVersion, kArchivesManifestVersion, kRopeAboveManifestVersion})
        if (whole.substr(0, std::string(v).size() + 9) == std::string(v) + "\nlisting ") packed_version = v;
    if (!packed_version.empty())
    {
        const std::string v7 = packed_version + "\nlisting ";
        const size_t nl = whole.find('\n', v7.size());
        if (nl == std::string_view::npos) throw std::runtime_error("manifest: the last line has no line feed");
        const std::string v(whole.substr(v7.size(), nl - v7.size()));
        unsigned long long packed = 0, bytes = 0, dictionary = 0;
        char extra = 0;
        if (std::sscanf(v.c_str(), "%llu %llu %llu%c", &packed, &bytes, &dictionary, &extra) != 3 || dictionary > 0xFFFFFFFFull || dictionary == 0 ||
            packed > whole.size() - nl - 1 || bytes > (uint64_t(1) << 32))
            throw std::runtime_error("manifest: its listing's line is not three numbers that fit");
        const PackedStream ls{packed, bytes, uint32_t(dictionary)};
        const std::span<const uint8_t> carried(reinterpret_cast<const uint8_t*>(whole.data()) + nl + 1, size_t(packed));
        const std::vector<uint8_t> listing = unpack_stream(carried, ls, false);
        std::string text = packed_version + "\n";
        if (std::string_view(reinterpret_cast<const char*>(listing.data()), listing.size()).substr(0, 8) == "listing ")
            throw std::runtime_error("manifest: a listing inside a listing");
        text.append(listing.begin(), listing.end());
        text.append(whole.substr(nl + 1 + size_t(packed)));
        Manifest m = parse(text);
        m.listing_stream = ls;
        m.listing_carried.assign(carried.begin(), carried.end());
        m.listing_of = m.text();
        return m;
    }
    // Line by line, exactly as text() writes it, up to and including "end"; anything else is
    // refused rather than guessed at. After "end" a v3 manifest has its files' bytes, a v4 its two
    // streams, and the others nothing.
    std::vector<std::string> lines;
    size_t after = std::string_view::npos;
    for (size_t at = 0; at < whole.size();)
    {
        const size_t nl = whole.find('\n', at);
        if (nl == std::string_view::npos) throw std::runtime_error("manifest: the last line has no line feed");
        lines.emplace_back(whole.substr(at, nl - at));
        at = nl + 1;
        if (lines.back() == "end")
        {
            after = at;
            break;
        }
    }
    if (after == std::string_view::npos) throw std::runtime_error("manifest: no 'end'");
    Manifest m;
    auto field = [](const std::string& line, const char* key) {
        const std::string k = std::string(key) + " ";
        if (line.rfind(k, 0) != 0) throw std::runtime_error(std::string("manifest: expected '") + key + "'");
        return line.substr(k.size());
    };
    if (lines.size() < 5) throw std::runtime_error("manifest: too short");
    if (lines[0] == kAddressManifestVersion) m.with_addresses = true;
    else if (lines[0] == kInstallManifestVersion) m.with_contents = true;
    else if (lines[0] == kPackedManifestVersion) m.packed = m.with_contents = true;
    else if (lines[0] == kItemsManifestVersion) m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kMinimalManifestVersion) m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kCompactManifestVersion) m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kRopeManifestVersion) m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kRopeColumnsManifestVersion) m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kRopeLinesManifestVersion)
        m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kRopeGroupsManifestVersion)
        m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kGzipManifestVersion || lines[0] == kGraphManifestVersion || lines[0] == kMoreArchivesManifestVersion ||
             lines[0] == kNewestArchivesManifestVersion)
        m.v20 = lines[0] == kNewestArchivesManifestVersion,
        m.more_archives = lines[0] == kMoreArchivesManifestVersion || m.v20, m.remake_gz = m.binary = m.rope_above = m.archives = m.pieces = m.trimmed = m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact =
            m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kRopeAboveManifestVersion)
        m.rope_above = m.archives = m.pieces = m.trimmed = m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items =
            m.packed = m.with_contents = true;
    else if (lines[0] == kArchivesManifestVersion)
        m.archives = m.pieces = m.trimmed = m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed =
            m.with_contents = true;
    else if (lines[0] == kPiecesManifestVersion)
        m.pieces = m.trimmed = m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] == kTrimmedManifestVersion)
        m.trimmed = m.rope_groups = m.rope_lines = m.rope_columns = m.roped = m.compact = m.minimal = m.items = m.packed = m.with_contents = true;
    else if (lines[0] != kManifestVersion) throw std::runtime_error("not a sieve manifest (" + lines[0].substr(0, 40) + ")");
    if (m.packed) m.streams.assign(whole.begin() + std::ptrdiff_t(after), whole.end());
    else if (m.with_contents) m.contents.assign(whole.begin() + std::ptrdiff_t(after), whole.end());
    else if (after != whole.size()) throw std::runtime_error("manifest: something after 'end'");
    auto stream_of = [&](const std::string& v, const char* name) {
        unsigned long long packed = 0, bytes = 0, dictionary = 0;
        char extra = 0;
        if (std::sscanf(v.c_str(), "%llu %llu %llu%c", &packed, &bytes, &dictionary, &extra) != 3 || dictionary > 0xFFFFFFFFull)
            throw std::runtime_error(std::string("manifest: its ") + name + " stream's line is not three numbers");
        return PackedStream{packed, bytes, uint32_t(dictionary)};
    };
    size_t first = 4;
    bool root_given = true;
    if (m.compact)
    {
        // v7: each line in its order, those the rest gives left out.
        first = 1;
        auto next_is = [&](const char* k) { return first < lines.size() && lines[first].rfind(k, 0) == 0; };
        root_given = next_is("root ");
        if (root_given) m.root = lines[first++].substr(5);
        if (next_is("stream x86 ")) m.x86 = stream_of(lines[first++].substr(11), "x86");
        if (next_is("stream lzma2 ")) m.lzma2 = stream_of(lines[first++].substr(13), "lzma2");
        // Only v12 carries the lzma2 stream as it is (dictionary 0); the x86 stream never is.
        if ((m.x86.packed && m.x86.dictionary == 0) || (m.lzma2.packed && m.lzma2.dictionary == 0 && (!m.trimmed || m.lzma2.packed != m.lzma2.bytes)))
            throw std::runtime_error("manifest: a stream carried as it is where its version has none");
        // v12: no items line where there are no items.
        if (m.trimmed && !next_is("stream items ")) m.item_stream = PackedStream{0, 0, 0};
        else m.item_stream = stream_of(field(lines.at(first++), "stream items"), "items");
        if ((m.item_stream.dictionary == 0 && m.item_stream.packed != m.item_stream.bytes) || m.item_stream.packed > m.streams.size())
            throw std::runtime_error("manifest: its items stream's line does not fit what it carries");
        if (m.trimmed && !next_is("stream rope ")) m.roped = false; // v12: no rope line where no file is pulled tight
        if (m.roped)
        {
            // v8: the rope stream, carried as it is (its codes are coded already).
            m.rope_stream = stream_of(field(lines.at(first++), "stream rope"), "rope");
            if (m.rope_stream.dictionary != 0 || m.rope_stream.packed != m.rope_stream.bytes ||
                m.rope_stream.packed + m.item_stream.packed > m.streams.size())
                throw std::runtime_error("manifest: its rope stream's line does not fit what it carries");
        }
        while (next_is("line "))
        {
            const std::string v = lines[first++].substr(5);
            const size_t a = v.find(' '), b = a == std::string::npos ? a : v.find(' ', a + 1);
            if (b == std::string::npos || v.substr(0, a) != std::to_string(m.lines.size()))
                throw std::runtime_error("manifest: its lines are not numbered from 0 in order");
            const std::string w = v.substr(a + 1, b - a - 1);
            if (w == kCodedPlaces)
            {
                m.line_codings.push_back(w);
                m.line_widths.push_back(0);
            }
            else if (!w.empty() && w.size() <= 9 && w.find_first_not_of("0123456789") == std::string::npos)
            {
                m.line_codings.emplace_back();
                m.line_widths.push_back(uint32_t(std::stoul(w)));
            }
            else throw std::runtime_error("manifest: a line's places are neither a width nor a coding this Sieve knows (" + w + ")");
            m.lines.push_back(v.substr(b + 1));
        }
        m.tree = field(lines.at(first++), "tree");
    }
    else
    {
    m.root = field(lines[1], "root");
    m.files = std::stoull(field(lines[2], "files"));
    m.bytes = std::stoull(field(lines[3], "bytes"));
    }
    if (m.packed && !m.compact)
    {
        if (lines.size() < 7) throw std::runtime_error("manifest: too short");
        for (const auto& [name, s] : {std::pair<const char*, PackedStream*>{"x86", &m.x86}, {"lzma2", &m.lzma2}})
        {
            const std::string v = field(lines[first++], (std::string("stream ") + name).c_str());
            unsigned long long packed = 0, bytes = 0, dictionary = 0;
            char extra = 0;
            if (std::sscanf(v.c_str(), "%llu %llu %llu%c", &packed, &bytes, &dictionary, &extra) != 3 || dictionary > 0xFFFFFFFFull)
                throw std::runtime_error(std::string("manifest: its ") + name + " stream's line is not three numbers");
            if (packed && dictionary == 0) throw std::runtime_error(std::string("manifest: its ") + name + " stream is carried as it is, which only v12 does");
            *s = PackedStream{packed, bytes, uint32_t(dictionary)};
        }
        // v6: the items stream (its places, raw at dictionary 0, else LZMA2), the last of what
        // follows "end".
        if (m.minimal)
        {
            const std::string v = field(lines[first++], "stream items");
            unsigned long long packed = 0, bytes = 0, dictionary = 0;
            char extra = 0;
            if (std::sscanf(v.c_str(), "%llu %llu %llu%c", &packed, &bytes, &dictionary, &extra) != 3 || dictionary > 0xFFFFFFFFull ||
                (dictionary == 0 && packed != bytes))
                throw std::runtime_error("manifest: its items stream's line is not three numbers");
            m.item_stream = PackedStream{packed, bytes, uint32_t(dictionary)};
            if (packed > m.streams.size()) throw std::runtime_error("manifest: its items stream is longer than what it carries");
        }
        // v5: its lines, in order from 0, and the tree's SHA-256.
        while (m.items && first < lines.size() && lines[first].rfind("line ", 0) == 0)
        {
            const std::string v = lines[first++].substr(5);
            size_t sp = v.find(' ');
            if (sp == std::string::npos || v.substr(0, sp) != std::to_string(m.lines.size()))
                throw std::runtime_error("manifest: its lines are not numbered from 0 in order");
            std::string rest = v.substr(sp + 1);
            if (m.minimal)
            {
                // v6: the bytes a place on the line takes, then the line.
                sp = rest.find(' ');
                const std::string w = rest.substr(0, sp);
                if (sp == std::string::npos || w.empty() || w.size() > 9 || w.find_first_not_of("0123456789") != std::string::npos)
                    throw std::runtime_error("manifest: a line's width is not a number");
                m.line_widths.push_back(uint32_t(std::stoul(w)));
                rest = rest.substr(sp + 1);
            }
            m.lines.push_back(rest);
        }
        if (m.items) m.tree = field(lines[first++], "tree");
    }
    const bool graph_allowed = lines[0] == kGraphManifestVersion || lines[0] == kMoreArchivesManifestVersion || lines[0] == kNewestArchivesManifestVersion;
    const bool graph_needed = lines[0] == kGraphManifestVersion; // (v19's is allowed, not needed)
    for (size_t i = first; i + 1 < lines.size(); ++i)
    {
        if (lines[i].rfind("g\t", 0) == 0)
        {
            // v18: the graph part, the last of the listing.
            if (!graph_allowed) throw std::runtime_error("manifest: a graph part where its version has none");
            m.graph += lines[i].substr(2) + "\n";
            continue;
        }
        if (!m.graph.empty()) throw std::runtime_error("manifest: line " + std::to_string(i + 1) + " after its graph part");
        std::vector<std::string> f;
        for (size_t at = 0;;)
        {
            const size_t tab = lines[i].find('\t', at);
            f.push_back(lines[i].substr(at, tab == std::string::npos ? std::string::npos : tab - at));
            if (tab == std::string::npos) break;
            at = tab + 1;
        }
        if (m.archives && f.size() == 5 && f[0] == "u")
        {
            // v14: an archive carried as its contents.
            Manifest::UnpackedArchive u;
            u.format = f[1];
            if (f[2].empty() || f[2].size() > 19 || f[2].find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("manifest: line " + std::to_string(i + 1) + " is not an archive's line");
            u.size = std::stoull(f[2]);
            u.archive = f[3];
            u.folder = f[4];
            m.unpacked.push_back(u);
            continue;
        }
        ManifestEntry e;
        if (f.size() == 2 && f[0] == "d")
        {
            e.dir = true;
            e.path = f[1];
        }
        else if (m.compact && f.size() == 3 && f[0] == "i" && !f[1].empty() && f[1].size() <= 9 && f[1].find_first_not_of("0123456789") == std::string::npos)
        {
            // v7: an item, its size and SHA-256 those of what is made (the tree's checks it).
            e.size = kSizeAsMade;
            e.sha256 = "-";
            e.way = "item " + f[1];
            e.path = f[2];
        }
        else if (m.compact && f[0] == "f" && f.size() == 4)
        {
            // v7: a file without its own SHA-256 (the tree's covers it).
            e.size = std::stoull(f[1]);
            e.sha256 = "-";
            e.way = f[2];
            e.path = f[3];
            const bool gz = m.remake_gz && e.way.rfind("gz ", 0) == 0;
            const bool bz2 = m.more_archives && e.way.rfind("bz2 ", 0) == 0, zst = m.more_archives && e.way.rfind("zst ", 0) == 0;
            const bool lz4 = m.v20 && e.way.rfind("lz4 ", 0) == 0, br = m.v20 && e.way.rfind("br ", 0) == 0,
                       lzma = m.v20 && e.way.rfind("lzma ", 0) == 0;
            if ((m.archives && e.way.rfind("xz ", 0) == 0) || gz || bz2 || zst || lz4 || br || lzma)
            {
                // v14: "xz <preset> <check> <inner size> <way>"; v17: "gz <level> <memory level>
                // <header> <inner size> <way>"; v19: "bz2 <block size> <inner size> <way>", "zst
                // <version> <level> <checksum> <content size> <inner size> <way>": the recipe, then
                // the inner way.
                // v20: "lz4 <level> <block size> <linked> <block checksum> <content checksum> <content
                // size> <inner size> <way>", "br <quality> <window> <inner size> <way>", "lzma <preset>
                // <inner size> <way>".
                const int words = gz ? 5 : bz2 ? 3 : zst ? 6 : lz4 ? 8 : br ? 4 : lzma ? 3 : 4;
                size_t at = 0;
                for (int k = 0; k < words && at != std::string::npos; ++k) at = e.way.find(' ', at + 1);
                if (at == std::string::npos) throw std::runtime_error("manifest: " + e.path + "'s way has no inner way");
                e.remake = e.way.substr(0, at);
                e.way = e.way.substr(at + 1);
            }
        }
        else if (f[0] == "f" && f.size() == (m.with_addresses || m.packed ? 5u : 4u))
        {
            e.size = std::stoull(f[1]);
            e.sha256 = f[2];
            if (m.with_addresses) e.address = f[3];
            if (m.packed) e.way = f[3];
            e.path = f.back();
        }
        else throw std::runtime_error("manifest: line " + std::to_string(i + 1) + " is not an entry");
        // Nothing may point outside the folder being installed: no absolute path, no drive, no
        // backslash, and no part that is "." or ".." (a name with dots in it is fine).
        bool bad = e.path.empty() || e.path[0] == '/' || e.path.find(':') != std::string::npos || e.path.find('\\') != std::string::npos;
        for (size_t at = 0; !bad && at <= e.path.size();)
        {
            size_t slash = e.path.find('/', at);
            if (slash == std::string::npos) slash = e.path.size();
            const std::string part = e.path.substr(at, slash - at);
            bad = part == "." || part == ".." || (part.empty() && slash != e.path.size());
            at = slash + 1;
        }
        if (bad) throw std::runtime_error("manifest: refused path " + e.path);
        m.entries.push_back(std::move(e));
    }
    if (graph_needed && m.graph.empty()) throw std::runtime_error("manifest: v18 without its graph part");
    if (m.minimal)
    {
        // v6: every item's place from the items stream, in the listing's order, back into its way
        // as v5 writes it, so that what follows is v5's unpacking.
        if (m.roped)
        {
            m.rope_carried.assign(m.streams.end() - std::ptrdiff_t(m.rope_stream.packed), m.streams.end());
            m.streams.resize(m.streams.size() - size_t(m.rope_stream.packed));
        }
        m.items_carried.assign(m.streams.end() - std::ptrdiff_t(m.item_stream.packed), m.streams.end());
        m.streams.resize(m.streams.size() - size_t(m.item_stream.packed));
        const std::vector<uint8_t> places = m.item_stream.dictionary == 0 ? m.items_carried : unpack_stream(m.items_carried, m.item_stream, false);
        if (places.size() != m.item_stream.bytes) throw std::runtime_error("manifest: its items stream is not the size its line says");
        size_t at = 0;
        for (ManifestEntry& e : m.entries)
        {
            if (e.dir || e.way.rfind("item ", 0) != 0) continue;
            if (e.way.find(' ', 5) != std::string::npos) throw std::runtime_error("manifest: " + e.path + "'s way is not 'item K'");
            const size_t k = item_line_of(e, m.lines.size());
            if (m.compact && !m.line_codings.at(k).empty())
            {
                // v7: a coded place, a varint of its length first.
                uint64_t n = 0;
                for (int shift = 0;; shift += 7)
                {
                    if (at >= places.size() || shift > 56) throw std::runtime_error("the installer's items stream ends before " + e.path);
                    const uint8_t b = places[at++];
                    n |= uint64_t(b & 0x7F) << shift;
                    if (!(b & 0x80)) break;
                }
                if (places.size() - at < n) throw std::runtime_error("the installer's items stream ends before " + e.path);
                static const char hx[] = "0123456789abcdef";
                std::string code = " h";
                for (uint64_t i = 0; i < n; ++i) code += {hx[places[at + i] >> 4], hx[places[at + i] & 15]};
                e.way += code;
                at += size_t(n);
                continue;
            }
            const uint32_t width = m.line_widths.at(k);
            if (places.size() - at < width) throw std::runtime_error("the installer's items stream ends before " + e.path);
            e.way += " " + hex_place(places.data() + at, width);
            at += width;
        }
        if (at != places.size()) throw std::runtime_error("manifest: its items stream holds more than its items");
    }
    if (m.compact)
    {
        // v7: one file with no root is named as the root; the files and their bytes are counted.
        if (!root_given)
        {
            if (m.entries.size() != 1 || m.entries[0].dir) throw std::runtime_error("manifest: no root, and not one file");
            m.root = m.entries[0].path;
        }
    }
    if (m.packed) m.contents = unpack_v4(m); // every file's bytes, each by its way
    if (m.compact)
    {
        m.files = 0;
        m.bytes = 0;
        for (const ManifestEntry& e : m.entries)
            if (!e.dir)
            {
                ++m.files;
                m.bytes += e.size;
            }
    }
    if (m.items && tree_sha256(m) != m.tree) throw std::runtime_error("these Sieve instructions do not give the tree they were made from (its SHA-256 differs)");
    if (m.with_contents)
    {
        uint64_t sum = 0;
        for (const ManifestEntry& e : m.entries) sum += e.dir ? 0 : e.size;
        if (sum != m.bytes || m.contents.size() != m.bytes)
            throw std::runtime_error("manifest: its files' bytes are not the size its lines add up to");
    }
    return m;
}

std::string tree_sha256(const Manifest& m)
{
    Sha256 h;
    size_t at = 0;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        if (m.contents.size() - at < e.size) throw std::runtime_error("manifest: its files' bytes end before " + e.path);
        const std::vector<uint8_t> bytes(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
        at += size_t(e.size);
        h.update(e.path + "\n" + sha256_hex(bytes) + "\n");
    }
    return Sha256::hex(h.finish());
}

void set_item_maker(ItemMaker maker) { item_maker() = std::move(maker); }
void set_remaker(Remaker r) { remaker() = std::move(r); }
bool has_item_maker() { return bool(item_maker()); }

BigUint read_address_file(const fs::path& file, bool hex)
{
    const auto bytes = read_file_bytes(file);
    if (hex)
    {
        std::string h(bytes.begin(), bytes.end());
        h.erase(std::remove_if(h.begin(), h.end(), [](char c) { return c == '\n' || c == '\r' || c == ' '; }), h.end());
        return h.empty() ? BigUint() : BigUint::from_hex(h);
    }
    return address_of_raw(bytes);
}

BigUint address_of_raw(const std::vector<uint8_t>& bytes)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string h;
    h.reserve(bytes.size() * 2);
    for (uint8_t b : bytes)
    {
        h += kHex[b >> 4];
        h += kHex[b & 15];
    }
    return h.empty() ? BigUint() : BigUint::from_hex(h);
}

fs::path own_executable(const char* argv0)
{
#ifdef _WIN32
    std::wstring w(1024, L'\0');
    for (;;)
    {
        const DWORD n = GetModuleFileNameW(nullptr, w.data(), DWORD(w.size()));
        if (n == 0) break;
        if (n < w.size())
        {
            w.resize(n);
            return fs::path(w);
        }
        w.resize(w.size() * 2);
    }
#else
    std::error_code ec;
    const fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return self;
#endif
    return fs::absolute(fs::path(argv0 ? argv0 : ""));
}

std::optional<fs::path> installer_program_beside(const fs::path& dir)
{
    // Beside the caller (a build folder, or sieve in tools\), else in the tools folder beside it
    // (the hallway, at the top of an installation).
    for (const fs::path& d : {dir, dir / "tools"})
        for (const char* name : {"sieve-install.exe", "sieve-install"})
        {
            std::error_code ec;
            if (fs::is_regular_file(d / name, ec)) return d / name;
        }
    return std::nullopt;
}

namespace {

constexpr size_t kMagic = sizeof(kAttachedMagic) - 1, kTrailer = 8 + kMagic;

// Where the program itself ends: its whole length, or where what is attached to it begins.
uint64_t program_length(const std::vector<uint8_t>& f, uint64_t* attached = nullptr)
{
    if (attached) *attached = 0;
    if (f.size() < kTrailer || std::memcmp(f.data() + f.size() - kMagic, kAttachedMagic, kMagic) != 0) return f.size();
    uint64_t n = 0;
    for (int i = 7; i >= 0; --i) n = n << 8 | f[f.size() - kTrailer + size_t(i)];
    if (n > f.size() - kTrailer) throw std::runtime_error("the installer attached to the program is damaged (its length is wrong)");
    if (attached) *attached = n;
    return f.size() - kTrailer - n;
}

} // namespace

void write_installer_program(const fs::path& program, const BigUint& address, const fs::path& out)
{
    const auto p = read_file_bytes(program);
    const uint64_t length = program_length(p);
    std::vector<uint8_t> raw;
    if (!address.is_zero())
    {
        std::string h = address.to_hex();
        if (h.size() % 2) h.insert(h.begin(), '0');
        auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
        raw.resize(h.size() / 2);
        for (size_t i = 0; i < raw.size(); ++i) raw[i] = uint8_t(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
    }
    {
        std::ofstream o(out, std::ios::binary);
        o.write(reinterpret_cast<const char*>(p.data()), std::streamsize(length));
        o.write(reinterpret_cast<const char*>(raw.data()), std::streamsize(raw.size()));
        uint64_t n = raw.size();
        for (int i = 0; i < 8; ++i, n >>= 8) o.put(char(n & 0xFF));
        o.write(kAttachedMagic, std::streamsize(kMagic));
        if (!o) throw std::runtime_error("cannot write " + out.generic_string());
    }
    // Runnable where that is a permission (it is not on Windows, where .exe says so).
    std::error_code ec;
    fs::permissions(out, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);
}

std::optional<BigUint> attached_address(const fs::path& program)
{
    std::error_code ec;
    if (!fs::is_regular_file(program, ec)) return std::nullopt;
    const auto f = read_file_bytes(program);
    uint64_t n = 0;
    const uint64_t at = program_length(f, &n);
    if (at == f.size()) return std::nullopt; // nothing attached
    return address_of_raw(std::vector<uint8_t>(f.begin() + std::ptrdiff_t(at), f.begin() + std::ptrdiff_t(at + n)));
}

void write_address_file(const fs::path& file, const BigUint& address, bool hex)
{
    std::ofstream out(file, std::ios::binary);
    if (hex) out << (address.is_zero() ? std::string("0") : address.to_hex()) << "\n";
    else if (!address.is_zero())
    {
        std::string h = address.to_hex();
        if (h.size() % 2) h.insert(h.begin(), '0');
        auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
        std::vector<char> raw(h.size() / 2);
        for (size_t i = 0; i < raw.size(); ++i) raw[i] = char(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
        out.write(raw.data(), std::streamsize(raw.size()));
    }
    if (!out) throw std::runtime_error("cannot write " + file.generic_string());
}

Manifest manifest_of_installer(const BigUint& address)
{
    const auto bytes = file_at(address);
    Manifest m = Manifest::parse(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    if (!m.with_addresses && !m.with_contents) throw std::runtime_error("that is a manifest without the files in it (v1): nothing to install from");
    return m;
}

Manifest installable_manifest(const fs::path& file, bool hex)
{
    if (!hex)
    {
        if (const auto attached = attached_address(file)) return manifest_of_installer(*attached);
        std::ifstream in(file, std::ios::binary);
        char head[16] = {};
        in.read(head, sizeof head);
        if (in.gcount() == 16 && std::memcmp(head, "sieve-manifest-v", 16) == 0)
        {
            const auto bytes = read_file_bytes(file);
            Manifest m = Manifest::parse(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
            if (!m.with_addresses && !m.with_contents)
                throw std::runtime_error("that is a manifest without the files in it (v1): nothing to install from");
            return m;
        }
    }
    return manifest_of_installer(read_address_file(file, hex));
}

std::string install_times_text(double decode_seconds, double write_seconds)
{
    auto s = [](double v) {
        char b[32];
        std::snprintf(b, sizeof b, "%.3f s", v);
        return std::string(b);
    };
    return "decoded in " + s(decode_seconds) + ", checked and written in " + s(write_seconds);
}

bool install_tree(const Manifest& m, const fs::path& dest, bool force, const InstallProgress& progress, const std::atomic<bool>* cancel)
{
    auto cancelled = [cancel] { return cancel && cancel->load(); };
    auto path_of = [&](const std::string& p) { return dest / fs::path(std::u8string(p.begin(), p.end())); };
    // Phase 0: every file cut from the contents (v3) or read back from its address (v2) and
    // checked, before anything is written.
    std::vector<std::pair<const ManifestEntry*, std::vector<uint8_t>>> files;
    for (const ManifestEntry& e : m.entries)
        if (!e.dir) files.emplace_back(&e, std::vector<uint8_t>{});
    size_t at = 0;
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (cancelled()) return false;
        const ManifestEntry& e = *files[i].first;
        std::vector<uint8_t> bytes;
        if (m.with_contents)
        {
            if (m.contents.size() - at < e.size) throw std::runtime_error("the installer ends before " + e.path);
            bytes.assign(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
            at += size_t(e.size);
        }
        else bytes = file_at(e.address == "0" ? BigUint() : BigUint::from_hex(e.address));
        // (An item's own SHA-256 is "-": the tree's, checked when the manifest was read, covers it.)
        const std::string sha = sha256_hex(bytes);
        if (bytes.size() != e.size || (sha != e.sha256 && !(m.items && e.sha256 == "-")))
            throw std::runtime_error("the installer does not give " + e.path + " (its size or SHA-256 is wrong)");
        vault::check_known(bytes, sha, e.path); // the vault: nothing it withholds is ever written
        if (fs::exists(path_of(e.path)) && !force) throw std::runtime_error(e.path + " is already there");
        files[i].second = std::move(bytes);
        if (progress) progress(0, i + 1, files.size(), e.path);
    }
    // Phase 1: the folders, then the files, remembering what was made so a cancel can undo it.
    std::vector<fs::path> made_dirs, made_files;
    auto undo = [&] {
        for (auto it = made_files.rbegin(); it != made_files.rend(); ++it) fs::remove(*it);
        for (auto it = made_dirs.rbegin(); it != made_dirs.rend(); ++it)
        {
            std::error_code ec;
            fs::remove(*it, ec); // only if empty: a folder that held something before is kept
        }
    };
    auto make_dirs = [&](const fs::path& d) {
        std::vector<fs::path> chain;
        for (fs::path p = d; !p.empty() && !fs::exists(p); p = p.parent_path()) chain.push_back(p);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            fs::create_directory(*it);
            made_dirs.push_back(*it);
        }
    };
    try
    {
        make_dirs(dest);
        for (const ManifestEntry& e : m.entries)
            if (e.dir) make_dirs(path_of(e.path));
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (cancelled())
            {
                undo();
                return false;
            }
            const fs::path p = path_of(files[i].first->path);
            make_dirs(p.parent_path());
            const bool existed = fs::exists(p);
            std::ofstream out(p, std::ios::binary);
            out.write(reinterpret_cast<const char*>(files[i].second.data()), std::streamsize(files[i].second.size()));
            if (!out) throw std::runtime_error("cannot write " + p.generic_string());
            if (!existed) made_files.push_back(p);
            if (progress) progress(1, i + 1, files.size(), files[i].first->path);
        }
    }
    catch (...)
    {
        undo();
        throw;
    }
    return true;
}

} // namespace sieve::cli
