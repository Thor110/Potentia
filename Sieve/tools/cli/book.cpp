#include "book.hpp"

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
    for (const auto& f : shape_fields(line.kind))
    {
        if (f == "alphabet") a.opts[f] = line.alphabet->id();
        else if (f == "length") a.opts[f] = std::to_string(line.space.unit_length());
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
    o << kBookFormat << "\n";
    for (const BookSection& s : b.sections)
    {
        const LineKind kind = line_from_string(s.shape.get("line"));
        o << "section " << s.role << "\n"
          << "line " << to_string(kind) << "\n";
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
    if (next() != kBookFormat) throw std::invalid_argument(std::string("not a ") + kBookFormat + " record");
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
        for (const auto& f : shape_fields(kind)) s.shape.opts[f] = field(f);
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
