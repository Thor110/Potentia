// Sieve CLI -- an installer's files packed (pack.hpp).

#include "cli/pack.hpp"

#include "sieve/filekind.hpp"

#include <lzma.h>

#include <algorithm>
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

void add_packed(Manifest& m, const fs::path& root_in, PackReport* report)
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

    // Each file's way: a program in the x86 stream; a text made from another's lines, a mask; the rest as it is.
    for (size_t i = 0; i < files.size(); ++i) files[i]->way = is_program(bytes[i]) ? "x86" : "raw";
    std::vector<std::vector<std::string_view>> lines(files.size());
    std::vector<bool> text(files.size(), false);
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way == "raw" && std::find(bytes[i].begin(), bytes[i].end(), uint8_t('\n')) != bytes[i].end())
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

    // The streams: the programs; then every file carried as it is, and then the masks, in order.
    std::vector<uint8_t> x86, plain;
    PackReport r;
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
    for (size_t i = 0; i < files.size(); ++i)
        if (derived[i])
        {
            plain.insert(plain.end(), masks[i].begin(), masks[i].end());
            ++r.derived;
            r.derived_bytes += bytes[i].size();
            r.mask_bytes += masks[i].size();
            r.derivations.emplace_back(files[i]->path, files[base[i]]->path);
        }
    m.x86 = {0, x86.size(), dictionary_for(x86.size())};
    m.lzma2 = {0, plain.size(), dictionary_for(plain.size())};
    const std::vector<uint8_t> px = pack_stream(x86, m.x86.dictionary, true), pp = pack_stream(plain, m.lzma2.dictionary, false);
    m.x86.packed = px.size();
    m.lzma2.packed = pp.size();
    m.streams = px;
    m.streams.insert(m.streams.end(), pp.begin(), pp.end());
    m.contents.clear();
    for (const auto& b : bytes) m.contents.insert(m.contents.end(), b.begin(), b.end());
    m.packed = m.with_contents = true;
    m.with_addresses = false;
    r.packed = m.streams.size();

    // Where it is no smaller than the files as they are, they are carried so (v3).
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
    o << "packed    " << r.packed << " bytes (" << pct(r.packed) << " of " << original << "): " << r.x86 << " program(s) in the x86 stream ("
      << r.x86_bytes << " bytes), " << r.raw << " file(s) as they are (" << r.raw_bytes << " bytes), " << r.derived
      << " derived from another's lines (" << r.derived_bytes << " bytes, as " << r.mask_bytes << " bytes of masks)\n";
    for (const auto& [d, b] : r.derivations) o << "          " << d << ": lines of " << b << "\n";
    return o.str();
}

} // namespace sieve::cli
