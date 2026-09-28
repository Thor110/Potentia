// Sieve hallway — the filter designer's model (designer_model.hpp): writing and reading the file,
// testing it from a scratch folder, and saving it as a version.

#include "designer_model.hpp"

#include "cli/dictionaries.hpp"
#include "cli/filter_config.hpp"
#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cctype>
#include <atomic>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace hallway::design {

namespace fs = std::filesystem;

namespace {

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// A value written on a header line: a `;` would start a comment, and a line break a new line.
std::string clean(std::string s)
{
    for (char& c : s)
        if (c == ';' || c == '\n' || c == '\r') c = ',';
    return s;
}

std::string trim(const std::string& s)
{
    const size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

// The line without its comment: from a `;` outside "quotes".
std::string strip_comment(const std::string& s)
{
    bool quoted = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '"' && (i == 0 || s[i - 1] != '\\')) quoted = !quoted;
        if (s[i] == ';' && !quoted) return s.substr(0, i);
    }
    return s;
}

std::vector<std::string> words(const std::string& s)
{
    std::vector<std::string> out;
    std::istringstream in(s);
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}

std::string read_file(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream o;
    o << in.rdbuf();
    return o.str();
}

// A scratch folder for this process: the file and its lists are written here to be compiled.
fs::path scratch()
{
    static const fs::path dir = [] {
        std::error_code ec;
        fs::path base = fs::temp_directory_path(ec);
        if (ec) base = ".";
        static std::atomic<unsigned> n{0};
        const fs::path d = base / ("sieve-designer-" + std::to_string(uintptr_t(&n) % 100000) + "-" + std::to_string(n++));
        fs::create_directories(d, ec);
        return d;
    }();
    return dir;
}

} // namespace

// ---------------------------------------------------------------- the file

uint32_t Doc::written_format() const
{
    const bool needs_v2 = symbols.find('*') != std::string::npos && symbols != "any";
    const bool choice = std::any_of(params.begin(), params.end(), [](const Param& p) { return p.kind == "choice"; });
    return format >= 2 || needs_v2 || choice ? 2 : 1;
}

std::string Doc::to_text() const
{
    std::ostringstream o;
    o << (written_format() >= 2 ? "sieve-filter-v2" : "sieve-filter-v1") << "\n; Made with the Sieve filter designer.\n";
    for (const auto& [node, p] : layout) o << "; designer: node " << node << " " << int(p.x) << " " << int(p.y) << "\n";
    o << "id        " << id << "\n"
      << "version   " << version << "\n"
      << "author    " << (trim(author).empty() ? std::string("anonymous") : clean(author)) << "\n"
      << "origin    " << origin << "\n"
      << "lines    ";
    for (const auto& l : lines) o << " " << l;
    o << "\n"
      << "symbols   " << symbols << "\n";
    if (!trim(describe).empty()) o << "describe  " << clean(describe) << "\n";
    for (const auto& r : prerequisites)
    {
        if (trim(r.name).empty()) continue;
        o << "requires  " << r.name;
        for (const auto& [k, v] : r.pins) o << "  " << k << "=" << v;
        o << "\n";
    }
    for (const auto& p : params)
    {
        if (p.kind == "dict") o << "param     " << p.name << "  dict  " << (trim(p.def).empty() ? std::string("default") : p.def);
        else if (p.kind == "choice") o << "param     " << p.name << "  choice  " << p.def << "  " << p.choices;
        else o << "param     " << p.name << "  int  " << p.def << "  " << p.min << "  " << p.max;
        if (!trim(p.text).empty()) o << "  " << clean(p.text);
        o << "\n";
    }
    o << "\n";
    if (form == "tokens")
    {
        o << "tokens    separator " << separator << "\n"
          << "edges     " << (cut ? "cut" : "whole") << "\n";
        for (const auto& s : sets) o << "set       " << s.name << "  " << s.source << "\n";
        for (const auto& [a, b] : follows) o << "follow    " << a << "  " << b << "\n";
        const bool all_first = std::all_of(sets.begin(), sets.end(), [](const Set& s) { return s.first; });
        const bool all_last = std::all_of(sets.begin(), sets.end(), [](const Set& s) { return s.last; });
        const bool any_first = std::any_of(sets.begin(), sets.end(), [](const Set& s) { return s.first; });
        const bool any_last = std::any_of(sets.begin(), sets.end(), [](const Set& s) { return s.last; });
        if (!all_first && any_first)
        {
            o << "first    ";
            for (const auto& s : sets)
                if (s.first) o << " " << s.name;
            o << "\n";
        }
        if (!all_last && any_last)
        {
            o << "last     ";
            for (const auto& s : sets)
                if (s.last) o << " " << s.name;
            o << "\n";
        }
    }
    else
        for (const auto& l : table) o << l << "\n";
    o << "end\n";
    return o.str();
}

Doc Doc::from_text(const std::string& text, const fs::path& folder)
{
    // The parser is the judge of whether it is a plugin at all.
    (void)sieve::parse_plugin(text, "check", u8(folder));
    Doc d;
    d.lines.clear();
    d.form = "table";
    std::istringstream in(text);
    std::string raw;
    bool first_line = true, in_table = false;
    std::vector<std::string> first, last;
    bool has_first = false, has_last = false;
    while (std::getline(in, raw))
    {
        const std::string t = trim(raw);
        if (first_line)
        {
            if (t.empty() || t[0] == ';' || t[0] == '#') continue;
            d.format = t == "sieve-filter-v2" ? 2 : 1;
            first_line = false;
            continue;
        }
        if (t.rfind("; designer: node ", 0) == 0)
        {
            const auto w = words(t.substr(17));
            if (w.size() == 3) d.layout[w[0]] = {std::stof(w[1]), std::stof(w[2])};
            continue;
        }
        if (t.empty() || t[0] == ';' || t[0] == '#')
        {
            if (in_table) d.table.push_back(raw);
            continue;
        }
        const std::string body = trim(strip_comment(t));
        const auto w = words(body);
        const std::string& k = w[0];
        const std::string rest = trim(body.substr(k.size()));
        if (k == "end") break;
        if (k == "class" || k == "states" || k == "start" || k == "accept" || k == "t" || k == "for" || k == "done" || in_table)
        {
            in_table = true;
            d.table.push_back(raw);
            continue;
        }
        if (k == "id") d.id = rest;
        else if (k == "version") d.version = uint32_t(std::stoul(rest));
        else if (k == "author") d.author = rest;
        else if (k == "origin") d.origin = rest;
        else if (k == "lines") d.lines = words(rest);
        else if (k == "symbols") d.symbols = rest;
        else if (k == "describe") d.describe = rest;
        else if (k == "requires")
        {
            Prerequisite r;
            r.name = w[1];
            for (size_t i = 2; i < w.size(); ++i)
            {
                const size_t eq = w[i].find('=');
                r.pins.emplace_back(w[i].substr(0, eq), w[i].substr(eq + 1));
            }
            d.prerequisites.push_back(r);
        }
        else if (k == "param")
        {
            Param p;
            p.name = w[1];
            p.kind = w[2];
            if (p.kind == "dict")
            {
                p.def = w[3] == "default" ? "" : w[3];
                for (size_t i = 4; i < w.size(); ++i) p.text += (p.text.empty() ? "" : " ") + w[i];
            }
            else if (p.kind == "choice")
            {
                p.def = w[3];
                p.choices = w[4];
                for (size_t i = 5; i < w.size(); ++i) p.text += (p.text.empty() ? "" : " ") + w[i];
            }
            else
            {
                p.def = w[3];
                p.min = w[4];
                p.max = w[5];
                for (size_t i = 6; i < w.size(); ++i) p.text += (p.text.empty() ? "" : " ") + w[i];
            }
            d.params.push_back(p);
        }
        else if (k == "tokens")
        {
            d.form = "tokens";
            d.separator = trim(rest.substr(rest.find("separator") + 9));
        }
        else if (k == "edges") d.cut = rest == "cut";
        else if (k == "set")
        {
            d.form = "tokens";
            d.sets.push_back({w[1], w[2], true, true});
        }
        else if (k == "follow") d.follows.emplace_back(w[1], w[2]);
        else if (k == "first")
        {
            has_first = true;
            first.assign(w.begin() + 1, w.end());
        }
        else if (k == "last")
        {
            has_last = true;
            last.assign(w.begin() + 1, w.end());
        }
    }
    for (auto& s : d.sets)
    {
        if (has_first) s.first = std::find(first.begin(), first.end(), s.name) != first.end();
        if (has_last) s.last = std::find(last.begin(), last.end(), s.name) != last.end();
        if (s.source.rfind("list:", 0) == 0)
        {
            const std::string file = s.source.substr(5);
            d.lists[file] = read_file(folder / from_u8(file));
        }
    }
    if (d.lines.empty()) d.lines = {"text"};
    d.data_folder = folder;
    return d;
}

Doc Doc::new_tokens()
{
    Doc d;
    d.id = "my-words";
    d.describe = "Every token is a word of the set.";
    d.params.push_back({"dictionary", "dict", "", "", "", "the word list (a registered dictionary)"});
    d.sets.push_back({"word", "dict:{dictionary}", true, true});
    return d;
}

Doc Doc::new_table()
{
    Doc d;
    d.id = "my-table";
    d.form = "table";
    d.describe = "No two SPACEs in a row, and at least one letter.";
    d.table = {"class     space   \" \"", "class     letter  a-z", "states    4", "start     0", "accept    2 3", "t   0   space   1",
               "t   0   letter  2",     "t   1   letter  2",     "t   2   letter  2", "t   2   space   3", "t   3   letter  2"};
    return d;
}

// ---------------------------------------------------------------- testing

sieve::FilterLine test_line(const std::string& symbols, uint32_t length)
{
    sieve::FilterLine l;
    l.length = length;
    if (symbols == sieve::kNotesSymbolsId || symbols == "notes*") // notes*: every note line, tested on notes104
    {
        l.kind = "audio";
        l.symbols_id = sieve::kNotesSymbolsId;
        l.base = sieve::kNoteSymbols;
        return l;
    }
    if (symbols.rfind("palette:", 0) == 0)
    {
        const sieve::Palette& p = sieve::palette_by_id(symbols.substr(8));
        l.kind = "image";
        l.symbols_id = "image/" + p.id() + "/" + std::to_string(length) + "x1";
        l.base = p.size();
        l.width = length;
        l.height = 1;
        l.frames = 1;
        return l;
    }
    const sieve::Alphabet& a = sieve::alphabet_of(symbols == "any" ? "lower27" : symbols);
    l.kind = "text";
    l.symbols_id = a.id();
    l.base = a.size();
    l.alphabet = &a;
    return l;
}

namespace {

std::string unit_text(const sieve::FilterLine& l, const std::vector<uint32_t>& u)
{
    if (l.alphabet)
    {
        std::u32string s;
        for (uint32_t c : u) s += l.alphabet->symbol(c);
        return "\"" + sieve::utf8_encode(s) + "\"";
    }
    std::string s;
    for (uint32_t c : u) s += (s.empty() ? "" : ",") + std::to_string(c);
    return s;
}

// The files a tagged set reads ("tags:a.tsv+b.tsv:Np" -> a.tsv, b.tsv).
std::vector<std::string> tag_files(const Doc& d)
{
    std::vector<std::string> out;
    for (const auto& s : d.sets)
    {
        if (s.source.rfind("tags:", 0) != 0) continue;
        const std::string rest = s.source.substr(5);
        const size_t colon = rest.rfind(':');
        std::string files = colon == std::string::npos ? rest : rest.substr(0, colon);
        size_t at = 0;
        while (at <= files.size())
        {
            const size_t plus = std::min(files.find('+', at), files.size());
            const std::string f = files.substr(at, plus - at);
            if (!f.empty() && std::find(out.begin(), out.end(), f) == out.end()) out.push_back(f);
            at = plus + 1;
        }
    }
    return out;
}

fs::path data_folder_of(const Doc& d)
{
    return d.data_folder.empty() ? filters_folder() : d.data_folder;
}

// The file and its lists in the scratch folder, parsed. Tagged lists are copied there from where
// they are (once: a copy of the same size and time is kept).
std::shared_ptr<const sieve::PluginDef> staged(const Doc& d)
{
    const fs::path dir = scratch();
    for (const std::string& f : tag_files(d))
    {
        std::error_code ec;
        const fs::path from = data_folder_of(d) / from_u8(f), to = dir / from_u8(f);
        if (!fs::exists(from, ec)) continue; // the compiler names the missing file
        const bool same = fs::exists(to, ec) && fs::file_size(to, ec) == fs::file_size(from, ec) &&
                          fs::last_write_time(to, ec) == fs::last_write_time(from, ec);
        if (same) continue;
        fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
        if (!ec) fs::last_write_time(to, fs::last_write_time(from, ec), ec);
    }
    for (const auto& [file, words] : d.lists)
    {
        std::ofstream out(dir / from_u8(file), std::ios::binary);
        out << words;
    }
    const std::string text = d.to_text();
    return sieve::parse_plugin(text, sieve::Sha256::hex(sieve::Sha256::hash(text)), u8(dir));
}

} // namespace

namespace {

// The file's line numbers in an error, as the table editor numbers its lines ("line 11" of the
// file is "table line 1" when the table starts there).
std::string in_table_terms(const Doc& d, const std::string& error)
{
    if (d.form != "table" || d.table.empty()) return error;
    const size_t at = error.find("line ");
    if (at == std::string::npos) return error;
    size_t end = at + 5;
    while (end < error.size() && std::isdigit(static_cast<unsigned char>(error[end]))) ++end;
    if (end == at + 5) return error;
    const long file_line = std::stol(error.substr(at + 5, end - at - 5));
    Doc plain = d;
    plain.layout.clear();
    std::istringstream in(plain.to_text());
    std::string l;
    long n = 0, first = 0;
    while (std::getline(in, l))
    {
        ++n;
        if (l == d.table.front()) { first = n; break; }
    }
    if (first == 0 || file_line < first || file_line >= first + long(d.table.size())) return error;
    return error.substr(0, at) + "table line " + std::to_string(file_line - first + 1) + error.substr(end);
}

} // namespace

TestResult test(const Doc& d, uint32_t length, Progress* progress)
{
    TestResult r;
    auto say = [&](const std::string& s) {
        if (progress) progress->set(s);
    };
    try
    {
        r.line = test_line(d.symbols, std::max<uint32_t>(1, length));
        say("writing the file and its word lists");
        const auto p = staged(d);
        static const sieve::cli::AppResources resources;
        sieve::Dfa dfa = sieve::compile_plugin(*p, r.line, {}, resources, &r.declared, &r.data, [&](const std::string& s) { say(s); });
        r.minimal = dfa.states();
        r.dfa = std::make_shared<const sieve::Dfa>(std::move(dfa));
        if (sieve::DfaRanker::table_bytes(r.dfa->states(), r.dfa->base, r.line.length) <= sieve::kPluginTableBudget)
        {
            say("counting the survivors at length " + std::to_string(r.line.length) + " (" + std::to_string(r.minimal) + " states)");
            const sieve::DfaRanker rk(*r.dfa, r.line.length);
            r.counted = true;
            r.survivors = rk.count().to_decimal();
            sieve::BigUint ex = sieve::BigUint::pow(r.line.base, r.line.length);
            ex -= rk.count();
            r.excluded = ex.to_decimal();
            if (!rk.count().is_zero())
            {
                // Five survivors spread through the list: the first, a quarter, half, three
                // quarters of the way, and the last.
                std::vector<sieve::BigUint> ks;
                for (uint32_t q = 0; q <= 4; ++q)
                {
                    sieve::BigUint k = rk.count();
                    k -= sieve::BigUint(1);
                    k.mul_small(q);
                    k.divmod_small(4);
                    if (ks.empty() || !(ks.back() == k)) ks.push_back(k);
                }
                say("picking survivors by rank");
                for (const auto& k : ks) r.samples.push_back(unit_text(r.line, rk.unrank(k)));
            }
        }
    }
    catch (const std::exception& e)
    {
        r.error = in_table_terms(d, e.what());
    }
    return r;
}

std::string judge(const Doc& d, const TestResult& t, const std::string& text)
{
    if (!t.dfa) return "the filter does not compile yet";
    if (!t.line.alphabet) return "typed text is for filters of a text line";
    std::vector<uint32_t> u;
    for (char32_t cp : sieve::utf8_decode(text))
    {
        const auto dd = t.line.alphabet->digit_of(cp);
        if (!dd) return "'" + sieve::utf8_encode(std::u32string(1, cp)) + "' is not a symbol of " + t.line.symbols_id;
        u.push_back(*dd);
    }
    if (u.empty()) return "type some text to judge";
    // The separators, for naming the token a failure is in.
    std::vector<bool> sep(t.line.base, false);
    if (d.form == "tokens")
        for (char32_t cp : sieve::utf8_decode(d.separator))
            if (cp != U'"')
                if (const auto dd = t.line.alphabet->digit_of(cp)) sep[*dd] = true;
    auto token_at = [&](size_t i) {
        size_t a = i, b = i;
        while (a > 0 && !sep[u[a - 1]]) --a;
        while (b < u.size() && !sep[u[b]]) ++b;
        std::u32string s;
        for (size_t k = a; k < b; ++k) s += t.line.alphabet->symbol(u[k]);
        return "'" + sieve::utf8_encode(s) + "' (characters " + std::to_string(a + 1) + " to " + std::to_string(b) + ")";
    };
    int32_t s = t.dfa->start;
    for (size_t i = 0; i < u.size(); ++i)
    {
        const int32_t n = t.dfa->step(s, u[i]);
        if (n < 0)
        {
            if (d.form == "tokens")
            {
                if (sep[u[i]])
                    return i > 0 && !sep[u[i - 1]] ? "fails: the token " + token_at(i - 1) + " is not a word that can come there"
                                                   : "fails at character " + std::to_string(i + 1) + ": two separators in a row";
                return "fails: the token " + token_at(i) + " is not the start of a word that can come there";
            }
            return "fails at character " + std::to_string(i + 1) + ": no transition takes it from the state the text before leaves";
        }
        s = n;
    }
    if (t.dfa->accept[size_t(s)]) return "passes";
    if (d.form == "tokens")
        return u.empty() || sep[u.back()] ? "fails: it may not end with this token"
                                          : "fails: the last token " + token_at(u.size() - 1) + " is not a word that may end it";
    return "fails: it ends in a state that does not accept";
}

std::vector<std::string> relations(const Doc& d, const TestResult& t, Progress* progress)
{
    std::vector<std::string> out;
    if (!t.dfa) return out;
    static const sieve::cli::AppResources resources;
    std::vector<const sieve::FilterSpec*> others;
    for (const sieve::FilterSpec& other : sieve::plugin_registry())
        if (other.name() != d.name() && other.applies(t.line)) others.push_back(&other);
    size_t k = 0;
    for (const sieve::FilterSpec* op : others)
    {
        const sieve::FilterSpec& other = *op;
        if (progress) progress->set("comparing with " + other.name() + " (" + std::to_string(++k) + " of " + std::to_string(others.size()) + ")");
        try
        {
            const auto f = other.make(t.line, {}, resources);
            const sieve::Dfa* o = sieve::plugin_dfa(*f);
            if (!o) continue;
            const bool in = sieve::subset(*t.dfa, *o), out_ = sieve::subset(*o, *t.dfa);
            if (in && out_) out.push_back("the same rule as " + other.name() + ": a duplicate");
            else if (in) out.push_back("stricter than " + other.name() + " (everything it keeps, that keeps)");
            else if (out_) out.push_back("looser than " + other.name() + " (it keeps everything that keeps)");
        }
        catch (const std::exception&)
        {
        }
    }
    if (out.empty()) out.push_back("no other custom filter for this line is the same, stricter or looser");
    return out;
}

// ---------------------------------------------------------------- saving

fs::path filters_folder()
{
    const fs::path dir = sieve::cli::install_dir();
    return (dir.empty() ? fs::path(".") : dir) / "filters";
}

std::string save(const Doc& d, uint32_t& next_version)
{
    next_version = 0;
    const fs::path folder = filters_folder();
    std::error_code ec;
    fs::create_directories(folder, ec);
    const std::string text = d.to_text();
    // Check it compiles first (from the scratch folder, with its lists).
    try
    {
        (void)staged(d);
    }
    catch (const std::exception& e)
    {
        return std::string("not saved: ") + e.what();
    }
    const fs::path file = folder / from_u8(d.file_name());
    if (fs::exists(file, ec))
    {
        if (read_file(file) == text) return d.file_name() + " is saved already, exactly as it is";
        // A version is never edited: the next one free.
        uint32_t v = d.version + 1;
        while (fs::exists(folder / from_u8(d.id + "-v" + std::to_string(v) + ".sfilter"), ec) || sieve::find_filter(d.id + "-v" + std::to_string(v))) ++v;
        next_version = v;
        return d.file_name() + " exists with other contents, and a version is never edited: save as version " + std::to_string(v) + "?";
    }
    if (const sieve::FilterSpec* f = sieve::find_filter(d.name()); f && f->name() == d.name())
    {
        uint32_t v = d.version + 1;
        while (sieve::find_filter(d.id + "-v" + std::to_string(v)) || fs::exists(folder / from_u8(d.id + "-v" + std::to_string(v) + ".sfilter"), ec)) ++v;
        next_version = v;
        return d.name() + " is a filter already: save as version " + std::to_string(v) + "?";
    }
    // Word lists: a list another filter may use is never changed underneath it.
    for (const auto& [name, words] : d.lists)
    {
        const fs::path lp = folder / from_u8(name);
        if (fs::exists(lp, ec) && read_file(lp) != words) return "not saved: " + name + " exists in the filters folder with other words; give this list a new name";
    }
    // Tagged lists opened from elsewhere go beside it too; one of that name already there must be
    // the same file.
    std::vector<std::pair<fs::path, fs::path>> copies;
    for (const std::string& f : tag_files(d))
    {
        const fs::path from = data_folder_of(d) / from_u8(f), to = folder / from_u8(f);
        if (fs::equivalent(from, to, ec)) continue;
        if (fs::exists(to, ec))
        {
            if (read_file(to) != read_file(from)) return "not saved: " + f + " exists in the filters folder with other contents; give it a new name";
            continue;
        }
        copies.emplace_back(from, to);
    }
    for (const auto& [name, words] : d.lists)
    {
        std::ofstream out(folder / from_u8(name), std::ios::binary);
        out << words;
    }
    for (const auto& [from, to] : copies)
    {
        fs::copy_file(from, to, fs::copy_options::skip_existing, ec);
        if (ec) return "not saved: cannot copy " + u8(from.filename());
    }
    {
        std::ofstream out(file, std::ios::binary);
        if (!out) return "not saved: cannot write " + u8(file);
        out << text;
    }
    try
    {
        auto def = sieve::parse_plugin(text, sieve::Sha256::hex(sieve::Sha256::hash(text)), u8(folder));
        if (!sieve::add_plugin(sieve::plugin_spec(def))) return "saved to " + u8(file) + " (restart to use it: a filter of that name was already in use)";
    }
    catch (const std::exception& e)
    {
        return "saved to " + u8(file) + ", but it did not load: " + e.what();
    }
    return "saved to " + u8(file) + ": it is in the CUSTOM FILTERS tab now";
}

} // namespace hallway::design
