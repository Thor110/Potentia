// Sieve — Sieve's own files kept as .sieve, read on the spot and cached (sieve_cache.hpp).
#include "cli/sieve_cache.hpp"

#include "cli/dictionaries.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "cli/plugins.hpp"
#include "sieve/paths.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>

namespace sieve::cli {

namespace fs = std::filesystem;
using sieve::from_u8;
using sieve::u8;

namespace {

std::vector<uint8_t> read_all(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot be read");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A .sieve's members, decoded in memory under the roof, each checked.
std::vector<std::pair<std::string, std::vector<uint8_t>>> members_of(const std::vector<uint8_t>& raw, uint64_t most)
{
    DecodeLimit roof(most); // (a few bytes in one of Sieve's folders never unfold to gigabytes)
    // The packer may have mixed a file's text with Sieve's word lists: what reads them set up first,
    // as every program does before it installs (Sieve's own files are read at start-up, before most of it).
    if (!mix_words()) use_item_maker();
    const Manifest m = manifest_of_installer_raw(raw);
    if (!m.with_contents) throw std::runtime_error("it is not instructions that hold their files");
    return files_of_manifest(m);
}

} // namespace

fs::path decoded_sieve(const fs::path& sieve_file, const std::string& kind, uint64_t most)
{
    const std::vector<uint8_t> raw = read_all(sieve_file);
    const std::string sha = Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(raw.data()), raw.size())));
    const fs::path dir = plugin_cache_dir() / from_u8(kind) / sha;
    std::error_code ec;
    // Decoded before: every member as the index says (its SHA-256 and size), or decoded again.
    if (std::ifstream idx(dir / "index", std::ios::binary); idx)
    {
        bool good = true;
        size_t n = 0;
        std::string line;
        while (good && std::getline(idx, line))
        {
            const size_t a = line.find(' '), b = a == std::string::npos ? a : line.find(' ', a + 1);
            if (b == std::string::npos) { good = false; break; }
            const std::string want = line.substr(0, a), path = line.substr(b + 1);
            try
            {
                const auto bytes = read_all(dir / from_u8(path));
                good = std::to_string(bytes.size()) == line.substr(a + 1, b - a - 1) &&
                       Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()))) == want;
            }
            catch (const std::exception&)
            {
                good = false;
            }
            ++n;
        }
        if (good && n) return dir;
    }
    // Decoded now, into a folder beside it, then put in its place whole.
    const auto members = members_of(raw, most);
    const fs::path part = dir.string() + ".part";
    fs::remove_all(part, ec);
    fs::create_directories(part, ec);
    std::string index;
    for (const auto& [path, bytes] : members)
    {
        const fs::path to = part / from_u8(path);
        fs::create_directories(to.parent_path(), ec);
        std::ofstream out(to, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        out.close();
        if (!out) throw std::runtime_error("cannot write its decoded copy in " + u8(part));
        index += Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()))) + " " + std::to_string(bytes.size()) + " " + path + "\n";
    }
    {
        std::ofstream out(part / "index", std::ios::binary);
        out << index;
        out.close();
        if (!out) throw std::runtime_error("cannot write its decoded copy in " + u8(part));
    }
    fs::remove_all(dir, ec);
    fs::rename(part, dir, ec);
    if (ec) throw std::runtime_error("cannot put its decoded copy in " + u8(dir));
    return dir;
}

void pack_folder_sieve(const fs::path& folder, const fs::path& out, uint64_t most)
{
    Manifest m = walk_folder(folder);
    add_packed_newest(m, folder, nullptr, 1.0); // (the newest instructions, as the hallway's own saves are made)
    const std::vector<uint8_t> raw = raw_of_file(m.file());
    // Read back and checked before it is kept: nothing is saved that will not load.
    const auto back = members_of(raw, most);
    if (back.empty()) throw std::runtime_error("the folder holds no files");
    const fs::path part = out.string() + ".part";
    {
        std::ofstream o(part, std::ios::binary);
        o.write(reinterpret_cast<const char*>(raw.data()), std::streamsize(raw.size()));
        o.close();
        if (!o) throw std::runtime_error("cannot write " + u8(out));
    }
    std::error_code ec;
    fs::rename(part, out, ec);
    if (ec)
    {
        fs::remove(part, ec);
        throw std::runtime_error("cannot write " + u8(out));
    }
}

std::vector<std::pair<std::string, std::vector<uint8_t>>> files_in_sieve(const fs::path& sieve_file, uint64_t most)
{
    return members_of(read_all(sieve_file), most);
}

std::vector<std::string> catalogue_folder(const fs::path& folder, std::vector<std::string>* notes)
{
    std::vector<std::string> out = {folder.string()};
    std::error_code ec;
    std::vector<fs::path> packed;
    for (const auto& e : fs::directory_iterator(folder, ec))
        if (e.path().extension() == ".sieve") packed.push_back(e.path());
    std::sort(packed.begin(), packed.end());
    for (const fs::path& p : packed)
        try
        {
            out.push_back(decoded_sieve(p, "catalogue", kCatalogueSieveRoof).string());
        }
        catch (const std::exception& x)
        {
            if (notes) notes->push_back(u8(p) + ": " + x.what());
        }
    return out;
}

std::vector<std::string> catalogue_folders(std::vector<std::string>* notes)
{
    std::vector<std::string> out;
    for (const char* name : {"catalogue", "languages"})
    {
        std::vector<fs::path> where = {fs::path("data") / name};
        if (const auto exe = executable_dir(); !exe.empty())
            where.push_back(exe / name), where.push_back(exe / ".." / name), where.push_back(exe / ".." / "data" / name); // (a release keeps sieve in tools/)
        for (const auto& w : where)
        {
            std::error_code ec;
            if (!fs::is_directory(w, ec)) continue;
            for (auto& f : catalogue_folder(w, notes)) out.push_back(std::move(f));
            break;
        }
    }
    return out;
}

} // namespace sieve::cli
