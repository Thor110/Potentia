// Sieve CLI -- an installer's files packed (pack.hpp).

#include "cli/pack.hpp"

#include "sieve/filekind.hpp"
#include "sieve/rope_wave.hpp"
#include "unpack_7z.hpp"

#include <lzma.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

// The smallest LZMA2 dictionary size that holds `n` bytes: 2 or 3 times a power of two, at least
// 4 KiB, at most 64 MiB (preset 9's).
uint32_t dictionary_for(uint64_t n)
{
    for (uint32_t p = 0; p < 40; ++p)
    {
        const uint64_t d = uint64_t(2 | (p & 1)) << (p / 2 + 11);
        if (d >= 4096 && (d >= n || d >= (uint64_t(64) << 20))) return uint32_t(d);
    }
    return uint32_t(64) << 20;
}

// Raw LZMA2 (after the x86 filter, with `x86`), at preset 9 extreme, with dictionary `dict`.
std::vector<uint8_t> pack_stream(const std::vector<uint8_t>& data, uint32_t dict, bool x86, uint32_t preset = 9 | LZMA_PRESET_EXTREME)
{
    if (data.empty()) return {};
    lzma_options_lzma opt;
    if (lzma_lzma_preset(&opt, preset)) throw std::runtime_error("liblzma: no such preset");
    opt.dict_size = dict;
    const lzma_filter plain[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    const lzma_filter branch[] = {{LZMA_FILTER_X86, nullptr}, {LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    std::vector<uint8_t> out(data.size() + data.size() / 16 + 4096);
    size_t pos = 0;
    const lzma_ret r = lzma_raw_buffer_encode(x86 ? branch : plain, nullptr, data.data(), data.size(), out.data(), &pos, out.size());
    if (r != LZMA_OK) throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
    out.resize(pos);
    return out;
}

// What a stream of `data` packs to, quickly (preset 6): for choosing between two ways.
uint64_t estimate(const std::vector<uint8_t>& data) { return data.empty() ? 0 : pack_stream(data, dictionary_for(data.size()), false, 6).size(); }

// v13: a file cut into pieces, each coded by the way that suits it: LZMA2 (z, carried in the lzma2
// stream) where its bytes repeat, the rope (r, all of them joined and coded as one) where they do
// not repeat but are unevenly spread. A phone's photo: its metadata to LZMA2, its picture to the
// rope. The pieces in order, each its kind and length; empty where one way would take it all.
struct Pieces
{
    std::vector<std::pair<char, uint64_t>> list;
    std::vector<uint8_t> z, r;
};

Pieces pieces_of(const std::vector<uint8_t>& b)
{
    constexpr size_t B = 4096;
    Pieces out;
    if (b.size() < 4 * B) return out;
    // Each byte's cost under the whole file's counts (what the rope pays at least, near enough), and
    // each block's under LZMA2 alone: a block is the rope's unless LZMA2 codes it smaller.
    std::vector<uint64_t> count(256, 0);
    for (const uint8_t v : b) ++count[v];
    std::vector<double> bits(256, 0);
    for (int v = 0; v < 256; ++v)
        if (count[size_t(v)]) bits[size_t(v)] = std::log2(double(b.size()) / double(count[size_t(v)]));
    auto rope_cost = [&](size_t from, size_t to) {
        double c = 0;
        for (size_t i = from; i < to; ++i) c += bits[b[i]];
        return c / 8;
    };
    auto lzma_cost = [&](size_t from, size_t to) {
        const std::vector<uint8_t> part(b.begin() + std::ptrdiff_t(from), b.begin() + std::ptrdiff_t(to));
        return double(pack_stream(part, dictionary_for(part.size()), false, 6).size());
    };
    const size_t blocks = (b.size() + B - 1) / B;
    std::vector<char> kind(blocks);
    std::vector<double> gain(blocks); // what LZMA2 saves over the rope on the block (negative: it costs)
    for (size_t k = 0; k < blocks; ++k)
    {
        const size_t from = k * B, to = std::min(b.size(), from + B);
        gain[k] = rope_cost(from, to) - lzma_cost(from, to);
        kind[k] = gain[k] > 0 ? 'z' : 'r';
    }
    // Runs: one that saves less than 64 bytes over the other way goes to its neighbours' way.
    for (int pass = 0; pass < 2; ++pass)
        for (size_t a = 0; a < blocks;)
        {
            size_t e = a;
            double g = 0;
            while (e < blocks && kind[e] == kind[a]) g += kind[a] == 'z' ? gain[e] : -gain[e], ++e;
            const bool inner = a > 0 && e < blocks;
            if (g < 64 && (inner || pass == 1) && !(a == 0 && e == blocks))
                for (size_t k = a; k < e; ++k) kind[k] = kind[a] == 'z' ? 'r' : 'z';
            a = e;
        }
    std::vector<std::pair<char, uint64_t>> runs;
    for (size_t k = 0; k < blocks; ++k)
    {
        const uint64_t len = std::min(b.size(), (k + 1) * B) - k * B;
        if (!runs.empty() && runs.back().first == kind[k]) runs.back().second += len;
        else runs.emplace_back(kind[k], len);
    }
    if (runs.size() < 2) return out;
    // Each boundary moved, in steps of 64 bytes up to a block either way, to where the two ways
    // cost least: the LZMA2 run packed again at each step (where it is no longer than 1 MiB), the
    // rope's side counted over the two blocks around the boundary.
    uint64_t start = 0;
    for (size_t j = 0; j + 1 < runs.size(); ++j)
    {
        const uint64_t edge = start + runs[j].second, end = edge + runs[j + 1].second;
        const bool z_left = runs[j].first == 'z';
        const uint64_t zlen = z_left ? runs[j].second : runs[j + 1].second;
        const uint64_t lo0 = std::max(start + 64, edge >= B ? edge - B : 0), hi0 = std::min(end - 64, edge + B);
        if (zlen <= (uint64_t(1) << 20) && lo0 <= edge && edge <= hi0)
        {
            const uint64_t lo = std::max(start + 64, edge >= B ? edge - B : 0), hi = std::min(end - 64, edge + B);
            const uint64_t wlo = edge >= B ? edge - B : 0, whi = std::min(uint64_t(b.size()), edge + B);
            double best = -1;
            uint64_t best_e = edge;
            for (uint64_t e = lo + (edge - lo) % 64; e <= hi; e += 64)
            {
                const double c = z_left ? lzma_cost(size_t(start), size_t(e)) + rope_cost(size_t(e), size_t(whi))
                                        : lzma_cost(size_t(e), size_t(end)) + rope_cost(size_t(wlo), size_t(e));
                if (best < 0 || c < best)
                {
                    best = c;
                    best_e = e;
                }
            }
            runs[j].second = best_e - start;
            runs[j + 1].second = end - best_e;
        }
        start += runs[j].second;
    }
    uint64_t pos = 0;
    for (const auto& [k, len] : runs)
    {
        (k == 'z' ? out.z : out.r).insert((k == 'z' ? out.z : out.r).end(), b.begin() + std::ptrdiff_t(pos), b.begin() + std::ptrdiff_t(pos + len));
        pos += len;
    }
    out.list = std::move(runs);
    return out;
}

std::string pieces_text(const std::vector<std::pair<char, uint64_t>>& list)
{
    std::string t;
    for (const auto& [k, len] : list) t += std::string(t.empty() ? "" : " ") + k + std::to_string(len);
    return t;
}

// ---- v14: archives ("Optimise Archive Formats")

bool is_xz(const std::vector<uint8_t>& b)
{
    static const uint8_t sig[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
    return b.size() >= 32 && std::equal(sig, sig + 6, b.begin());
}

// What an xz file holds, or nothing if liblzma cannot read it (or it is not one stream).
std::optional<std::vector<uint8_t>> xz_inner(const std::vector<uint8_t>& b)
{
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_stream_decoder(&z, UINT64_MAX, 0) != LZMA_OK) return std::nullopt;
    std::vector<uint8_t> out;
    std::vector<uint8_t> buf(1 << 20);
    z.next_in = b.data();
    z.avail_in = b.size();
    lzma_ret r = LZMA_OK;
    while (r == LZMA_OK)
    {
        z.next_out = buf.data();
        z.avail_out = buf.size();
        r = lzma_code(&z, LZMA_FINISH);
        out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
    }
    const bool whole = r == LZMA_STREAM_END && z.avail_in == 0;
    lzma_end(&z);
    if (!whole) return std::nullopt;
    return out;
}

const char* xz_check_name(int id) { return id == 0 ? "none" : id == 1 ? "crc32" : id == 4 ? "crc64" : id == 10 ? "sha256" : nullptr; }

} // namespace

std::vector<uint8_t> remake_archive(const std::string& recipe, const std::vector<uint8_t>& inner)
{
    // "xz <preset>[e] <check> <inner size>": liblzma's single-call encoder, as xz writes one stream.
    char kind[8] = {0}, preset[8] = {0}, check[8] = {0};
    unsigned long long size = 0;
    char extra = 0;
    if (std::sscanf(recipe.c_str(), "%7s %7s %7s %llu%c", kind, preset, check, &size, &extra) != 4 || std::string(kind) != "xz" || size != inner.size())
        throw std::runtime_error("an archive's recipe this Sieve does not know: " + recipe);
    const std::string ps(preset), cs(check);
    if (ps.empty() || ps[0] < '0' || ps[0] > '9' || (ps.size() == 2 && ps[1] != 'e') || ps.size() > 2)
        throw std::runtime_error("an xz preset this Sieve does not know: " + recipe);
    const uint32_t level = uint32_t(ps[0] - '0') | (ps.size() == 2 ? LZMA_PRESET_EXTREME : 0);
    int id = -1;
    for (const int c : {0, 1, 4, 10})
        if (cs == xz_check_name(c)) id = c;
    if (id < 0) throw std::runtime_error("an xz check this Sieve does not know: " + recipe);
    // The streaming encoder, as the xz program uses: the single-call one writes each block's sizes
    // in its header, which xz does not, so its files would not come out the same.
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_easy_encoder(&z, level, lzma_check(id)) != LZMA_OK) throw std::runtime_error("liblzma could not remake an xz file");
    std::vector<uint8_t> out(lzma_stream_buffer_bound(inner.size()));
    z.next_in = inner.data();
    z.avail_in = inner.size();
    z.next_out = out.data();
    z.avail_out = out.size();
    const lzma_ret r = lzma_code(&z, LZMA_FINISH);
    const size_t pos = out.size() - z.avail_out;
    lzma_end(&z);
    if (r != LZMA_STREAM_END) throw std::runtime_error("liblzma could not remake an xz file");
    out.resize(pos);
    return out;
}

namespace {

// The recipe that remakes an xz file exactly from what it holds ("xz 6 crc64"), or "" if none does:
// the presets whose dictionary is the one its first block names, each with and without extreme.
std::string xz_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner)
{
    const char* check = xz_check_name(b[7]);
    if (b[6] != 0 || !check) return "";
    // The first block's header: its filters, the last LZMA2's dictionary byte.
    uint32_t dict = 0;
    {
        size_t at = 12;
        const size_t hsize = (size_t(b[at]) + 1) * 4;
        const uint8_t flags = b[at + 1];
        size_t p = at + 2;
        auto vli = [&]() {
            uint64_t v = 0;
            for (int k = 0; k < 9 && p < at + hsize; ++k)
            {
                const uint8_t c = b[p++];
                v |= uint64_t(c & 0x7F) << (7 * k);
                if (!(c & 0x80)) break;
            }
            return v;
        };
        if (b[at] != 0 && at + hsize <= b.size())
        {
            if (flags & 0x40) vli();
            if (flags & 0x80) vli();
            for (int f = 0; f <= (flags & 3); ++f)
            {
                const uint64_t id = vli(), n = vli();
                if (id == 0x21 && n == 1 && p < b.size() && b[p] <= 40) dict = b[p] == 40 ? 0xFFFFFFFFu : (2u | (b[p] & 1u)) << (b[p] / 2 + 11);
                p += size_t(n);
            }
        }
    }
    static const uint32_t preset_dict[10] = {1u << 18, 1u << 20, 1u << 21, 1u << 22, 1u << 22, 1u << 23, 1u << 23, 1u << 24, 1u << 25, 1u << 26};
    for (const int level : {6, 9, 5, 7, 8, 4, 3, 2, 1, 0})
        for (const bool extreme : {false, true})
        {
            if (dict && preset_dict[level] != dict) continue;
            const std::string recipe = "xz " + std::to_string(level) + (extreme ? "e" : "") + " " + check + " " + std::to_string(inner.size());
            if (remake_archive(recipe, inner) == b) return recipe;
        }
    return "";
}

// An archive's contents' folder (or file): its name without the extension, else with ".contents".
std::string contents_name(const std::string& path, const char* ext)
{
    const std::string e(ext);
    if (path.size() > e.size() && path.compare(path.size() - e.size(), e.size(), e) == 0) return path.substr(0, path.size() - e.size());
    return path + ".contents";
}

} // namespace

FileWays file_ways(const std::vector<uint8_t>& bytes, double rope_seconds, uint32_t rope_newest, bool pieces)
{
    auto varint_of = [](uint64_t n) {
        uint64_t v = 1;
        while (n >>= 7) ++v;
        return v;
    };
    FileWays w;
    w.lzma2 = estimate(bytes);
    const RopeSearch found = rope_search(bytes, rope_seconds, rope_newest);
    w.tried += found.tried;
    w.rope = found.best;
    w.rope_code = rope_code(bytes, found.best);
    w.rope_bytes = w.rope_code.size() + varint_of(w.rope_code.size());
    if (pieces)
    {
        Pieces p = pieces_of(bytes);
        if (!p.list.empty())
        {
            const RopeSearch pr = rope_search(p.r, rope_seconds, rope_newest);
            w.tried += pr.tried;
            w.has_pieces = true;
            w.pieces_rope = pr.best;
            w.pieces_code = rope_code(p.r, pr.best);
            w.pieces_way = "pieces " + pieces_text(p.list) + " rope " + pr.best.text();
            w.pieces_bytes = estimate(p.z) + w.pieces_code.size() + varint_of(w.pieces_code.size()) + w.pieces_way.size();
            w.pieces_z = std::move(p.z);
        }
    }
    return w;
}

namespace {

// A text's lines, as locate.cpp's unpacking splits them (at each line feed).
std::vector<std::string_view> lines_of(const std::vector<uint8_t>& t)
{
    std::vector<std::string_view> out;
    const char* p = reinterpret_cast<const char*>(t.data());
    size_t at = 0;
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i] == '\n')
        {
            out.emplace_back(p + at, i - at);
            at = i + 1;
        }
    out.emplace_back(p + at, t.size() - at);
    return out;
}

// The mask of `base`'s lines that makes `text` (its lines in base's order), or nothing.
std::optional<std::vector<uint8_t>> mask_of(const std::vector<std::string_view>& text, const std::vector<std::string_view>& base)
{
    if (text.size() > base.size()) return std::nullopt;
    std::vector<uint8_t> mask((base.size() + 7) / 8, 0);
    size_t j = 0;
    for (size_t i = 0; i < base.size() && j < text.size(); ++i)
        if (base[i] == text[j])
        {
            mask[i / 8] |= uint8_t(1u << (i % 8));
            ++j;
        }
    if (j != text.size()) return std::nullopt;
    return mask;
}

bool is_program(const std::vector<uint8_t>& b)
{
    const std::string k = file_kind(b, b.size());
    return k == "EXE" || k == "ELF";
}

} // namespace

namespace {

// The line an item's way ("item K <place>") names.
size_t item_line_of_way(const std::string& way) { return size_t(std::stoull(way.substr(5, way.find(' ', 5) - 5))); }

// A place (hex) as `width` raw bytes, most significant first.
std::vector<uint8_t> place_bytes_of(const std::string& hex, uint32_t width)
{
    std::string h = BigUint::from_hex(hex).to_hex(size_t(width) * 2);
    std::vector<uint8_t> out(width);
    auto digit = [](char c) { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; };
    for (uint32_t i = 0; i < width && 2 * i + 1 < h.size(); ++i) out[i] = uint8_t(digit(h[2 * i]) * 16 + digit(h[2 * i + 1]));
    return out;
}

} // namespace

void add_packed(Manifest& m, const fs::path& root_in, PackReport* report, const ItemFinder* items, bool item_hashes, const PlaceCoder* coder,
                double rope_seconds, uint32_t rope_newest, bool trim, bool pieces, bool optimise_archives)
{
    // Every file's bytes, checked against the walk: from the contents where a v3 manifest has them,
    // else read from the folder (the current one, for a file named without one).
    std::vector<ManifestEntry*> files;
    for (ManifestEntry& e : m.entries)
        if (!e.dir) files.push_back(&e);
    std::vector<std::vector<uint8_t>> bytes(files.size());
    size_t at = 0;
    const bool have = m.with_contents && !m.packed && m.contents.size() == m.bytes;
    const fs::path root = have ? fs::path() : fs::absolute(root_in.empty() ? fs::path(".") : root_in).lexically_normal();
    for (size_t i = 0; i < files.size(); ++i)
    {
        const ManifestEntry& e = *files[i];
        if (have)
        {
            bytes[i].assign(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
            at += size_t(e.size);
        }
        else bytes[i] = read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end())));
        if (bytes[i].size() != e.size || sha256_hex(bytes[i]) != e.sha256) throw std::runtime_error(e.path + " changed while the folder was being read");
    }

    // v14, "Optimise Archive Formats" (asked for, never by default): an xz file remade exactly from
    // what it holds where liblzma does that, its contents carried; else, and a 7z always (Sieve has
    // no 7-Zip encoder), the archive carried as its contents, in a folder (or file) named after it,
    // and said so. Where that name is taken, or the archive cannot be read, it stays as it is.
    std::vector<std::vector<uint8_t>> original(files.size()); // a remade file's own bytes
    m.unpacked.clear();
    m.archives = false;
    if (optimise_archives && m.compact)
    {
        std::unordered_set<std::string> taken;
        for (const ManifestEntry& e : m.entries) taken.insert(e.dir && !e.path.empty() && e.path.back() == '/' ? e.path.substr(0, e.path.size() - 1) : e.path);
        const bool one = m.one_file_root();
        std::vector<ManifestEntry> entries;
        std::vector<std::vector<uint8_t>> carried, kept;
        size_t fi = 0;
        auto add_file = [&](ManifestEntry e, std::vector<uint8_t> b, std::vector<uint8_t> orig) {
            e.size = (orig.empty() ? b : orig).size();
            e.sha256 = sha256_hex(orig.empty() ? b : orig);
            entries.push_back(std::move(e));
            carried.push_back(std::move(b));
            kept.push_back(std::move(orig));
        };
        std::unordered_set<std::string> dirs; // the folders listed, each ending in '/'
        for (const ManifestEntry& e : m.entries)
            if (e.dir) dirs.insert(e.path);
        auto add_dir = [&](const std::string& d) { // d ends in '/'
            if (!dirs.insert(d).second) return;
            ManifestEntry de;
            de.dir = true;
            de.path = d;
            entries.push_back(de);
            carried.emplace_back();
            kept.emplace_back();
        };
        auto add_dirs = [&](const std::string& path) { // every folder above path
            for (size_t at = path.find('/'); at != std::string::npos; at = path.find('/', at + 1)) add_dir(path.substr(0, at + 1));
        };
        for (ManifestEntry& e : m.entries)
        {
            if (e.dir)
            {
                entries.push_back(e); // listed already in dirs
                carried.emplace_back();
                kept.emplace_back();
                continue;
            }
            std::vector<uint8_t>& b = bytes[fi++];
            // Only where it comes out clearly smaller: the archive as it is (packed, if that is smaller)
            // against what it holds, packed by LZMA2 or pulled tight by the rope (the search is
            // remembered, so packing it later does not search again), its names and recipe counted.
            const uint64_t as_is = std::min<uint64_t>(b.size(), estimate(b));
            auto worth = [&](const std::vector<uint8_t>& inner, uint64_t extra, bool one_file = true) {
                uint64_t cost = estimate(inner);
                if (one_file && rope_seconds >= 0 && inner.size() >= 512) cost = std::min<uint64_t>(cost, uint64_t(rope_search(inner, rope_seconds, rope_newest).estimate));
                // By a clear margin (64 bytes and 0.5%): the estimate is LZMA2 at preset 6, the stream
                // preset 9, and a near thing is not worth giving up the archive as it was.
                return cost + extra + 64 + as_is / 200 < as_is;
            };
            if (is_xz(b))
                if (auto inner = xz_inner(b); inner && worth(*inner, 40))
                {
                    const std::string recipe = xz_recipe(b, *inner);
                    if (!recipe.empty())
                    {
                        ManifestEntry r = e;
                        r.remake = recipe;
                        add_file(r, std::move(*inner), std::move(b));
                        continue;
                    }
                    const std::string name = contents_name(e.path, ".xz");
                    if (!taken.count(name))
                    {
                        taken.insert(name);
                        m.unpacked.push_back({"xz", e.path, name, e.size});
                        ManifestEntry c = e;
                        c.path = name;
                        add_file(c, std::move(*inner), {});
                        continue;
                    }
                }
            if (sieve::install::is_7z(b))
            {
                std::vector<sieve::install::InMemory> all;
                try
                {
                    all = sieve::install::read_7z(b);
                }
                catch (const std::exception&)
                {
                    all.clear();
                }
                const std::string folder = contents_name(e.path, ".7z");
                std::vector<uint8_t> joined;
                uint64_t names = 0;
                for (const auto& in : all)
                {
                    joined.insert(joined.end(), in.bytes.begin(), in.bytes.end());
                    names += folder.size() + in.entry.path.size() + 12;
                }
                // (The rope's estimate only for an archive of one file: for more, their bytes joined
                // are not what the rope would be given.)
                size_t nfiles = 0;
                for (const auto& in : all) nfiles += !in.entry.dir;
                if (!all.empty() && !taken.count(folder) && worth(joined, names, nfiles == 1))
                {
                    taken.insert(folder);
                    m.unpacked.push_back({"7z", e.path, folder, e.size});
                    const std::string prefix = one ? std::string() : folder + "/";
                    if (!one) add_dirs(prefix);
                    for (auto& in : all)
                    {
                        const std::string path = prefix + in.entry.path;
                        add_dirs(path);
                        if (in.entry.dir)
                        {
                            add_dir(path + "/");
                            continue;
                        }
                        ManifestEntry fe;
                        fe.path = path;
                        add_file(fe, std::move(in.bytes), {});
                    }
                    if (one) m.root = folder; // one archive located alone: its contents' folder is the root
                    continue;
                }
            }
            add_file(e, std::move(b), {});
        }
        if (!m.unpacked.empty() || std::any_of(entries.begin(), entries.end(), [](const ManifestEntry& x) { return !x.remake.empty(); }))
        {
            m.archives = true;
            m.entries.clear();
            bytes.clear();
            original.clear();
            for (size_t k = 0; k < entries.size(); ++k)
            {
                m.entries.push_back(std::move(entries[k]));
                if (!m.entries.back().dir)
                {
                    bytes.push_back(std::move(carried[k]));
                    original.push_back(std::move(kept[k]));
                }
            }
            files.clear();
            for (ManifestEntry& e : m.entries)
                if (!e.dir) files.push_back(&e);
            m.files = files.size();
            m.bytes = 0;
            for (const ManifestEntry* f : files) m.bytes += f->size;
        }
        else
        {
            // Nothing changed: the bytes back where they were (the entries are the walk's still).
            bytes.clear();
            for (size_t k = 0; k < entries.size(); ++k)
                if (!entries[k].dir) bytes.push_back(std::move(carried[k]));
        }
    }

    // Each file's way: an item of one of Sieve's lines, its place there (v5); a program in the x86
    // stream; a text made from another's lines, a mask; the rest as it is.
    std::vector<bool> item(files.size(), false);
    std::vector<size_t> used_lines;
    for (size_t i = 0; i < files.size(); ++i)
    {
        files[i]->way = is_program(bytes[i]) && files[i]->remake.empty() ? "x86" : "raw";
        if (!items || bytes[i].empty() || !files[i]->remake.empty()) continue;
        if (const auto found = (*items)(bytes[i]))
        {
            // v6 carries an item only where its place is shorter than the file: the least of the two.
            if (m.minimal && m.line_widths.at(found->first) >= bytes[i].size()) continue;
            // Lines are numbered in the order the files first use them.
            auto slot = std::find(used_lines.begin(), used_lines.end(), found->first);
            if (slot == used_lines.end()) slot = used_lines.insert(used_lines.end(), found->first);
            files[i]->way = "item " + std::to_string(slot - used_lines.begin()) + " " + found->second;
            item[i] = true;
        }
    }
    // v7: each line's places coded where every one has a code and they come to less than raw, each
    // as it would be carried (as it is, or LZMA2 where that is smaller): near-identical models'
    // places pack to almost nothing raw, while their codes, numbers far apart, do not. A code's
    // length, as a varint, goes before it.
    std::vector<std::string> coding(m.lines.size()); // by the line's place in m.lines
    if (m.compact)
        for (size_t slot = 0; slot < used_lines.size(); ++slot)
        {
            const size_t line = used_lines[slot];
            if (!coder) continue;
            std::vector<uint8_t> raw, coded;
            bool all = true;
            std::vector<std::pair<size_t, std::vector<uint8_t>>> codes;
            for (size_t i = 0; i < files.size() && all; ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    const std::string& w = files[i]->way;
                    const std::string place = w.substr(w.find(' ', 5) + 1);
                    auto c = (*coder)(line, place);
                    if (!c)
                    {
                        all = false;
                        continue;
                    }
                    const std::vector<uint8_t> p = place_bytes_of(place, m.line_widths.at(line));
                    raw.insert(raw.end(), p.begin(), p.end());
                    uint64_t n = c->size();
                    while (n >= 0x80)
                    {
                        coded.push_back(uint8_t(n | 0x80));
                        n >>= 7;
                    }
                    coded.push_back(uint8_t(n));
                    coded.insert(coded.end(), c->begin(), c->end());
                    codes.emplace_back(i, std::move(*c));
                }
            if (!all) continue;
            const uint64_t raw_carried = std::min<uint64_t>(raw.size(), estimate(raw));
            const uint64_t coded_carried = std::min<uint64_t>(coded.size(), estimate(coded));
            if (coded_carried >= raw_carried) continue;
            coding[line] = kCodedPlaces;
            static const char hx[] = "0123456789abcdef";
            for (auto& [i, c] : codes)
            {
                std::string h = "item " + std::to_string(slot) + " h";
                for (const uint8_t b : c) h += {hx[b >> 4], hx[b & 15]};
                files[i]->way = std::move(h);
            }
        }
    // v6: each line's items carried as places only where that is smaller, as it would be carried:
    // their places (raw, or LZMA2 where that is smaller) against their files packed together. Files
    // that are near one another (a sample of a line's places) pack to almost nothing as files and
    // stay files; a model alone is shorter as its digits. Measured at the quick preset the
    // derivations use, both alike.
    if (m.minimal)
        for (size_t slot = 0; slot < used_lines.size(); ++slot)
        {
            std::vector<uint8_t> as_files, as_places;
            for (size_t i = 0; i < files.size(); ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    as_files.insert(as_files.end(), bytes[i].begin(), bytes[i].end());
                    const std::string& w = files[i]->way;
                    const std::string place = w.substr(w.find(' ', 5) + 1);
                    if (!coding[used_lines[slot]].empty())
                    {
                        // v7: a coded place, its length before it, as the items stream carries it.
                        std::vector<uint8_t> c;
                        for (size_t h = 1; h + 1 < place.size(); h += 2)
                            c.push_back(uint8_t(std::stoul(place.substr(h, 2), nullptr, 16)));
                        uint64_t n = c.size();
                        while (n >= 0x80)
                        {
                            as_places.push_back(uint8_t(n | 0x80));
                            n >>= 7;
                        }
                        as_places.push_back(uint8_t(n));
                        as_places.insert(as_places.end(), c.begin(), c.end());
                        continue;
                    }
                    const std::vector<uint8_t> p = place_bytes_of(place, m.line_widths.at(used_lines[slot]));
                    as_places.insert(as_places.end(), p.begin(), p.end());
                }
            const uint64_t places = std::min<uint64_t>(as_places.size(), estimate(as_places));
            if (places < estimate(as_files)) continue;
            for (size_t i = 0; i < files.size(); ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    files[i]->way = is_program(bytes[i]) ? "x86" : "raw";
                    item[i] = false;
                }
        }
    // The lines some item still uses, numbered again in the order the files first use them.
    if (m.minimal)
    {
        std::vector<size_t> still;
        for (size_t i = 0; i < files.size(); ++i)
            if (item[i])
            {
                const size_t old = item_line_of_way(files[i]->way);
                auto at = std::find(still.begin(), still.end(), used_lines[old]);
                if (at == still.end()) at = still.insert(still.end(), used_lines[old]);
                const std::string& w = files[i]->way;
                files[i]->way = "item " + std::to_string(at - still.begin()) + w.substr(w.find(' ', 5));
            }
        used_lines = std::move(still);
    }
    std::vector<std::vector<std::string_view>> lines(files.size());
    std::vector<bool> text(files.size(), false);
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way == "raw" && files[i]->remake.empty() && std::find(bytes[i].begin(), bytes[i].end(), uint8_t('\n')) != bytes[i].end())
        {
            text[i] = true;
            lines[i] = lines_of(bytes[i]);
        }
    std::vector<size_t> order(files.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return bytes[a].size() > bytes[b].size(); }); // largest first
    std::vector<bool> base_of_some(files.size(), false), derived(files.size(), false);
    std::vector<std::vector<uint8_t>> masks(files.size());
    std::vector<size_t> base(files.size(), 0);
    std::unordered_map<size_t, std::unordered_set<std::string_view>> line_sets; // a base's lines, for a quick refusal
    for (const size_t t : order)
    {
        if (!text[t] || base_of_some[t]) continue;
        std::optional<std::vector<uint8_t>> best;
        size_t best_base = 0;
        for (const size_t b : order)
        {
            if (b == t || !text[b] || derived[b] || lines[b].size() < lines[t].size()) continue;
            if (best && (lines[b].size() + 7) / 8 >= best->size()) continue; // a smaller mask is had already
            auto [it, made] = line_sets.try_emplace(b);
            if (made) it->second.insert(lines[b].begin(), lines[b].end());
            bool all = true;
            for (const auto& l : lines[t])
                if (!it->second.count(l))
                {
                    all = false;
                    break;
                }
            if (!all) continue;
            if (auto mk = mask_of(lines[t], lines[b]))
            {
                best = std::move(mk);
                best_base = b;
            }
        }
        // Only where the mask packs smaller than the text itself.
        if (best && estimate(*best) < estimate(bytes[t]))
        {
            derived[t] = true;
            base_of_some[best_base] = true;
            masks[t] = std::move(*best);
            base[t] = best_base;
            files[t]->way = "lines " + std::to_string(best_base);
        }
    }

    // v8: a file carried by the tightened rope (rope-wave-v1) where its code, searched for within the
    // time given, is smaller than the file packed alone. Not a file with a copy in the folder: the
    // LZMA2 stream carries a copy for almost nothing, a rope code each time; nor a small one.
    std::vector<std::vector<uint8_t>> rope_codes(files.size());
    std::vector<RopeParams> rope_params(files.size());
    std::vector<std::vector<uint8_t>> piece_z(files.size()); // v13: a file's LZMA2 pieces, joined
    PackReport r;
    m.roped = m.rope_columns = m.rope_lines = m.rope_groups = m.pieces = m.rope_above = false;
    if (m.compact && rope_seconds >= 0)
    {
        std::unordered_map<std::string, size_t> copies;
        for (size_t i = 0; i < files.size(); ++i) ++copies[sha256_hex(bytes[i])];
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (files[i]->way != "raw" || bytes[i].size() < 512 || copies[sha256_hex(bytes[i])] > 1) continue;
            // The smallest of LZMA2 alone, the rope, and (v13) the file in pieces (file_ways).
            FileWays fw = file_ways(bytes[i], rope_seconds, rope_newest, pieces);
            r.rope_tried += fw.tried;
            std::string way;
            std::vector<uint8_t> code;
            RopeParams params;
            uint64_t best = fw.lzma2;
            if (fw.rope_bytes < best)
            {
                best = fw.rope_bytes;
                way = "rope " + fw.rope.text();
                code = std::move(fw.rope_code);
                params = fw.rope;
            }
            if (fw.has_pieces && fw.pieces_bytes < best)
            {
                best = fw.pieces_bytes;
                way = fw.pieces_way;
                code = std::move(fw.pieces_code);
                params = fw.pieces_rope;
                piece_z[i] = std::move(fw.pieces_z);
            }
            if (way.empty()) continue;
            rope_codes[i] = std::move(code);
            rope_params[i] = params;
            files[i]->way = way;
            m.roped = true;
            if (way.rfind("pieces ", 0) == 0)
            {
                m.pieces = true; // v13
                ++r.pieces;
                r.pieces_lzma2 += piece_z[i].size();
                r.pieces_rope += bytes[i].size() - piece_z[i].size();
            }
            if (params.columns) m.rope_columns = true;    // v9
            if (params.columns == 2) m.rope_lines = true; // v10
            if (params.grouped) m.rope_groups = m.rope_lines = true; // v11 (which reads v10's ways too)
            if (params.above) m.rope_above = true;                   // v15
        }
    }

    // The streams: the programs; then every file carried as it is, and then the masks, in order.
    std::vector<uint8_t> x86, plain;
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way == "x86")
        {
            x86.insert(x86.end(), bytes[i].begin(), bytes[i].end());
            ++r.x86;
            r.x86_bytes += bytes[i].size();
        }
        else if (files[i]->way == "raw")
        {
            plain.insert(plain.end(), bytes[i].begin(), bytes[i].end());
            ++r.raw;
            r.raw_bytes += bytes[i].size();
        }
        else if (files[i]->way.rfind("pieces ", 0) == 0) plain.insert(plain.end(), piece_z[i].begin(), piece_z[i].end()); // v13: its LZMA2 pieces, at its turn
    for (size_t i = 0; i < files.size(); ++i)
        if (derived[i])
        {
            plain.insert(plain.end(), masks[i].begin(), masks[i].end());
            ++r.derived;
            r.derived_bytes += bytes[i].size();
            r.mask_bytes += masks[i].size();
            r.derivations.emplace_back(files[i]->path, files[base[i]]->path);
        }
    // v8: the rope stream, each code's length then the code, in the listing's order.
    m.rope_carried.clear();
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way.rfind("rope ", 0) == 0 || files[i]->way.rfind("pieces ", 0) == 0)
        {
            uint64_t len = rope_codes[i].size();
            while (len >= 0x80)
            {
                m.rope_carried.push_back(uint8_t(len | 0x80));
                len >>= 7;
            }
            m.rope_carried.push_back(uint8_t(len));
            m.rope_carried.insert(m.rope_carried.end(), rope_codes[i].begin(), rope_codes[i].end());
            ++r.roped;
            r.rope_bytes += bytes[i].size();
        }
    r.rope_carried = m.rope_carried.size();
    r.rope_columns = m.rope_columns;
    r.rope_lines = m.rope_lines;
    r.rope_groups = m.rope_groups;
    m.rope_stream = {m.rope_carried.size(), m.rope_carried.size(), 0};
    m.x86 = {0, x86.size(), dictionary_for(x86.size())};
    m.lzma2 = {0, plain.size(), dictionary_for(plain.size())};
    const std::vector<uint8_t> px = pack_stream(x86, m.x86.dictionary, true), pp = pack_stream(plain, m.lzma2.dictionary, false);
    m.x86.packed = px.size();
    m.lzma2.packed = pp.size();
    m.streams = px;
    // v12: the files carried as they are where LZMA2 makes them no smaller (a PNG, a JPEG, a 7z).
    bool lzma2_as_is = false;
    if (trim && m.compact && !plain.empty() && pp.size() >= plain.size())
    {
        lzma2_as_is = true;
        m.lzma2 = {plain.size(), plain.size(), 0};
        m.streams.insert(m.streams.end(), plain.begin(), plain.end());
    }
    else m.streams.insert(m.streams.end(), pp.begin(), pp.end());
    m.contents.clear();
    for (size_t i = 0; i < bytes.size(); ++i) // a remade file's own bytes, not what it holds
    {
        const std::vector<uint8_t>& b = original[i].empty() ? bytes[i] : original[i];
        m.contents.insert(m.contents.end(), b.begin(), b.end());
    }
    m.packed = m.with_contents = true;
    m.with_addresses = false;
    r.packed = m.streams.size();
    // v5: the lines the items use, renumbered, and the tree's SHA-256 over every file.
    // v7 and v8 keep their form with no items too (no lines, an empty items stream): what they
    // leave out and the listing packed help any folder, and the tree's SHA-256 is their check.
    m.items = !used_lines.empty() || m.compact;
    if (m.items)
    {
        std::vector<std::string> renumbered, codings;
        std::vector<uint32_t> widths;
        for (size_t k : used_lines)
        {
            renumbered.push_back(m.lines.at(k));
            if (m.minimal) widths.push_back(m.line_widths.at(k));
            if (m.compact) codings.push_back(coding.at(k));
            if (m.compact && !coding[k].empty()) r.coded_lines.push_back(m.lines[k]);
        }
        m.lines = std::move(renumbered);
        m.line_widths = std::move(widths);
        m.line_codings = std::move(codings);
        r.compact = m.compact;
        if (m.minimal)
        {
            // v6: the places, carried raw or as LZMA2, whichever is smaller.
            const std::vector<uint8_t> places = m.items_stream();
            r.item_places = places.size();
            const uint32_t dict = dictionary_for(places.size());
            std::vector<uint8_t> packed = places.empty() ? std::vector<uint8_t>() : pack_stream(places, dict, false);
            if (!places.empty() && packed.size() < places.size())
            {
                m.item_stream = {packed.size(), places.size(), dict};
                m.items_carried = std::move(packed);
            }
            else
            {
                m.item_stream = {places.size(), places.size(), 0};
                m.items_carried = places;
            }
            r.item_carried = m.items_carried.size();
        }
        for (size_t i = 0; i < files.size(); ++i)
            if (item[i])
            {
                if (!item_hashes) files[i]->sha256 = "-";
                ++r.items;
                r.item_bytes += bytes[i].size();
                r.item_text += m.minimal ? files[i]->way.find(' ', 5) : files[i]->way.size();
                if (m.minimal) r.item_raw_places += m.line_widths.at(item_line_of_way(files[i]->way));
            }
        // v7: every file's own SHA-256 left out, as an item's is: the tree's covers them all.
        if (m.compact && !item_hashes)
            for (ManifestEntry* f : files) f->sha256 = "-";
        m.tree = tree_sha256(m);
        // v12, only where it differs from v11: no items line where there are no items, or the
        // lzma2 stream carried as it is.
        if (m.archives) m.pieces = true; // v14 reads v13's ways
        for (const ManifestEntry* f : files) r.remade += !f->remake.empty();
        r.unpacked = m.unpacked.size();
        m.trimmed = trim && m.compact && (lzma2_as_is || m.pieces || (m.item_stream.packed == 0 && m.item_stream.bytes == 0));
        r.pieces_made = m.pieces;
        r.archives_made = m.archives;
        r.rope_above = m.rope_above;
        r.trimmed = m.trimmed;
        r.lzma2_as_is = lzma2_as_is;
    }
    else
    {
        m.lines.clear();
        m.line_widths.clear();
        m.line_codings.clear();
        m.minimal = m.compact = m.roped = m.rope_columns = m.rope_lines = m.rope_groups = m.rope_above = false; // no items: v4, as before
    }

    // Where it is no smaller than the files as they are, they are carried so (v3). (Not with items:
    // they are what makes v5 smaller.)
    if (!m.items)
    {
        Manifest plain_m = m;
        plain_m.packed = false;
        plain_m.streams.clear();
        for (ManifestEntry& e : plain_m.entries) e.way.clear();
        if (plain_m.text().size() + plain_m.contents.size() <= m.text().size() + m.streams.size())
        {
            m = std::move(plain_m);
            r.kept_v3 = true;
            if (report) *report = std::move(r);
            return;
        }
    }
    // v7: the listing carried as LZMA2 where that, with the line saying so, is smaller (a listing of
    // many files repeats itself: paths, and the SHA-256 of every copy of a file).
    m.listing_stream = {};
    m.listing_carried.clear();
    m.listing_of.clear();
    if (m.compact)
    {
        const std::string t = m.text();
        const std::vector<uint8_t> listing(t.begin() + std::ptrdiff_t(t.find('\n') + 1), t.end());
        const uint32_t dict = dictionary_for(listing.size());
        std::vector<uint8_t> packed = pack_stream(listing, dict, false);
        const std::string line = "listing " + std::to_string(packed.size()) + " " + std::to_string(listing.size()) + " " + std::to_string(dict) + "\n";
        if (line.size() + packed.size() < listing.size())
        {
            m.listing_stream = {packed.size(), listing.size(), dict};
            m.listing_carried = std::move(packed);
            m.listing_of = t;
            r.listing = listing.size();
            r.listing_carried = line.size() + m.listing_carried.size();
        }
    }
    // Read back before it is used: unpacked, every file as it was.
    const std::vector<uint8_t> whole = m.file();
    const Manifest back = Manifest::parse(std::string_view(reinterpret_cast<const char*>(whole.data()), whole.size()));
    if (back.contents != m.contents) throw std::runtime_error("packing the installer did not give its files back (a fault in the packer)");
    if (report) *report = std::move(r);
}

std::string pack_report_text(const PackReport& r, uint64_t original)
{
    std::ostringstream o;
    if (r.kept_v3)
    {
        o << "packed    not: packing would not make it smaller, so its files are carried as they are (sieve-manifest-v3)\n";
        return o.str();
    }
    auto pct = [&](uint64_t v) {
        char b[32];
        std::snprintf(b, sizeof b, "%.2f%%", original ? 100.0 * double(v) / double(original) : 100.0);
        return std::string(b);
    };
    if (r.compact)
    {
        const char* version = r.rope_above ? kRopeAboveManifestVersion : r.archives_made ? kArchivesManifestVersion : r.pieces_made ? kPiecesManifestVersion : r.trimmed ? kTrimmedManifestVersion : r.rope_groups ? kRopeGroupsManifestVersion : r.rope_lines ? kRopeLinesManifestVersion : r.rope_columns ? kRopeColumnsManifestVersion
                            : r.roped ? kRopeManifestVersion : kCompactManifestVersion;
        if (r.items)
        {
            o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, as " << r.item_places
              << " bytes of places";
            if (!r.coded_lines.empty())
                o << " (" << r.item_raw_places << " raw; " << kCodedPlaces << " on " << r.coded_lines.size() << " line(s))";
            o << ", carried in " << r.item_carried << "; " << version << ", made again on installing)\n";
        }
        else o << "items     none; " << version << "\n";
        if (r.listing_carried) o << "listing   " << r.listing << " bytes, carried as LZMA2 in " << r.listing_carried << "\n";
        if (r.remade) o << "remade    " << r.remade << " archive(s) carried as what they hold, remade exactly on installing (" << kArchivesManifestVersion << ")\n";
        if (r.unpacked)
            o << "unpacked  " << r.unpacked << " archive(s) carried as their contents, not as the archives (Optimise Archive Formats; " << kArchivesManifestVersion
              << ")\n";
        if (r.pieces)
            o << "pieces    " << r.pieces << " file(s) cut into pieces: " << r.pieces_lzma2 << " bytes packed by LZMA2, " << r.pieces_rope
              << " pulled tight by the rope (" << kPiecesManifestVersion << ")\n";
        if (r.lzma2_as_is) o << "as is     the files packed by LZMA2 carried as they are instead: it made them no smaller (" << kTrimmedManifestVersion << ")\n";
        if (r.roped)
            o << "rope      " << r.roped << " file(s) pulled tight by " << (r.rope_above ? "rope-wave-v1 to v5" : r.rope_groups ? "rope-wave-v1 to v4" : r.rope_lines ? "rope-wave-v1 to v3" : r.rope_columns ? "rope-wave-v1 and v2" : "rope-wave-v1") << " (" << r.rope_bytes << " bytes, carried in " << r.rope_carried
              << "; " << version << ")\n";
    }
    else if (r.items && r.item_places)
        o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, as " << r.item_places
          << " bytes of places, carried in " << r.item_carried << ", and " << r.item_text
          << " characters of the listing; sieve-manifest-v6, made again on installing)\n";
    else if (r.items)
        o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, named in " << r.item_text
          << " characters of the listing; sieve-manifest-v5, made again on installing)\n";
    o << "packed    " << r.packed << " bytes (" << pct(r.packed) << " of " << original << "): " << r.x86 << " program(s) in the x86 stream ("
      << r.x86_bytes << " bytes), " << r.raw << " file(s) as they are (" << r.raw_bytes << " bytes), " << r.derived
      << " derived from another's lines (" << r.derived_bytes << " bytes, as " << r.mask_bytes << " bytes of masks)\n";
    for (const auto& [d, b] : r.derivations) o << "          " << d << ": lines of " << b << "\n";
    return o.str();
}

} // namespace sieve::cli
