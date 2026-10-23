#include "cli/site.hpp"

#include "cli/locate.hpp"
#include "cli/map.hpp"
#include "cli/map_sieve.hpp"
#include "cli/pack.hpp"
#include "cli/web_host.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

std::string lower(std::string s)
{
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

fs::path path_of(const std::string& rel) { return fs::path(std::u8string(rel.begin(), rel.end())); }

std::string rel_of(const fs::path& p)
{
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

std::string ext_of(const std::string& rel)
{
    const size_t slash = rel.rfind('/'), dot = rel.rfind('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
    return lower(rel.substr(dot + 1));
}

bool is_html(const std::string& rel) { return ext_of(rel) == "html" || ext_of(rel) == "htm"; }

std::string dir_of(const std::string& rel)
{
    const size_t slash = rel.rfind('/');
    return slash == std::string::npos ? "" : rel.substr(0, slash + 1);
}

std::string name_of(const std::string& rel)
{
    const size_t slash = rel.rfind('/');
    return slash == std::string::npos ? rel : rel.substr(slash + 1);
}

std::string text_of(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

void write_text(const fs::path& file, const std::string& text)
{
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out.write(text.data(), std::streamsize(text.size()));
    if (!out) throw std::runtime_error("cannot write " + rel_of(file));
}

// ---- A page's tags: just enough of HTML to find the links in it.

struct Attr
{
    std::string name;        // lower case
    size_t begin = 0, end = 0; // the whole attribute
    bool has_value = false;
    size_t value_begin = 0, value_end = 0; // the value, inside any quotes
};

struct Tag
{
    std::string name; // lower case
    size_t close = 0; // where its ">" (or "/>") is
    std::vector<Attr> attrs;
    const Attr* attr(const std::string& n) const
    {
        for (const Attr& a : attrs)
            if (a.name == n) return &a;
        return nullptr;
    }
};

std::vector<Tag> tags_of(const std::string& h)
{
    std::vector<Tag> tags;
    size_t i = 0;
    auto skip_to = [&](const std::string& end) {
        const size_t at = h.find(end, i);
        i = at == std::string::npos ? h.size() : at + end.size();
    };
    while ((i = h.find('<', i)) != std::string::npos)
    {
        if (h.compare(i, 4, "<!--") == 0)
        {
            skip_to("-->");
            continue;
        }
        if (i + 1 >= h.size() || !std::isalpha(static_cast<unsigned char>(h[i + 1])))
        {
            ++i; // an end tag, a declaration, or a "<" in text
            continue;
        }
        Tag t;
        size_t j = i + 1;
        while (j < h.size() && std::isalnum(static_cast<unsigned char>(h[j]))) ++j;
        t.name = lower(h.substr(i + 1, j - i - 1));
        while (j < h.size())
        {
            while (j < h.size() && std::isspace(static_cast<unsigned char>(h[j]))) ++j;
            if (j >= h.size()) break;
            if (h[j] == '>' || (h[j] == '/' && j + 1 < h.size() && h[j + 1] == '>'))
            {
                t.close = j;
                break;
            }
            Attr a;
            a.begin = j;
            const size_t n0 = j;
            while (j < h.size() && !std::isspace(static_cast<unsigned char>(h[j])) && h[j] != '=' && h[j] != '>' &&
                   !(h[j] == '/' && j + 1 < h.size() && h[j + 1] == '>'))
                ++j;
            if (j == n0) // a lone "/" or "=": step over it
            {
                ++j;
                continue;
            }
            a.name = lower(h.substr(n0, j - n0));
            size_t k = j;
            while (k < h.size() && std::isspace(static_cast<unsigned char>(h[k]))) ++k;
            if (k < h.size() && h[k] == '=')
            {
                ++k;
                while (k < h.size() && std::isspace(static_cast<unsigned char>(h[k]))) ++k;
                a.has_value = true;
                if (k < h.size() && (h[k] == '"' || h[k] == '\''))
                {
                    const char q = h[k];
                    const size_t e = h.find(q, k + 1);
                    a.value_begin = k + 1;
                    a.value_end = e == std::string::npos ? h.size() : e;
                    j = e == std::string::npos ? h.size() : e + 1;
                }
                else
                {
                    a.value_begin = k;
                    while (k < h.size() && !std::isspace(static_cast<unsigned char>(h[k])) && h[k] != '>') ++k;
                    a.value_end = k;
                    j = k;
                }
            }
            a.end = j;
            t.attrs.push_back(a);
        }
        if (t.close == 0) break; // a tag that never closes: the page ends here
        i = t.close + 1;
        tags.push_back(std::move(t));
        // Scripts and styles: not HTML inside, skipped whole.
        const std::string& n = tags.back().name;
        if (n == "script" || n == "style")
        {
            const size_t at = lower(h.substr(i)).find("</" + n);
            i = at == std::string::npos ? h.size() : i + at;
        }
    }
    return tags;
}

// The attribute of a tag that names a file, if it is one this reads.
const Attr* link_of(const Tag& t)
{
    if (t.name == "img" || t.name == "source") return t.attr("src");
    if (t.name == "a") return t.attr("href");
    return nullptr;
}

std::string url_decode(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i] == '%' && i + 2 < s.size() && std::isxdigit(static_cast<unsigned char>(s[i + 1])) && std::isxdigit(static_cast<unsigned char>(s[i + 2])))
        {
            out += char(std::stoi(s.substr(i + 1, 2), nullptr, 16));
            i += 2;
        }
        else out += s[i];
    return out;
}

// Where a link in a page (at `page`, a path in the site) leads, as a path in the site; nothing for a
// link with a scheme, from the server's root, to an anchor alone, or out of the site.
std::optional<std::string> target_of(const std::string& page, const std::string& value)
{
    if (value.empty() || value[0] == '#' || value[0] == '/' || value[0] == '?') return std::nullopt;
    const size_t colon = value.find(':'), slash = value.find('/');
    if (colon != std::string::npos && (slash == std::string::npos || colon < slash)) return std::nullopt; // a scheme
    const std::string path = url_decode(value.substr(0, value.find_first_of("?#")));
    std::vector<std::string> parts;
    const std::string whole = dir_of(page) + path;
    size_t at = 0;
    while (at <= whole.size())
    {
        const size_t next = std::min(whole.find('/', at), whole.size());
        const std::string part = whole.substr(at, next - at);
        if (part == "..")
        {
            if (parts.empty()) return std::nullopt;
            parts.pop_back();
        }
        else if (!part.empty() && part != ".") parts.push_back(part);
        at = next + 1;
    }
    std::string out;
    for (const std::string& p : parts) out += (out.empty() ? "" : "/") + p;
    if (out.empty()) return std::nullopt;
    return out;
}

// The stated SHA-256 in an integrity attribute ("sha256-<base64>" among any others), as written.
std::optional<std::string> stated_of(const std::string& integrity)
{
    size_t at = 0;
    while (at < integrity.size())
    {
        while (at < integrity.size() && std::isspace(static_cast<unsigned char>(integrity[at]))) ++at;
        const size_t end = std::min(integrity.find_first_of(" \t\r\n", at), integrity.size());
        const std::string token = integrity.substr(at, end - at);
        if (token.rfind("sha256-", 0) == 0) return token.substr(0, token.find('?'));
        at = end;
    }
    return std::nullopt;
}

// A file as Sieve instructions, the newest version, nothing carried as items of Sieve's lines (the
// extension shows none: they need lines and models to be made again).
void make_sieve(const fs::path& file, const fs::path& out, double seconds)
{
    Manifest m = manifest_of_file(file);
    m.contents = read_file_bytes(file);
    m.with_contents = true;
    m.minimal = m.compact = true;
    m.lines.clear();
    add_packed(m, file.parent_path(), nullptr, nullptr, false, nullptr, seconds, 5, true, true, false, true, true);
    fs::create_directories(out.parent_path());
    write_address_file(out, binary_address(m.file()), false);
}

std::vector<std::string> files_of(const fs::path& root)
{
    std::vector<std::string> out;
    for (const auto& e : fs::recursive_directory_iterator(root))
        if (e.is_regular_file()) out.push_back(rel_of(fs::relative(e.path(), root)));
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace

std::string integrity_of(const std::vector<uint8_t>& bytes)
{
    const std::string hex = sha256_hex(bytes);
    std::vector<uint8_t> raw(32);
    for (size_t k = 0; k < 32; ++k) raw[k] = uint8_t(std::stoi(hex.substr(2 * k, 2), nullptr, 16));
    return "sha256-" + base64_of(raw.data(), raw.size());
}

SiteReport build_site(const fs::path& src_in, const fs::path& out_in, const SiteOptions& o)
{
    const fs::path src = fs::weakly_canonical(src_in), out = fs::weakly_canonical(out_in);
    if (!fs::is_directory(src)) throw std::invalid_argument(rel_of(src) + " is not a folder");
    const std::string s = rel_of(src) + "/", t = rel_of(out) + "/";
    if (t.rfind(s, 0) == 0 || s.rfind(t, 0) == 0) throw std::invalid_argument("the site and where it is built must not be inside one another");
    SiteReport rep;
    const std::vector<std::string> files = files_of(src);
    const std::set<std::string> present(files.begin(), files.end());
    std::set<std::string> plain(o.plain.begin(), o.plain.end());

    // What becomes a .sieve, and its name: NAME.sieve, or the whole name and .sieve where that is
    // taken (by a file of the site, or by two of them in one folder).
    std::map<std::string, std::string> made; // its path in the site -> its .sieve's
    std::vector<std::string> sieve_pages;
    {
        std::map<std::string, std::vector<std::string>> wanted; // a .sieve's path -> the files that want it
        for (const std::string& f : files)
        {
            const std::string e = ext_of(f);
            const bool page = is_html(f) && o.pages && name_of(f) != "index.html" && !plain.count(f);
            const bool asset = !is_html(f) && std::find(o.sieve_kinds.begin(), o.sieve_kinds.end(), e) != o.sieve_kinds.end();
            if (!page && !asset) continue;
            wanted[f.substr(0, f.size() - e.size() - 1) + ".sieve"].push_back(f);
            if (page) sieve_pages.push_back(f);
        }
        for (const auto& [name, who] : wanted)
            for (const std::string& f : who) made[f] = who.size() == 1 && !present.count(name) ? name : f + ".sieve";
        for (const auto& [f, m] : made)
            if (present.count(m)) throw std::runtime_error(f + " would be made " + m + ", which the site has already");
    }

    // Replaced only if empty, or a site this built (its site.sieve there): never a folder named by mistake.
    if (fs::exists(out) && (!fs::is_directory(out) || (!fs::is_empty(out) && !fs::exists(out / "site.sieve"))))
        throw std::invalid_argument(rel_of(out) + " is there and is not a site sieve site build made (no site.sieve): remove it, or name another");
    fs::remove_all(out);
    fs::create_directories(out);
    // Everything else as it is (the pages after); the pictures and the like made.
    for (const std::string& f : files)
    {
        rep.files += 1;
        if (is_html(f)) continue;
        if (const auto it = made.find(f); it != made.end())
        {
            make_sieve(src / path_of(f), out / path_of(it->second), o.search_per_file);
            rep.made += 1;
        }
        else
        {
            fs::create_directories((out / path_of(f)).parent_path());
            fs::copy_file(src / path_of(f), out / path_of(f));
        }
    }

    std::map<std::string, std::string> integrity; // a .sieve made or kept -> what links to it state
    auto stated = [&](const std::string& sieve_path) {
        auto it = integrity.find(sieve_path);
        if (it == integrity.end()) it = integrity.emplace(sieve_path, integrity_of(read_file_bytes(out / path_of(sieve_path)))).first;
        return it->second;
    };
    // A page with each link to a .sieve (made, or the site's own) given that file's hash.
    auto rewrite = [&](const std::string& page, const std::string& html) {
        struct Edit
        {
            size_t at, len;
            std::string with;
        };
        std::vector<Edit> edits;
        for (const Tag& tag : tags_of(html))
        {
            const Attr* link = link_of(tag);
            if (!link || !link->has_value || tag.attr("data-sieve-keep")) continue;
            const std::string value = html.substr(link->value_begin, link->value_end - link->value_begin);
            const auto target = target_of(page, value);
            if (!target) continue;
            std::string sieve_path;
            if (const auto it = made.find(*target); it != made.end())
            {
                sieve_path = it->second;
                // The value as written, its last part's name changed (any query or anchor kept).
                const size_t end = value.find_first_of("?#") == std::string::npos ? value.size() : value.find_first_of("?#");
                const std::string ext = ext_of(*target);
                std::string seg = value.substr(0, end);
                const bool whole = name_of(sieve_path) == name_of(*target) + ".sieve";
                seg = whole ? seg + ".sieve" : seg.substr(0, seg.size() - ext.size() - 1) + ".sieve";
                edits.push_back({link->value_begin, end, seg});
            }
            else if (ext_of(*target) == "sieve" && present.count(*target)) sieve_path = *target;
            else continue;
            const std::string with = stated(sieve_path);
            if (const Attr* old = tag.attr("integrity"); old && old->has_value)
                edits.push_back({old->value_begin, old->value_end - old->value_begin, with});
            else edits.push_back({tag.close, 0, " integrity=\"" + with + "\""});
            rep.links += 1;
        }
        std::sort(edits.begin(), edits.end(), [](const Edit& a, const Edit& b) { return a.at > b.at; });
        std::string h = html;
        for (const Edit& e : edits) h.replace(e.at, e.len, e.with);
        return h;
    };

    // The pages made into .sieve files, each after those it names, so the hash it states is final.
    std::map<std::string, std::string> html;
    for (const std::string& f : files)
        if (is_html(f)) html[f] = text_of(src / path_of(f));
    std::map<std::string, std::set<std::string>> needs;
    for (const std::string& p : sieve_pages)
        for (const Tag& tag : tags_of(html[p]))
            if (const Attr* link = link_of(tag); link && link->has_value && !tag.attr("data-sieve-keep"))
                if (const auto target = target_of(p, html[p].substr(link->value_begin, link->value_end - link->value_begin));
                    target && *target != p && made.count(*target) && is_html(*target))
                    needs[p].insert(*target);
    std::set<std::string> done;
    std::random_device rd;
    const fs::path tmp = fs::temp_directory_path() / ("sieve-site-" + std::to_string(rd()) + std::to_string(rd()));
    struct Cleanup
    {
        fs::path p;
        ~Cleanup()
        {
            std::error_code ec;
            fs::remove_all(p, ec);
        }
    } cleanup{tmp};
    while (done.size() < sieve_pages.size())
    {
        bool any = false;
        for (const std::string& p : sieve_pages)
        {
            if (done.count(p)) continue;
            if (!std::all_of(needs[p].begin(), needs[p].end(), [&](const std::string& n) { return done.count(n) != 0; })) continue;
            const fs::path page = tmp / path_of(std::to_string(done.size())) / path_of(name_of(p)); // its own name, for its .sieve
            write_text(page, rewrite(p, html[p]));
            make_sieve(page, out / path_of(made[p]), o.search_per_file);
            done.insert(p);
            rep.made += 1;
            rep.pages_made += 1;
            any = true;
        }
        if (!any)
        {
            std::string left;
            for (const std::string& p : sieve_pages)
                if (!done.count(p)) left += (left.empty() ? "" : ", ") + p;
            throw std::runtime_error("these pages name each other, so none of them can state the others' hashes (changing one would change the "
                                     "others): " + left + ". Keep one of them plain (--plain), so the others name it by name only.");
        }
    }
    // The plain pages, last: they may name any of them.
    for (const auto& [f, text] : html)
        if (!made.count(f)) write_text(out / path_of(f), rewrite(f, text));

    // The site's map: every file, named by its SHA-256 (sieve-manifest-v18); its own hash is the root.
    Map map = map_of_folder(out, name_of(rel_of(src)));
    map.name = name_of(rel_of(src));
    write_map_instructions(map, out / "site.sieve", 0);
    rep.map_sha256 = sha256_hex(read_file_bytes(out / "site.sieve"));
    return rep;
}

SiteCheck check_site(const fs::path& dir)
{
    SiteCheck c;
    const std::vector<std::string> files = files_of(dir);
    const std::set<std::string> present(files.begin(), files.end());
    std::map<std::string, std::string> own; // a file's integrity, as links state it
    auto integrity = [&](const std::string& f) {
        auto it = own.find(f);
        if (it == own.end()) it = own.emplace(f, integrity_of(read_file_bytes(dir / path_of(f)))).first;
        return it->second;
    };
    // Every hash a page states, against the file it names.
    auto check_page = [&](const std::string& page, const std::string& shown_as, const std::string& h) {
        for (const Tag& tag : tags_of(h))
        {
            const Attr* link = link_of(tag);
            if (!link || !link->has_value) continue;
            const auto target = target_of(page, h.substr(link->value_begin, link->value_end - link->value_begin));
            if (!target) continue;
            if (tag.attr("data-sieve-keep"))
            {
                c.kept += 1; // left as written, on purpose
                continue;
            }
            const Attr* given = tag.attr("integrity");
            const bool sieve = ext_of(*target) == "sieve";
            if (!given || !given->has_value)
            {
                if (sieve) c.mismatches.push_back(shown_as + ": links to " + *target + " with no hash");
                continue;
            }
            c.checked += 1;
            const auto stated = stated_of(h.substr(given->value_begin, given->value_end - given->value_begin));
            if (!present.count(*target)) c.mismatches.push_back(shown_as + ": names " + *target + ", which is not there");
            else if (!stated) c.mismatches.push_back(shown_as + ": states no SHA-256 for " + *target);
            else if (*stated != integrity(*target)) c.mismatches.push_back(shown_as + ": the hash it states for " + *target + " is not that file's");
        }
    };
    for (const std::string& f : files)
    {
        if (is_html(f)) check_page(f, f, text_of(dir / path_of(f)));
        else if (ext_of(f) == "sieve" && f != "site.sieve")
        {
            // A page held in a .sieve: read as the extension reads it (one file, nothing but
            // decoding), and its links checked from where the .sieve is.
            try
            {
                const WebDecoded d = web_decode(read_file_bytes(dir / path_of(f)), integrity(f));
                if (d.type == "text/html") check_page(dir_of(f) + d.name, f + " (" + d.name + ")", std::string(d.bytes.begin(), d.bytes.end()));
            }
            catch (const std::exception&)
            {
                // not a page the extension would show (many files, say): no links to check
            }
        }
    }
    // The site's map: every file it names is here and is the same, and every file is in it.
    if (present.count("site.sieve"))
    {
        const Map map = read_map_file(dir / "site.sieve");
        std::set<std::string> named;
        for (const MapNode& n : map.nodes)
        {
            if (n.folder() || n.kind == MapNode::Held) continue;
            named.insert(n.path);
            c.checked += 1;
            if (!present.count(n.path)) c.mismatches.push_back("site.sieve: names " + n.path + ", which is not there");
            else if (sha256_hex(read_file_bytes(dir / path_of(n.path))) != n.sha256) c.mismatches.push_back("site.sieve: " + n.path + " is not the file it names");
        }
        for (const std::string& f : files)
            if (f != "site.sieve" && !named.count(f)) c.mismatches.push_back("site.sieve: does not name " + f);
    }
    else c.mismatches.push_back("no site.sieve (the site's map)");
    return c;
}

} // namespace sieve::cli
