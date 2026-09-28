// Sieve — filter plugins (plugin.hpp): reading a sieve-filter-v1 file, compiling it for a line and a
// set of parameter values into a minimal automaton, and the filter that runs it.
//
// Parsing checks the file's shape once (header, parameters, the nesting of `for` and `done`);
// compiling runs its body with the parameters' values, much as a template is filled in, and
// reports anything wrong with the line number: a state out of range, a symbol the line does not
// have, a transition given two targets. Everything is integers, so the oracle compiles the same
// file to the same automaton.

#include "sieve/plugin.hpp"

#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <map>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include <stdexcept>

namespace sieve {

namespace {

struct Tok
{
    std::string text;
    bool quoted = false;
};

struct Stmt
{
    int line = 0;
    std::vector<Tok> toks;
    std::string rest; // the raw text after the keyword (for author, describe, a parameter's text)
};

[[noreturn]] void fail(int line, const std::string& what)
{
    throw std::invalid_argument("line " + std::to_string(line) + ": " + what);
}

bool is_space(char c) { return c == ' ' || c == '\t'; }

// One line into tokens: runs of non-spaces (spaces inside {braces} kept), "quoted" strings with
// \" \\ \n \t escapes; `;` outside quotes ends the line.
std::vector<Tok> tokenize(const std::string& s, int line)
{
    std::vector<Tok> out;
    size_t i = 0;
    while (i < s.size())
    {
        if (is_space(s[i])) { ++i; continue; }
        if (s[i] == ';') break;
        Tok t;
        if (s[i] == '"')
        {
            t.quoted = true;
            ++i;
            bool closed = false;
            while (i < s.size())
            {
                const char c = s[i++];
                if (c == '"') { closed = true; break; }
                if (c == '\\')
                {
                    if (i >= s.size()) fail(line, "a quoted string ends in a backslash");
                    const char e = s[i++];
                    t.text += e == 'n' ? '\n' : e == 't' ? '\t' : e;
                    if (e != 'n' && e != 't' && e != '"' && e != '\\') fail(line, std::string("unknown escape \\") + e);
                }
                else t.text += c;
            }
            if (!closed) fail(line, "a quoted string is not closed");
        }
        else
        {
            int depth = 0;
            while (i < s.size() && (depth > 0 || (!is_space(s[i]) && s[i] != ';' && s[i] != '"')))
            {
                if (s[i] == '{') ++depth;
                if (s[i] == '}' && --depth < 0) fail(line, "a } without its {");
                t.text += s[i++];
            }
            if (depth != 0) fail(line, "a { without its }");
        }
        out.push_back(std::move(t));
    }
    return out;
}

// The raw text after the first token (the keyword), less a comment and surrounding spaces.
std::string rest_after_keyword(const std::string& s)
{
    size_t i = 0;
    while (i < s.size() && is_space(s[i])) ++i;
    while (i < s.size() && !is_space(s[i])) ++i;
    std::string r = s.substr(i);
    const size_t semi = r.find(';');
    if (semi != std::string::npos) r.resize(semi);
    const size_t a = r.find_first_not_of(" \t"), b = r.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : r.substr(a, b - a + 1);
}

bool is_name(const std::string& s)
{
    if (s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    return std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
}

bool is_id(const std::string& s)
{
    if (s.empty() || !(s[0] >= 'a' && s[0] <= 'z')) return false;
    return std::all_of(s.begin(), s.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; });
}

// ---------------------------------------------------------------- expressions

using Env = std::map<std::string, int64_t>;

constexpr int64_t kLimit = int64_t(1) << 62; // every value stays within +-2^62

int64_t checked(int64_t v, int line)
{
    if (v > kLimit || v < -kLimit) fail(line, "a number is too large");
    return v;
}

class Expr
{
public:
    // v2: comparisons, && || !, min max abs (sieve-filter-v2); v1 reads exactly as it always has.
    Expr(const std::string& s, const Env& env, int line, bool v2 = false) : s_(s), env_(env), line_(line), v2_(v2) {}
    int64_t value()
    {
        const int64_t v = v2_ ? disjunction() : sum();
        skip();
        if (i_ != s_.size()) fail(line_, "cannot read the expression '" + s_ + "'");
        return v;
    }

private:
    void skip()
    {
        while (i_ < s_.size() && is_space(s_[i_])) ++i_;
    }
    bool at(const char* op)
    {
        skip();
        const size_t n = std::char_traits<char>::length(op);
        return s_.compare(i_, n, op) == 0;
    }
    int64_t disjunction()
    {
        int64_t v = conjunction();
        while (at("||"))
        {
            i_ += 2;
            const int64_t r = conjunction();
            v = (v != 0 || r != 0) ? 1 : 0;
        }
        return v;
    }
    int64_t conjunction()
    {
        int64_t v = comparison();
        while (at("&&"))
        {
            i_ += 2;
            const int64_t r = comparison();
            v = (v != 0 && r != 0) ? 1 : 0;
        }
        return v;
    }
    int64_t comparison()
    {
        int64_t v = sum();
        for (;;)
        {
            // Two-character operators before one.
            const char* ops[] = {"==", "!=", "<=", ">=", "<", ">"};
            const char* op = nullptr;
            for (const char* o : ops)
                if (at(o))
                {
                    op = o;
                    break;
                }
            if (!op) return v;
            i_ += std::char_traits<char>::length(op);
            const int64_t r = sum();
            const std::string o = op;
            v = o == "==" ? v == r : o == "!=" ? v != r : o == "<=" ? v <= r : o == ">=" ? v >= r : o == "<" ? v < r : v > r;
        }
    }
    int64_t sum()
    {
        int64_t v = product();
        for (;;)
        {
            skip();
            if (i_ < s_.size() && (s_[i_] == '+' || s_[i_] == '-'))
            {
                const char op = s_[i_++];
                const int64_t r = product();
                v = checked(op == '+' ? v + r : v - r, line_);
            }
            else return v;
        }
    }
    int64_t product()
    {
        int64_t v = unary();
        for (;;)
        {
            skip();
            if (i_ < s_.size() && (s_[i_] == '*' || s_[i_] == '/' || s_[i_] == '%'))
            {
                const char op = s_[i_++];
                const int64_t r = unary();
                if (op == '*')
                {
                    if (v != 0 && (r > kLimit / (v < 0 ? -v : v) || r < -kLimit / (v < 0 ? -v : v))) fail(line_, "a number is too large");
                    v = checked(v * r, line_);
                }
                else
                {
                    if (v < 0 || r <= 0) fail(line_, std::string(op == '/' ? "/" : "%") + " is for a number of 0 or more by one of 1 or more");
                    v = op == '/' ? v / r : v % r;
                }
            }
            else return v;
        }
    }
    int64_t unary()
    {
        skip();
        if (i_ < s_.size() && s_[i_] == '-')
        {
            ++i_;
            return checked(-unary(), line_);
        }
        if (v2_ && i_ < s_.size() && s_[i_] == '!' && !at("!="))
        {
            ++i_;
            return unary() == 0 ? 1 : 0;
        }
        return atom();
    }
    // min(a, b), max(a, b), abs(a): after the name, its arguments in brackets.
    int64_t call(const std::string& name)
    {
        skip();
        if (i_ >= s_.size() || s_[i_] != '(') fail(line_, name + " takes its arguments in brackets: " + name + "(...)");
        ++i_;
        std::vector<int64_t> args{disjunction()};
        while (at(","))
        {
            ++i_;
            args.push_back(disjunction());
        }
        skip();
        if (i_ >= s_.size() || s_[i_] != ')') fail(line_, "a ( without its ) in '" + s_ + "'");
        ++i_;
        const size_t want = name == "abs" ? 1 : 2;
        if (args.size() != want) fail(line_, name + " takes " + std::to_string(want) + (want == 1 ? " argument" : " arguments"));
        if (name == "abs") return checked(args[0] < 0 ? -args[0] : args[0], line_);
        return name == "min" ? std::min(args[0], args[1]) : std::max(args[0], args[1]);
    }
    int64_t atom()
    {
        skip();
        if (i_ >= s_.size()) fail(line_, "an expression ends too soon: '" + s_ + "'");
        const char c = s_[i_];
        if (c == '(')
        {
            ++i_;
            const int64_t v = v2_ ? disjunction() : sum();
            skip();
            if (i_ >= s_.size() || s_[i_] != ')') fail(line_, "a ( without its ) in '" + s_ + "'");
            ++i_;
            return v;
        }
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            int64_t v = 0;
            while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_])))
            {
                v = v * 10 + (s_[i_++] - '0');
                if (v > kLimit) fail(line_, "a number is too large");
            }
            return v;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            std::string name;
            while (i_ < s_.size() && (std::isalnum(static_cast<unsigned char>(s_[i_])) || s_[i_] == '_')) name += s_[i_++];
            if (v2_ && (name == "min" || name == "max" || name == "abs")) return call(name);
            const auto it = env_.find(name);
            if (it == env_.end()) fail(line_, "'" + name + "' is not a parameter or a for variable");
            return it->second;
        }
        fail(line_, "cannot read the expression '" + s_ + "'");
    }

    const std::string& s_;
    const Env& env_;
    int line_;
    bool v2_ = false;
    size_t i_ = 0;
};

// Names a v2 file keeps for itself: the functions and the line's constants.
bool reserved_v2(const std::string& n)
{
    return n == "min" || n == "max" || n == "abs" || n == "BASE" || n == "PITCHES" || n == "DURATIONS" || n == "LOW";
}

// A number field: an integer, a name, or {an expression}.
int64_t eval(const Tok& t, const Env& env, int line, bool v2 = false)
{
    if (t.quoted) fail(line, "expected a number, got a quoted string");
    std::string s = t.text;
    if (s.size() >= 2 && s.front() == '{' && s.back() == '}') s = s.substr(1, s.size() - 2);
    else if (s.find_first_of("{}") != std::string::npos) fail(line, "an expression must be all inside {braces}: '" + t.text + "'");
    return Expr(s, env, line, v2).value();
}

// "a..b" into its two halves (outside braces), or nothing.
bool split_range(const std::string& s, std::string& a, std::string& b)
{
    int depth = 0;
    for (size_t i = 0; i + 1 < s.size(); ++i)
    {
        if (s[i] == '{') ++depth;
        if (s[i] == '}') --depth;
        if (depth == 0 && s[i] == '.' && s[i + 1] == '.')
        {
            a = s.substr(0, i);
            b = s.substr(i + 2);
            return true;
        }
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------- the parsed file

struct PluginDef
{
    PluginHeader header;
    std::vector<Stmt> body;          // class, states, start, accept, t, for, done (in order)
    std::vector<size_t> done_of;     // for a `for` at body[i], the index of its `done`; for an `if`, its `fi`
    std::vector<size_t> else_of;     // for an `if` at body[i], the index of its `else`, or of its `fi`
    bool v2 = false;                 // sieve-filter-v2
    // The token form.
    struct WordSet
    {
        std::string name, kind, source; // kind "dict" or "list"
        int line = 0;
    };
    std::vector<Tok> separator;
    int separator_line = 0;
    bool cut = false;
    std::vector<WordSet> sets;
    std::vector<std::pair<std::string, std::string>> follows;
    std::vector<std::string> first, last;
    bool has_first = false, has_last = false;
    std::string folder;
};

const PluginHeader& plugin_header(const PluginDef& p) { return p.header; }

std::shared_ptr<const PluginDef> parse_plugin(const std::string& text, const std::string& sha256, const std::string& folder)
{
    auto def = std::make_shared<PluginDef>();
    PluginHeader& h = def->header;
    h.sha256 = sha256;
    def->folder = folder;
    bool token_form = false, table_form = false, has_edges = false;
    std::set<std::string> set_names;
    std::istringstream in(text);
    std::string raw;
    int line = 0;
    bool first = true, ended = false, has_states = false, has_start = false, has_version = false;
    std::vector<size_t> open_for;
    std::vector<std::pair<size_t, size_t>> pairs; // each for, and its done; each if, and its fi
    std::map<size_t, size_t> else_at;             // each if with an else, and its else
    std::set<std::string> seen_header, class_names;
    while (std::getline(in, raw))
    {
        ++line;
        if (!raw.empty() && raw.back() == '\r') fail(line, "line breaks must be line feeds only");
        const size_t first_char = raw.find_first_not_of(" \t");
        if (first_char == std::string::npos || raw[first_char] == '#' || raw[first_char] == ';') continue;
        const std::vector<Tok> toks = tokenize(raw, line);
        if (toks.empty()) continue;
        if (ended) fail(line, "nothing may follow end");
        if (first)
        {
            if (toks.size() != 1 || toks[0].quoted || (toks[0].text != kPluginFormat && toks[0].text != kPluginFormat2))
                fail(line, std::string("a plugin starts with ") + kPluginFormat + " or " + kPluginFormat2);
            def->v2 = toks[0].text == kPluginFormat2;
            h.format = def->v2 ? 2 : 1;
            first = false;
            continue;
        }
        if (toks[0].quoted) fail(line, "a line starts with a keyword");
        const std::string& k = toks[0].text;
        auto want = [&](size_t n) {
            if (toks.size() != n) fail(line, k + " takes " + std::to_string(n - 1) + (n == 2 ? " value" : " values"));
        };
        auto once = [&]() {
            if (!seen_header.insert(k).second) fail(line, k + " is given twice");
        };
        if (k == "id")
        {
            once();
            want(2);
            if (!is_id(toks[1].text)) fail(line, "an id is lower-case letters, digits and hyphens, starting with a letter");
            h.id = toks[1].text;
        }
        else if (k == "version")
        {
            once();
            want(2);
            const std::string& v = toks[1].text;
            if (v.empty() || v.size() > 9 || !std::all_of(v.begin(), v.end(), [](char c) { return c >= '0' && c <= '9'; }) || v[0] == '0')
                fail(line, "a version is a whole number from 1");
            h.version = uint32_t(std::stoul(v));
            has_version = true;
        }
        else if (k == "author")
        {
            once();
            h.author = rest_after_keyword(raw);
            if (h.author.empty()) fail(line, "author needs a name");
        }
        else if (k == "origin")
        {
            once();
            want(2);
            if (toks[1].text != "human" && toks[1].text != "ai-directed" && toks[1].text != "ai") fail(line, "origin is human, ai-directed or ai");
            h.origin = toks[1].text;
        }
        else if (k == "lines")
        {
            once();
            if (toks.size() < 2) fail(line, "lines needs at least one line");
            for (size_t i = 1; i < toks.size(); ++i)
            {
                const std::string& l = toks[i].text;
                if (l != "text" && l != "image" && l != "video" && l != "audio") fail(line, "a line is text, image, video or audio");
                h.lines.push_back(l);
            }
        }
        else if (k == "symbols")
        {
            once();
            want(2);
            h.symbols = toks[1].text;
            if (h.symbols.find('*') != std::string::npos && h.symbols != "any" &&
                (!def->v2 || h.symbols.back() != '*' || h.symbols.find('*') + 1 != h.symbols.size() || h.symbols.size() < 2))
                fail(line, def->v2 ? "a family of symbols ends in one * (notes*)" : "symbols ending in * need sieve-filter-v2");
        }
        else if (k == "describe")
        {
            once();
            h.describe = rest_after_keyword(raw);
        }
        else if (k == "requires")
        {
            // A filter by its full name (<id>-v<version>), then any settings it pins, NAME=VALUE.
            if (toks.size() < 2) fail(line, "requires NAME-vN [SETTING=VALUE ...]");
            FilterSpec::Prerequisite r;
            r.name = toks[1].text;
            const size_t v = r.name.rfind("-v");
            if (toks[1].quoted || v == std::string::npos || v == 0 || v + 2 >= r.name.size() ||
                !std::all_of(r.name.begin() + long(v) + 2, r.name.end(), [](char c) { return c >= '0' && c <= '9'; }))
                fail(line, "requires names a filter with its version, as <id>-v<n>");
            for (const auto& q : h.prerequisites)
                if (q.name == r.name) fail(line, r.name + " is required twice");
            for (size_t i = 2; i < toks.size(); ++i)
            {
                const size_t eq = toks[i].text.find('=');
                if (toks[i].quoted || eq == std::string::npos || eq == 0 || eq + 1 >= toks[i].text.size())
                    fail(line, "a pinned setting is NAME=VALUE: '" + toks[i].text + "'");
                const std::string key = toks[i].text.substr(0, eq);
                if (r.values.count(key)) fail(line, key + " is pinned twice");
                r.values[key] = toks[i].text.substr(eq + 1);
            }
            h.prerequisites.push_back(std::move(r));
        }
        else if (k == "param")
        {
            if (toks.size() < 4 || (toks.size() < 6 && toks[2].text != "dict" && toks[2].text != "choice"))
                fail(line, def->v2 ? "param NAME int DEFAULT MIN MAX [text], param NAME choice DEFAULT A,B,C [text], or param NAME dict DEFAULT [text]"
                                   : "param NAME int DEFAULT MIN MAX [text], or param NAME dict DEFAULT [text]");
            FilterParam p;
            p.key = toks[1].text;
            if (!is_name(p.key)) fail(line, "a parameter's name is letters, digits and _, starting with a letter");
            if (def->v2 && reserved_v2(p.key)) fail(line, p.key + " is kept for the format (a function or a constant of the line)");
            for (const auto& q : h.params)
                if (q.key == p.key) fail(line, "parameter " + p.key + " is declared twice");
            if (toks[2].text == "choice")
            {
                // param NAME choice DEFAULT A,B,C [text]: one of a list, by name (v2).
                if (!def->v2) fail(line, "a choice parameter needs sieve-filter-v2");
                if (toks.size() < 5) fail(line, "param NAME choice DEFAULT A,B,C [text]");
                p.kind = FilterParam::Kind::Text;
                std::string list = toks[4].text;
                size_t at = 0;
                while (at <= list.size())
                {
                    const size_t comma = std::min(list.find(',', at), list.size());
                    const std::string c = list.substr(at, comma - at);
                    if (c.empty() || !std::all_of(c.begin(), c.end(), [](char ch) { return std::isalnum(static_cast<unsigned char>(ch)) || ch == '#' || ch == '-'; }))
                        fail(line, "a choice is letters, digits, # and -: '" + c + "'");
                    if (std::find(p.choices.begin(), p.choices.end(), c) != p.choices.end()) fail(line, "choice " + c + " is listed twice");
                    p.choices.push_back(c);
                    at = comma + 1;
                }
                if (std::find(p.choices.begin(), p.choices.end(), toks[3].text) == p.choices.end())
                    fail(line, "the default " + toks[3].text + " is not one of the choices");
                p.default_value = toks[3].text;
                std::string desc;
                for (size_t i = 5; i < toks.size(); ++i) desc += (desc.empty() ? "" : " ") + toks[i].text;
                p.description = desc;
                h.params.push_back(p);
                continue;
            }
            if (toks[2].text == "dict")
            {
                // A dictionary setting: a registered id, or `default` (the registry's default).
                if (toks.size() < 4) fail(line, "param NAME dict DEFAULT [text]");
                p.kind = FilterParam::Kind::Text;
                p.registry = "dictionary";
                p.default_value = toks[3].text == "default" ? "" : toks[3].text;
                std::string desc;
                for (size_t i = 4; i < toks.size(); ++i) desc += (desc.empty() ? "" : " ") + toks[i].text;
                p.description = desc;
                h.params.push_back(p);
                continue;
            }
            if (toks[2].text != "int") fail(line, def->v2 ? "a parameter's kind is int, choice or dict" : "a parameter's kind is int or dict");
            p.kind = FilterParam::Kind::Integer;
            const Env none;
            const int64_t dflt = eval(toks[3], none, line, def->v2), lo = eval(toks[4], none, line, def->v2), hi = eval(toks[5], none, line, def->v2);
            if (lo > hi || dflt < lo || dflt > hi) fail(line, "a parameter's default must lie between its minimum and maximum");
            p.default_value = std::to_string(dflt);
            p.min = lo;
            p.max = hi;
            std::string desc;
            for (size_t i = 6; i < toks.size(); ++i) desc += (desc.empty() ? "" : " ") + toks[i].text;
            p.description = desc;
            h.params.push_back(p);
        }
        else if (k == "tokens")
        {
            once();
            token_form = true;
            if (toks.size() < 3 || toks[1].text != "separator") fail(line, "tokens separator SYMBOLS...");
            def->separator.assign(toks.begin() + 2, toks.end());
            def->separator_line = line;
        }
        else if (k == "edges")
        {
            once();
            want(2);
            if (toks[1].text != "whole" && toks[1].text != "cut") fail(line, "edges is whole or cut");
            def->cut = toks[1].text == "cut";
            has_edges = true;
        }
        else if (k == "set")
        {
            want(3);
            token_form = true;
            if (!is_name(toks[1].text)) fail(line, "a set's name is letters, digits and _, starting with a letter");
            if (!set_names.insert(toks[1].text).second) fail(line, "set " + toks[1].text + " is declared twice");
            if (set_names.size() > 62) fail(line, "at most 62 sets");
            const std::string& src = toks[2].text;
            const size_t colon = src.find(':');
            const std::string kind = colon == std::string::npos ? "" : src.substr(0, colon);
            if (toks[2].quoted || (kind != "dict" && kind != "list" && kind != "tags") || colon + 1 >= src.size())
                fail(line, "a set's words come from dict:ID, dict:{PARAM}, list:FILE or tags:FILE:TAGS");
            if (kind == "tags")
            {
                const size_t last = src.rfind(':');
                if (last <= colon + 1 || last + 1 >= src.size()) fail(line, "a tagged set is tags:FILE:TAGS (the tags a word must carry one of)");
            }
            def->sets.push_back({toks[1].text, kind, src.substr(colon + 1), line});
        }
        else if (k == "follow")
        {
            want(3);
            token_form = true;
            def->follows.emplace_back(toks[1].text, toks[2].text);
        }
        else if (k == "first" || k == "last")
        {
            once();
            token_form = true;
            if (toks.size() < 2) fail(line, k + " names at least one set");
            auto& v = k == "first" ? def->first : def->last;
            for (size_t i = 1; i < toks.size(); ++i) v.push_back(toks[i].text);
            (k == "first" ? def->has_first : def->has_last) = true;
        }
        else if (k == "class")
        {
            if (!open_for.empty()) fail(line, "a class is declared outside any for");
            if (toks.size() < 3) fail(line, "class NAME SYMBOLS...");
            if (!is_name(toks[1].text)) fail(line, "a class's name is letters, digits and _, starting with a letter");
            if (!class_names.insert(toks[1].text).second) fail(line, "class " + toks[1].text + " is declared twice");
            def->body.push_back({line, toks, {}});
        }
        else if (k == "states")
        {
            if (!open_for.empty() || has_states) fail(line, "states is given once, outside any for");
            want(2);
            has_states = true;
            def->body.push_back({line, toks, {}});
        }
        else if (k == "start")
        {
            if (!open_for.empty() || has_start) fail(line, "start is given once, outside any for");
            if (!has_states) fail(line, "states comes before start");
            want(2);
            has_start = true;
            def->body.push_back({line, toks, {}});
        }
        else if (k == "accept")
        {
            if (!has_states) fail(line, "states comes before accept");
            if (toks.size() < 2) fail(line, "accept needs at least one state");
            def->body.push_back({line, toks, {}});
        }
        else if (k == "t")
        {
            if (!has_states) fail(line, "states comes before any transition");
            want(4);
            def->body.push_back({line, toks, {}});
        }
        else if (k == "for")
        {
            want(4);
            if (!is_name(toks[1].text)) fail(line, "a for variable's name is letters, digits and _, starting with a letter");
            if (def->v2 && reserved_v2(toks[1].text)) fail(line, toks[1].text + " is kept for the format (a function or a constant of the line)");
            open_for.push_back(def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (k == "done")
        {
            want(1);
            if (open_for.empty() || def->body[open_for.back()].toks[0].text != "for") fail(line, "done without its for");
            const size_t f = open_for.back();
            open_for.pop_back();
            pairs.emplace_back(f, def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (def->v2 && k == "if")
        {
            // if {EXPR} ... [else ...] fi (v2): blocks share the stack with for, so they nest.
            want(2);
            if (!has_states) fail(line, "states comes before any if");
            open_for.push_back(def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (def->v2 && k == "else")
        {
            want(1);
            if (open_for.empty() || def->body[open_for.back()].toks[0].text != "if") fail(line, "else without its if");
            if (else_at.count(open_for.back())) fail(line, "an if has one else");
            else_at[open_for.back()] = def->body.size();
            def->body.push_back({line, toks, {}});
        }
        else if (def->v2 && k == "fi")
        {
            want(1);
            if (open_for.empty() || def->body[open_for.back()].toks[0].text != "if") fail(line, "fi without its if");
            const size_t f = open_for.back();
            open_for.pop_back();
            pairs.emplace_back(f, def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (k == "end")
        {
            want(1);
            if (!open_for.empty())
                fail(line, def->body[open_for.back()].toks[0].text == "if" ? "an if is not closed by fi" : "a for is not closed by done");
            ended = true;
        }
        else fail(line, "unknown keyword '" + k + "'");
    }
    if (first) throw std::invalid_argument(std::string("empty: a plugin starts with ") + kPluginFormat);
    if (!ended) fail(line, "a plugin ends with end");
    const char* required[] = {"id", "version", "author", "origin", "lines", "symbols"};
    for (const char* r : required)
        if (!seen_header.count(r)) throw std::invalid_argument(std::string("the header has no ") + r);
    if (!has_version) throw std::invalid_argument("the header has no version");
    table_form = !def->body.empty();
    if (token_form && table_form) throw std::invalid_argument("a plugin is a table (states, transitions) or tokens (sets), not both");
    if (token_form)
    {
        h.form = "tokens";
        if (def->separator.empty()) throw std::invalid_argument("the token form needs tokens separator SYMBOLS");
        if (!has_edges) throw std::invalid_argument("the token form needs edges whole or edges cut");
        if (def->sets.empty()) throw std::invalid_argument("the token form needs at least one set");
        auto known = [&](const std::string& s) {
            if (!set_names.count(s)) throw std::invalid_argument("'" + s + "' is not a set");
        };
        for (const auto& [a, b] : def->follows)
        {
            known(a);
            known(b);
        }
        for (const auto& s : def->first) known(s);
        for (const auto& s : def->last) known(s);
        for (const auto& s : def->sets)
            if (s.kind == "dict" && s.source.size() > 2 && s.source.front() == '{')
            {
                const std::string pn = s.source.substr(1, s.source.size() - 2);
                const bool ok = std::any_of(h.params.begin(), h.params.end(), [&](const FilterParam& q) { return q.key == pn && q.registry == "dictionary"; });
                if (s.source.back() != '}' || !ok) fail(s.line, "dict:{" + pn + "} needs a dict parameter called " + pn);
            }
        return def;
    }
    h.form = "table";
    if (!has_states || !has_start) throw std::invalid_argument("a table plugin needs states and start");
    def->done_of.assign(def->body.size(), 0);
    def->else_of.assign(def->body.size(), 0);
    for (const auto& [f, d] : pairs)
    {
        def->done_of[f] = d;
        def->else_of[f] = else_at.count(f) ? else_at[f] : d;
    }
    for (const auto& p : h.params)
        if (class_names.count(p.key)) throw std::invalid_argument("parameter " + p.key + " has the same name as a class");
    return def;
}

bool plugin_applies(const PluginDef& p, const FilterLine& line)
{
    const PluginHeader& h = p.header;
    if (std::find(h.lines.begin(), h.lines.end(), line.kind) == h.lines.end()) return false;
    if (line.base == 0 || line.base > 65536) return false; // a table of states x symbols must stay small
    if (h.symbols == "any") return true;
    if (h.format >= 2 && h.symbols.size() >= 2 && h.symbols.back() == '*') // a family: notes*
        return line.symbols_id.compare(0, h.symbols.size() - 1, h.symbols, 0, h.symbols.size() - 1) == 0;
    if (h.symbols.rfind("palette:", 0) == 0)
    {
        if (line.kind != "image" && line.kind != "video") return false;
        const std::string want = h.symbols.substr(8);
        const size_t a = line.symbols_id.find('/'), b = line.symbols_id.find('/', a == std::string::npos ? 0 : a + 1);
        return a != std::string::npos && b != std::string::npos && line.symbols_id.substr(a + 1, b - a - 1) == want;
    }
    return line.symbols_id == h.symbols;
}

// ---------------------------------------------------------------- compiling

namespace {

class Compiler
{
public:
    Compiler(const PluginDef& p, const FilterLine& line, const FilterValues& values) : p_(p), line_(line)
    {
        if (p.v2)
        {
            // The line's constants (v2).
            env_["BASE"] = int64_t(line.base);
            if (is_note_symbols(line.symbols_id))
            {
                // notes104 (25, 4, 60) or a notes2 set: one voice's events (the stack gives a
                // plugin one voice at a time).
                const NoteSet set = note_set_of(line.symbols_id);
                env_["PITCHES"] = int64_t(set.pitches());
                env_["DURATIONS"] = int64_t(set.duration_count());
                env_["LOW"] = int64_t(set.low);
            }
        }
        for (const auto& par : p.header.params)
        {
            if (par.kind == FilterParam::Kind::Text && !par.choices.empty() && p.v2)
            {
                // A choice: its place in the list.
                const auto it = values.find(par.key);
                const std::string v = it == values.end() ? par.default_value : it->second;
                const auto c = std::find(par.choices.begin(), par.choices.end(), v);
                if (c == par.choices.end()) throw std::invalid_argument(p.header.name() + ": " + par.key + " must be one of its choices, got '" + v + "'");
                env_[par.key] = int64_t(c - par.choices.begin());
                continue;
            }
            if (par.kind != FilterParam::Kind::Integer) continue;
            const auto it = values.find(par.key);
            int64_t v = 0;
            if (it == values.end()) v = std::stoll(par.default_value);
            else
            {
                size_t used = 0;
                try { v = std::stoll(it->second, &used); } catch (const std::exception&) { used = 0; }
                if (it->second.empty() || used != it->second.size())
                    throw std::invalid_argument(p.header.name() + ": " + par.key + " must be a whole number, got '" + it->second + "'");
            }
            if (v < par.min || v > par.max)
                throw std::invalid_argument(p.header.name() + ": " + par.key + " must be " + std::to_string(par.min) + ".." + std::to_string(par.max));
            env_[par.key] = v;
        }
    }

    Dfa run(size_t* declared)
    {
        B_ = line_.base;
        classes();
        run_block(0, p_.body.size());
        if (N_ < 0 || start_ < 0) throw std::invalid_argument("a table plugin needs states and start");
        if (declared) *declared = size_t(N_);
        Dfa d;
        d.base = B_;
        d.start = int32_t(start_);
        d.next = std::move(next_);
        d.accept = std::move(accept_);
        return d;
    }

private:
    // A symbol of the line by code point (text alphabets only).
    uint32_t symbol_of(char32_t cp, int line) const
    {
        if (!line_.alphabet) fail(line, "a quoted or lettered symbol needs a text line (use @n for a symbol by its digit)");
        const auto d = line_.alphabet->digit_of(cp);
        if (!d) fail(line, "U+" + hex(cp) + " is not a symbol of " + line_.symbols_id);
        return *d;
    }
    static std::string hex(char32_t cp)
    {
        char b[16];
        std::snprintf(b, sizeof b, "%04X", unsigned(cp));
        return b;
    }
    uint32_t digit(const std::string& expr, int line) const
    {
        const int64_t v = eval(Tok{expr, false}, env_, line, p_.v2);
        if (v < 0 || v >= int64_t(B_)) fail(line, "@" + std::to_string(v) + " is not a symbol of this line (it has " + std::to_string(B_) + ")");
        return uint32_t(v);
    }

    // One class item's symbols.
    void item(const Tok& t, int line, std::set<uint32_t>& out) const
    {
        if (t.quoted)
        {
            for (char32_t cp : utf8_decode(t.text)) out.insert(symbol_of(cp, line));
            return;
        }
        const std::string& s = t.text;
        if (!s.empty() && s[0] == '@')
        {
            std::string a, b;
            if (split_range(s, a, b))
            {
                if (b.empty() || b[0] != '@') fail(line, "a range of digits is @a..@b");
                const uint32_t lo = digit(a.substr(1), line), hi = digit(b.substr(1), line);
                if (lo > hi) fail(line, "a range runs upwards: " + s);
                for (uint32_t d = lo; d <= hi; ++d) out.insert(d);
            }
            else out.insert(digit(s.substr(1), line));
            return;
        }
        const std::u32string u = utf8_decode(s);
        if (u.size() == 3 && u[1] == U'-')
        {
            if (u[0] > u[2]) fail(line, "a range runs upwards: " + s);
            for (char32_t cp = u[0]; cp <= u[2]; ++cp) out.insert(symbol_of(cp, line));
            return;
        }
        fail(line, "cannot read the symbols '" + s + "' (use \"quotes\", a-z, @n, @a..@b or *)");
    }

    void classes()
    {
        const Stmt* rest = nullptr;
        std::map<uint32_t, std::string> owner;
        for (const Stmt& st : p_.body)
        {
            if (st.toks[0].text != "class") continue;
            const std::string& name = st.toks[1].text;
            if (st.toks.size() == 3 && !st.toks[2].quoted && st.toks[2].text == "*")
            {
                if (rest) fail(st.line, "only one class may be *");
                rest = &st;
                continue;
            }
            std::set<uint32_t> syms;
            for (size_t i = 2; i < st.toks.size(); ++i)
            {
                if (!st.toks[i].quoted && st.toks[i].text == "*") fail(st.line, "* stands alone in its class");
                item(st.toks[i], st.line, syms);
            }
            for (uint32_t d : syms)
            {
                const auto [it, added] = owner.emplace(d, name);
                if (!added) fail(st.line, "symbol @" + std::to_string(d) + " is in both " + it->second + " and " + name);
            }
            classes_[name].assign(syms.begin(), syms.end());
        }
        if (rest)
        {
            std::vector<uint32_t> r;
            for (uint32_t d = 0; d < B_; ++d)
                if (!owner.count(d)) r.push_back(d);
            classes_[rest->toks[1].text] = r;
        }
    }

    int64_t state(const Tok& t, int line) const
    {
        const int64_t v = eval(t, env_, line, p_.v2);
        if (v < 0 || v >= N_) fail(line, "state " + std::to_string(v) + " does not exist (there are " + std::to_string(N_) + ")");
        return v;
    }

    void tick(int line)
    {
        if (++steps_ > 50'000'000) fail(line, "the plugin runs too many lines (over 50 million)");
    }

    void run_block(size_t from, size_t to)
    {
        for (size_t i = from; i < to; ++i)
        {
            const Stmt& st = p_.body[i];
            const std::string& k = st.toks[0].text;
            tick(st.line);
            if (k == "class" || k == "done" || k == "fi") continue;
            if (k == "if")
            {
                // The lines to its else (or fi) when true, from its else to its fi when not.
                const bool yes = eval(st.toks[1], env_, st.line, p_.v2) != 0;
                const size_t els = p_.else_of[i], fi = p_.done_of[i];
                if (yes) run_block(i + 1, els);
                else if (els != fi) run_block(els + 1, fi);
                i = fi;
                continue;
            }
            if (k == "states")
            {
                N_ = eval(st.toks[1], env_, st.line, p_.v2);
                if (N_ < 1 || N_ > 2'000'000) fail(st.line, "states must be 1 to 2,000,000");
                if (double(N_) * double(B_) > 2e8) fail(st.line, "states x symbols is over 200 million: too large a table");
                next_.assign(size_t(N_) * B_, Dfa::kDead);
                accept_.assign(size_t(N_), 0);
            }
            else if (k == "start") start_ = state(st.toks[1], st.line);
            else if (k == "accept")
            {
                for (size_t j = 1; j < st.toks.size(); ++j)
                {
                    std::string a, b;
                    if (!st.toks[j].quoted && split_range(st.toks[j].text, a, b))
                    {
                        const int64_t lo = state(Tok{a, false}, st.line), hi = state(Tok{b, false}, st.line);
                        if (lo > hi) fail(st.line, "a range runs upwards: " + st.toks[j].text);
                        for (int64_t s = lo; s <= hi; ++s) accept_[size_t(s)] = 1;
                    }
                    else accept_[size_t(state(st.toks[j], st.line))] = 1;
                }
            }
            else if (k == "t")
            {
                const int64_t from_s = state(st.toks[1], st.line), to_s = state(st.toks[3], st.line);
                const Tok& sym = st.toks[2];
                // A class by name, or symbols written as in a class: "x", a-z, @n, @a..@b.
                std::vector<uint32_t> own;
                const std::vector<uint32_t>* syms = nullptr;
                if (const auto it = classes_.find(sym.text); !sym.quoted && it != classes_.end()) syms = &it->second;
                else
                {
                    if (!sym.quoted && is_name(sym.text)) fail(st.line, "'" + sym.text + "' is not a class");
                    std::set<uint32_t> s;
                    item(sym, st.line, s);
                    own.assign(s.begin(), s.end());
                    syms = &own;
                }
                for (uint32_t c : *syms)
                {
                    int32_t& cell = next_[size_t(from_s) * B_ + c];
                    if (cell != Dfa::kDead && cell != int32_t(to_s))
                        fail(st.line, "state " + std::to_string(from_s) + " on symbol @" + std::to_string(c) + " goes to both " + std::to_string(cell) +
                                          " and " + std::to_string(to_s));
                    cell = int32_t(to_s);
                }
            }
            else if (k == "for")
            {
                const std::string& var = st.toks[1].text;
                if (env_.count(var)) fail(st.line, "'" + var + "' is already a parameter or a for variable");
                const int64_t lo = eval(st.toks[2], env_, st.line, p_.v2), hi = eval(st.toks[3], env_, st.line, p_.v2);
                const size_t done = p_.done_of[i];
                for (int64_t v = lo; v <= hi; ++v)
                {
                    env_[var] = v;
                    run_block(i + 1, done);
                    tick(st.line);
                }
                env_.erase(var);
                i = done;
            }
        }
    }

    const PluginDef& p_;
    const FilterLine& line_;
    Env env_;
    uint32_t B_ = 0;
    int64_t N_ = -1, start_ = -1;
    uint64_t steps_ = 0;
    std::map<std::string, std::vector<uint32_t>> classes_;
    std::vector<int32_t> next_;
    std::vector<uint8_t> accept_;
};

// ---------------------------------------------------------------- the token form

// Words as the line's symbols, in one trie; each node knows which sets it ends a word of. The DFA
// is made directly (a subset construction folded in): between tokens the state is the set of
// readings the last token can have; inside a token it is the trie node and the readings the
// token before it could have. With `edges cut`, a token touching the unit's start runs through a
// suffix automaton of the words instead, so it may be any suffix (or, touching both ends, any
// substring) of a word.
class TokenCompiler
{
public:
    TokenCompiler(const PluginDef& p, const FilterLine& line, const FilterValues& values, const FilterResources& res,
                  const std::function<void(const std::string&)>& step)
        : p_(p), line_(line), values_(values), res_(res), B_(line.base), step_(step)
    {
    }

    Dfa run(size_t* declared, std::string* data)
    {
        separators();
        say("reading the word sets");
        words(data);
        distinct_words_ = size_t(std::count_if(ends_.begin(), ends_.end(), [](uint64_t e) { return e != 0; }));
        say("sharing the words' endings (" + std::to_string(kids_.size()) + " trie nodes)");
        share_endings();
        relations();
        if (p_.cut)
        {
            say("building the suffix automaton for cut edges (" + std::to_string(all_words_.size()) + " words)");
            build_dawg();
        }
        say("building the automaton from " + std::to_string(all_words_.size()) + " words (" + std::to_string(kids_.size()) + " word nodes)");
        return build(declared);
    }

private:
    static constexpr uint64_t kStart = uint64_t(1) << 63; // "no token yet", as a reading

    uint32_t symbol(char32_t cp, bool& ok) const
    {
        const auto d = line_.alphabet ? line_.alphabet->digit_of(cp) : std::nullopt;
        ok = d.has_value();
        return d ? *d : 0;
    }

    void separators()
    {
        if (!line_.alphabet) fail(p_.separator_line, "the token form needs a text line");
        sep_.assign(B_, 0);
        for (const Tok& t : p_.separator)
        {
            std::u32string u = t.quoted ? utf8_decode(t.text) : std::u32string();
            if (!t.quoted)
            {
                const std::u32string r = utf8_decode(t.text);
                if (r.size() == 3 && r[1] == U'-')
                    for (char32_t c = r[0]; c <= r[2]; ++c) u.push_back(c);
                else fail(p_.separator_line, "separator symbols are \"quoted\" or a range a-z");
            }
            for (char32_t cp : u)
            {
                bool ok = false;
                const uint32_t d = symbol(cp, ok);
                if (!ok) fail(p_.separator_line, "a separator is not a symbol of " + line_.symbols_id);
                sep_[d] = 1;
            }
        }
    }

    int32_t child(int32_t n, uint32_t c) const
    {
        for (const auto& [s, t] : kids_[size_t(n)])
            if (s == c) return t;
        return -1;
    }

    void add_word(const std::vector<uint32_t>& w, size_t set)
    {
        int32_t n = 0;
        for (uint32_t c : w)
        {
            int32_t t = child(n, c);
            if (t < 0)
            {
                t = int32_t(kids_.size());
                kids_[size_t(n)].emplace_back(c, t);
                kids_.emplace_back();
                ends_.push_back(0);
            }
            n = t;
        }
        ends_[size_t(n)] |= uint64_t(1) << set;
        all_words_.push_back(w);
    }

    std::string read_list(const std::string& file, int line) const
    {
        const std::string path = (p_.folder.empty() ? std::string() : p_.folder + "/") + file;
        std::ifstream in(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
        if (!in) fail(line, "cannot read " + file);
        std::ostringstream o;
        o << in.rdbuf();
        return o.str();
    }

    // A word in the line's symbols: as it is, or lower-cased (A-Z and the Latin-1 capitals) on a
    // line with no capitals. False when it cannot be spelled, or holds a separator.
    bool spell(const std::string& w, std::vector<uint32_t>& digits) const
    {
        const bool fold = !line_.alphabet->has_uppercase();
        digits.clear();
        for (char32_t cp : utf8_decode(w))
        {
            bool ok = false;
            uint32_t d = symbol(cp, ok);
            if (!ok && fold && ((cp >= U'A' && cp <= U'Z') || (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7))) d = symbol(cp + 32, ok);
            if (!ok || sep_[d]) return false;
            digits.push_back(d);
        }
        return !digits.empty();
    }

    // The trie shrunk to the smallest graph with the same words and the same sets at each word's
    // end (words that end alike share their endings): bottom up, a node is the same as another
    // with the same sets and the same children. The grammar's states are multiplied by this, so
    // it matters: Moby's 180,000 words are about 450,000 trie nodes, and a few tens of thousands
    // after this.
    void share_endings()
    {
        const size_t n = kids_.size();
        std::vector<int32_t> canon(n, -1);
        std::vector<std::vector<std::pair<uint32_t, int32_t>>> nk;
        std::vector<uint64_t> ne;
        std::map<std::vector<uint64_t>, int32_t> seen;
        // Post-order without recursion (words can be long).
        std::vector<std::pair<int32_t, size_t>> stack{{0, 0}};
        while (!stack.empty())
        {
            auto& [node, k] = stack.back();
            if (k < kids_[size_t(node)].size())
            {
                const int32_t c = kids_[size_t(node)][k++].second;
                stack.emplace_back(c, 0);
                continue;
            }
            auto ch = kids_[size_t(node)];
            for (auto& e : ch) e.second = canon[size_t(e.second)];
            std::sort(ch.begin(), ch.end());
            std::vector<uint64_t> sig{ends_[size_t(node)]};
            for (const auto& [sym, c] : ch) sig.push_back(uint64_t(sym) << 32 | uint32_t(c));
            const auto [it, added] = seen.emplace(std::move(sig), int32_t(nk.size()));
            if (added)
            {
                nk.push_back(std::move(ch));
                ne.push_back(ends_[size_t(node)]);
            }
            canon[size_t(node)] = it->second;
            stack.pop_back();
        }
        // The root is found last: number it 0.
        const int32_t root = canon[0], m = int32_t(nk.size());
        auto renum = [&](int32_t x) { return x == root ? 0 : x < root ? x + 1 : x; };
        kids_.assign(size_t(m), {});
        ends_.assign(size_t(m), 0);
        for (int32_t x = 0; x < m; ++x)
        {
            auto ch = nk[size_t(x)];
            for (auto& e : ch) e.second = renum(e.second);
            kids_[size_t(renum(x))] = std::move(ch);
            ends_[size_t(renum(x))] = ne[size_t(x)];
        }
    }

    void words(std::string* data)
    {
        kids_.assign(1, {});
        ends_.assign(1, 0);
        for (size_t i = 0; i < p_.sets.size(); ++i)
        {
            const auto& s = p_.sets[i];
            std::vector<std::string> list;
            std::string hash;
            if (s.kind == "dict")
            {
                std::string id = s.source;
                if (id.size() > 2 && id.front() == '{')
                {
                    const std::string pn = id.substr(1, id.size() - 2);
                    const auto it = values_.find(pn);
                    id = it != values_.end() ? it->second : "";
                    for (const auto& q : p_.header.params)
                        if (q.key == pn && it == values_.end()) id = q.default_value;
                }
                const auto d = res_.dictionary(id);
                if (!d) fail(s.line, "no dictionary " + (id.empty() ? std::string("(the default)") : id));
                list = d->words();
                hash = "dict:" + (id.empty() ? std::string("default") : id) + "=" + d->sha256();
                // A dictionary's words that the line cannot spell are left out, as dictionaries
                // leave out words with other letters.
                for (const std::string& w : list)
                {
                    std::vector<uint32_t> digits;
                    bool fits = true;
                    for (char32_t cp : utf8_decode(w))
                    {
                        bool ok = false;
                        const uint32_t dd = symbol(cp, ok);
                        if (!ok || sep_[dd]) fits = false;
                        digits.push_back(dd);
                    }
                    if (fits && !digits.empty()) add_word(digits, i);
                }
            }
            else if (s.kind == "tags")
            {
                // A tagged list: word<TAB>tags a line, each tag one character. The set is the
                // words carrying any of the tags asked for. Words are taken as the line can spell
                // them: as they are, or, on a line without capitals, in lower case; others are left
                // out, as a dictionary's are.
                // Several files may be read as one, joined with + (Moby, and its inflections).
                const size_t cut = s.source.rfind(':');
                // TAGS, or TAGS-EXCLUDED: carrying any of the first and none of the second.
                const std::string files = s.source.substr(0, cut), spec = s.source.substr(cut + 1);
                const size_t dash = spec.find('-');
                const std::string want = spec.substr(0, dash), unwanted = dash == std::string::npos ? std::string() : spec.substr(dash + 1);
                std::string text, hashes;
                for (size_t at = 0;;)
                {
                    const size_t plus = files.find('+', at);
                    const std::string one = files.substr(at, plus == std::string::npos ? std::string::npos : plus - at);
                    const std::string t = read_list(one, s.line);
                    hashes += (hashes.empty() ? "" : "+") + Sha256::hex(Sha256::hash(t));
                    text += t;
                    if (!text.empty() && text.back() != '\n') text += '\n';
                    if (plus == std::string::npos) break;
                    at = plus + 1;
                }
                hash = "tags:" + s.source + "=" + hashes;
                // A word carries the tags of all its lines, in every file and every spelling that
                // comes to the same word on this line ("OF" and "of" on a line without capitals),
                // and is judged by them together: Moby's "OF" (a noun) does not make "of", a
                // preposition, a noun where prepositions are excluded.
                std::unordered_map<std::u32string, std::string> tags_of;
                std::vector<std::u32string> order;
                std::istringstream lines(text);
                std::string l;
                while (std::getline(lines, l))
                {
                    const size_t tab = l.find('\t');
                    if (tab == std::string::npos || tab == 0) continue;
                    std::vector<uint32_t> digits;
                    if (!spell(l.substr(0, tab), digits)) continue;
                    const std::u32string key(digits.begin(), digits.end());
                    auto [it, fresh] = tags_of.try_emplace(key);
                    if (fresh) order.push_back(key);
                    it->second += l.substr(tab + 1);
                }
                for (const std::u32string& key : order)
                {
                    const std::string& tags = tags_of[key];
                    if (tags.find_first_of(want) == std::string::npos) continue;
                    if (!unwanted.empty() && tags.find_first_of(unwanted) != std::string::npos) continue;
                    add_word(std::vector<uint32_t>(key.begin(), key.end()), i);
                }
            }
            else
            {
                const std::string path = (p_.folder.empty() ? std::string() : p_.folder + "/") + s.source;
                std::ifstream in(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
                if (!in) fail(s.line, "cannot read the word list " + s.source);
                std::ostringstream o;
                o << in.rdbuf();
                const std::string text = o.str();
                hash = "list:" + s.source + "=" + Sha256::hex(Sha256::hash(text));
                std::istringstream lines(text);
                std::string w;
                int n = 0;
                while (std::getline(lines, w))
                {
                    ++n;
                    if (w.empty()) continue;
                    std::vector<uint32_t> digits;
                    for (char32_t cp : utf8_decode(w))
                    {
                        bool ok = false;
                        const uint32_t dd = symbol(cp, ok);
                        if (!ok || sep_[dd]) fail(s.line, s.source + " line " + std::to_string(n) + ": '" + w + "' is not a word of " + line_.symbols_id + "'s symbols");
                        digits.push_back(dd);
                    }
                    add_word(digits, i);
                }
            }
            if (data) *data += (data->empty() ? "" : " ") + s.name + "=" + hash;
        }
    }

    void relations()
    {
        const size_t n = p_.sets.size();
        auto index = [&](const std::string& name) {
            for (size_t i = 0; i < n; ++i)
                if (p_.sets[i].name == name) return i;
            return size_t(0);
        };
        const uint64_t all = n >= 64 ? ~uint64_t(0) : (uint64_t(1) << n) - 1;
        follow_.assign(n, p_.follows.empty() ? all : 0);
        for (const auto& [a, b] : p_.follows) follow_[index(a)] |= uint64_t(1) << index(b);
        first_ = p_.has_first ? 0 : all;
        for (const auto& s : p_.first) first_ |= uint64_t(1) << index(s);
        last_ = p_.has_last ? 0 : all;
        for (const auto& s : p_.last) last_ |= uint64_t(1) << index(s);
        all_ = all;
    }

    // The sets a token may be after a token with these readings (kStart: none yet).
    uint64_t allowed_after(uint64_t prev) const
    {
        uint64_t allowed = (prev & kStart) ? first_ : 0;
        for (size_t i = 0; i < follow_.size(); ++i)
            if (prev & (uint64_t(1) << i)) allowed |= follow_[i];
        return allowed;
    }
    // The readings a finished token at node n can have, given the sets it may be. Inside a token
    // the state keeps only those (not the readings before it), so fewer states are made.
    uint64_t readings(int32_t n, uint64_t allowed) const { return ends_[size_t(n)] & allowed; }

    // The suffix automaton of every word (a generalized one: each word added from the root).
    struct Sam
    {
        int32_t len = 0, link = -1;
        std::map<uint32_t, int32_t> next;
        bool suffix = false;
    };
    int32_t sam_add(int32_t last, uint32_t c)
    {
        auto clone_of = [&](int32_t p, int32_t q) {
            const int32_t cl = int32_t(sam_.size());
            Sam copy = sam_[size_t(q)];
            copy.len = sam_[size_t(p)].len + 1;
            copy.suffix = false;
            sam_.push_back(copy);
            while (p >= 0)
            {
                auto it = sam_[size_t(p)].next.find(c);
                if (it == sam_[size_t(p)].next.end() || it->second != q) break;
                it->second = cl;
                p = sam_[size_t(p)].link;
            }
            sam_[size_t(q)].link = cl;
            return cl;
        };
        if (const auto it = sam_[size_t(last)].next.find(c); it != sam_[size_t(last)].next.end())
        {
            const int32_t q = it->second;
            if (sam_[size_t(last)].len + 1 == sam_[size_t(q)].len) return q;
            return clone_of(last, q);
        }
        const int32_t cur = int32_t(sam_.size());
        sam_.push_back({sam_[size_t(last)].len + 1, -1, {}, false});
        int32_t p = last;
        while (p >= 0 && !sam_[size_t(p)].next.count(c))
        {
            sam_[size_t(p)].next[c] = cur;
            p = sam_[size_t(p)].link;
        }
        if (p < 0) sam_[size_t(cur)].link = 0;
        else
        {
            const int32_t q = sam_[size_t(p)].next[c];
            if (sam_[size_t(p)].len + 1 == sam_[size_t(q)].len) sam_[size_t(cur)].link = q;
            else
            {
                const int32_t cl = clone_of(p, q);
                sam_[size_t(cur)].link = cl;
            }
        }
        return cur;
    }
    void build_dawg()
    {
        sam_.assign(1, {});
        for (const auto& w : all_words_)
        {
            int32_t last = 0;
            for (uint32_t c : w) last = sam_add(last, c);
        }
        // Suffix states: walk each word again, then up its suffix links.
        for (const auto& w : all_words_)
        {
            int32_t s = 0;
            for (uint32_t c : w) s = sam_[size_t(s)].next.at(c);
            for (; s > 0 && !sam_[size_t(s)].suffix; s = sam_[size_t(s)].link) sam_[size_t(s)].suffix = true;
        }
    }

    // DFA states: 0 start, 1 after a leading separator, then B (between), In (in a token) and
    // Suf (in a cut first token), numbered as found.
    enum Kind : uint8_t { kS0, kS1, kB, kIn, kSuf };
    struct Key
    {
        Kind kind;
        int32_t node;
        uint64_t mask;
        bool operator==(const Key& o) const { return kind == o.kind && node == o.node && mask == o.mask; }
    };
    struct KeyHash
    {
        size_t operator()(const Key& k) const { return std::hash<uint64_t>()(k.mask * 1000003u ^ (uint64_t(uint32_t(k.node)) << 3) ^ k.kind); }
    };

    Dfa build(size_t* declared)
    {
        std::unordered_map<Key, int32_t, KeyHash> id;
        std::vector<Key> keys;
        auto get = [&](const Key& k) {
            const auto [it, added] = id.emplace(k, int32_t(keys.size()));
            if (added)
            {
                keys.push_back(k);
                if (keys.size() > 5'000'000) throw std::invalid_argument("the token form makes over 5 million states: too many sets or words");
            }
            return it->second;
        };
        get({kS0, 0, 0});
        Dfa d;
        d.base = B_;
        d.start = 0;
        for (size_t i = 0; i < keys.size(); ++i)
        {
            const Key k = keys[i];
            bool acc = false;
            switch (k.kind)
            {
            case kS0:
            case kS1: break;
            case kB: acc = (k.mask & last_) != 0; break;
            case kIn: acc = p_.cut || (readings(k.node, k.mask) & last_) != 0; break;
            case kSuf: acc = true; break; // touching both ends: any substring
            }
            d.accept.push_back(acc ? 1 : 0);
            for (uint32_t c = 0; c < B_; ++c)
            {
                int32_t t = Dfa::kDead;
                if (sep_[c])
                {
                    if (k.kind == kS0) t = get({kS1, 0, 0});
                    else if (k.kind == kIn)
                    {
                        const uint64_t r = readings(k.node, k.mask);
                        if (r) t = get({kB, 0, r});
                    }
                    else if (k.kind == kSuf && sam_[size_t(k.node)].suffix) t = get({kB, 0, all_}); // a cut token: any set
                }
                else
                {
                    if (k.kind == kS0 && p_.cut)
                    {
                        const auto it = sam_[0].next.find(c);
                        if (it != sam_[0].next.end()) t = get({kSuf, it->second, 0});
                    }
                    else if (k.kind == kS0 || k.kind == kS1 || k.kind == kB)
                    {
                        const int32_t n = child(0, c);
                        if (n >= 0) t = get({kIn, n, allowed_after(k.kind == kB ? k.mask : kStart)});
                    }
                    else if (k.kind == kIn)
                    {
                        const int32_t n = child(k.node, c);
                        if (n >= 0) t = get({kIn, n, k.mask});
                    }
                    else if (k.kind == kSuf)
                    {
                        const auto it = sam_[size_t(k.node)].next.find(c);
                        if (it != sam_[size_t(k.node)].next.end()) t = get({kSuf, it->second, 0});
                    }
                }
                d.next.push_back(t);
            }
        }
        // For the token form, "declared" is the number of distinct words: how many states are made
        // on the way depends on how the automaton is built, and the oracle builds it another way.
        if (declared) *declared = distinct_words_;
        return d;
    }

    const PluginDef& p_;
    const FilterLine& line_;
    const FilterValues& values_;
    const FilterResources& res_;
    uint32_t B_;
    std::vector<uint8_t> sep_;
    std::vector<std::vector<std::pair<uint32_t, int32_t>>> kids_;
    std::vector<uint64_t> ends_;
    std::vector<std::vector<uint32_t>> all_words_;
    std::vector<uint64_t> follow_;
    uint64_t first_ = 0, last_ = 0, all_ = 0;
    std::vector<Sam> sam_;
    const std::function<void(const std::string&)>& step_;
    size_t distinct_words_ = 0;
    void say(const std::string& s) const
    {
        if (step_) step_(s);
    }
};

} // namespace

Dfa compile_plugin(const PluginDef& p, const FilterLine& line, const FilterValues& values, const FilterResources& resources,
                   size_t* declared_states, std::string* data, const std::function<void(const std::string&)>& step)
{
    auto say = [&](const std::string& s) {
        if (step) step(s);
    };
    if (!plugin_applies(p, line)) throw std::invalid_argument(p.header.name() + " is not written for this line (" + line.kind + ", " + line.symbols_id + ")");
    // Compiled automata are kept for the process: the same file, symbols and settings always
    // compile to the same automaton (a dictionary is pinned by its hash, so its id is enough), and
    // a large one takes seconds, where the menu and the hallway rebuild stacks often.
    struct Compiled
    {
        Dfa dfa;
        size_t declared = 0;
        std::string data;
    };
    static std::mutex mx;
    static std::map<std::string, std::shared_ptr<const Compiled>> cache;
    std::string key = p.header.sha256 + "|" + line.symbols_id + "|" + std::to_string(line.base);
    for (const auto& par : p.header.params)
    {
        const auto it = values.find(par.key);
        key += "|" + par.key + "=" + (it == values.end() ? par.default_value : it->second);
    }
    {
        std::lock_guard<std::mutex> lock(mx);
        if (const auto it = cache.find(key); it != cache.end() && !p.header.sha256.empty())
        {
            if (declared_states) *declared_states = it->second->declared;
            if (data) *data = it->second->data;
            say("compiled already (kept from before)");
            return it->second->dfa;
        }
    }
    try
    {
        auto c = std::make_shared<Compiled>();
        Dfa made;
        if (p.header.form == "tokens") made = TokenCompiler(p, line, values, resources, step).run(&c->declared, &c->data);
        else
        {
            say("running the table");
            Compiler tc(p, line, values);
            made = tc.run(&c->declared);
        }
        say("minimising " + std::to_string(made.states()) + " states");
        c->dfa = minimise(made);
        if (declared_states) *declared_states = c->declared;
        if (data) *data = c->data;
        std::lock_guard<std::mutex> lock(mx);
        if (cache.size() > 64) cache.clear();
        cache[key] = c;
        return c->dfa;
    }
    catch (const std::invalid_argument& e)
    {
        throw std::invalid_argument(p.header.name() + ": " + e.what());
    }
}

// ---------------------------------------------------------------- the filter

namespace {

class PluginFilter : public Filter
{
public:
    PluginFilter(Dfa dfa, uint32_t length, std::string provenance) : dfa_(std::move(dfa))
    {
        provenance_ = std::move(provenance);
        if (DfaRanker::table_bytes(dfa_.states(), dfa_.base, length) <= kPluginTableBudget) ranker_ = std::make_unique<DfaRanker>(dfa_, length);
    }
    bool passes(std::span<const uint32_t> unit) const override { return dfa_.accepts(unit); }
    const Ranker* ranker() const override { return ranker_.get(); }
    const Dfa& dfa() const { return dfa_; }

private:
    Dfa dfa_;
    std::unique_ptr<DfaRanker> ranker_;
};

} // namespace

const Dfa* plugin_dfa(const Filter& f)
{
    const auto* p = dynamic_cast<const PluginFilter*>(&f);
    return p ? &p->dfa() : nullptr;
}

std::unique_ptr<Filter> make_dfa_filter(Dfa dfa, uint32_t length, std::string provenance)
{
    return std::make_unique<PluginFilter>(std::move(dfa), length, std::move(provenance));
}

FilterSpec plugin_spec(std::shared_ptr<const PluginDef> p)
{
    const PluginHeader& h = p->header;
    FilterSpec s;
    s.id = h.id;
    s.version = h.version;
    s.title = h.id;
    s.description = h.describe.empty() ? std::string("A filter plugin.") : h.describe; // who made it: author, origin
    s.params = h.params;
    s.author = h.author;
    s.origin = h.origin;
    s.plugin_sha256 = h.sha256;
    s.prerequisites = h.prerequisites;
    s.applies = [p](const FilterLine& l) { return plugin_applies(*p, l); };
    s.make = [p](const FilterLine& l, const FilterValues& v, const FilterResources& r) -> std::unique_ptr<Filter> {
        std::string prov = "plugin sha256=" + p->header.sha256;
        for (const auto& par : p->header.params)
        {
            const auto it = v.find(par.key);
            prov += " " + par.key + "=" + (it == v.end() ? par.default_value : it->second);
        }
        std::string data;
        Dfa d = compile_plugin(*p, l, v, r, nullptr, &data);
        if (!data.empty()) prov += " " + data;
        return std::make_unique<PluginFilter>(std::move(d), l.length, prov);
    };
    return s;
}

// ---------------------------------------------------------------- the registry

namespace {

std::deque<FilterSpec>& plugins()
{
    static std::deque<FilterSpec> list;
    return list;
}
std::mutex g_plugins_mx;

bool g_registered = false;

} // namespace

void register_plugins(std::vector<FilterSpec> specs)
{
    std::lock_guard<std::mutex> lock(g_plugins_mx);
    if (g_registered) return; // once, at start-up; add_plugin adds more later
    g_registered = true;
    for (auto& s : specs) plugins().push_back(std::move(s));
}

bool add_plugin(FilterSpec spec)
{
    std::lock_guard<std::mutex> lock(g_plugins_mx);
    g_registered = true;
    for (const auto& f : plugins())
        if (f.name() == spec.name()) return false;
    for (const auto& f : filter_registry())
        if (f.id == spec.id) return false;
    plugins().push_back(std::move(spec)); // a deque: the others stay where they are
    return true;
}

const std::deque<FilterSpec>& plugin_registry() { return plugins(); }

} // namespace sieve
