#include "filter_config.hpp"

#include "dictionaries.hpp"
#include "models.hpp"

#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

const char* kSections[4] = {"text", "image", "audio", "video"};

std::string trim(std::string s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    const auto b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

} // namespace

const char* to_string(FilterMode m)
{
    switch (m)
    {
    case FilterMode::Off: return "off";
    case FilterMode::Mark: return "mark";
    case FilterMode::Hide: return "hide";
    case FilterMode::Compact: return "compact";
    }
    return "off";
}

FilterMode filter_mode_from_string(const std::string& s)
{
    if (s == "off") return FilterMode::Off;
    if (s == "mark") return FilterMode::Mark;
    if (s == "hide") return FilterMode::Hide;
    if (s == "compact") return FilterMode::Compact;
    throw std::invalid_argument("filter mode must be off, mark, hide or compact, not '" + s + "'");
}

bool LineFilters::is_enabled(const std::string& name) const
{
    return std::find(enabled.begin(), enabled.end(), name) != enabled.end();
}

void LineFilters::set_enabled(const std::string& name, bool on)
{
    const auto it = std::find(enabled.begin(), enabled.end(), name);
    if (on && it == enabled.end()) enabled.push_back(name);
    if (!on && it != enabled.end()) enabled.erase(it);
}

fs::path FilterConfig::default_path()
{
    const fs::path exe = executable_dir();
    return (exe.empty() ? fs::path(".") : exe) / "sieve-filters.ini";
}

FilterConfig FilterConfig::load(const fs::path& path)
{
    FilterConfig c;
    std::ifstream in(path);
    if (!in) return c;
    std::string line, section;
    int line_no = 0;
    while (std::getline(in, line))
    {
        ++line_no;
        const size_t semi = line.find(';');
        if (semi != std::string::npos) line = line.substr(0, semi);
        line = trim(line);
        if (line.empty()) continue;
        if (line.front() == '[')
        {
            if (line.back() != ']') throw std::runtime_error(path.string() + " line " + std::to_string(line_no) + ": bad section");
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) throw std::runtime_error(path.string() + " line " + std::to_string(line_no) + ": expected key = value");
        const std::string key = trim(line.substr(0, eq)), value = trim(line.substr(eq + 1));
        const size_t dot = section.find('.');
        const std::string line_name = section.substr(0, dot);
        const auto li = std::find(std::begin(kSections), std::end(kSections), line_name) - std::begin(kSections);
        if (li >= 4) throw std::runtime_error(path.string() + ": unknown section [" + section + "]");
        LineFilters& lf = c.lines[li];
        if (dot == std::string::npos)
        {
            if (key == "mode") lf.mode = filter_mode_from_string(value);
            else if (key == "filters")
            {
                lf.enabled.clear();
                std::stringstream ss(value);
                std::string name;
                while (std::getline(ss, name, ','))
                    if (!trim(name).empty()) lf.enabled.push_back(trim(name));
            }
            else throw std::runtime_error(path.string() + ": unknown key '" + key + "' in [" + section + "]");
        }
        else lf.values[section.substr(dot + 1)][key] = value;
    }
    return c;
}

void FilterConfig::save(const fs::path& path) const
{
    std::ostringstream o;
    o << "; Sieve filter stack, one section per line. Edited by the hallway's setup menu (magnifying\n"
      << "; glass beside each line); hand edits are welcome.\n"
      << ";   mode     off | mark (dim failing books) | hide (leave them out) | compact (only survivors)\n"
      << ";   filters  the ticked filters, comma-separated (list them with: sieve filters --line LINE)\n"
      << "; A [line.filter] section holds that filter's parameters.\n";
    for (int i = 0; i < 4; ++i)
    {
        const LineFilters& lf = lines[i];
        o << "\n[" << kSections[i] << "]\nmode = " << to_string(lf.mode) << "\nfilters = ";
        for (size_t k = 0; k < lf.enabled.size(); ++k) o << (k ? ", " : "") << lf.enabled[k];
        o << "\n";
        for (const auto& [name, vals] : lf.values)
        {
            if (vals.empty()) continue;
            o << "\n[" << kSections[i] << "." << name << "]\n";
            for (const auto& [k, v] : vals) o << k << " = " << v << "\n";
        }
    }
    std::ofstream out(path, std::ios::binary);
    out << o.str();
    if (!out) throw std::runtime_error("cannot write " + path.string());
}

std::shared_ptr<const Dictionary> AppResources::dictionary(const std::string& id) const
{
    static std::mutex mu;
    static std::map<std::string, std::shared_ptr<const Dictionary>> cache;
    std::lock_guard<std::mutex> lock(mu);
    auto& slot = cache[id];
    if (!slot)
    {
        const ResolvedDictionary rd = resolve_dictionary(id);
        auto d = std::make_shared<const Dictionary>(Dictionary::load_file(rd.path));
        if (!rd.expected_sha256.empty() && d->sha256() != rd.expected_sha256)
            throw std::runtime_error("dictionary '" + rd.id + "' does not match its pinned SHA-256");
        slot = d;
    }
    return slot;
}

std::shared_ptr<const CharModel> AppResources::model(const std::string& id, const std::string& symbols) const
{
    const LoadedModel m = resolve_model(id, symbols);
    if (!m.model) throw std::invalid_argument("no model for " + symbols + " (see: sieve models)");
    return m.model;
}

FilterLine filter_line(const Line& line)
{
    FilterLine f;
    f.kind = to_string(line.kind);
    f.symbols_id = line.space.symbols_id();
    f.base = line.space.base();
    f.length = line.space.unit_length();
    f.alphabet = line.alphabet;
    if (line.kind == LineKind::Image || line.kind == LineKind::Video)
    {
        f.width = line.image.width;
        f.height = line.image.height;
        f.frames = line.image.frames;
    }
    return f;
}

FilterStack build_stack(const Line& line, const LineFilters& settings) { return build_stack(filter_line(line), settings); }

FilterStack build_stack(const FilterLine& fl, const LineFilters& settings)
{
    std::vector<FilterStack::Entry> entries;
    for (const auto& name : settings.enabled)
    {
        const FilterSpec* spec = find_filter(name);
        if (!spec) throw std::invalid_argument("unknown filter '" + name + "' (see: sieve filters)");
        if (!spec->applies(fl)) continue;
        const auto it = settings.values.find(spec->name());
        entries.push_back({spec, it == settings.values.end() ? FilterValues{} : it->second});
    }
    static const AppResources resources;
    return FilterStack(fl, entries, resources);
}

} // namespace sieve::cli
