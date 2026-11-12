// Documents read as their text (doc_text.hpp).
#include "cli/doc_text.hpp"

#include "cli/compress.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace sieve::cli {

namespace {

void put_utf8(std::string& out, uint32_t c)
{
    if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = 0xFFFD;
    if (c < 0x80) out += char(c);
    else if (c < 0x800) out += char(0xC0 | (c >> 6)), out += char(0x80 | (c & 0x3F));
    else if (c < 0x10000) out += char(0xE0 | (c >> 12)), out += char(0x80 | ((c >> 6) & 0x3F)), out += char(0x80 | (c & 0x3F));
    else
        out += char(0xF0 | (c >> 18)), out += char(0x80 | ((c >> 12) & 0x3F)), out += char(0x80 | ((c >> 6) & 0x3F)),
            out += char(0x80 | (c & 0x3F));
}

// Windows-1252's 0x80 to 0x9F (the rest is Latin-1); those it leaves undefined kept as they are.
constexpr uint16_t kCp1252[32] = {0x20AC, 0x81,   0x201A, 0x192,  0x201E, 0x2026, 0x2020, 0x2021, 0x2C6,  0x2030, 0x160,
                                  0x2039, 0x152,  0x8D,   0x17D,  0x8F,   0x90,   0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                  0x2013, 0x2014, 0x2DC,  0x2122, 0x161,  0x203A, 0x153,  0x9D,   0x17E,  0x178};

uint32_t cp1252(uint8_t b) { return b >= 0x80 && b < 0xA0 ? kCp1252[b - 0x80] : b; }

bool valid_utf8(std::string_view s)
{
    for (size_t i = 0; i < s.size();)
    {
        const uint8_t c = uint8_t(s[i]);
        const size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (n == 0 || i + n > s.size()) return false;
        for (size_t k = 1; k < n; ++k)
            if ((uint8_t(s[i + k]) >> 6) != 2) return false;
        i += n;
    }
    return true;
}

// Text that is not UTF-8 taken as Windows-1252.
std::string as_utf8(std::string_view s)
{
    if (valid_utf8(s)) return std::string(s);
    std::string out;
    for (const char c : s) put_utf8(out, cp1252(uint8_t(c)));
    return out;
}

const std::map<std::string, uint32_t, std::less<>>& entities()
{
    static const std::map<std::string, uint32_t, std::less<>> m = [] {
        std::map<std::string, uint32_t, std::less<>> t = {
            {"amp", '&'},       {"lt", '<'},        {"gt", '>'},        {"quot", '"'},      {"apos", '\''},     {"nbsp", ' '},
            {"mdash", 0x2014},  {"ndash", 0x2013},  {"hellip", 0x2026}, {"lsquo", 0x2018},  {"rsquo", 0x2019},  {"sbquo", 0x201A},
            {"ldquo", 0x201C},  {"rdquo", 0x201D},  {"bdquo", 0x201E},  {"bull", 0x2022},   {"euro", 0x20AC},   {"trade", 0x2122},
            {"dagger", 0x2020}, {"Dagger", 0x2021}, {"permil", 0x2030}, {"lsaquo", 0x2039}, {"rsaquo", 0x203A}, {"OElig", 0x152},
            {"oelig", 0x153},   {"Scaron", 0x160},  {"scaron", 0x161},  {"Yuml", 0x178},    {"fnof", 0x192},    {"thinsp", ' '},
            {"ensp", ' '},      {"emsp", ' '},      {"minus", 0x2212},  {"larr", 0x2190},   {"rarr", 0x2192},   {"uarr", 0x2191},
            {"darr", 0x2193},   {"harr", 0x2194},   {"shy", 0}};
        // Latin-1's named characters, 0xA1 to 0xFF, in order.
        const char* const latin1[] = {"iexcl",  "cent",   "pound",  "curren", "yen",    "brvbar", "sect",   "uml",    "copy",
                                      "ordf",   "laquo",  "not",    "",       "reg",    "macr",   "deg",    "plusmn", "sup2",
                                      "sup3",   "acute",  "micro",  "para",   "middot", "cedil",  "sup1",   "ordm",   "raquo",
                                      "frac14", "frac12", "frac34", "iquest", "Agrave", "Aacute", "Acirc",  "Atilde", "Auml",
                                      "Aring",  "AElig",  "Ccedil", "Egrave", "Eacute", "Ecirc",  "Euml",   "Igrave", "Iacute",
                                      "Icirc",  "Iuml",   "ETH",    "Ntilde", "Ograve", "Oacute", "Ocirc",  "Otilde", "Ouml",
                                      "times",  "Oslash", "Ugrave", "Uacute", "Ucirc",  "Uuml",   "Yacute", "THORN",  "szlig",
                                      "agrave", "aacute", "acirc",  "atilde", "auml",   "aring",  "aelig",  "ccedil", "egrave",
                                      "eacute", "ecirc",  "euml",   "igrave", "iacute", "icirc",  "iuml",   "eth",    "ntilde",
                                      "ograve", "oacute", "ocirc",  "otilde", "ouml",   "divide", "oslash", "ugrave", "uacute",
                                      "ucirc",  "uuml",   "yacute", "thorn",  "yuml"};
        for (uint32_t i = 0; i < sizeof latin1 / sizeof *latin1; ++i)
            if (*latin1[i]) t.emplace(latin1[i], 0xA1 + i);
        return t;
    }();
    return m;
}

// &name; &#N; &#xH; read; anything else left as it is.
std::string decode_entities(std::string_view s)
{
    std::string out;
    for (size_t i = 0; i < s.size();)
    {
        if (s[i] == '&')
        {
            const size_t semi = s.find(';', i + 1);
            if (semi != std::string_view::npos && semi - i <= 12)
            {
                const std::string_view name = s.substr(i + 1, semi - i - 1);
                uint32_t c = 0;
                bool ok = false;
                if (name.size() >= 2 && name[0] == '#')
                {
                    const bool hex = name[1] == 'x' || name[1] == 'X';
                    const std::string_view digits = name.substr(hex ? 2 : 1);
                    ok = !digits.empty() && digits.size() <= 7;
                    for (const char d : digits)
                    {
                        const int v = std::isdigit(uint8_t(d)) ? d - '0' : hex && std::isxdigit(uint8_t(d)) ? std::tolower(uint8_t(d)) - 'a' + 10 : -1;
                        if (v < 0) ok = false;
                        else c = c * (hex ? 16 : 10) + uint32_t(v);
                    }
                }
                else if (const auto it = entities().find(name); it != entities().end())
                    c = it->second, ok = true;
                if (ok)
                {
                    if (c) put_utf8(out, c);
                    i = semi + 1;
                    continue;
                }
            }
        }
        out += s[i++];
    }
    return out;
}

// A tag's or text's place in markup (XML, or HTML read the same way).
struct Event
{
    enum Kind { Start, End, Empty, Text } kind;
    std::string_view name;  // a tag's name as written (with any prefix)
    std::string_view attrs; // a start tag's attributes, as written
    std::string_view text;  // Text: as written (entities not yet read), or CDATA's
    bool cdata = false;
};

void walk(std::string_view x, const std::function<void(const Event&)>& f)
{
    size_t i = 0;
    while (i < x.size())
    {
        if (x[i] != '<')
        {
            const size_t j = std::min(x.find('<', i), x.size());
            f({Event::Text, {}, {}, x.substr(i, j - i)});
            i = j;
            continue;
        }
        if (x.compare(i, 4, "<!--") == 0)
        {
            const size_t e = x.find("-->", i + 4);
            i = e == std::string_view::npos ? x.size() : e + 3;
            continue;
        }
        if (x.compare(i, 9, "<![CDATA[") == 0)
        {
            const size_t e = std::min(x.find("]]>", i + 9), x.size());
            f({Event::Text, {}, {}, x.substr(i + 9, e - i - 9), true});
            i = std::min(e + 3, x.size());
            continue;
        }
        // A tag runs to the first '>' outside quotes.
        size_t j = i + 1;
        char quote = 0;
        for (; j < x.size(); ++j)
        {
            if (quote) quote = x[j] == quote ? 0 : quote;
            else if (x[j] == '"' || x[j] == '\'') quote = x[j];
            else if (x[j] == '>') break;
        }
        const std::string_view tag = x.substr(i + 1, std::min(j, x.size()) - i - 1);
        i = std::min(j + 1, x.size());
        if (tag.empty() || tag[0] == '!' || tag[0] == '?') continue;
        const bool end = tag[0] == '/', empty = !end && tag.back() == '/';
        const std::string_view body = tag.substr(end ? 1 : 0, tag.size() - (end ? 1 : 0) - (empty ? 1 : 0));
        size_t n = 0;
        while (n < body.size() && !std::isspace(uint8_t(body[n])) && body[n] != '/') ++n;
        if (n == 0) continue;
        f({end ? Event::End : empty ? Event::Empty : Event::Start, body.substr(0, n), body.substr(n), {}});
    }
}

std::string_view local(std::string_view name)
{
    const size_t c = name.rfind(':');
    return c == std::string_view::npos ? name : name.substr(c + 1);
}

std::string lower(std::string_view s)
{
    std::string t(s);
    for (char& c : t) c = char(std::tolower(uint8_t(c)));
    return t;
}

// An attribute's value (entities read), by its name as written; nothing where it is not there.
std::optional<std::string> attr(std::string_view attrs, std::string_view name)
{
    for (size_t at = 0; (at = attrs.find(name, at)) != std::string_view::npos; at += name.size())
    {
        if (at > 0 && !std::isspace(uint8_t(attrs[at - 1]))) continue;
        size_t k = at + name.size();
        while (k < attrs.size() && std::isspace(uint8_t(attrs[k]))) ++k;
        if (k >= attrs.size() || attrs[k] != '=') continue;
        ++k;
        while (k < attrs.size() && std::isspace(uint8_t(attrs[k]))) ++k;
        if (k >= attrs.size()) return std::nullopt;
        const char q = attrs[k];
        if (q == '"' || q == '\'')
        {
            const size_t e = attrs.find(q, k + 1);
            return decode_entities(attrs.substr(k + 1, (e == std::string_view::npos ? attrs.size() : e) - k - 1));
        }
        size_t e = k;
        while (e < attrs.size() && !std::isspace(uint8_t(attrs[e]))) ++e;
        return decode_entities(attrs.substr(k, e - k));
    }
    return std::nullopt;
}

// Text built a piece at a time: runs of white space made one space (unless kept), and line breaks
// asked for at the ends of blocks, never more than a blank line, never at the start.
struct Writer
{
    std::string out;
    bool space = false;

    void text(std::string_view t, bool keep_space = false)
    {
        for (const char c : t)
        {
            if (!keep_space && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'))
            {
                space = true;
                continue;
            }
            if (keep_space && c == '\r') continue;
            if (space && !out.empty() && out.back() != '\n') out += ' ';
            space = false;
            out += c;
        }
    }
    void raw(std::string_view t)
    {
        if (space && !out.empty() && out.back() != '\n' && t != "\n") out += ' ';
        space = false;
        out += t;
    }
    void lines(int n)
    {
        while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
        space = false;
        if (out.empty()) return;
        int have = 0;
        for (size_t i = out.size(); i-- > 0 && out[i] == '\n';) ++have;
        for (; have < n; ++have) out += '\n';
    }
    std::string done()
    {
        while (!out.empty() && (out.back() == '\n' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
        if (!out.empty()) out += '\n';
        return std::move(out);
    }
};

} // namespace

std::string html_text(std::string_view input)
{
    const std::string html = as_utf8(input);
    // Script, style and the like hold no text to read, and may hold '<' that is not a tag: cut out.
    std::string kept;
    {
        const std::string low = lower(html);
        size_t i = 0;
        while (i < html.size())
        {
            size_t next = std::string::npos, which = 0;
            static const char* const kSkip[] = {"script", "style", "noscript", "template"};
            for (size_t k = 0; k < 4; ++k)
            {
                const size_t at = low.find(std::string("<") + kSkip[k], i);
                if (at < next)
                {
                    const char after = at + 1 + std::strlen(kSkip[k]) < low.size() ? low[at + 1 + std::strlen(kSkip[k])] : '>';
                    if (after == '>' || after == '/' || std::isspace(uint8_t(after))) next = at, which = k;
                }
            }
            if (next == std::string::npos)
            {
                kept.append(html, i, std::string::npos);
                break;
            }
            kept.append(html, i, next - i);
            const size_t close = low.find(std::string("</") + kSkip[which], next);
            if (close == std::string::npos) break;
            const size_t gt = low.find('>', close);
            i = gt == std::string::npos ? html.size() : gt + 1;
        }
    }
    static const std::map<std::string, int, std::less<>> kBlocks = {
        {"p", 2},       {"h1", 2},     {"h2", 2},      {"h3", 2},     {"h4", 2},          {"h5", 2},      {"h6", 2},
        {"title", 2},   {"blockquote", 2}, {"pre", 2}, {"table", 2},  {"ul", 2},          {"ol", 2},      {"dl", 2},
        {"section", 2}, {"article", 2}, {"aside", 2},  {"figure", 2}, {"br", 1},          {"li", 1},      {"tr", 1},
        {"div", 1},     {"dt", 1},     {"dd", 1},      {"hr", 1},     {"header", 1},      {"footer", 1},  {"nav", 1},
        {"figcaption", 1}, {"caption", 1}, {"option", 1}, {"body", 1}, {"main", 1},       {"address", 1}, {"center", 1}};
    Writer w;
    int pre = 0;
    walk(kept, [&](const Event& e) {
        if (e.kind == Event::Text)
        {
            w.text(e.cdata ? std::string(e.text) : decode_entities(e.text), pre > 0);
            return;
        }
        const std::string name = lower(local(e.name));
        if (name == "pre") pre = std::max(0, pre + (e.kind == Event::Start ? 1 : e.kind == Event::End ? -1 : 0));
        if (name == "td" || name == "th")
        {
            if (e.kind != Event::Start) w.space = true;
            return;
        }
        if (const auto it = kBlocks.find(name); it != kBlocks.end()) w.lines(it->second);
    });
    return w.done();
}

namespace {

std::string word_text(const std::map<std::string, std::vector<uint8_t>>& zip)
{
    const auto it = zip.find("word/document.xml");
    if (it == zip.end()) throw std::runtime_error("no word/document.xml: not a Word document");
    const std::string_view x(reinterpret_cast<const char*>(it->second.data()), it->second.size());
    Writer w;
    int run_text = 0, para_props = 0;
    walk(x, [&](const Event& e) {
        if (e.kind == Event::Text)
        {
            if (run_text > 0) w.text(e.cdata ? std::string(e.text) : decode_entities(e.text), true);
            return;
        }
        const std::string_view n = e.name;
        const int d = e.kind == Event::Start ? 1 : e.kind == Event::End ? -1 : 0;
        if (n == "w:t") run_text = std::max(0, run_text + d);
        else if (n == "w:pPr") para_props = std::max(0, para_props + d);
        else if (para_props == 0 && n == "w:tab" && e.kind != Event::End) w.raw("\t");
        else if ((n == "w:br" || n == "w:cr") && e.kind != Event::End) w.raw("\n");
        else if (n == "w:noBreakHyphen" && e.kind != Event::End) w.raw("-");
        else if (n == "w:p" && e.kind != Event::Start) w.raw("\n");
    });
    return w.done();
}

std::string open_document_text(const std::map<std::string, std::vector<uint8_t>>& zip)
{
    const auto it = zip.find("content.xml");
    if (it == zip.end()) throw std::runtime_error("no content.xml: not an OpenDocument");
    const std::string_view x(reinterpret_cast<const char*>(it->second.data()), it->second.size());
    Writer w;
    int para = 0;
    walk(x, [&](const Event& e) {
        if (e.kind == Event::Text)
        {
            if (para > 0) w.text(e.cdata ? std::string(e.text) : decode_entities(e.text), true);
            return;
        }
        const std::string_view n = e.name;
        if (n == "text:p" || n == "text:h")
        {
            if (e.kind == Event::Start) ++para;
            else
            {
                if (e.kind == Event::End) para = std::max(0, para - 1);
                w.raw("\n");
            }
        }
        else if (para > 0 && e.kind != Event::End)
        {
            if (n == "text:s")
            {
                int count = 1;
                if (const auto c = attr(e.attrs, "text:c")) count = std::clamp(std::atoi(c->c_str()), 1, 1000);
                w.raw(std::string(size_t(count), ' '));
            }
            else if (n == "text:tab") w.raw("\t");
            else if (n == "text:line-break") w.raw("\n");
        }
    });
    return w.done();
}

// "a/b/../c%20d.xhtml#x" as the path it names in the zip.
std::string resolve(const std::string& base, std::string href)
{
    if (const size_t h = href.find('#'); h != std::string::npos) href.resize(h);
    std::string decoded;
    for (size_t i = 0; i < href.size(); ++i)
    {
        if (href[i] == '%' && i + 2 < href.size() && std::isxdigit(uint8_t(href[i + 1])) && std::isxdigit(uint8_t(href[i + 2])))
        {
            decoded += char(std::stoi(href.substr(i + 1, 2), nullptr, 16));
            i += 2;
        }
        else decoded += href[i];
    }
    std::vector<std::string> parts;
    std::string piece;
    for (const char c : base + decoded + "/")
    {
        if (c != '/')
        {
            piece += c;
            continue;
        }
        if (piece == "..")
        {
            if (!parts.empty()) parts.pop_back();
        }
        else if (!piece.empty() && piece != ".") parts.push_back(piece);
        piece.clear();
    }
    std::string out;
    for (const auto& p : parts) out += (out.empty() ? "" : "/") + p;
    return out;
}

std::string epub_text(const std::map<std::string, std::vector<uint8_t>>& zip)
{
    auto file = [&](const std::string& name) -> std::string_view {
        const auto it = zip.find(name);
        if (it == zip.end()) throw std::runtime_error("no " + name + " in the book");
        return {reinterpret_cast<const char*>(it->second.data()), it->second.size()};
    };
    std::string opf_path;
    walk(file("META-INF/container.xml"), [&](const Event& e) {
        if (opf_path.empty() && e.kind != Event::End && local(e.name) == "rootfile")
            if (const auto p = attr(e.attrs, "full-path")) opf_path = *p;
    });
    if (opf_path.empty()) throw std::runtime_error("the book names no package (rootfile)");
    const size_t slash = opf_path.rfind('/');
    const std::string base = slash == std::string::npos ? "" : opf_path.substr(0, slash + 1);
    std::map<std::string, std::string> items; // id to href
    std::vector<std::string> spine;
    walk(file(opf_path), [&](const Event& e) {
        if (e.kind == Event::End) return;
        const std::string_view n = local(e.name);
        if (n == "item")
        {
            const auto id = attr(e.attrs, "id"), href = attr(e.attrs, "href");
            if (id && href) items[*id] = *href;
        }
        else if (n == "itemref")
            if (const auto id = attr(e.attrs, "idref")) spine.push_back(*id);
    });
    if (spine.empty()) throw std::runtime_error("the book has no reading order (spine)");
    std::string out;
    for (const std::string& id : spine)
    {
        const auto it = items.find(id);
        if (it == items.end()) continue;
        const std::string chapter = html_text(file(resolve(base, it->second)));
        if (chapter.empty()) continue;
        if (!out.empty()) out += '\n'; // (each chapter ends its line: a blank line between them)
        out += chapter;
    }
    return out;
}

std::string rtf_text(std::string_view r)
{
    if (r.substr(0, 5) != "{\\rtf") throw std::runtime_error("not RTF ({\\rtf at its start)");
    struct State
    {
        bool skip = false;
        int uc = 1;
    };
    static const char* const kSkipped[] = {"fonttbl",   "colortbl",   "stylesheet", "info",       "pict",        "object",
                                           "header",    "headerl",    "headerr",    "headerf",    "footer",      "footerl",
                                           "footerr",   "footerf",    "fldinst",    "themedata",  "colorschememapping",
                                           "latentstyles", "datastore", "xmlnstbl", "listtable", "listoverridetable",
                                           "rsidtbl",   "generator",  "filetbl",    "revtbl",     "pgdsctbl",    "footnote",
                                           "annotation", "bkmkstart", "bkmkend",    "nonshppict", "shp",         "mmathPr"};
    std::vector<State> st{State{}};
    Writer w;
    int unicode_skip = 0; // characters still to skip after \uN (its stand-in)
    auto put = [&](uint32_t c) {
        if (unicode_skip > 0)
        {
            --unicode_skip;
            return;
        }
        if (st.back().skip) return;
        std::string s;
        put_utf8(s, c);
        w.raw(s);
    };
    for (size_t i = 0; i < r.size();)
    {
        const char c = r[i];
        if (c == '{')
        {
            st.push_back(st.back());
            ++i;
            continue;
        }
        if (c == '}')
        {
            if (st.size() > 1) st.pop_back();
            ++i;
            continue;
        }
        if (c == '\r' || c == '\n')
        {
            ++i;
            continue;
        }
        if (c != '\\')
        {
            put(cp1252(uint8_t(c)));
            ++i;
            continue;
        }
        // A control word or symbol.
        if (i + 1 >= r.size()) break;
        const char n = r[i + 1];
        if (!std::isalpha(uint8_t(n)))
        {
            i += 2;
            if (n == '\'' && i + 2 <= r.size() && std::isxdigit(uint8_t(r[i])) && std::isxdigit(uint8_t(r[i + 1])))
            {
                put(cp1252(uint8_t(std::stoi(std::string(r.substr(i, 2)), nullptr, 16))));
                i += 2;
            }
            else if (n == '*') st.back().skip = true;
            else if (n == '~') put(' ');
            else if (n == '_') put('-');
            else if (n == '\\' || n == '{' || n == '}') put(uint8_t(n));
            else if ((n == '\n' || n == '\r') && !st.back().skip) w.raw("\n");
            continue;
        }
        size_t j = i + 1;
        while (j < r.size() && std::isalpha(uint8_t(r[j]))) ++j;
        const std::string word(r.substr(i + 1, j - i - 1));
        bool has = false;
        long param = 0;
        const bool neg = j < r.size() && r[j] == '-';
        size_t k = j + (neg ? 1 : 0);
        while (k < r.size() && std::isdigit(uint8_t(r[k])) && k - j < 10) param = param * 10 + (r[k++] - '0'), has = true;
        if (neg) param = -param;
        i = has ? k : j;
        if (i < r.size() && r[i] == ' ') ++i;
        if (std::find_if(std::begin(kSkipped), std::end(kSkipped), [&](const char* s) { return word == s; }) != std::end(kSkipped))
            st.back().skip = true;
        else if (st.back().skip) continue;
        else if (word == "par" || word == "line" || word == "sect" || word == "page") w.raw("\n");
        else if (word == "tab") w.raw("\t");
        else if (word == "uc" && has) st.back().uc = int(std::clamp(param, 0L, 8L));
        else if (word == "u" && has)
        {
            put(uint32_t(param < 0 ? param + 65536 : param));
            unicode_skip = st.back().uc;
        }
        else if (word == "emdash") put(0x2014);
        else if (word == "endash") put(0x2013);
        else if (word == "lquote") put(0x2018);
        else if (word == "rquote") put(0x2019);
        else if (word == "ldblquote") put(0x201C);
        else if (word == "rdblquote") put(0x201D);
        else if (word == "bullet") put(0x2022);
    }
    return w.done();
}

} // namespace

std::optional<std::string> document_kind(const std::string& name)
{
    const size_t dot = name.rfind('.');
    if (dot == std::string::npos) return std::nullopt;
    const std::string ext = lower(name.substr(dot + 1));
    if (ext == "html" || ext == "htm" || ext == "xhtml") return "HTML";
    if (ext == "docx") return "Word";
    if (ext == "odt") return "OpenDocument";
    if (ext == "epub") return "EPUB";
    if (ext == "rtf") return "RTF";
    return std::nullopt;
}

std::string document_text(const std::string& kind, std::span<const uint8_t> bytes)
{
    const std::string_view s(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (kind == "HTML") return html_text(s);
    if (kind == "RTF") return rtf_text(s);
    if (kind == "Word" || kind == "OpenDocument" || kind == "EPUB")
    {
        const std::map<std::string, std::vector<uint8_t>> zip = libarchive_read(bytes, true);
        return kind == "Word" ? word_text(zip) : kind == "OpenDocument" ? open_document_text(zip) : epub_text(zip);
    }
    throw std::runtime_error("not a document Sieve reads as text: " + kind);
}

} // namespace sieve::cli
