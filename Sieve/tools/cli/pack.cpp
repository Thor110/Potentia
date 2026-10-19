// Sieve CLI -- an installer's files packed (pack.hpp).

#include "cli/pack.hpp"

#include "sieve/filekind.hpp"
#include "sieve/rope_wave.hpp"

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
                double rope_seconds)
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

    // Each file's way: an item of one of Sieve's lines, its place there (v5); a program in the x86
    // stream; a text made from another's lines, a mask; the rest as it is.
    std::vector<bool> item(files.size(), false);
    std::vector<size_t> used_lines;
    for (size_t i = 0; i < files.size(); ++i)
    {
        files[i]->way = is_program(bytes[i]) ? "x86" : "raw";
        if (!items || bytes[i].empty()) continue;
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

    // v8: a file carried by the tightened rope (rope-wave-v1) where its code, searched for within the
    // time given, is smaller than the file packed alone. Not a file with a copy in the folder: the
    // LZMA2 stream carries a copy for almost nothing, a rope code each time; nor a small one.
    std::vector<std::vector<uint8_t>> rope_codes(files.size());
    std::vector<RopeParams> rope_params(files.size());
    PackReport r;
    m.roped = false;
    if (m.compact && rope_seconds >= 0)
    {
        std::unordered_map<std::string, size_t> copies;
        for (size_t i = 0; i < files.size(); ++i) ++copies[sha256_hex(bytes[i])];
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (files[i]->way != "raw" || bytes[i].size() < 512 || copies[sha256_hex(bytes[i])] > 1) continue;
            const RopeSearch found = rope_search(bytes[i], rope_seconds);
            r.rope_tried += found.tried;
            std::vector<uint8_t> code = rope_code(bytes[i], found.best);
            uint64_t len = code.size(), varint = 1;
            while (len >>= 7) ++varint;
            if (code.size() + varint >= estimate(bytes[i])) continue;
            rope_codes[i] = std::move(code);
            rope_params[i] = found.best;
            files[i]->way = "rope " + found.best.text();
            m.roped = true;
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
        if (files[i]->way.rfind("rope ", 0) == 0)
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
    m.rope_stream = {m.rope_carried.size(), m.rope_carried.size(), 0};
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
    }
    else
    {
        m.lines.clear();
        m.line_widths.clear();
        m.line_codings.clear();
        m.minimal = m.compact = m.roped = false; // no items: v4, as before
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
        const char* version = r.roped ? kRopeManifestVersion : kCompactManifestVersion;
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
        if (r.roped)
            o << "rope      " << r.roped << " file(s) pulled tight by rope-wave-v1 (" << r.rope_bytes << " bytes, carried in " << r.rope_carried
              << "; sieve-manifest-v8)\n";
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
