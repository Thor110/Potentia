// Sieve — the catalogue of formats, stage 1 (see catalogue.hpp).
#include "sieve/catalogue.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/animation.hpp"
#include "sieve/character.hpp"
#include "sieve/filekind.hpp"
#include "sieve/importers.hpp"
#include "sieve/raid.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <deque>
#include <map>
#include <set>
#include <string>

namespace sieve {

std::string FormatEntry::text() const
{
    std::string t = std::string(kFormatVersion) + "\nid " + id + "\nkind " + kind + "\nname " + name + "\n";
    for (const auto& l : links) t += "link " + l.type + " " + l.to + "\n";
    if (!recognised.empty()) t += "recognised " + recognised + "\n";
    if (!sieve.empty()) t += "sieve " + sieve + "\n";
    if (!source.empty()) t += "source " + source + "\n";
    return t;
}

std::string FormatEntry::version() const { return Sha256::hex(Sha256::hash(text())); }

namespace {

FormatEntry general(std::string id, std::string name, std::vector<std::string> parents)
{
    FormatEntry e;
    e.id = std::move(id);
    e.kind = "general";
    e.name = std::move(name);
    for (auto& p : parents) e.links.push_back({"part-of", std::move(p)});
    e.source = "the catalogue's general layer";
    return e;
}

std::vector<FormatEntry> gather()
{
    std::vector<FormatEntry> out;
    // The general layer: every other entry leads up through these to "information".
    out.push_back(general("information", "Information", {}));
    for (auto& g : std::vector<FormatEntry>{
             general("data", "Data: anything stored as bytes", {"information"}),
             general("language", "Language", {"information"}),
             general("writing", "Writing: language set down in symbols", {"language"}),
             general("text", "Text", {"writing", "data"}),
             general("pictographs", "Pictographs, ideograms and emoji", {"writing"}),
             general("notation", "Notation: music, mathematics, chemistry", {"writing"}),
             general("genetic", "Genetic sequences", {"information"}),
             general("encoding", "Encodings: how symbols become bytes", {"data"}),
             general("compression", "Compression", {"data"}),
             general("container", "Containers: several streams in one file", {"data"}),
             general("archive", "Archives: files packed together", {"container", "compression"}),
             general("image", "Images", {"data"}),
             general("sound", "Sound", {"data"}),
             general("music", "Music", {"sound", "notation"}),
             general("video", "Video", {"image", "sound"}),
             general("document", "Documents", {"text", "image"}),
             general("executable", "Programs", {"data"}),
             general("model-3d", "3D models", {"data"}),
             general("character", "Characters (Sieve's character system)", {"model-3d"}),
             general("animation", "Animation", {"model-3d"}),
         })
        out.push_back(std::move(g));

    // File kinds (file-kinds-v1), each with the signature it is told by.
    static const std::map<std::string, std::pair<std::string, std::string>> kKinds = {
        {"PNG", {"PNG picture", "image"}},         {"7Z", {"7z archive", "archive"}},           {"XZ", {"xz stream", "compression"}},
        {"MANIFEST", {"Sieve manifest", "document"}}, {"BOOK", {"Sieve book", "text"}},         {"GIF", {"GIF picture", "image"}},
        {"WAV", {"WAV sound", "sound"}},           {"AVI", {"AVI video", "video"}},             {"WEBP", {"WebP picture", "image"}},
        {"MP4", {"MP4 (ISO base media)", "video"}}, {"ZIP", {"ZIP archive", "archive"}},        {"ELF", {"ELF program", "executable"}},
        {"PDF", {"PDF document", "document"}},     {"RAR", {"RAR archive", "archive"}},         {"OGG", {"Ogg container", "container"}},
        {"FLAC", {"FLAC sound", "sound"}},         {"MID", {"MIDI music", "music"}},            {"JPG", {"JPEG picture", "image"}},
        {"MP3", {"MP3 sound (ID3-tagged)", "sound"}}, {"BZ2", {"bzip2 stream", "compression"}}, {"GZ", {"gzip stream", "compression"}},
        {"EXE", {"Windows or DOS program (MZ)", "executable"}}, {"BMP", {"BMP picture", "image"}}, {"TXT", {"Readable text", "text"}},
        {"EMPTY", {"An empty file", "data"}},     {"?", {"Bytes of no known kind", "data"}},
    };
    std::map<std::string, std::string> sigs;
    for (const FileSignature& s : file_signatures())
    {
        std::string hex;
        static const char* kHex = "0123456789ABCDEF";
        for (uint32_t i = 0; i < s.end; ++i)
        {
            if (!hex.empty()) hex += ' ';
            if (s.fixed[i] < 0) hex += "..";
            else hex += kHex[s.fixed[i] >> 4], hex += kHex[s.fixed[i] & 15];
        }
        std::string& slot = sigs[s.kind];
        slot += (slot.empty() ? "" : " or ") + hex;
    }
    for (const std::string& k : file_kinds())
    {
        FormatEntry e;
        std::string id;
        for (const char c : k) id += char(std::tolower(static_cast<unsigned char>(c)));
        if (k == "?") id = "unknown-bytes";
        e.id = "file-" + id;
        e.kind = "file";
        const auto it = kKinds.find(k);
        e.name = it != kKinds.end() ? it->second.first : k;
        e.links.push_back({"part-of", it != kKinds.end() ? it->second.second : std::string("data")});
        if (sigs.count(k)) e.recognised = "first bytes " + sigs[k];
        else if (k == "TXT") e.recognised = "every one of the first 16 bytes readable (20-7E, 80-FF, tab, line feed, carriage return)";
        else if (k == "EMPTY") e.recognised = "no bytes";
        e.sieve = "recognised by file-kinds-v1 and packed by sieve locate (routes: docs/FORMATS.md)";
        e.source = "file-kinds-v1 (sieve/filekind.hpp)";
        out.push_back(std::move(e));
    }

    // The model importers.
    for (const ImportFormat& f : import_formats())
    {
        FormatEntry e;
        e.id = "model-" + std::string(f.id);
        e.kind = "importer";
        e.name = std::string(f.name);
        e.links.push_back({"part-of", "model-3d"});
        if (f.id == "obj" || f.id == "gltf") e.links.push_back({"part-of", "text"});
        e.recognised = "the extension ." + std::string(f.extensions) + (f.id == "glb" ? "; first bytes 67 6C 54 46 (glTF)" : f.id == "gltf" ? "; a JSON object with \"asset\"" : "");
        e.sieve = "imported (sieve import, Ctrl+I): carries " + std::string(f.carries) + "; leaves " + std::string(f.leaves);
        e.source = "the importers (sieve/importers.hpp)";
        out.push_back(std::move(e));
    }

    // Alphabets and Unicode blocks.
    for (const std::string& id : alphabet_ids())
    {
        const Alphabet& a = alphabet_by_id(id);
        FormatEntry e;
        e.id = "alphabet-" + id;
        e.kind = "alphabet";
        e.name = id + ": " + a.description();
        e.links.push_back({"part-of", id == "bytes256" ? "data" : id == "dna4" ? "genetic" : "writing"});
        if (id == "dna4") e.links.push_back({"part-of", "writing"});
        e.recognised = std::to_string(a.size()) + " symbols, pinned in order";
        e.sieve = "a built-in alphabet of the pages line (SPECIFICATIONS 3)";
        e.source = "the built-in alphabets (sieve/alphabet.hpp)";
        out.push_back(std::move(e));
    }
    for (const Block& b : blocks())
    {
        FormatEntry e;
        e.id = "script-" + std::string(b.id);
        e.kind = "script";
        e.name = std::string(b.name);
        const std::string bid(b.id);
        const bool picture = bid.find("emoji") != std::string::npos || bid.find("emoticon") != std::string::npos ||
                             bid.find("pictograph") != std::string::npos || bid.find("hieroglyph") != std::string::npos ||
                             bid.find("mahjong") != std::string::npos || bid.find("domino") != std::string::npos ||
                             bid.find("playing-card") != std::string::npos;
        const bool notation = bid.find("music") != std::string::npos || bid.find("math") != std::string::npos;
        e.links.push_back({"part-of", picture ? "pictographs" : notation ? "notation" : "writing"});
        char range[64];
        std::snprintf(range, sizeof range, "U+%04X-U+%04X", unsigned(b.range.first), unsigned(b.range.last));
        e.recognised = range;
        e.sieve = "a Unicode block for the pages line's alphabets (sieve alphabets --spec " + bid + ")";
        e.source = "the named Unicode blocks (sieve/alphabet.hpp)";
        out.push_back(std::move(e));
    }

    // Sieve's own formats.
    auto own = [&](std::string id, std::string name, std::string parent, std::string recognised, std::string what) {
        FormatEntry e;
        e.id = std::move(id);
        e.kind = "sieve";
        e.name = std::move(name);
        e.links.push_back({"part-of", std::move(parent)});
        e.recognised = std::move(recognised);
        e.sieve = std::move(what);
        e.source = "Sieve's own formats (SPECIFICATIONS)";
        out.push_back(std::move(e));
    };
    own("sieve-file", "A .sieve file: Sieve's packed folder", "archive", "the extension .sieve; its head names its version",
        "made by sieve locate, installed by sieve-install, read by the hallway (docs/SIEVE-FORMAT.md)");
    own(kCharacterVersion, "A character unfolded from a seed", "character", std::string("first line \"") + kCharacterVersion + " seed=...\"",
        "the CHARACTER page and the hallway (SPECIFICATIONS 12.0j)");
    own(kCharacterVersion2, "A character holding models", "character", std::string("first line \"") + kCharacterVersion2 + " seed=...\"",
        "the CHARACTER page and the hallway (SPECIFICATIONS 12.0j)");
    own(kCharacterVersion3, "A character of given spheres (an imported skeleton)", "character",
        std::string("first line \"") + kCharacterVersion3 + " spheres=N\"", "Ctrl+K and sieve import (SPECIFICATIONS 12.0j)");
    own("sieve-species-v1", "A species: a character's settings with the seed free", "character", "\"sieve-species-v1 limbs=...\"",
        "named on the CHARACTER page, grouped on YOURS (IDEAS 19.152)");
    own(kAnimationVersion, "An animation: turns per bone per frame", "animation", std::string("first line \"") + kAnimationVersion + " bones=...\"",
        "the CHARACTER page and the hallway (SPECIFICATIONS 12.0k)");
    own(kRaidManifestVersion, "A version of a save in the infinite RAID", "container", std::string("first line \"") + kRaidManifestVersion + "\"",
        "sieve raid (SPECIFICATIONS 12.0l)");
    own(kFormatVersion, "An entry of this catalogue", "document", std::string("first line \"") + kFormatVersion + "\"", "sieve catalogue");
    return out;
}

} // namespace

const std::vector<FormatEntry>& format_catalogue()
{
    static const std::vector<FormatEntry> all = gather();
    return all;
}

const FormatEntry* format_by_id(std::string_view id)
{
    static const std::map<std::string, size_t, std::less<>> index = [] {
        std::map<std::string, size_t, std::less<>> m;
        const auto& all = format_catalogue();
        for (size_t i = 0; i < all.size(); ++i) m.emplace(all[i].id, i);
        return m;
    }();
    const auto it = index.find(id);
    return it == index.end() ? nullptr : &format_catalogue()[it->second];
}

std::vector<std::string> format_lineage(std::string_view id)
{
    // The shortest chain upward by part-of links.
    std::map<std::string, std::string> from;
    std::deque<std::string> todo{std::string(id)};
    from[std::string(id)] = "";
    while (!todo.empty())
    {
        const std::string at = todo.front();
        todo.pop_front();
        if (at == "information")
        {
            std::vector<std::string> chain;
            for (std::string x = at; !x.empty(); x = from[x]) chain.push_back(x);
            std::reverse(chain.begin(), chain.end());
            return chain;
        }
        const FormatEntry* e = format_by_id(at);
        if (!e) continue;
        for (const auto& l : e->links)
            if (l.type == "part-of" && !from.count(l.to)) from[l.to] = at, todo.push_back(l.to);
    }
    return {};
}

std::vector<std::string> format_path(std::string_view a, std::string_view b)
{
    if (!format_by_id(a) || !format_by_id(b)) return {};
    // Links followed either way.
    std::map<std::string, std::set<std::string>> next;
    for (const auto& e : format_catalogue())
        for (const auto& l : e.links) next[e.id].insert(l.to), next[l.to].insert(e.id);
    std::map<std::string, std::string> from;
    std::deque<std::string> todo{std::string(a)};
    from[std::string(a)] = "";
    while (!todo.empty())
    {
        const std::string at = todo.front();
        todo.pop_front();
        if (at == b)
        {
            std::vector<std::string> chain;
            for (std::string x = at; !x.empty(); x = from[x]) chain.push_back(x);
            std::reverse(chain.begin(), chain.end());
            return chain;
        }
        for (const auto& n : next[at])
            if (!from.count(n)) from[n] = at, todo.push_back(n);
    }
    return {};
}

} // namespace sieve
