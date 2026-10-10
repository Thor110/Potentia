// Sieve — a language model's tokenizer (see llm_tokenizer.hpp).
#include "sieve/llm_tokenizer.hpp"

#include "sieve/unicode_classes.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace sieve::llm {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("tokenizer.json: " + what); }

// GPT-2's table: the bytes that are printable stand as themselves, the rest from U+0100 on.
const std::array<char32_t, 256>& byte_table()
{
    static const std::array<char32_t, 256> t = [] {
        std::array<char32_t, 256> m{};
        std::array<bool, 256> kept{};
        for (int b = '!'; b <= '~'; ++b) kept[size_t(b)] = true;
        for (int b = 0xA1; b <= 0xAC; ++b) kept[size_t(b)] = true;
        for (int b = 0xAE; b <= 0xFF; ++b) kept[size_t(b)] = true;
        char32_t next = 256;
        for (int b = 0; b < 256; ++b) m[size_t(b)] = kept[size_t(b)] ? char32_t(b) : next++;
        return m;
    }();
    return t;
}

bool flag(const json::Value& v, const char* key, bool otherwise)
{
    const json::Value* f = v.find(key);
    return f && !f->is_null() ? f->boolean() : otherwise;
}

void check_byte_level(const json::Value& p)
{
    if (p.at("type").string() != "ByteLevel") bad("a pre-tokenizer other than ByteLevel");
    if (flag(p, "add_prefix_space", false)) bad("ByteLevel with add_prefix_space is not covered");
    if (!flag(p, "use_regex", true)) bad("ByteLevel without its pattern is not covered");
}

bool is_space(char32_t c) { return unicode::is_white_space(c); }
bool is_letter(char32_t c) { return unicode::is_letter(c); }
bool is_number(char32_t c) { return unicode::is_number(c); }
bool is_other(char32_t c) { return !is_space(c) && !is_letter(c) && !is_number(c); }

// The GPT-2 pattern over one run of text (see the header), its pieces' bounds.
void gpt2_split(const std::u32string& t, size_t from, size_t to, std::vector<std::pair<size_t, size_t>>& out)
{
    size_t i = from;
    while (i < to)
    {
        const char32_t c = t[i];
        // 's 't 're 've 'm 'll 'd
        if (c == U'\'' && i + 1 < to)
        {
            const char32_t a = t[i + 1], b = i + 2 < to ? t[i + 2] : 0;
            size_t n = 0;
            if (a == U's' || a == U't' || a == U'm' || a == U'd') n = 2;
            else if ((a == U'r' && b == U'e') || (a == U'v' && b == U'e') || (a == U'l' && b == U'l')) n = 3;
            if (n)
            {
                out.emplace_back(i, i + n);
                i += n;
                continue;
            }
        }
        // ` ?\p{L}+`, ` ?\p{N}+`, ` ?[^\s\p{L}\p{N}]+`
        size_t j = i;
        if (c == U' ' && i + 1 < to && !is_space(t[i + 1])) j = i + 1;
        const char32_t d = t[j];
        if (!is_space(d))
        {
            bool (*same)(char32_t) = is_letter(d) ? is_letter : is_number(d) ? is_number : is_other;
            size_t k = j;
            while (k < to && same(t[k])) ++k;
            out.emplace_back(i, k);
            i = k;
            continue;
        }
        // `\s+(?!\S)`, then `\s+`
        size_t k = i;
        while (k < to && is_space(t[k])) ++k;
        if (k < to && k - i > 1) --k; // the last space goes with what follows
        out.emplace_back(i, k);
        i = k;
    }
}

} // namespace

char32_t byte_char(uint8_t b) { return byte_table()[b]; }

std::optional<uint8_t> char_byte(char32_t c)
{
    static const std::unordered_map<char32_t, uint8_t> back = [] {
        std::unordered_map<char32_t, uint8_t> m;
        for (int b = 0; b < 256; ++b) m[byte_table()[size_t(b)]] = uint8_t(b);
        return m;
    }();
    const auto it = back.find(c);
    if (it == back.end()) return std::nullopt;
    return it->second;
}

Tokenizer::Tokenizer(const json::Value& j)
{
    if (const json::Value* n = j.find("normalizer"); n && !n->is_null()) bad("a normalizer is not covered");
    const json::Value& pre = j.at("pre_tokenizer");
    if (pre.at("type").string() == "Sequence")
    {
        const auto& seq = pre.at("pretokenizers").array();
        if (seq.size() != 2 || seq[0].at("type").string() != "Digits" || !flag(seq[0], "individual_digits", false))
            bad("a pre-tokenizer sequence other than Digits (individual) then ByteLevel");
        check_byte_level(seq[1]);
        digits_ = true;
    }
    else check_byte_level(pre);
    const json::Value& m = j.at("model");
    if (m.at("type").string() != "BPE") bad("a model other than BPE");
    if (flag(m, "byte_fallback", false)) bad("byte fallback is not covered");
    if (flag(m, "ignore_merges", false)) bad("ignore_merges is not covered");
    for (const char* k : {"continuing_subword_prefix", "end_of_word_suffix"})
        if (const json::Value* v = m.find(k); v && !v->is_null()) bad(std::string(k) + " is not covered");

    for (const auto& [text, id] : m.at("vocab").object())
    {
        const uint64_t i = id.u64();
        if (i > 10'000'000) bad("a token id past any vocabulary");
        if (tokens_.size() <= i) tokens_.resize(size_t(i + 1));
        tokens_[size_t(i)] = text;
        ids_[text] = uint32_t(i);
    }
    added_.assign(tokens_.size(), false);
    if (const json::Value* a = j.find("added_tokens"))
        for (const json::Value& t : a->array())
        {
            const uint64_t i = t.at("id").u64();
            if (i > 10'000'000) bad("a token id past any vocabulary");
            const std::string& text = t.at("content").string();
            if (text.empty()) bad("an empty added token");
            if (tokens_.size() <= i)
            {
                tokens_.resize(size_t(i + 1));
                added_.resize(size_t(i + 1), false);
            }
            tokens_[size_t(i)] = text;
            ids_[text] = uint32_t(i);
            added_[size_t(i)] = true;
            added_list_.emplace_back(text, uint32_t(i));
        }
    std::stable_sort(added_list_.begin(), added_list_.end(), [](const auto& x, const auto& y) { return x.first.size() > y.first.size(); });
    for (size_t i = 0; i < tokens_.size(); ++i)
        if (tokens_[i].empty()) bad("no token for id " + std::to_string(i));

    uint32_t rank = 0;
    for (const json::Value& mv : m.at("merges").array())
    {
        std::string key;
        if (mv.is_string()) key = mv.string();
        else
        {
            const auto& p = mv.array();
            if (p.size() != 2) bad("a merge that is not a pair");
            key = p[0].string() + " " + p[1].string();
        }
        const size_t sp = key.find(' ');
        if (sp == std::string::npos || key.find(' ', sp + 1) != std::string::npos) bad("a merge that is not two symbols: " + key);
        if (!ids_.count(key.substr(0, sp) + key.substr(sp + 1))) bad("a merge whose result is not a token: " + key);
        ranks_.emplace(key, rank++);
    }
}

std::optional<uint32_t> Tokenizer::find(std::string_view token) const
{
    const auto it = ids_.find(std::string(token));
    if (it == ids_.end()) return std::nullopt;
    return it->second;
}

std::string Tokenizer::bytes_of(uint32_t id) const
{
    if (id >= tokens_.size()) throw std::invalid_argument("no token " + std::to_string(id));
    if (added_[id]) return tokens_[id];
    std::string out;
    for (const char32_t c : utf8_decode(tokens_[id]))
    {
        const auto b = char_byte(c);
        if (!b) throw std::invalid_argument("token " + std::to_string(id) + " is not written in byte characters");
        out.push_back(char(*b));
    }
    return out;
}

std::string Tokenizer::decode(const std::vector<uint32_t>& ids) const
{
    std::string out;
    for (const uint32_t id : ids) out += bytes_of(id);
    return out;
}

std::vector<std::string> Tokenizer::pieces(std::string_view utf8) const
{
    const std::u32string t = utf8_decode(utf8);
    std::vector<std::pair<size_t, size_t>> bounds;
    if (digits_)
    {
        // Every number character alone, the runs between them split by the pattern.
        size_t run = 0;
        for (size_t i = 0; i <= t.size(); ++i)
            if (i == t.size() || is_number(t[i]))
            {
                if (i > run) gpt2_split(t, run, i, bounds);
                if (i < t.size()) bounds.emplace_back(i, i + 1);
                run = i + 1;
            }
    }
    else gpt2_split(t, 0, t.size(), bounds);
    std::vector<std::string> out;
    out.reserve(bounds.size());
    for (const auto& [a, b] : bounds) out.push_back(utf8_encode(std::u32string_view(t).substr(a, b - a)));
    return out;
}

std::vector<uint32_t> Tokenizer::bpe(const std::string& piece) const
{
    // The piece's bytes as byte characters, one symbol each, merged lowest rank first.
    // A byte with no token of its own is left out (the vocabulary has no unknown token), as the
    // library the tokenizer comes from does.
    std::vector<std::string> sym;
    for (const char ch : piece)
        if (std::string s = utf8_encode(byte_char(uint8_t(ch))); ids_.count(s)) sym.push_back(std::move(s));
    for (;;)
    {
        size_t best = sym.size();
        uint32_t best_rank = UINT32_MAX;
        for (size_t i = 0; i + 1 < sym.size(); ++i)
        {
            const auto it = ranks_.find(sym[i] + " " + sym[i + 1]);
            if (it != ranks_.end() && it->second < best_rank)
            {
                best_rank = it->second;
                best = i;
            }
        }
        if (best == sym.size()) break;
        sym[best] += sym[best + 1];
        sym.erase(sym.begin() + std::ptrdiff_t(best) + 1);
    }
    std::vector<uint32_t> out;
    out.reserve(sym.size());
    for (const std::string& s : sym)
    {
        const auto it = ids_.find(s);
        if (it == ids_.end()) throw std::invalid_argument("tokenizer: no token for a merged symbol");
        out.push_back(it->second);
    }
    return out;
}

std::vector<uint32_t> Tokenizer::encode(std::string_view utf8, bool special) const
{
    std::vector<uint32_t> out;
    auto plain = [&](std::string_view text) {
        for (const std::string& p : pieces(text))
        {
            const std::vector<uint32_t> ids = bpe(p);
            out.insert(out.end(), ids.begin(), ids.end());
        }
    };
    size_t at = 0;
    while (at < utf8.size())
    {
        // The earliest added token in what is left (the longest of those starting there).
        size_t where = std::string_view::npos;
        const std::pair<std::string, uint32_t>* which = nullptr;
        if (special)
            for (const auto& a : added_list_)
            {
                const size_t w = utf8.find(a.first, at);
                if (w < where)
                {
                    where = w;
                    which = &a;
                }
            }
        if (!which)
        {
            plain(utf8.substr(at));
            break;
        }
        if (where > at) plain(utf8.substr(at, where - at));
        out.push_back(which->second);
        at = where + which->first.size();
    }
    return out;
}

} // namespace sieve::llm
