#include "dictionaries.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace sieve::cli {

namespace fs = std::filesystem;

fs::path executable_dir()
{
#ifdef _WIN32
    std::wstring buf(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) return {};
    buf.resize(n);
    return fs::path(buf).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) return {};
    std::error_code ec;
    const fs::path p = fs::weakly_canonical(fs::path(buf.c_str()), ec);
    return ec ? fs::path(buf.c_str()).parent_path() : p.parent_path();
#else
    std::error_code ec;
    const fs::path p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : p.parent_path();
#endif
}

const DictionaryEntry& Registry::default_entry() const
{
    for (const auto& e : entries)
        if (e.is_default) return e;
    throw std::runtime_error("dictionaries.tsv marks no dictionary as default");
}

const DictionaryEntry* Registry::find(const std::string& id) const
{
    for (const auto& e : entries)
        if (e.id == id) return &e;
    return nullptr;
}

Registry load_registry()
{
    std::vector<fs::path> folders = {fs::path("data") / "dictionaries"};
    if (const fs::path exe = executable_dir(); !exe.empty()) folders.push_back(exe / "dictionaries");

    for (const auto& folder : folders)
    {
        const fs::path manifest = folder / "dictionaries.tsv";
        if (!fs::exists(manifest)) continue;
        std::ifstream in(manifest);
        if (!in) throw std::runtime_error("cannot read " + manifest.string());

        Registry r;
        r.folder = folder;
        std::string line;
        int line_no = 0, defaults = 0;
        while (std::getline(in, line))
        {
            ++line_no;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> f;
            std::stringstream ss(line);
            std::string field;
            while (std::getline(ss, field, '\t')) f.push_back(field);
            if (f.size() != 6)
                throw std::runtime_error(manifest.string() + " line " + std::to_string(line_no) +
                                         ": expected 6 tab-separated fields, found " + std::to_string(f.size()));
            DictionaryEntry e{f[0], f[1], f[2], f[4], f[5], f[3] == "yes", folder / f[1]};
            if (e.id.empty() || e.id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") != std::string::npos)
                throw std::runtime_error(manifest.string() + " line " + std::to_string(line_no) + ": bad id '" + e.id + "'");
            if (e.sha256.size() != 64)
                throw std::runtime_error(manifest.string() + " line " + std::to_string(line_no) + ": sha256 must be 64 hex digits");
            if (r.find(e.id))
                throw std::runtime_error(manifest.string() + ": duplicate id '" + e.id + "'");
            defaults += e.is_default;
            r.entries.push_back(std::move(e));
        }
        if (defaults != 1)
            throw std::runtime_error(manifest.string() + ": exactly one dictionary must be marked default (found " +
                                     std::to_string(defaults) + ")");
        return r;
    }

    std::string msg = "cannot find the dictionary registry (dictionaries.tsv). Looked in:";
    for (const auto& f : folders) msg += "\n  " + fs::absolute(f).string();
    msg += "\nRun from the repository root, rebuild to copy dictionaries next to the executable, or pass --dict PATH.";
    throw std::runtime_error(msg);
}

ResolvedDictionary resolve_dictionary(const std::string& value)
{
    const bool looks_like_path = value.find('/') != std::string::npos || value.find('\\') != std::string::npos ||
                                 (value.size() > 4 && value.compare(value.size() - 4, 4, ".txt") == 0);
    if (looks_like_path) return {"file", value, ""};

    const Registry reg = load_registry();
    std::string id = value;
    if (id == "35" || id == "60" || id == "80") id = "scowl-en-" + id; // short forms
    const DictionaryEntry* e = id.empty() ? &reg.default_entry() : reg.find(id);
    if (!e)
    {
        std::string msg = "unknown dictionary '" + value + "'. Available:";
        for (const auto& d : reg.entries) msg += " " + d.id;
        msg += " (see: sieve dicts), or pass a file path.";
        throw std::invalid_argument(msg);
    }
    if (!fs::exists(e->path)) throw std::runtime_error("dictionary '" + e->id + "' is listed but its file is missing: " + e->path.string());
    return {e->id, e->path.string(), e->sha256};
}

} // namespace sieve::cli
