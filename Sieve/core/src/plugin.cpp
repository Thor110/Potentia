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
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <map>
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
    Expr(const std::string& s, const Env& env, int line) : s_(s), env_(env), line_(line) {}
    int64_t value()
    {
        const int64_t v = sum();
        skip();
        if (i_ != s_.size()) fail(line_, "cannot read the expression '" + s_ + "'");
        return v;
    }

private:
    void skip()
    {
        while (i_ < s_.size() && is_space(s_[i_])) ++i_;
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
        return atom();
    }
    int64_t atom()
    {
        skip();
        if (i_ >= s_.size()) fail(line_, "an expression ends too soon: '" + s_ + "'");
        const char c = s_[i_];
        if (c == '(')
        {
            ++i_;
            const int64_t v = sum();
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
            const auto it = env_.find(name);
            if (it == env_.end()) fail(line_, "'" + name + "' is not a parameter or a for variable");
            return it->second;
        }
        fail(line_, "cannot read the expression '" + s_ + "'");
    }

    const std::string& s_;
    const Env& env_;
    int line_;
    size_t i_ = 0;
};

// A number field: an integer, a name, or {an expression}.
int64_t eval(const Tok& t, const Env& env, int line)
{
    if (t.quoted) fail(line, "expected a number, got a quoted string");
    std::string s = t.text;
    if (s.size() >= 2 && s.front() == '{' && s.back() == '}') s = s.substr(1, s.size() - 2);
    else if (s.find_first_of("{}") != std::string::npos) fail(line, "an expression must be all inside {braces}: '" + t.text + "'");
    return Expr(s, env, line).value();
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
    std::vector<size_t> done_of;     // for a `for` at body[i], the index of its `done`
};

const PluginHeader& plugin_header(const PluginDef& p) { return p.header; }

std::shared_ptr<const PluginDef> parse_plugin(const std::string& text, const std::string& sha256)
{
    auto def = std::make_shared<PluginDef>();
    PluginHeader& h = def->header;
    h.sha256 = sha256;
    std::istringstream in(text);
    std::string raw;
    int line = 0;
    bool first = true, ended = false, has_states = false, has_start = false, has_version = false;
    std::vector<size_t> open_for;
    std::vector<std::pair<size_t, size_t>> pairs; // each for, and its done
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
            if (toks.size() != 1 || toks[0].quoted || toks[0].text != kPluginFormat) fail(line, std::string("a plugin starts with ") + kPluginFormat);
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
        }
        else if (k == "describe")
        {
            once();
            h.describe = rest_after_keyword(raw);
        }
        else if (k == "param")
        {
            if (toks.size() < 6) fail(line, "param NAME int DEFAULT MIN MAX [text]");
            FilterParam p;
            p.key = toks[1].text;
            if (!is_name(p.key)) fail(line, "a parameter's name is letters, digits and _, starting with a letter");
            for (const auto& q : h.params)
                if (q.key == p.key) fail(line, "parameter " + p.key + " is declared twice");
            if (toks[2].text != "int") fail(line, "a parameter's kind is int (the only kind in sieve-filter-v1)");
            p.kind = FilterParam::Kind::Integer;
            const Env none;
            const int64_t dflt = eval(toks[3], none, line), lo = eval(toks[4], none, line), hi = eval(toks[5], none, line);
            if (lo > hi || dflt < lo || dflt > hi) fail(line, "a parameter's default must lie between its minimum and maximum");
            p.default_value = std::to_string(dflt);
            p.min = lo;
            p.max = hi;
            std::string desc;
            for (size_t i = 6; i < toks.size(); ++i) desc += (desc.empty() ? "" : " ") + toks[i].text;
            p.description = desc;
            h.params.push_back(p);
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
            open_for.push_back(def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (k == "done")
        {
            want(1);
            if (open_for.empty()) fail(line, "done without its for");
            const size_t f = open_for.back();
            open_for.pop_back();
            pairs.emplace_back(f, def->body.size());
            def->body.push_back({line, toks, {}});
        }
        else if (k == "end")
        {
            want(1);
            if (!open_for.empty()) fail(line, "a for is not closed by done");
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
    if (!has_states || !has_start) throw std::invalid_argument("a table plugin needs states and start");
    def->done_of.assign(def->body.size(), 0);
    for (const auto& [f, d] : pairs) def->done_of[f] = d;
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
        for (const auto& par : p.header.params)
        {
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
        const int64_t v = eval(Tok{expr, false}, env_, line);
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
        const int64_t v = eval(t, env_, line);
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
            if (k == "class" || k == "done") continue;
            if (k == "states")
            {
                N_ = eval(st.toks[1], env_, st.line);
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
                const int64_t lo = eval(st.toks[2], env_, st.line), hi = eval(st.toks[3], env_, st.line);
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

} // namespace

Dfa compile_plugin(const PluginDef& p, const FilterLine& line, const FilterValues& values, size_t* declared_states)
{
    if (!plugin_applies(p, line)) throw std::invalid_argument(p.header.name() + " is not written for this line (" + line.kind + ", " + line.symbols_id + ")");
    Compiler c(p, line, values);
    try
    {
        return minimise(c.run(declared_states));
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

FilterSpec plugin_spec(std::shared_ptr<const PluginDef> p)
{
    const PluginHeader& h = p->header;
    FilterSpec s;
    s.id = h.id;
    s.version = h.version;
    s.title = h.id;
    s.description = (h.describe.empty() ? std::string("A filter plugin.") : h.describe) + " (plugin by " + h.author + ", " + h.origin + "; " +
                    h.sha256.substr(0, 12) + ")";
    s.params = h.params;
    s.author = h.author;
    s.origin = h.origin;
    s.plugin_sha256 = h.sha256;
    s.applies = [p](const FilterLine& l) { return plugin_applies(*p, l); };
    s.make = [p](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        std::string prov = "plugin sha256=" + p->header.sha256;
        for (const auto& par : p->header.params)
        {
            const auto it = v.find(par.key);
            prov += " " + par.key + "=" + (it == v.end() ? par.default_value : it->second);
        }
        return std::make_unique<PluginFilter>(compile_plugin(*p, l, v), l.length, prov);
    };
    return s;
}

// ---------------------------------------------------------------- the registry

namespace {

std::vector<FilterSpec>& plugins()
{
    static std::vector<FilterSpec> list;
    return list;
}

bool g_registered = false;

} // namespace

void register_plugins(std::vector<FilterSpec> specs)
{
    if (g_registered) return; // once: specs are handed out by pointer, so the list never changes after
    g_registered = true;
    plugins() = std::move(specs);
}

const std::vector<FilterSpec>& plugin_registry() { return plugins(); }

} // namespace sieve
