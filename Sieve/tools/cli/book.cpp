#include "book.hpp"

#include "sieve/audio.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace {

// The shape fields of each line, in the order the record writes them.
std::vector<std::string> shape_fields(LineKind k)
{
    switch (k)
    {
    case LineKind::Text: return {"alphabet", "length", "canon"};
    case LineKind::Image: return {"width", "height", "palette"};
    case LineKind::Video: return {"width", "height", "frames", "palette"};
    case LineKind::Audio: return {"length"};
    }
    return {};
}

bool valid_role(const std::string& r)
{
    if (r.empty() || !(r[0] >= 'a' && r[0] <= 'z')) return false;
    for (char c : r)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    return true;
}

// A model named by its registry id (never a file path, which would not mean the same on
// another machine).
bool registry_model_id(const std::string& id)
{
    if (id.empty() || id == "file" || (id.size() > 6 && id.substr(id.size() - 6) == ".model")) return false;
    for (char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return false;
    return true;
}

bool valid_hex(const std::string& h)
{
    if (h.empty()) return false;
    for (char c : h)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// An audio section's note set (its "notes" field, v2) as the line options that make it.
void set_options_of(const std::string& id, Args& a)
{
    if (is_pcm_symbols(id))
    {
        const PcmFormat f = pcm_format_of(id);
        a.opts["note-set"] = "pcm";
        a.opts["rate"] = std::to_string(f.rate);
        a.opts["bits"] = std::to_string(f.bits);
        a.opts["channels"] = std::to_string(f.channels);
    }
    else if (is_notes3_symbols(id))
    {
        const Notes3Set n = notes3_set_of(id);
        a.opts["note-set"] = "notes3";
        a.opts["low"] = notes3_name(n.low);
        a.opts["high"] = notes3_name(n.high);
        a.opts["tpq"] = std::to_string(n.tpq);
        a.opts["longest"] = std::to_string(n.longest);
        a.opts["levels"] = std::to_string(n.levels);
        a.opts["voices"] = std::to_string(n.voices);
        a.opts["tempo"] = std::to_string(n.tempo);
        std::string ins;
        for (uint32_t i : n.instruments) ins += (ins.empty() ? "" : ",") + std::to_string(i);
        a.opts["instruments"] = ins;
    }
    else if (is_note_symbols(id) && id != kNotesSymbolsId)
    {
        const NoteSet n = note_set_of(id);
        a.opts["note-set"] = "notes2";
        a.opts["low"] = note_name(n.low);
        a.opts["high"] = note_name(n.high);
        a.opts["durations"] = n.durations;
        a.opts["voices"] = std::to_string(n.voices);
    }
    else throw std::invalid_argument("'" + id + "' is not a note set an audio section can name");
}

// The record spells the text canonicalisation in full; the line options take "v1"/"v2".
std::string canon_option(const std::string& v)
{
    const std::string prefix = "canon-text-";
    return v.rfind(prefix, 0) == 0 ? v.substr(prefix.size()) : v;
}

} // namespace

Line section_line(const BookSection& s)
{
    Args a = s.shape;
    if (a.opts.count("canon")) a.opts["canon"] = canon_option(a.opts["canon"]);
    if (const auto it = s.shape.opts.find("notes"); it != s.shape.opts.end())
    {
        set_options_of(it->second, a);
        a.opts.erase("notes");
        a.opts["model"] = "none";
        Line line = make_line(a);
        if (line.space.symbols_id() != it->second)
            throw std::invalid_argument("section '" + s.role + "': the notes " + it->second + " do not make that set");
        return line;
    }
    if (s.mode == "guided")
    {
        a.opts["model"] = s.model_id;
        Line line = make_line(a);
        if (!line.guided) throw std::invalid_argument("section '" + s.role + "': model " + s.model_id + " is not available");
        if (!s.model_sha256.empty() && line.guided->model().sha256() != s.model_sha256)
            throw std::invalid_argument("section '" + s.role + "': model " + s.model_id + " does not match the one the book was written with");
        return line;
    }
    a.opts["model"] = "none";
    return make_line(a);
}

BookSection make_section(const std::string& role, const Args& shape, const std::string& mode,
                         const std::vector<std::vector<uint32_t>>& units)
{
    if (!valid_role(role)) throw std::invalid_argument("a section label is lowercase letters, digits and '-': '" + role + "'");
    if (mode != "positional" && mode != "scrambled" && mode != "guided")
        throw std::invalid_argument("mode must be positional, scrambled or guided");
    BookSection s;
    s.role = role;
    s.mode = mode;
    // Keep only the fields the record writes, in canonical spelling.
    Args a;
    a.opts["line"] = to_string(line_from_string(shape.get("line", "text")));
    a.opts["key"] = shape.get("key", "sieve");
    if (a.opts["key"].empty() || a.opts["key"].find_first_of("\r\n") != std::string::npos)
        throw std::invalid_argument("a book's key must be one line of text");
    Args full = shape;
    if (mode != "guided") full.opts["model"] = "none";
    const Line line = make_line(full);
    // An audio section on any set but notes104 names it (the notes field, sieve-book-v2), and
    // its length is then per voice or channel, as the line's own --length is.
    const bool named = line.kind == LineKind::Audio && line.space.symbols_id() != kNotesSymbolsId;
    if (named) a.opts["notes"] = line.space.symbols_id();
    for (const auto& f : shape_fields(line.kind))
    {
        if (f == "alphabet") a.opts[f] = line.alphabet->id();
        else if (f == "length") a.opts[f] = std::to_string(line.space.unit_length() / (named ? strands_of(line) : 1));
        else if (f == "canon") a.opts[f] = to_string(line.canon);
        else if (f == "width") a.opts[f] = std::to_string(line.image.width);
        else if (f == "height") a.opts[f] = std::to_string(line.image.height);
        else if (f == "frames") a.opts[f] = std::to_string(line.image.frames);
        else if (f == "palette") a.opts[f] = line.image.palette->id();
    }
    s.shape = a;
    if (mode == "guided")
    {
        if (!line.guided) throw std::invalid_argument("guided order needs a text line with a model (see: sieve models)");
        if (!registry_model_id(line.model_id))
            throw std::invalid_argument("a book's guided pages need a registered model (see: sieve models), not a model file");
        s.model_id = line.model_id;
        s.model_sha256 = line.guided->model().sha256();
        for (const auto& u : units) s.addresses.push_back(line.guided->code(u).hex);
    }
    else
        for (const auto& u : units) s.addresses.push_back(line.space.address_of(u, address_mode_from_string(mode)));
    return s;
}

std::vector<DecodedSection> decode_book(const Book& b)
{
    std::vector<DecodedSection> out;
    for (const BookSection& s : b.sections)
    {
        DecodedSection d{&s, section_line(s), {}};
        for (const std::string& addr : s.addresses)
        {
            if (s.mode == "guided")
            {
                const GuidedLine& g = *d.line.guided;
                auto u = g.unit_at(g.point_of(addr));
                if (g.code(u).hex != addr)
                    throw std::invalid_argument("section '" + s.role + "': " + addr + " is a point inside a unit's arc, not the unit's own address");
                d.units.push_back(std::move(u));
            }
            else
            {
                if (addr.size() != d.line.space.hex_width())
                    throw std::invalid_argument("section '" + s.role + "': addresses on this line are " + std::to_string(d.line.space.hex_width()) +
                                                " hex digits, not " + std::to_string(addr.size()));
                d.units.push_back(d.line.space.unit_at(addr, address_mode_from_string(s.mode)));
            }
        }
        out.push_back(std::move(d));
    }
    return out;
}

std::string book_id(const std::vector<DecodedSection>& sections)
{
    std::ostringstream o;
    o << kBookIdVersion << "\n";
    for (const auto& d : sections)
    {
        o << "section " << d.section->role << "\n"
          << "shape " << to_string(d.line.kind) << "/" << d.line.space.symbols_id() << "/L" << d.line.space.unit_length() << "\n"
          << "units " << d.units.size() << "\n";
        for (const auto& u : d.units) o << d.line.space.address_of(u, AddressMode::Positional) << "\n";
    }
    return Sha256::hex(Sha256::hash(o.str()));
}

Args line_shape(const Line& line)
{
    Args a;
    a.opts["line"] = to_string(line.kind);
    a.opts["key"] = line.space.key();
    a.opts["model"] = "none";
    switch (line.kind)
    {
    case LineKind::Text:
        a.opts["alphabet"] = line.alphabet->id();
        a.opts["length"] = std::to_string(line.space.unit_length());
        a.opts["canon"] = to_string(line.canon);
        break;
    case LineKind::Image:
    case LineKind::Video:
        a.opts["width"] = std::to_string(line.image.width);
        a.opts["height"] = std::to_string(line.image.height);
        if (line.kind == LineKind::Video) a.opts["frames"] = std::to_string(line.image.frames);
        a.opts["palette"] = line.image.palette->id();
        break;
    case LineKind::Audio:
        if (line.space.symbols_id() != kNotesSymbolsId) set_options_of(line.space.symbols_id(), a);
        a.opts["length"] = std::to_string(line.space.unit_length() / strands_of(line));
        break;
    }
    return a;
}

Book composition_record(const Line& cover, const Line* title, const Line& unit, const CompositionSpace::Parts& p, const std::string& mode)
{
    if (mode != "positional" && mode != "scrambled") throw std::invalid_argument("a track's or movie's record is positional or scrambled");
    auto blank = [](const Space::Digits& d) { return std::all_of(d.begin(), d.end(), [](uint32_t x) { return x == 0; }); };
    Book b;
    if (!blank(p.cover)) b.sections.push_back(make_section("cover", line_shape(cover), mode, {p.cover}));
    if (title && !blank(p.title)) b.sections.push_back(make_section("title", line_shape(*title), mode, {p.title}));
    b.sections.push_back(make_section("units", line_shape(unit), mode, p.units));
    b.id = book_id(decode_book(b));
    if (book_id(decode_book(parse_book(serialise_book(b)))) != b.id) throw std::runtime_error("the record does not read back as the same item");
    return b;
}

CompositionSpace::Parts record_composition(const std::vector<DecodedSection>& sections, const CompositionSpace& space)
{
    const Space& cover = space.cover_space();
    const std::optional<Space>& title = space.title_space();
    const Space& unit = space.unit_space();
    CompositionSpace::Parts p;
    p.cover.assign(cover.unit_length(), 0);
    if (title) p.title.assign(title->unit_length(), 0);
    for (const auto& d : sections)
    {
        const std::string& role = d.section->role;
        const Space& sp = d.line.space;
        auto shape = [](const Space& x) { return x.symbols_id() + " (" + std::to_string(x.unit_length()) + " symbols a unit)"; };
        if ((role == "cover" || role == "title") && d.units.size() != 1)
            throw std::invalid_argument("its " + role + " must be exactly one unit (it has " + std::to_string(d.units.size()) + ")");
        if (role == "cover")
        {
            if (sp.symbols_id() != cover.symbols_id()) throw std::invalid_argument("its cover is " + sp.symbols_id() + ", not " + cover.symbols_id());
            p.cover = d.units.front();
        }
        else if (role == "title")
        {
            if (!title) throw std::invalid_argument("it has a title, and this line's titles are off");
            if (sp.symbols_id() != title->symbols_id() || sp.unit_length() != title->unit_length())
                throw std::invalid_argument("its title is " + shape(sp) + ", not " + shape(*title));
            p.title = d.units.front();
        }
        else if (role == "units")
        {
            if (sp.symbols_id() != unit.symbols_id() || sp.unit_length() != unit.unit_length())
                throw std::invalid_argument("its units are " + shape(sp) + ", not " + shape(unit));
            p.units = d.units;
        }
    }
    if (p.units.size() > space.units())
        throw std::invalid_argument("it has " + std::to_string(p.units.size()) + " units, more than the line's " + std::to_string(space.units()));
    p.units.resize(space.units(), Space::Digits(unit.unit_length(), 0));
    return p;
}

std::string record_line(const std::vector<DecodedSection>& sections)
{
    for (const auto& d : sections)
    {
        if (d.section->role == "pages") return "books";
        if (d.section->role == "units" && d.line.kind == LineKind::Audio) return "tracks";
        if (d.section->role == "units" && d.line.kind == LineKind::Video) return "movies";
    }
    return "";
}

BookSpace::Parts record_parts(const std::vector<DecodedSection>& sections, const BookSpace& space)
{
    const Space& page = space.page_space();
    const Space& cover = space.cover_space();
    BookSpace::Parts p;
    p.cover.assign(cover.unit_length(), 0);
    p.title.assign(page.unit_length(), 0);
    for (const auto& d : sections)
    {
        const std::string& role = d.section->role;
        const Space& sp = d.line.space;
        if ((role == "cover" || role == "title") && d.units.size() != 1)
            throw std::invalid_argument("its " + role + " must be exactly one unit (it has " + std::to_string(d.units.size()) + ")");
        if (role == "cover")
        {
            if (sp.symbols_id() != cover.symbols_id())
                throw std::invalid_argument("its cover is " + sp.symbols_id() + ", not " + cover.symbols_id());
            p.cover = d.units.front();
        }
        else if (role == "title" && sp.symbols_id() == page.symbols_id() && sp.unit_length() < page.unit_length())
        {
            // A title bound shorter than a page (bind --title-length) is the same title followed
            // by blank space (padding is digit 0 in every alphabet).
            p.title = d.units.front();
            p.title.resize(page.unit_length(), 0);
        }
        else if (role == "title" || role == "pages")
        {
            if (sp.symbols_id() != page.symbols_id() || sp.unit_length() != page.unit_length())
                throw std::invalid_argument("its " + role + (role == "title" ? " is on " : " are ") + sp.symbols_id() + " pages of " +
                                            std::to_string(sp.unit_length()) + " characters, not " + page.symbols_id() + " pages of " +
                                            std::to_string(page.unit_length()));
            if (role == "title") p.title = d.units.front();
            else p.pages = d.units;
        }
    }
    if (p.pages.size() > space.pages())
        throw std::invalid_argument("it has " + std::to_string(p.pages.size()) + " pages, more than the line's " + std::to_string(space.pages()));
    p.pages.resize(space.pages(), Space::Digits(page.unit_length(), 0));
    return p;
}

std::string serialise_book(const Book& b)
{
    std::ostringstream o;
    // v1 unless a section names its note set (v2): a record that needs nothing new reads anywhere.
    const bool v2 = std::any_of(b.sections.begin(), b.sections.end(), [](const BookSection& s) { return s.shape.has("notes"); });
    o << (v2 ? kBookFormat2 : kBookFormat) << "\n";
    for (const BookSection& s : b.sections)
    {
        const LineKind kind = line_from_string(s.shape.get("line"));
        o << "section " << s.role << "\n"
          << "line " << to_string(kind) << "\n";
        if (s.shape.has("notes")) o << "notes " << s.shape.get("notes") << "\n";
        for (const auto& f : shape_fields(kind)) o << f << " " << s.shape.get(f) << "\n";
        o << "key " << s.shape.get("key") << "\n"
          << "mode " << s.mode << "\n";
        if (s.mode == "guided") o << "model " << s.model_id << " " << s.model_sha256 << "\n";
        o << "units " << s.addresses.size() << "\n";
        for (const auto& a : s.addresses) o << a << "\n";
    }
    o << "end\n"
      << "id " << b.id << "\n";
    return o.str();
}

Book parse_book(std::string_view text)
{
    std::istringstream in{std::string(text)};
    std::string line;
    int line_no = 0;
    auto next = [&]() {
        if (!std::getline(in, line)) throw std::invalid_argument("the book ends early (line " + std::to_string(line_no + 1) + ")");
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return line;
    };
    auto field = [&](const std::string& key) {
        const std::string l = next();
        if (l.rfind(key + " ", 0) != 0) throw std::invalid_argument("book line " + std::to_string(line_no) + ": expected '" + key + " ...'");
        return l.substr(key.size() + 1);
    };
    const std::string format = next();
    if (format != kBookFormat && format != kBookFormat2) throw std::invalid_argument(std::string("not a ") + kBookFormat + " or v2 record");
    const bool v2 = format == kBookFormat2;
    Book b;
    for (;;)
    {
        const std::string l = next();
        if (l == "end") break;
        if (l.rfind("section ", 0) != 0) throw std::invalid_argument("book line " + std::to_string(line_no) + ": expected 'section LABEL' or 'end'");
        BookSection s;
        s.role = l.substr(8);
        if (!valid_role(s.role)) throw std::invalid_argument("book line " + std::to_string(line_no) + ": bad section label");
        const LineKind kind = line_from_string(field("line"));
        s.shape.opts["line"] = to_string(kind);
        const std::vector<std::string> fields = shape_fields(kind);
        for (size_t i = 0; i < fields.size(); ++i)
        {
            // v2: an audio section may name its note set first.
            if (i == 0 && v2 && kind == LineKind::Audio)
            {
                const std::string l2 = next();
                if (l2.rfind("notes ", 0) == 0)
                {
                    s.shape.opts["notes"] = l2.substr(6);
                    s.shape.opts[fields[i]] = field(fields[i]);
                    continue;
                }
                if (l2.rfind(fields[i] + " ", 0) != 0)
                    throw std::invalid_argument("book line " + std::to_string(line_no) + ": expected 'notes ...' or '" + fields[i] + " ...'");
                s.shape.opts[fields[i]] = l2.substr(fields[i].size() + 1);
                continue;
            }
            s.shape.opts[fields[i]] = field(fields[i]);
        }
        s.shape.opts["key"] = field("key");
        s.mode = field("mode");
        if (s.mode != "positional" && s.mode != "scrambled" && s.mode != "guided")
            throw std::invalid_argument("book line " + std::to_string(line_no) + ": mode must be positional, scrambled or guided");
        if (s.mode == "guided")
        {
            const std::string m = field("model");
            const size_t sp = m.find(' ');
            if (sp == std::string::npos) throw std::invalid_argument("book line " + std::to_string(line_no) + ": expected 'model ID SHA256'");
            s.model_id = m.substr(0, sp);
            s.model_sha256 = m.substr(sp + 1);
            if (s.model_sha256.size() != 64 || !valid_hex(s.model_sha256) || !registry_model_id(s.model_id))
                throw std::invalid_argument("book line " + std::to_string(line_no) + ": expected 'model ID SHA256' with a registered model id and its 64-digit hash");
        }
        const unsigned long long count = parse_whole(field("units"), "book line " + std::to_string(line_no) + ": units");
        for (unsigned long long k = 0; k < count; ++k)
        {
            const std::string a = next();
            if (!valid_hex(a)) throw std::invalid_argument("book line " + std::to_string(line_no) + ": not a lowercase hex address");
            s.addresses.push_back(a);
        }
        b.sections.push_back(std::move(s));
    }
    b.id = field("id");
    if (b.id.size() != 64 || !valid_hex(b.id)) throw std::invalid_argument("book line " + std::to_string(line_no) + ": the id must be 64 hex digits");
    std::string rest;
    while (std::getline(in, rest))
        if (!rest.empty() && rest != "\r") throw std::invalid_argument("text after the book's id line");
    // Strict: only the canonical spelling of every field is accepted (no "+0400", no double
    // spaces, no other names for a line), so one book has exactly one record in each ordering.
    std::string normal(text);
    normal.erase(std::remove(normal.begin(), normal.end(), '\r'), normal.end());
    while (normal.size() > 1 && normal.back() == '\n' && normal[normal.size() - 2] == '\n') normal.pop_back();
    if (serialise_book(b) != normal) throw std::invalid_argument("the book record is not in canonical form (see: sieve help bind)");
    return b;
}

} // namespace sieve::cli
