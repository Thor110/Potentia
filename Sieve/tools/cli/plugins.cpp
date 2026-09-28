// Sieve — finding and loading filter plugins (plugins.hpp).

#include "cli/plugins.hpp"

#include "cli/dictionaries.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

std::vector<PluginFile> load()
{
    std::vector<fs::path> folders;
    if (const fs::path dir = install_dir(); !dir.empty()) folders.push_back(dir / "filters");
    folders.push_back(fs::path("data") / "filters"); // running from the repository
    std::vector<fs::path> files;
    std::set<fs::path> seen;
    for (const fs::path& f : folders)
    {
        std::error_code ec;
        if (!fs::is_directory(f, ec)) continue;
        std::vector<fs::path> here;
        for (const auto& e : fs::directory_iterator(f, ec))
            if (e.path().extension() == ".sfilter") here.push_back(e.path());
        std::sort(here.begin(), here.end());
        for (const fs::path& p : here)
        {
            const fs::path key = fs::weakly_canonical(p, ec);
            if (seen.insert(ec ? p : key).second) files.push_back(p);
        }
    }

    std::vector<PluginFile> out;
    std::vector<std::shared_ptr<const PluginDef>> defs;
    for (const fs::path& p : files)
    {
        PluginFile pf;
        pf.path = u8(p);
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

const std::vector<PluginFile>& load_plugins()
{
    static std::once_flag once;
    static std::vector<PluginFile> list;
    std::call_once(once, [] { list = load(); });
    return list;
}

} // namespace sieve::cli
