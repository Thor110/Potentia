#include "cli/site.hpp"

#include "cli/locate.hpp"
#include "cli/map.hpp"
#include "cli/map_sieve.hpp"
#include "cli/items.hpp"
#include "cli/pack.hpp"
#include "cli/web_host.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string_view>

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
// A page read for its links: HTML, and PHP (always kept as it is: its server runs it).
bool is_page(const std::string& rel) { return is_html(rel) || ext_of(rel) == "php"; }

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
    out.close(); // (checked once closed: a small file is written as it closes)
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

// `raw`, if given, gets what is not HTML: the insides of scripts, styles and PHP's <?...?>.
std::vector<Tag> tags_of(const std::string& h, std::vector<std::string_view>* raw = nullptr)
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
        if (h.compare(i, 2, "<?") == 0) // PHP (or an XML declaration): code, never rewritten
        {
            const size_t from = i;
            skip_to("?>");
            if (raw) raw->push_back(std::string_view(h).substr(from, i - from));
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
            // ("</script" in any case, found where it stands: copying the rest of the page for each
            // was quadratic, 35 s for a 4 MB page of 4,000 scripts.)
            const std::string want = "</" + n;
            const auto it = std::search(h.begin() + std::ptrdiff_t(i), h.end(), want.begin(), want.end(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == static_cast<unsigned char>(b);
            });
            const size_t end = size_t(it - h.begin());
            if (raw) raw->push_back(std::string_view(h).substr(i, end - i));
            i = end;
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

// A script or stylesheet a page loads (`<script src>`, `<link rel=stylesheet|modulepreload|preload
// href>`): given its file's `integrity`, which browsers enforce on these themselves, so a script or
// stylesheet swapped on the server is not run (Edward, 25 October 2026: "unless the pages that load
// them also point to their hashes").
const Attr* resource_of(const Tag& t, const std::string& h)
{
    if (t.name == "script") return t.attr("src");
    if (t.name != "link") return nullptr;
    const Attr* rel = t.attr("rel");
    if (!rel || !rel->has_value) return nullptr;
    const std::string r = " " + lower(h.substr(rel->value_begin, rel->value_end - rel->value_begin)) + " ";
    if (r.find(" stylesheet ") == std::string::npos && r.find(" modulepreload ") == std::string::npos && r.find(" preload ") == std::string::npos)
        return nullptr;
    return t.attr("href");
}

// The site's table: every file named by its SHA-256 (a map: the newest instructions), at the site's top as
// index.sieve, so the extension and a crawler know where to look (25 October 2026; site.sieve
// before, which check still reads).
constexpr const char* kTable = "index.sieve";
constexpr const char* kOldTable = "site.sieve";

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

// Where the site is served from ("/", or "/Potentia/"), with a "/" at each end; empty: not given.
std::string served_path(std::string p)
{
    if (p.empty()) return p;
    if (p.front() != '/') p = "/" + p;
    if (p.back() != '/') p += "/";
    return p;
}

// Where a link in a page (at `page`, a path in the site) leads, as a path in the site; nothing for a
// link with a scheme, to an anchor alone, or out of the site. A link from the server's root ("/a.png")
// is the site's when the site is served from a path it starts with (`served`, as served_path gives
// it: "/" for a site at the server's root); else, or with none given, it is left alone.
std::optional<std::string> target_of(const std::string& page, const std::string& value, const std::string& served)
{
    if (value.empty() || value[0] == '#' || value[0] == '?') return std::nullopt;
    if (value.rfind("//", 0) == 0) return std::nullopt; // another server ("//host/a.png")
    const bool rooted = value[0] == '/';
    if (rooted && (served.empty() || value.rfind(served, 0) != 0)) return std::nullopt;
    const size_t colon = value.find(':'), slash = value.find('/');
    if (!rooted && colon != std::string::npos && (slash == std::string::npos || colon < slash)) return std::nullopt; // a scheme
    const std::string path = url_decode(value.substr(rooted ? served.size() : 0, value.find_first_of("?#") - (rooted ? served.size() : 0)));
    std::vector<std::string> parts;
    const std::string whole = rooted ? path : dir_of(page) + path;
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

// A file as Sieve instructions, the newest version (add_packed_newest), nothing carried as items of Sieve's lines (the
// extension shows none: they need lines and models to be made again).
void make_sieve(const fs::path& file, const fs::path& out, double seconds)
{
    Manifest m = manifest_of_file(file);
    m.contents = read_file_bytes(file);
    m.with_contents = true;
    m.minimal = m.compact = true;
    m.lines.clear();
    add_packed_newest(m, file.parent_path(), nullptr, seconds, false, false);
    fs::create_directories(out.parent_path());
    write_raw_address_file(out, raw_of_file(m.file()), false);
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

namespace {

// Whether `text` names the file called `name` (its last part), as a whole name: not inside a longer
// one ("a.png" is not named by "data.png").
bool names(std::string_view text, const std::string& name)
{
    auto part = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.'; };
    for (size_t at = text.find(name); at != std::string_view::npos; at = text.find(name, at + 1))
    {
        const bool before = at == 0 || !part(text[at - 1]);
        const bool after = at + name.size() == text.size() || !part(text[at + name.size()]);
        if (before && after) return true;
    }
    return false;
}

// A loop among the pages, each needing the next: the pages in it, the first again at the end.
std::vector<std::string> loop_of(const std::vector<std::string>& pages, const std::map<std::string, std::set<std::string>>& needs)
{
    std::map<std::string, int> state; // 0 not seen, 1 on the path, 2 done
    std::vector<std::string> path, found;
    std::function<bool(const std::string&)> walk = [&](const std::string& p) {
        state[p] = 1;
        path.push_back(p);
        if (const auto it = needs.find(p); it != needs.end())
            for (const std::string& n : it->second)
            {
                if (state[n] == 1)
                {
                    found.assign(std::find(path.begin(), path.end(), n), path.end());
                    found.push_back(n);
                    return true;
                }
                if (state[n] == 0 && walk(n)) return true;
            }
        path.pop_back();
        state[p] = 2;
        return false;
    };
    for (const std::string& p : pages)
        if (state[p] == 0 && walk(p)) return found;
    return {};
}

} // namespace

SiteReport build_site(const fs::path& src_in, const fs::path& out_in, const SiteOptions& o)
{
    const fs::path src = fs::weakly_canonical(src_in), out = fs::weakly_canonical(out_in);
    if (!fs::is_directory(src)) throw std::invalid_argument(rel_of(src) + " is not a folder");
    const std::string s = rel_of(src) + "/", t = rel_of(out) + "/";
    if (t.rfind(s, 0) == 0 || s.rfind(t, 0) == 0) throw std::invalid_argument("the site and where it is built must not be inside one another");
    SiteReport rep;
    const std::string served = served_path(o.served_at);
    const std::vector<std::string> files = files_of(src);
    const std::set<std::string> present(files.begin(), files.end());
    const std::set<std::string> plain(o.plain.begin(), o.plain.end());
    for (const std::string& p : plain)
        if (!present.count(p)) throw std::invalid_argument("--plain names " + p + ", which the site does not have");
    for (const char* t : {kTable, kOldTable})
        if (present.count(t)) throw std::invalid_argument(std::string("the site has a file ") + t + " at its top, the name of the table the build writes there");

    // Every page read, for the links its tags give (what is made into .sieve files and given hashes)
    // and for what names a file where the extension cannot follow: CSS, scripts, PHP, and every other
    // attribute (srcset, style, <link>, <video>...). A file named there is kept as it is as well.
    std::map<std::string, std::string> html; // every page: its text
    std::set<std::string> linked;            // files a page's <img>, <source> or <a> names
    std::set<std::string> scripted;          // pages with a script (a <script>, or an on... attribute)
    std::vector<std::pair<std::string, std::string>> code; // where (a file), and text naming files there
    for (const std::string& f : files)
    {
        const std::string e = ext_of(f);
        if (is_page(f))
        {
            const std::string& h = html[f] = text_of(src / path_of(f));
            std::vector<std::string_view> raw;
            for (const Tag& tag : tags_of(h, &raw))
            {
                const Attr* link = link_of(tag);
                if (tag.name == "script") scripted.insert(f);
                for (const Attr& a : tag.attrs)
                {
                    if (a.name.size() > 2 && a.name.rfind("on", 0) == 0) scripted.insert(f);
                    if (!a.has_value || a.name == "integrity" || a.name == "data-sieve-keep") continue;
                    const std::string value = h.substr(a.value_begin, a.value_end - a.value_begin);
                    if (&a == link && !tag.attr("data-sieve-keep"))
                    {
                        if (const auto target = target_of(f, value, served); target && present.count(*target)) linked.insert(*target);
                    }
                    else if (&a != link) code.push_back({f, value});
                }
            }
            for (const std::string_view r : raw) code.push_back({f, std::string(r)});
        }
        else if (e == "css" || e == "js" || e == "mjs" || e == "json" || e == "xml" || e == "svg" || e == "webmanifest")
            code.push_back({f, text_of(src / path_of(f))});
    }
    auto named_where = [&](const std::string& f) -> std::string {
        const std::string n = name_of(f);
        for (const auto& [where, text] : code)
            if (where != f && names(text, n)) return where;
        return "";
    };

    // What becomes a .sieve: a picture or the like that a tag names, and with `pages`, a page a tag
    // names (but every index.html, those kept plain, and any named where the extension cannot follow).
    std::map<std::string, std::string> made; // its path in the site -> its .sieve's
    std::set<std::string> originals_too;     // made, and kept as it is as well
    std::vector<std::string> sieve_pages;
    size_t unlinked = 0;
    {
        std::map<std::string, std::vector<std::string>> wanted; // a .sieve's path -> the files that want it
        for (const std::string& f : files)
        {
            const std::string e = ext_of(f);
            const bool page = is_html(f) && o.pages && name_of(f) != "index.html" && !plain.count(f);
            const bool asset = !is_page(f) && std::find(o.sieve_kinds.begin(), o.sieve_kinds.end(), e) != o.sieve_kinds.end();
            if (!page && !asset) continue;
            if (!linked.count(f))
            {
                if (asset) unlinked += 1;
                continue;
            }
            if (page && scripted.count(f))
            {
                rep.notes.push_back("kept plain: " + f + " (it has a script, which the extension's viewer does not run)");
                continue;
            }
            const std::string where = named_where(f);
            if (page && !where.empty())
            {
                rep.notes.push_back("kept plain: " + f + " (" + where + " names it, where the extension cannot follow)");
                continue;
            }
            if (asset && (!where.empty() || o.keep_originals))
            {
                originals_too.insert(f);
                if (!where.empty()) rep.notes.push_back("kept as it is too, beside its .sieve: " + f + " (" + where + " names it, where the extension cannot follow)");
            }
            wanted[f.substr(0, f.size() - e.size() - 1) + ".sieve"].push_back(f);
            if (page) sieve_pages.push_back(f);
        }
        for (const auto& [name, who] : wanted)
            for (const std::string& f : who) made[f] = who.size() == 1 && !present.count(name) && name != kTable ? name : f + ".sieve";
        for (const auto& [f, m] : made)
            if (present.count(m)) throw std::runtime_error(f + " would be made " + m + ", which the site has already");
    }
    if (unlinked)
        rep.notes.insert(rep.notes.begin(), std::to_string(unlinked) + " picture" + (unlinked == 1 ? "" : "s") +
                                                " no page names in an <img>, <source> or <a>: kept as " + (unlinked == 1 ? "it is" : "they are"));

    // The pages made into .sieve files, each after those it names, so the hash it states is final:
    // pages that name one another cannot be, and are refused before anything is made, with a set of
    // pages to keep plain that would let the rest be made.
    std::map<std::string, std::set<std::string>> needs;
    for (const std::string& p : sieve_pages)
        for (const Tag& tag : tags_of(html[p]))
            if (const Attr* link = link_of(tag); link && link->has_value && !tag.attr("data-sieve-keep"))
                if (const auto target = target_of(p, html[p].substr(link->value_begin, link->value_end - link->value_begin), served);
                    target && *target != p && made.count(*target) && is_html(*target))
                    needs[p].insert(*target);
    if (const std::vector<std::string> loop = loop_of(sieve_pages, needs); !loop.empty())
    {
        // Pages to keep plain: from each loop left, the page the most others name, until none is left.
        auto left = needs;
        std::vector<std::string> pages = sieve_pages, keep;
        for (std::vector<std::string> l = loop; !l.empty(); l = loop_of(pages, left))
        {
            std::string best = l[0];
            size_t most = 0;
            for (size_t k = 0; k + 1 < l.size(); ++k)
            {
                size_t n = 0;
                for (const auto& [p, ns] : left) n += ns.count(l[k]);
                if (n > most) most = n, best = l[k];
            }
            keep.push_back(best);
            pages.erase(std::find(pages.begin(), pages.end(), best));
            left.erase(best);
            for (auto& [p, ns] : left) ns.erase(best);
        }
        std::string shown, list;
        for (const std::string& p : loop) shown += (shown.empty() ? "" : " -> ") + p;
        for (const std::string& p : plain) list += (list.empty() ? "" : ",") + p;
        for (const std::string& p : keep) list += (list.empty() ? "" : ",") + p;
        throw std::runtime_error("these pages link in a loop: " + shown + ". A page can state the hash of a page it links to, but pages that link to "
                                 "one another cannot each state the other's (changing one would change the other). Kept plain, these would let "
                                 "the rest be made: --plain " + list);
    }

    // Replaced only if empty, or a site this built (its table there): never a folder named by mistake.
    if (fs::exists(out) && (!fs::is_directory(out) || (!fs::is_empty(out) && !fs::exists(out / kTable) && !fs::exists(out / kOldTable))))
        throw std::invalid_argument(rel_of(out) + " is there and is not a site sieve site build made (no index.sieve): remove it, or name another");
    fs::remove_all(out);
    fs::create_directories(out);
    std::random_device rd;
    const fs::path tmp = fs::temp_directory_path() / ("sieve-site-" + std::to_string(rd()) + std::to_string(rd()));
    struct Cleanup
    {
        fs::path tmp, out;
        bool done = false;
        ~Cleanup()
        {
            std::error_code ec;
            fs::remove_all(tmp, ec);
            if (!done) fs::remove_all(out, ec); // a build that failed leaves nothing half made
        }
    } cleanup{tmp, out};

    const size_t to_make = made.size();
    auto making = [&](const std::string& f) {
        if (o.progress) o.progress(rep.made + 1, to_make, f);
    };
    // Everything else as it is (the pages after); the pictures and the like made.
    for (const std::string& f : files)
    {
        rep.files += 1;
        if (is_page(f)) continue;
        if (const auto it = made.find(f); it != made.end())
        {
            making(f);
            make_sieve(src / path_of(f), out / path_of(it->second), o.search_per_file);
            rep.made += 1;
            if (!originals_too.count(f)) continue;
        }
        fs::create_directories((out / path_of(f)).parent_path());
        fs::copy_file(src / path_of(f), out / path_of(f));
    }

    std::map<std::string, std::string> integrity; // a .sieve made or kept -> what links to it state
    auto stated = [&](const std::string& sieve_path) {
        auto it = integrity.find(sieve_path);
        if (it == integrity.end()) it = integrity.emplace(sieve_path, integrity_of(read_file_bytes(out / path_of(sieve_path)))).first;
        return it->second;
    };
    // A page with each link to a .sieve (made, or the site's own) given that file's hash.
    auto rewrite = [&](const std::string& page, const std::string& text) {
        struct Edit
        {
            size_t at, len;
            std::string with;
        };
        std::vector<Edit> edits;
        for (const Tag& tag : tags_of(text))
        {
            if (const Attr* res = resource_of(tag, text); res && res->has_value && !tag.attr("data-sieve-keep"))
            {
                const auto target = target_of(page, text.substr(res->value_begin, res->value_end - res->value_begin), served);
                if (target && present.count(*target) && !made.count(*target))
                {
                    const std::string with = integrity_of(read_file_bytes(src / path_of(*target)));
                    if (const Attr* old = tag.attr("integrity"); old && old->has_value)
                        edits.push_back({old->value_begin, old->value_end - old->value_begin, with});
                    else edits.push_back({tag.close, 0, " integrity=\"" + with + "\""});
                    rep.resources += 1;
                }
                continue;
            }
            const Attr* link = link_of(tag);
            if (!link || !link->has_value || tag.attr("data-sieve-keep")) continue;
            const std::string value = text.substr(link->value_begin, link->value_end - link->value_begin);
            const auto target = target_of(page, value, served);
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
        std::string h = text;
        for (const Edit& e : edits) h.replace(e.at, e.len, e.with);
        return h;
    };

    std::set<std::string> done;
    while (done.size() < sieve_pages.size())
        for (const std::string& p : sieve_pages)
        {
            if (done.count(p)) continue;
            if (!std::all_of(needs[p].begin(), needs[p].end(), [&](const std::string& n) { return done.count(n) != 0; })) continue;
            const fs::path page = tmp / path_of(std::to_string(done.size())) / path_of(name_of(p)); // its own name, for its .sieve
            write_text(page, rewrite(p, html[p]));
            making(p);
            make_sieve(page, out / path_of(made[p]), o.search_per_file);
            done.insert(p);
            rep.made += 1;
            rep.pages_made += 1;
        }
    // The plain pages, last: they may name any of them. An HTML page (not PHP, whose server writes
    // what it serves) is told where the site's table is, `<link rel="sieve-table">`, so the
    // extension finds it there and checks the page against it before showing it.
    for (const auto& [f, text] : html)
    {
        if (made.count(f)) continue;
        std::string h = rewrite(f, text);
        if (is_html(f))
        {
            std::string up;
            for (const char c : f)
                if (c == '/') up += "../";
            const std::string tag = "<link rel=\"sieve-table\" href=\"" + up + kTable + "\">";
            size_t at = 0; // after <head>, else after <html>, else first
            for (const Tag& t : tags_of(h))
                if (t.name == "head" || (t.name == "html" && !at))
                {
                    at = t.close + 1;
                    if (t.name == "head") break;
                }
            h.insert(at, at ? "\n" + tag : tag + "\n");
        }
        write_text(out / path_of(f), h);
    }

    // The site's map: every file, named by its SHA-256 (the newest instructions); its own hash is the root.
    Map map = map_of_folder(out, name_of(rel_of(src)));
    map.name = name_of(rel_of(src));
    write_map_instructions(map, out / kTable, 0);
    rep.map_sha256 = sha256_hex(read_file_bytes(out / kTable));
    cleanup.done = true;
    return rep;
}

SiteCheck check_site(const fs::path& dir, const std::string& served_at)
{
    SiteCheck c;
    const std::string served = served_path(served_at);
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
            const Attr* res = resource_of(tag, h);
            const Attr* link = res ? res : link_of(tag);
            if (!link || !link->has_value) continue;
            const std::string value = h.substr(link->value_begin, link->value_end - link->value_begin);
            const auto target = target_of(page, value, served);
            if (!target)
            {
                // A link from the server's root that states a hash, not read: the site is served
                // from somewhere --served-at did not say (or it was not given).
                if (value.size() > 1 && value[0] == '/' && value[1] != '/' && tag.attr("integrity")) c.unread += 1;
                continue;
            }
            if (tag.attr("data-sieve-keep"))
            {
                c.kept += 1; // left as written, on purpose
                continue;
            }
            const Attr* given = tag.attr("integrity");
            const bool sieve = ext_of(*target) == "sieve";
            if (!given || !given->has_value)
            {
                if (sieve && !res) c.mismatches.push_back(shown_as + ": links to " + *target + " with no hash");
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
        if (is_page(f)) check_page(f, f, text_of(dir / path_of(f)));
        else if (ext_of(f) == "sieve" && f != kTable && f != kOldTable)
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
    // The site's table: every file it names is here and is the same, and every file is in it.
    const std::string table = present.count(kTable) ? kTable : kOldTable;
    if (present.count(table))
    {
        const Map map = read_map_file(dir / path_of(table));
        std::set<std::string> named;
        for (const MapNode& n : map.nodes)
        {
            if (n.folder() || n.kind == MapNode::Held) continue;
            named.insert(n.path);
            c.checked += 1;
            if (!present.count(n.path)) c.mismatches.push_back(table + ": names " + n.path + ", which is not there");
            else if (sha256_hex(read_file_bytes(dir / path_of(n.path))) != n.sha256) c.mismatches.push_back(table + ": " + n.path + " is not the file it names");
        }
        for (const std::string& f : files)
            if (f != table && !named.count(f)) c.mismatches.push_back(table + ": does not name " + f);
    }
    else c.mismatches.push_back("no index.sieve (the site's table)");
    return c;
}

} // namespace sieve::cli
