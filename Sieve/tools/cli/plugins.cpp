// Sieve — finding and loading filter plugins (plugins.hpp).

#include "cli/plugins.hpp"

#include "cli/dictionaries.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"
#include "sieve/paths.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

using sieve::from_u8;
using sieve::u8;

std::vector<PluginFile> load()
{
    std::vector<fs::path> folders;
    if (const fs::path dir = install_dir(); !dir.empty()) folders.push_back(dir / "filters");
    folders.push_back(fs::path("data") / "filters"); // running from the repository
    std::vector<fs::path> files;
    std::set<fs::path> seen;
    std::map<fs::path, std::string> packed_from;                 // a decoded member: the .sieve it came from
    std::vector<std::pair<std::string, std::string>> refused;     // a .sieve that did not decode, and why
    for (const fs::path& f : folders)
    {
        std::error_code ec;
        if (!fs::is_directory(f, ec)) continue;
        std::vector<fs::path> here;
        std::vector<fs::path> packed;
        for (const auto& e : fs::directory_iterator(f, ec))
            if (e.path().extension() == ".sfilter") here.push_back(e.path());
            else if (e.path().extension() == ".sieve") packed.push_back(e.path());
        std::sort(here.begin(), here.end());
        std::sort(packed.begin(), packed.end());
        for (const fs::path& p : here)
        {
            const fs::path key = fs::weakly_canonical(p, ec);
            if (seen.insert(ec ? p : key).second) files.push_back(p);
        }
        // Filters kept as .sieve: decoded once into the cache (checked at every start), and their
        // .sfilter members read from there, so their lists are beside them.
        for (const fs::path& p : packed)
        {
            const fs::path key = fs::weakly_canonical(p, ec);
            if (!seen.insert(ec ? p : key).second) continue;
            try
            {
                const fs::path dir = decoded_filter_sieve(p);
                std::vector<fs::path> members;
                for (const auto& e : fs::recursive_directory_iterator(dir, ec))
                    if (e.path().extension() == ".sfilter") members.push_back(e.path());
                std::sort(members.begin(), members.end());
                if (members.empty()) throw std::runtime_error("it holds no .sfilter");
                for (const fs::path& m : members)
                {
                    files.push_back(m);
                    packed_from[m] = u8(p);
                }
            }
            catch (const std::exception& e)
            {
                refused.push_back({u8(p), e.what()});
            }
        }
    }

    std::vector<PluginFile> out;
    std::vector<std::shared_ptr<const PluginDef>> defs;
    for (const auto& [path, why] : refused) // (a .sieve that did not decode: listed with why, never silently)
    {
        PluginFile pf;
        pf.path = path;
        pf.packed = path;
        pf.error = why;
        out.push_back(pf);
        defs.push_back(nullptr);
    }
    for (const fs::path& p : files)
    {
        PluginFile pf;
        pf.path = u8(p);
        if (const auto it = packed_from.find(p); it != packed_from.end()) pf.packed = it->second;
        try
        {
            std::ifstream in(p, std::ios::binary);
            if (!in) throw std::runtime_error("cannot be read");
            std::ostringstream s;
            s << in.rdbuf();
            const std::string text = s.str();
            pf.sha256 = Sha256::hex(Sha256::hash(text));
            const std::u8string folder = p.parent_path().u8string();
            auto def = parse_plugin(text, pf.sha256, std::string(folder.begin(), folder.end()));
            pf.name = plugin_header(*def).name();
            for (const auto& b : filter_registry())
                if (b.id == plugin_header(*def).id) throw std::runtime_error("its id, " + b.id + ", is a built-in filter's");
            defs.push_back(def);
        }
        catch (const std::exception& e)
        {
            pf.error = e.what();
            defs.push_back(nullptr);
        }
        out.push_back(pf);
    }
    // One file per name: the same bytes twice is one plugin; different bytes under one name, neither.
    std::map<std::string, std::set<std::string>> shas;
    for (const auto& pf : out)
        if (pf.error.empty()) shas[pf.name].insert(pf.sha256);
    for (auto& pf : out)
        if (pf.error.empty() && shas[pf.name].size() > 1)
            pf.error = "another file is also " + pf.name + ", with different contents: a version is never edited, so neither is used";

    // Prerequisites: each must be a built-in filter or a plugin that loaded, and any setting it
    // pins must be one of that filter's, with a value it accepts. A plugin whose prerequisite was
    // refused is refused too, so this repeats until nothing changes.
    std::map<std::string, size_t> by_name; // loaded plugins, first file of each name
    for (size_t i = 0; i < out.size(); ++i)
        if (out[i].error.empty()) by_name.emplace(out[i].name, i);
    for (bool changed = true; changed;)
    {
        changed = false;
        for (size_t i = 0; i < out.size(); ++i)
        {
            PluginFile& pf = out[i];
            if (!pf.error.empty()) continue;
            for (const auto& r : plugin_header(*defs[i]).prerequisites)
            {
                std::string why;
                const FilterSpec* spec = nullptr;
                FilterSpec plugin_copy;
                if (r.name == pf.name) why = "it requires itself";
                else if (const auto it = by_name.find(r.name); it != by_name.end())
                {
                    if (!out[it->second].error.empty()) why = "it requires " + r.name + ", which was refused";
                    else
                    {
                        plugin_copy = plugin_spec(defs[it->second]);
                        spec = &plugin_copy;
                    }
                }
                else
                {
                    for (const auto& b : filter_registry())
                        if (b.name() == r.name) spec = &b;
                    if (!spec) why = "it requires " + r.name + ", which is not a built-in filter or a plugin that loaded";
                }
                if (spec && why.empty())
                    for (const auto& [key, value] : r.values)
                    {
                        if (std::none_of(spec->params.begin(), spec->params.end(), [&](const FilterParam& p) { return p.key == key; }))
                        {
                            why = "it pins " + key + " on " + r.name + ", which has no such setting";
                            break;
                        }
                        try
                        {
                            const FilterValues one{{key, value}};
                            for (const auto& p : spec->params)
                                if (p.key == key)
                                {
                                    if (p.kind == FilterParam::Kind::Integer) (void)param_int(*spec, one, key);
                                    else (void)param_value(*spec, one, key);
                                }
                        }
                        catch (const std::exception& e)
                        {
                            why = std::string("it pins a setting ") + r.name + " does not accept: " + e.what();
                            break;
                        }
                    }
                if (!why.empty())
                {
                    pf.error = why;
                    changed = true;
                    break;
                }
            }
        }
    }

    std::vector<FilterSpec> specs;
    std::set<std::string> registered;
    std::vector<PluginFile> listed;
    for (size_t i = 0; i < out.size(); ++i)
    {
        PluginFile& pf = out[i];
        if (pf.error.empty())
        {
            if (!registered.insert(pf.name).second) continue; // the same file again (the build's copy and the repository's)
            specs.push_back(plugin_spec(defs[i]));
        }
        listed.push_back(pf);
    }
    register_plugins(std::move(specs));
    return listed;
}

} // namespace

// One folder for every build (Debug, Release, a packaged copy), so a grammar compiled by any of
// them is there for the rest: SIEVE_CACHE if set; else the user's cache folder (Windows:
// %LOCALAPPDATA%\Sieve\cache; elsewhere $XDG_CACHE_HOME/sieve or ~/.cache/sieve); else the
// folder next to the executable. The files are checked by their SHA-256 on every load.
std::filesystem::path plugin_cache_dir()
{
    auto env = [](const char* name) -> std::string {
#ifdef _MSC_VER
        char* v = nullptr;
        size_t n = 0;
        std::string out;
        if (_dupenv_s(&v, &n, name) == 0 && v)
        {
            out = v;
            free(v);
        }
        return out;
#else
        const char* v = std::getenv(name);
        return v ? v : "";
#endif
    };
    auto utf8 = [](const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); };
    if (const std::string s = env("SIEVE_CACHE"); !s.empty()) return utf8(s);
#ifdef _WIN32
    if (const std::string s = env("LOCALAPPDATA"); !s.empty()) return utf8(s) / "Sieve" / "cache";
#else
    if (const std::string s = env("XDG_CACHE_HOME"); !s.empty()) return utf8(s) / "sieve";
    if (const std::string s = env("HOME"); !s.empty()) return utf8(s) / ".cache" / "sieve";
#endif
    return executable_dir() / "cache";
}

const std::vector<PluginFile>& load_plugins()
{
    static std::once_flag once;
    static std::vector<PluginFile> list;
    std::call_once(once, [] {
        // Compiled automata are kept on disk next to the executable, so a large grammar compiles
        // once rather than at every start (sieve/plugin.hpp). A folder that cannot be written
        // leaves them in memory only.
        set_plugin_cache_dir(plugin_cache_dir());
        list = load();
    });
    return list;
}


namespace {

std::vector<uint8_t> read_all(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot be read");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A filters .sieve's members, decoded in memory under the roof, each checked.
std::vector<std::pair<std::string, std::vector<uint8_t>>> members_of(const std::vector<uint8_t>& raw)
{
    DecodeLimit roof(kFilterSieveRoof); // (a few bytes in a filters folder never unfold to gigabytes)
    // The packer may have mixed a filter's text with Sieve's word lists: what reads them set up first,
    // as every program does before it installs (plugins load at start-up, before most of it).
    if (!mix_words()) use_item_maker();
    const Manifest m = manifest_of_installer_raw(raw);
    if (!m.with_contents) throw std::runtime_error("it is not instructions that hold their files");
    return files_of_manifest(m);
}

} // namespace

std::filesystem::path decoded_filter_sieve(const fs::path& sieve_file)
{
    const std::vector<uint8_t> raw = read_all(sieve_file);
    const std::string sha = Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(raw.data()), raw.size())));
    const fs::path dir = plugin_cache_dir() / "sfilters" / sha;
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
    const auto members = members_of(raw);
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

void pack_filter_sieve(const fs::path& folder, const fs::path& out)
{
    Manifest m = walk_folder(folder);
    add_packed_newest(m, folder, nullptr, 1.0); // (the newest instructions, as the hallway's own saves are made)
    const std::vector<uint8_t> raw = raw_of_file(m.file());
    // Read back and checked before it is kept: a filter is never saved as something that will not load.
    const auto back = members_of(raw);
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

std::vector<std::pair<std::string, std::string>> sfilters_in_sieve(const fs::path& sieve_file)
{
    std::vector<std::pair<std::string, std::string>> out;
    for (auto& [path, bytes] : members_of(read_all(sieve_file)))
        if (fs::path(from_u8(path)).extension() == ".sfilter") out.push_back({path, std::string(bytes.begin(), bytes.end())});
    return out;
}

fs::path sfilter_file(const fs::path& file)
{
    if (file.extension() != ".sieve") return file;
    const fs::path dir = decoded_filter_sieve(file);
    std::vector<fs::path> found;
    std::error_code ec;
    for (const auto& e : fs::recursive_directory_iterator(dir, ec))
        if (e.path().extension() == ".sfilter") found.push_back(e.path());
    if (found.empty()) throw std::runtime_error(u8(file) + " holds no .sfilter");
    std::sort(found.begin(), found.end());
    return found.front();
}

} // namespace sieve::cli
