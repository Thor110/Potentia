#include "filter_config.hpp"

#include "dictionaries.hpp"
#include "models.hpp"
#include "image_io.hpp"

#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <mutex>
#include <set>
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

const char* kBookParts[3] = {"cover", "title", "pages"};
const char* kCompositionParts[CompositionFilters::kParts] = {"cover", "title", "units", "joined"};
const char* kWorldParts[WorldFilters::kParts] = {"cover", "title", "models"};

void read_filters(LineFilters& lf, const std::string& value)
{
    lf.enabled.clear();
    std::stringstream ss(value);
    std::string name;
    while (std::getline(ss, name, ','))
    {
        if (!trim(name).empty() && !lf.is_enabled(trim(name))) lf.enabled.push_back(trim(name));
    }
}

void write_filters(std::ostream& o, const LineFilters& lf)
{
    o << "filters = ";
    for (size_t k = 0; k < lf.enabled.size(); ++k) o << (k ? ", " : "") << lf.enabled[k];
    o << "\n";
}

void write_values(std::ostream& o, const std::string& prefix, const LineFilters& lf)
{
    for (const auto& [name, vals] : lf.values)
    {
        if (vals.empty()) continue;
        o << "\n[" << prefix << "." << name << "]\n";
        for (const auto& [k, v] : vals) o << k << " = " << v << "\n";
    }
}

// Filters by their full names: "title" and "title-v1" (the newest version) are one filter, in the
// ticked list and in parameter sections alike, and it is ticked once.
void canonicalise(LineFilters& lf)
{
    auto full = [](const std::string& n) {
        const FilterSpec* f = find_filter(n);
        return f ? f->name() : n;
    };
    std::vector<std::string> names;
    for (const auto& n : lf.enabled)
        if (std::find(names.begin(), names.end(), full(n)) == names.end()) names.push_back(full(n));
    lf.enabled = std::move(names);
    std::map<std::string, FilterValues> values;
    for (const auto& [n, vals] : lf.values)
        for (const auto& [k, v] : vals) values[full(n)][k] = v;
    lf.values = std::move(values);
}

FilterMode mode_at(const std::string& value, const fs::path& path, int line_no)
{
    try
    {
        return filter_mode_from_string(value);
    }
    catch (const std::invalid_argument& e)
    {
        throw std::runtime_error(path.string() + " line " + std::to_string(line_no) + ": " + e.what());
    }
}

} // namespace

const char* BookFilters::part_name(int i) { return kBookParts[i]; }
const char* CompositionFilters::part_name(int i) { return kCompositionParts[i]; }
int CompositionFilters::part_index(const std::string& name)
{
    for (int i = 0; i < kParts; ++i)
        if (name == kCompositionParts[i]) return i;
    return -1;
}

const char* WorldFilters::part_name(int i) { return kWorldParts[i]; }
int WorldFilters::part_index(const std::string& name)
{
    for (int i = 0; i < kParts; ++i)
        if (name == kWorldParts[i]) return i;
    return -1;
}

int BookFilters::part_index(const std::string& name)
{
    for (int i = 0; i < 3; ++i)
        if (name == kBookParts[i]) return i;
    return -1;
}

const char* to_string(FilterMode m)
{
    switch (m)
    {
    case FilterMode::Off: return "off";
    case FilterMode::Mark: return "mark";
    case FilterMode::Hide: return "hide";
    case FilterMode::Compact: return "compact";
    case FilterMode::Excluded: return "excluded";
    case FilterMode::Full: return "full";
    }
    return "off";
}

FilterMode filter_mode_from_string(const std::string& s)
{
    if (s == "off") return FilterMode::Off;
    if (s == "mark") return FilterMode::Mark;
    if (s == "hide") return FilterMode::Hide;
    if (s == "compact") return FilterMode::Compact;
    if (s == "excluded") return FilterMode::Excluded;
    if (s == "full") return FilterMode::Full;
    throw std::invalid_argument("filter mode must be off, mark, hide, compact, full or excluded, not '" + s + "'");
}

const FilterValues& LineFilters::values_of(const std::string& name) const
{
    static const FilterValues none;
    const auto it = values.find(name);
    return it == values.end() ? none : it->second;
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
    const fs::path exe = install_dir(); // shared with the hallway, from tools\ too
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
        line = trim(line);
        // Comments: a line starting with ';' or '#', or ' ;' after a value (a ';' or '#' inside a
        // value, as in a file name, is kept).
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        for (size_t i = 1; i < line.size(); ++i)
            if (line[i] == ';' && (line[i - 1] == ' ' || line[i - 1] == '\t'))
            {
                line = trim(line.substr(0, i));
                break;
            }
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
        if (line_name == "tracks" || line_name == "movies")
        {
            CompositionFilters& cf = line_name == "tracks" ? c.tracks : c.movies;
            const std::string where = path.string() + ": unknown key '" + key + "' in [" + section + "]";
            if (dot == std::string::npos)
            {
                if (key != "mode") throw std::runtime_error(where);
                cf.mode = mode_at(value, path, line_no);
                continue;
            }
            const std::string rest = section.substr(dot + 1);
            const size_t dot2 = rest.find('.');
            const int pi = CompositionFilters::part_index(rest.substr(0, dot2));
            if (pi < 0) throw std::runtime_error(path.string() + ": unknown section [" + section + "] (" + line_name + " have cover, title, units and joined)");
            LineFilters& lf = cf.parts[pi];
            if (dot2 != std::string::npos) lf.values[rest.substr(dot2 + 1)][key] = value;
            else if (key == "filters") read_filters(lf, value);
            else throw std::runtime_error(where);
            continue;
        }
        if (line_name == "worlds")
        {
            const std::string where = path.string() + ": unknown key '" + key + "' in [" + section + "]";
            if (dot == std::string::npos)
            {
                if (key != "mode") throw std::runtime_error(where);
                c.worlds.mode = mode_at(value, path, line_no);
                continue;
            }
            const std::string rest = section.substr(dot + 1);
            const size_t dot2 = rest.find('.');
            const int pi = WorldFilters::part_index(rest.substr(0, dot2));
            if (pi < 0) throw std::runtime_error(path.string() + ": unknown section [" + section + "] (worlds have cover, title and models)");
            LineFilters& lf = c.worlds.parts[pi];
            if (dot2 != std::string::npos) lf.values[rest.substr(dot2 + 1)][key] = value;
            else if (key == "filters") read_filters(lf, value);
            else throw std::runtime_error(where);
            continue;
        }
        if (line_name == "books")
        {
            const std::string where = path.string() + ": unknown key '" + key + "' in [" + section + "]";
            if (dot == std::string::npos)
            {
                if (key != "mode") throw std::runtime_error(where);
                c.books.mode = mode_at(value, path, line_no);
                continue;
            }
            const std::string rest = section.substr(dot + 1);
            const size_t dot2 = rest.find('.');
            const int pi = BookFilters::part_index(rest.substr(0, dot2));
            if (pi < 0) throw std::runtime_error(path.string() + ": unknown section [" + section + "] (the books line has cover, title and pages)");
            LineFilters& lf = c.books.parts[pi];
            if (dot2 != std::string::npos) lf.values[rest.substr(dot2 + 1)][key] = value;
            else if (key == "filters") read_filters(lf, value);
            else throw std::runtime_error(where);
            continue;
        }
        const auto li = std::find(std::begin(kSections), std::end(kSections), line_name) - std::begin(kSections);
        if (li >= 4 && line_name != "models" && line_name != "binary")
            throw std::runtime_error(path.string() + ": unknown section [" + section + "]");
        LineFilters& lf = line_name == "models" ? c.models : line_name == "binary" ? c.binary : c.lines[li];
        if (dot == std::string::npos)
        {
            if (key == "mode") lf.mode = mode_at(value, path, line_no);
            else if (key == "filters") read_filters(lf, value);
            else throw std::runtime_error(path.string() + ": unknown key '" + key + "' in [" + section + "]");
        }
        else lf.values[section.substr(dot + 1)][key] = value;
    }
    for (auto& lf : c.lines) canonicalise(lf);
    for (auto& lf : c.books.parts) canonicalise(lf);
    for (auto& lf : c.tracks.parts) canonicalise(lf);
    for (auto& lf : c.movies.parts) canonicalise(lf);
    for (auto& lf : c.worlds.parts) canonicalise(lf);
    canonicalise(c.models);
    canonicalise(c.binary);
    return c;
}

void FilterConfig::save(const fs::path& path) const
{
    std::ostringstream o;
    o << "; Sieve filter stack, one section per line. Edited by the hallway's setup menu (magnifying\n"
      << "; glass beside each line); hand edits are welcome.\n"
      << ";   mode     off | mark (dim failing books) | hide (leave them out) | compact (only survivors)\n"
      << ";            | full (only survivors, each with its titles and covers)\n"
      << ";            | excluded (only the books that fail, in their places)\n"
      << ";   filters  the ticked filters, comma-separated (list them with: sieve filters --line LINE)\n"
      << "; A [line.filter] section holds that filter's parameters.\n";
    for (int i = 0; i < 4; ++i)
    {
        const LineFilters& lf = lines[i];
        o << "\n[" << kSections[i] << "]\nmode = " << to_string(lf.mode) << "\n";
        write_filters(o, lf);
        write_values(o, kSections[i], lf);
    }
    o << "\n; The books line: one mode; the title is judged as one page, the cover as a picture, and the\n"
      << "; pages as one continuous text (a word cut by a page break is judged whole).\n"
      << "[books]\nmode = " << to_string(books.mode) << "\n";
    for (int i = 0; i < 3; ++i)
    {
        o << "\n[books." << kBookParts[i] << "]\n";
        write_filters(o, books.parts[i]);
        write_values(o, std::string("books.") + kBookParts[i], books.parts[i]);
    }
    o << "\n; Tracks and movies: one mode each; the cover is judged as a picture, the title as a title,\n"
      << "; each unit on its own as a unit of audio or video, and the units joined as one longer unit.\n";
    for (const auto& [name, cf] : {std::pair<const char*, const CompositionFilters*>{"tracks", &tracks}, {"movies", &movies}})
    {
        o << "[" << name << "]\nmode = " << to_string(cf->mode) << "\n";
        for (int i = 0; i < CompositionFilters::kParts; ++i)
        {
            o << "\n[" << name << "." << kCompositionParts[i] << "]\n";
            write_filters(o, cf->parts[i]);
            write_values(o, std::string(name) + "." + kCompositionParts[i], cf->parts[i]);
        }
        o << "\n";
    }
    o << "; Worlds: one mode; the cover is judged as a picture, the title as a title, and each slot's model\n"
      << "; by the models line's filters (a slot's place has none).\n"
      << "[worlds]\nmode = " << to_string(worlds.mode) << "\n";
    for (int i = 0; i < WorldFilters::kParts; ++i)
    {
        o << "\n[worlds." << kWorldParts[i] << "]\n";
        write_filters(o, worlds.parts[i]);
        write_values(o, std::string("worlds.") + kWorldParts[i], worlds.parts[i]);
    }
    o << "\n; The models line: no filters are registered for it yet; its mode is kept for then. The binary\n"
      << "; line (one line, met at both ends) has its files' kinds: binary-kind-v1.\n";
    for (const auto& [name, lf] : {std::pair<const char*, const LineFilters*>{"models", &models}, {"binary", &binary}})
    {
        o << "\n[" << name << "]\nmode = " << to_string(lf->mode) << "\n";
        write_filters(o, *lf);
        write_values(o, name, *lf);
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

std::vector<std::string> tick_filter(LineFilters& settings, const std::string& name, bool on)
{
    std::vector<std::string> added;
    if (!on)
    {
        settings.set_enabled(name, false);
        return added;
    }
    std::vector<std::pair<std::string, const FilterValues*>> todo{{name, nullptr}};
    std::set<std::string> seen;
    while (!todo.empty())
    {
        const auto [n, pinned] = todo.back();
        todo.pop_back();
        if (!seen.insert(n).second) continue;
        if (!settings.is_enabled(n))
        {
            settings.set_enabled(n, true);
            if (n != name) added.push_back(n);
            if (pinned) // newly ticked: it takes the settings its dependant pins
                for (const auto& [k, v] : *pinned) settings.values[n][k] = v;
        }
        if (const FilterSpec* spec = find_filter(n))
            for (const auto& r : spec->prerequisites) todo.emplace_back(r.name, &r.values);
    }
    return added;
}

std::vector<std::string> tick_filter_by_hand(LineFilters& settings, const std::string& name, bool on, const FilterLine* line)
{
    std::vector<std::string> unticked;
    if (!on)
    {
        (void)tick_filter(settings, name, false);
        return unticked;
    }
    std::vector<std::string> fresh = tick_filter(settings, name, true);
    fresh.push_back(name);
    for (const std::string& other : std::vector<std::string>(settings.enabled))
    {
        if (std::find(fresh.begin(), fresh.end(), other) != fresh.end()) continue;
        const FilterSpec* b = find_filter(other);
        if (!b) continue;
        for (const std::string& n : fresh)
            if (const FilterSpec* a = find_filter(n); a && !filter_conflict(*a, *b, line, &settings.values_of(n), &settings.values_of(other)).empty())
            {
                settings.set_enabled(other, false);
                unticked.push_back(other);
                break;
            }
    }
    return unticked;
}

std::vector<std::string> prerequisite_notes(const LineFilters& settings)
{
    std::vector<std::string> notes;
    for (const auto& name : settings.enabled)
    {
        const FilterSpec* spec = find_filter(name);
        if (!spec) continue;
        for (const auto& r : spec->prerequisites)
        {
            if (!settings.is_enabled(r.name))
            {
                notes.push_back(name + " requires " + r.name + ", which is not ticked (it joins the stack anyway)");
                continue;
            }
            const FilterSpec* req = find_filter(r.name);
            const auto it = settings.values.find(r.name);
            const FilterValues have = it == settings.values.end() ? FilterValues{} : it->second;
            for (const auto& [k, v] : r.values)
            {
                const std::string now = req ? param_value(*req, have, k) : (have.count(k) ? have.at(k) : "");
                if (now != v) notes.push_back(name + " requires " + r.name + " at " + k + "=" + v + "; it is set to " + k + "=" + now);
            }
        }
    }
    return notes;
}

FilterStack build_stack(const FilterLine& fl, const LineFilters& given)
{
    // The ticked filters, and any prerequisite a hand-edited file left out (with its pinned settings).
    LineFilters settings = given;
    for (const auto& name : given.enabled) (void)tick_filter(settings, name, true);
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

FilterLine binary_filter_line(uint64_t max_bytes)
{
    FilterLine f;
    f.kind = "binary";
    f.symbols_id = "bytes256";
    f.base = 256;
    f.length = uint32_t(std::min<uint64_t>(max_bytes, 0xffffffffull));
    return f;
}

std::optional<KindCounter::Pattern> page_pattern(const Alphabet& a, uint32_t length)
{
    KindCounter::Pattern p;
    if (holds_all_bytes(a))
    {
        std::array<bool, 256> any;
        any.fill(true);
        p.allowed.assign(length, any); // every file of the length is a page
        return p;
    }
    std::array<bool, 256> symbols{};
    for (uint32_t d = 0; d < a.size(); ++d)
    {
        if (a.symbol(d) >= 0x80) return std::nullopt; // more than a byte: judged instead
        symbols[size_t(a.symbol(d))] = true;
    }
    p.allowed.assign(length, symbols); // a page's file is exactly its text (unit_file)
    return p;
}

std::optional<std::vector<uint32_t>> item_of(const Line& l, const std::vector<uint8_t>& b)
{
    try
    {
        switch (l.kind)
        {
        case LineKind::Text:
        {
            if (!l.alphabet) return std::nullopt;
            const std::string t(b.begin(), b.end());
            const std::u32string u = utf8_decode(t);
            if (u.size() != l.space.unit_length() || utf8_encode(u) != t) return std::nullopt;
            std::vector<uint32_t> unit;
            unit.reserve(u.size());
            for (char32_t c : u)
            {
                const auto d = l.alphabet->digit_of(c);
                if (!d) return std::nullopt;
                unit.push_back(*d);
            }
            return unit;
        }
        case LineKind::Image:
        case LineKind::Video:
        {
            // A picture: its PNG at one pixel a pixel, frames side by side a pixel apart.
            if (file_kind(b, b.size()) != "PNG") return std::nullopt;
            const auto frames = decode_image_frames(b.data(), b.size(), "the file");
            if (frames.size() != 1) return std::nullopt;
            const RgbaImage& sheet = frames[0];
            const uint32_t W = l.image.width, H = l.image.height, F = l.image.frames;
            if (sheet.height != H || sheet.width != F * W + (F - 1)) return std::nullopt;
            std::vector<RgbaImage> each;
            for (uint32_t f = 0; f < F; ++f)
            {
                RgbaImage one;
                one.width = W;
                one.height = H;
                for (uint32_t y = 0; y < H; ++y)
                    for (uint32_t x = 0; x < W; ++x)
                    {
                        const size_t at = (size_t(y) * sheet.width + f * (W + 1) + x) * 4;
                        one.rgba.insert(one.rgba.end(), sheet.rgba.begin() + std::ptrdiff_t(at), sheet.rgba.begin() + std::ptrdiff_t(at + 4));
                    }
                each.push_back(std::move(one));
            }
            std::vector<uint32_t> unit = canonicalise_image(each, l.image);
            if (unit_file(l, unit, 1) != b) return std::nullopt;
            return unit;
        }
        case LineKind::Audio:
        {
            if (file_kind(b, b.size()) != "MID") return std::nullopt;
            const NoteSet set = note_set_of(l.space.symbols_id());
            const uint32_t L = l.space.unit_length();
            const std::string n = midi_to_notation(b, set);
            const NotesCanonResult c = set.legacy ? canonicalise_notes(n, L) : canonicalise_notes2(n, set, L / set.voices);
            if (c.units.size() != 1) return std::nullopt;
            const std::string m = notes_to_midi(set, c.units[0]);
            if (std::vector<uint8_t>(m.begin(), m.end()) != b) return std::nullopt;
            return c.units[0];
        }
        }
    }
    catch (const std::exception&)
    {
    }
    return std::nullopt;
}

BinaryItems binary_items(const Line* pages, const Line* image, const Line* video, const Line* audio, const ModelSpace* models)
{
    BinaryItems items;
    auto guard = [](auto f) {
        return [f](const std::vector<uint8_t>& b) {
            try { return f(b); } catch (const std::exception&) { return false; }
        };
    };
    if (pages && pages->alphabet)
    {
        const Line* l = pages;
        items.pages = page_pattern(*l->alphabet, l->space.unit_length());
        if (!items.pages) items.judges.emplace_back("pages", guard([l](const std::vector<uint8_t>& b) { return item_of(*l, b).has_value(); }));
    }
    if (image || video)
        items.judges.emplace_back("pictures", guard([image, video](const std::vector<uint8_t>& b) {
            return (image && item_of(*image, b)) || (video && item_of(*video, b));
        }));
    if (audio) items.judges.emplace_back("melodies", guard([audio](const std::vector<uint8_t>& b) { return item_of(*audio, b).has_value(); }));
    if (models)
        items.judges.emplace_back("models", guard([models](const std::vector<uint8_t>& b) {
            const std::string t(b.begin(), b.end());
            return models->to_obj(models->from_obj(t)) == t;
        }));
    return items;
}

BinarySieve build_binary_sieve(const BinarySpace& space, const LineFilters& given, const BinaryItems* items)
{
    LineFilters settings = given;
    for (const auto& name : given.enabled) (void)tick_filter(settings, name, true);
    const FilterLine fl = binary_filter_line(space.max_bytes());
    std::vector<FilterStack::Entry> entries;
    for (const auto& name : settings.enabled)
    {
        const FilterSpec* spec = find_filter(name);
        if (!spec) throw std::invalid_argument("unknown filter '" + name + "' (see: sieve filters --line binary)");
        if (!spec->applies(fl)) continue;
        const auto it = settings.values.find(spec->name());
        entries.push_back({spec, it == settings.values.end() ? FilterValues{} : it->second});
    }
    return BinarySieve(space, entries, items);
}

FilterLine models_filter_line(uint32_t vertices, uint32_t faces, uint32_t coords)
{
    FilterLine f;
    f.kind = "models";
    f.symbols_id = "models/V" + std::to_string(vertices) + "/F" + std::to_string(faces) + "/C" + std::to_string(coords);
    f.width = vertices; // (the shape, for reference; a model is not one base, so base and length stay 0)
    f.height = faces;
    f.frames = coords;
    return f;
}

ModelSieve build_model_sieve(const ModelSpace& space, const LineFilters& given)
{
    LineFilters settings = given;
    for (const auto& name : given.enabled) (void)tick_filter(settings, name, true);
    const FilterLine fl = models_filter_line(space.vertices(), space.face_count(), space.coords());
    std::vector<FilterStack::Entry> entries;
    for (const auto& name : settings.enabled)
    {
        const FilterSpec* spec = find_filter(name);
        if (!spec) throw std::invalid_argument("unknown filter '" + name + "' (see: sieve filters --line models)");
        if (!spec->applies(fl)) continue;
        const auto it = settings.values.find(spec->name());
        entries.push_back({spec, it == settings.values.end() ? FilterValues{} : it->second});
    }
    return ModelSieve(space, entries);
}

BookStacks build_book_stacks(const Line& cover, const Line& page, uint32_t pages, const BookFilters& settings)
{
    BookStacks b;
    b.cover = build_stack(cover, settings.parts[0]);
    b.title = build_stack(page, settings.parts[1]);
    if (pages > 0)
    {
        FilterLine body = filter_line(page);
        const uint64_t n = uint64_t(pages) * body.length;
        if (n > 0xffffffffull) throw std::invalid_argument("the books' pages are too long to filter as one text");
        body.length = uint32_t(n);
        b.pages = build_stack(body, settings.parts[2]);
    }
    return b;
}

FilterLine joined_filter_line(const FilterLine& unit, uint32_t n)
{
    FilterLine f = unit;
    const uint64_t length = uint64_t(unit.length) * n;
    if (length > 0xffffffffull) throw std::invalid_argument("the units are too long to filter joined");
    f.length = uint32_t(length);
    f.frames *= n;
    return f;
}

WorldStacks build_world_stacks(const Line& cover, const std::optional<FilterLine>& title, const ModelSpace& models, const WorldFilters& settings)
{
    WorldStacks w;
    w.cover = build_stack(cover, settings.parts[0]);
    if (title) w.title = build_stack(*title, settings.parts[1]);
    w.models = build_model_sieve(models, settings.parts[2]);
    return w;
}

CompositionStacks build_composition_stacks(const Line& cover, const std::optional<FilterLine>& title, const Line& unit, uint32_t units,
                                           const CompositionFilters& settings)
{
    CompositionStacks c;
    c.cover = build_stack(cover, settings.parts[0]);
    if (title) c.title = build_stack(*title, settings.parts[1]);
    c.units = build_stack(unit, settings.parts[2]);
    if (!settings.parts[3].enabled.empty()) c.joined = build_stack(joined_filter_line(filter_line(unit), units), settings.parts[3]);
    return c;
}

} // namespace sieve::cli
