// Sieve CLI -- binary written as text, carried as what it is (spans-v1). See text_spans.hpp.

#include "cli/text_spans.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sieve::cli::spans {

namespace {

constexpr char kStandard[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char kUrlSafe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
constexpr uint8_t kBase64 = 0, kHex = 1 << 3, kList = 2 << 3, kKind = 3 << 3;
constexpr size_t kLongestString = 64; // a list's prefix, separator or line break

bool letter_or_digit(uint8_t c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); }
bool digit(uint8_t c) { return c >= '0' && c <= '9'; }
bool small_hex(uint8_t c) { return c >= 'a' && c <= 'f'; }
bool capital_hex(uint8_t c) { return c >= 'A' && c <= 'F'; }
bool hex_digit(uint8_t c) { return digit(c) || small_hex(c) || capital_hex(c); }
bool in_base64(uint8_t c) { return letter_or_digit(c) || c == '+' || c == '/' || c == '-' || c == '_'; }
int hex_value(uint8_t c) { return digit(c) ? c - '0' : small_hex(c) ? c - 'a' + 10 : c - 'A' + 10; }
int base64_value(uint8_t c, bool url)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (digit(c)) return c - '0' + 52;
    if (c == (url ? '-' : '+')) return 62;
    if (c == (url ? '_' : '/')) return 63;
    return -1;
}

void put_varint(std::vector<uint8_t>& o, uint64_t v)
{
    while (v >= 0x80)
    {
        o.push_back(uint8_t(v | 0x80));
        v >>= 7;
    }
    o.push_back(uint8_t(v));
}

uint64_t get_varint(std::span<const uint8_t> in, size_t& at)
{
    uint64_t v = 0;
    for (int shift = 0;; shift += 7)
    {
        if (at >= in.size() || shift > 56) throw std::runtime_error("spans-v1: the skeleton ends inside a number");
        const uint8_t b = in[at++];
        v |= uint64_t(b & 0x7F) << shift;
        if (!(b & 0x80))
        {
            if (b == 0 && shift > 0) throw std::runtime_error("spans-v1: a number in the skeleton is not written as Sieve writes it");
            return v;
        }
    }
}

size_t line_end(std::span<const uint8_t> f, size_t q) // its length, 0 for none
{
    if (q < f.size() && f[q] == '\n') return 1;
    if (q + 1 < f.size() && f[q] == '\r' && f[q + 1] == '\n') return 2;
    return 0;
}

// What may follow a span's last line: the file's end, a line end, or what closes a quotation, a tag
// or a bracket, or separates a field (not a space: so a word of text after a block is not read as
// its last line).
bool closes(std::span<const uint8_t> f, size_t q)
{
    if (q >= f.size() || line_end(f, q)) return true;
    const uint8_t c = f[q];
    return c == '"' || c == '\'' || c == '<' || c == '>' || c == ')' || c == ']' || c == '}' || c == ',' || c == ';' || c == ':' || c == '`';
}

// What a span of characters in lines would be, read from `p`: its lines (each a run of the class's
// characters), a line end between each two of the same kind, "=" at most twice at its end where
// `pads`. Its length in the file (0 for none), with `width` and `crlf` as read; the checks of what
// it holds come after.
template <class Class>
size_t read_lines(std::span<const uint8_t> f, size_t p, Class in_class, bool pads_allowed, uint32_t& width, bool& crlf)
{
    auto run = [&](size_t q) {
        size_t e = q;
        while (e < f.size() && in_class(f[e])) ++e;
        return e - q;
    };
    auto pads = [&](size_t q) {
        size_t n = 0;
        while (pads_allowed && n < 2 && q + n < f.size() && f[q + n] == '=') ++n;
        return n;
    };
    const size_t first = run(p);
    if (first == 0) return 0;
    width = 0;
    crlf = false;
    const size_t end = p + first + pads(p + first);
    const size_t eol = line_end(f, p + first);
    if (pads(p + first) || !eol) return end - p;
    // In lines: every line the first's width, and the last no wider, then something that closes it.
    crlf = eol == 2;
    const uint32_t w = uint32_t(first);
    size_t q = p + first; // the end of the last line taken
    size_t lines = 1;
    for (;;)
    {
        const size_t e = line_end(f, q);
        if (e != eol) break;
        const size_t s = q + e;
        const size_t r = run(s);
        if (r == 0 || r > w) break;
        const size_t pd = pads(s + r);
        if (r == w && !pd && line_end(f, s + r) == eol)
        {
            q = s + r;
            ++lines;
            continue;
        }
        if (closes(f, s + r + pd))
        {
            q = s + r + pd;
            ++lines;
        }
        break;
    }
    if (lines == 1) return end - p; // one line, its line end left in the text
    width = w;
    return q - p;
}

// Kept where written again it gives the file's characters back exactly.
bool gives_back(std::span<const uint8_t> f, const Span& s)
{
    const std::vector<uint8_t> again = written(s.bytes, s.form);
    return again.size() == s.length && std::equal(again.begin(), again.end(), f.begin() + std::ptrdiff_t(s.at));
}

// Base64 at `p`: its alphabet either (not both), padded or not, in one line or in lines.
std::optional<Span> base64_at(std::span<const uint8_t> f, size_t p, size_t smallest, size_t& next)
{
    uint32_t width = 0;
    bool crlf = false;
    const size_t length = read_lines(f, p, in_base64, true, width, crlf);
    next = p + std::max<size_t>(length, 1);
    if (length == 0) return std::nullopt;
    // Some of each of capitals, small letters and digits (a long word, a path or a name is rarely
    // all three).
    bool standard = false, url = false, upper = false, lower = false, digits = false;
    size_t chars = 0, padded = 0;
    for (size_t k = p; k < p + length; ++k)
    {
        const uint8_t c = f[k];
        if (c == '\r' || c == '\n') continue;
        if (c == '=')
        {
            ++padded;
            continue;
        }
        ++chars;
        standard |= c == '+' || c == '/';
        url |= c == '-' || c == '_';
        upper |= c >= 'A' && c <= 'Z';
        lower |= c >= 'a' && c <= 'z';
        digits |= digit(c);
    }
    const size_t bytes = chars * 3 / 4;
    if ((standard && url) || !upper || !lower || !digits || chars % 4 == 1 || bytes < smallest || (padded && (chars + padded) % 4 != 0)) return std::nullopt;
    Span s;
    s.at = p;
    s.length = length;
    s.form.width = width;
    s.form.how = uint8_t(kBase64 | (url ? 1 : 0) | (padded || chars % 4 == 0 ? 2 : 0) | (crlf ? 4 : 0));
    s.bytes.reserve(bytes);
    uint32_t acc = 0;
    int bits = 0;
    for (size_t k = p; k < p + length; ++k)
    {
        const int v = base64_value(f[k], url);
        if (v < 0) continue;
        acc = acc << 6 | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            s.bytes.push_back(uint8_t(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    if (!gives_back(f, s)) return std::nullopt;
    return s;
}

// Hex at `p`, two digits a byte, nothing between: in one line or in lines, its letters all small or
// all capitals, and some letters and some digits (a run of digits alone is a number).
std::optional<Span> hex_at(std::span<const uint8_t> f, size_t p, size_t smallest, size_t& next)
{
    uint32_t width = 0;
    bool crlf = false;
    const size_t length = read_lines(f, p, hex_digit, false, width, crlf);
    next = p + std::max<size_t>(length, 1);
    if (length == 0 || (p + length < f.size() && letter_or_digit(f[p + length]))) return std::nullopt;
    bool small = false, capital = false, digits = false;
    size_t chars = 0;
    for (size_t k = p; k < p + length; ++k)
    {
        const uint8_t c = f[k];
        if (c == '\r' || c == '\n') continue;
        ++chars;
        small |= small_hex(c);
        capital |= capital_hex(c);
        digits |= digit(c);
    }
    if ((small && capital) || !(small || capital) || !digits || chars % 2 || chars / 2 < smallest) return std::nullopt;
    Span s;
    s.at = p;
    s.length = length;
    s.form.width = width;
    s.form.how = uint8_t(kHex | (capital ? 1 : 0) | (crlf ? 4 : 0));
    s.bytes.reserve(chars / 2);
    int high = -1;
    for (size_t k = p; k < p + length; ++k)
    {
        if (f[k] == '\r' || f[k] == '\n') continue;
        if (high < 0) high = hex_value(f[k]);
        else
        {
            s.bytes.push_back(uint8_t(high << 4 | hex_value(f[k])));
            high = -1;
        }
    }
    if (!gives_back(f, s)) return std::nullopt;
    return s;
}

// A list of byte values at `p`, in `form` (0, 1: two hex digits, small or capitals; 2: decimal),
// each after `prefix`: the values and what lies between them (no letter or digit, at most 64
// bytes), the first between taken as the separator, the first other as the line break (every so
// many values), the list ending where that pattern does.
std::optional<Span> list_at(std::span<const uint8_t> f, size_t p, int form, std::string_view prefix, size_t smallest)
{
    // A value at q: its end, and its byte; 0 for none.
    auto value = [&](size_t q, uint8_t& v) -> size_t {
        if (f.size() - q < prefix.size() || !std::equal(prefix.begin(), prefix.end(), f.begin() + std::ptrdiff_t(q))) return 0;
        q += prefix.size();
        size_t e = q;
        if (form < 2)
        {
            if (f.size() - q < 2) return 0;
            for (size_t k = q; k < q + 2; ++k)
                if (!(digit(f[k]) || (form == 0 ? small_hex(f[k]) : capital_hex(f[k])))) return 0;
            v = uint8_t(hex_value(f[q]) << 4 | hex_value(f[q + 1]));
            e = q + 2;
        }
        else
        {
            unsigned n = 0;
            while (e < f.size() && digit(f[e]) && e - q < 4) n = n * 10 + unsigned(f[e++] - '0');
            if (e == q || e - q > 3 || n > 255 || (f[q] == '0' && e - q > 1)) return 0;
            v = uint8_t(n);
        }
        if (e < f.size() && letter_or_digit(f[e])) return 0; // (a value ends a word)
        return e;
    };
    // What lies between two values: up to the next value's start (where the prefix begins, or the
    // next letter or digit), none of it a letter or digit.
    auto between = [&](size_t q) {
        size_t e = q;
        while (e < f.size() && e - q <= kLongestString && !letter_or_digit(f[e]) &&
               !(!prefix.empty() && f.size() - e >= prefix.size() && std::equal(prefix.begin(), prefix.end(), f.begin() + std::ptrdiff_t(e))))
            ++e;
        return e;
    };
    uint8_t v = 0;
    size_t e = value(p, v);
    if (!e) return std::nullopt;
    Span s;
    s.bytes.push_back(v);
    std::vector<std::pair<size_t, size_t>> gaps; // each between: where, how long
    std::vector<size_t> ends{e};
    for (;;)
    {
        const size_t g = between(e);
        if (g - e > kLongestString || (g == e && prefix.empty())) break;
        const size_t ve = value(g, v);
        if (!ve) break;
        gaps.emplace_back(e, g - e);
        s.bytes.push_back(v);
        ends.push_back(ve);
        e = ve;
    }
    // The pattern: the separator, and the line break every `per` values.
    auto gap = [&](size_t k) { return std::string_view(reinterpret_cast<const char*>(f.data()) + gaps[k].first, gaps[k].second); };
    size_t count = s.bytes.size();
    std::string_view separator = gaps.empty() ? std::string_view() : gap(0), line;
    size_t per = 0;
    for (size_t k = 1; k < gaps.size(); ++k)
    {
        const std::string_view g = gap(k);
        if (per == 0)
        {
            if (g == separator) continue;
            per = k + 1, line = g; // (after the (k + 1)th value)
            continue;
        }
        if (g != ((k + 1) % per == 0 ? line : separator))
        {
            count = k + 1;
            break;
        }
    }
    if (count < smallest) return std::nullopt;
    s.bytes.resize(count);
    s.at = p;
    s.length = ends[count - 1] - p;
    s.form.how = uint8_t(kList | form);
    s.form.width = uint32_t(per);
    s.form.prefix = prefix;
    s.form.separator = separator;
    s.form.line = line;
    if (per >= count) // (no line break within it after all)
    {
        s.form.width = 0;
        s.form.line.clear();
    }
    if (!gives_back(f, s)) return std::nullopt;
    return s;
}

} // namespace

std::vector<uint8_t> written(std::span<const uint8_t> bytes, const Form& form)
{
    std::vector<uint8_t> out;
    const uint8_t kind = form.how & kKind;
    if (kind == kList)
    {
        static const char small[] = "0123456789abcdef", capital[] = "0123456789ABCDEF";
        const int value_form = form.how & 3;
        for (size_t k = 0; k < bytes.size(); ++k)
        {
            if (k)
            {
                const std::string& between = form.width && k % form.width == 0 ? form.line : form.separator;
                out.insert(out.end(), between.begin(), between.end());
            }
            out.insert(out.end(), form.prefix.begin(), form.prefix.end());
            const uint8_t b = bytes[k];
            if (value_form < 2)
            {
                const char* const d = value_form ? capital : small;
                out.push_back(uint8_t(d[b >> 4]));
                out.push_back(uint8_t(d[b & 15]));
            }
            else
            {
                if (b >= 100) out.push_back(uint8_t('0' + b / 100));
                if (b >= 10) out.push_back(uint8_t('0' + b / 10 % 10));
                out.push_back(uint8_t('0' + b % 10));
            }
        }
        return out;
    }
    std::string chars;
    if (kind == kHex)
    {
        static const char small[] = "0123456789abcdef", capital[] = "0123456789ABCDEF";
        const char* const d = (form.how & 1) ? capital : small;
        chars.reserve(bytes.size() * 2);
        for (const uint8_t b : bytes)
        {
            chars += d[b >> 4];
            chars += d[b & 15];
        }
    }
    else
    {
        const char* const a = (form.how & 1) ? kUrlSafe : kStandard;
        chars.reserve((bytes.size() + 2) / 3 * 4);
        size_t i = 0;
        for (; i + 3 <= bytes.size(); i += 3)
        {
            const uint32_t v = uint32_t(bytes[i]) << 16 | uint32_t(bytes[i + 1]) << 8 | bytes[i + 2];
            chars += a[v >> 18 & 63];
            chars += a[v >> 12 & 63];
            chars += a[v >> 6 & 63];
            chars += a[v & 63];
        }
        if (const size_t left = bytes.size() - i; left)
        {
            const uint32_t v = uint32_t(bytes[i]) << 16 | (left == 2 ? uint32_t(bytes[i + 1]) << 8 : 0);
            chars += a[v >> 18 & 63];
            chars += a[v >> 12 & 63];
            if (left == 2) chars += a[v >> 6 & 63];
            if (form.how & 2) chars.append(3 - left, '=');
        }
    }
    out.reserve(chars.size() + (form.width ? chars.size() / form.width * 2 : 0));
    for (size_t k = 0; k < chars.size(); ++k)
    {
        if (form.width && k && k % form.width == 0)
        {
            if (form.how & 4) out.push_back('\r');
            out.push_back('\n');
        }
        out.push_back(uint8_t(chars[k]));
    }
    return out;
}

std::vector<Span> spans_of(std::span<const uint8_t> file, size_t smallest)
{
    // Each kind looked for on its own, from each place where a word could start; then where two
    // overlap, the one holding more bytes kept.
    std::vector<Span> found;
    for (size_t p = 0; p < file.size();)
    {
        size_t next = p + 1;
        if (in_base64(file[p]) && (p == 0 || !in_base64(file[p - 1])))
            if (auto s = base64_at(file, p, smallest, next)) found.push_back(std::move(*s));
        p = next;
    }
    for (size_t p = 0; p < file.size();)
    {
        size_t next = p + 1;
        if (hex_digit(file[p]) && (p == 0 || !letter_or_digit(file[p - 1])))
            if (auto s = hex_at(file, p, smallest, next)) found.push_back(std::move(*s));
        p = next;
    }
    // Lists: a value starting a word (or after its prefix), in each form and with each prefix;
    // the longest of them at that place.
    static const std::string_view prefixes[] = {"", "0x", "0X", "\\x"};
    for (size_t p = 0; p < file.size();)
    {
        std::optional<Span> best;
        if (p == 0 || !letter_or_digit(file[p - 1]))
            for (const std::string_view prefix : prefixes)
            {
                if (prefix.empty() ? !hex_digit(file[p]) : file[p] != uint8_t(prefix[0])) continue;
                if (!prefix.empty() && p > 0 && file[p - 1] == '\\') continue;
                for (int form = 0; form < 3; ++form)
                    if (auto s = list_at(file, p, form, prefix, smallest); s && (!best || s->bytes.size() > best->bytes.size())) best = std::move(s);
            }
        if (best)
        {
            p = best->at + best->length;
            found.push_back(std::move(*best));
        }
        else
            ++p;
    }
    std::sort(found.begin(), found.end(), [](const Span& a, const Span& b) { return a.at < b.at; });
    std::vector<Span> kept;
    for (Span& s : found)
    {
        if (!kept.empty() && s.at < kept.back().at + kept.back().length)
        {
            if (s.bytes.size() > kept.back().bytes.size()) kept.back() = std::move(s);
            continue;
        }
        kept.push_back(std::move(s));
    }
    // (One kept in place of another can now overlap the one before it: dropped.)
    std::vector<Span> out;
    for (Span& s : kept)
        if (out.empty() || s.at >= out.back().at + out.back().length) out.push_back(std::move(s));
    return out;
}

std::vector<uint8_t> skeleton_of(std::span<const uint8_t> file, const std::vector<Span>& spans)
{
    std::vector<uint8_t> out;
    put_varint(out, spans.size());
    auto put_string = [&](const std::string& t) {
        put_varint(out, t.size());
        out.insert(out.end(), t.begin(), t.end());
    };
    size_t at = 0;
    for (const Span& s : spans)
    {
        put_varint(out, s.at - at);
        out.push_back(s.form.how);
        put_varint(out, s.form.width);
        if ((s.form.how & kKind) == kList)
        {
            put_string(s.form.prefix);
            put_string(s.form.separator);
            if (s.form.width) put_string(s.form.line);
        }
        at = s.at + s.length;
    }
    at = 0;
    for (const Span& s : spans)
    {
        out.insert(out.end(), file.begin() + std::ptrdiff_t(at), file.begin() + std::ptrdiff_t(s.at));
        at = s.at + s.length;
    }
    out.insert(out.end(), file.begin() + std::ptrdiff_t(at), file.end());
    return out;
}

uint64_t members_of(std::span<const uint8_t> skeleton)
{
    size_t at = 0;
    const uint64_t n = get_varint(skeleton, at);
    if (n == 0) throw std::runtime_error("spans-v1: a skeleton without a span");
    return n;
}

std::vector<uint8_t> made_from(std::span<const uint8_t> skeleton, const std::vector<std::span<const uint8_t>>& members)
{
    size_t at = 0;
    const uint64_t n = get_varint(skeleton, at);
    if (n == 0 || n != members.size()) throw std::runtime_error("spans-v1: the skeleton does not count its members");
    auto get_string = [&](std::string& t) {
        const uint64_t length = get_varint(skeleton, at);
        if (length > kLongestString || length > skeleton.size() - at) throw std::runtime_error("spans-v1: a list's string is not written as Sieve writes it");
        t.assign(reinterpret_cast<const char*>(skeleton.data()) + at, size_t(length));
        at += size_t(length);
    };
    std::vector<std::pair<uint64_t, Form>> table;
    table.reserve(members.size());
    for (uint64_t k = 0; k < n; ++k)
    {
        const uint64_t gap = get_varint(skeleton, at);
        if (at >= skeleton.size()) throw std::runtime_error("spans-v1: the skeleton ends inside its table");
        Form form;
        form.how = skeleton[at++];
        const uint64_t width = get_varint(skeleton, at);
        const uint8_t kind = form.how & kKind;
        // Only the bits each kind has: base64 bits 0 to 2; hex bits 0 and 2; a list its form (0 to 2).
        const bool allowed = kind == kBase64 ? (form.how & ~7) == 0 : kind == kHex ? (form.how & ~(kHex | 5)) == 0 : kind == kList && (form.how & ~(kList | 3)) == 0 && (form.how & 3) != 3;
        if (!allowed || width > (uint64_t(1) << 30)) throw std::runtime_error("spans-v1: a span is not written as Sieve writes it");
        form.width = uint32_t(width);
        if (kind == kList)
        {
            get_string(form.prefix);
            get_string(form.separator);
            if (form.width) get_string(form.line);
        }
        table.emplace_back(gap, std::move(form));
    }
    const std::span<const uint8_t> text = skeleton.subspan(at);
    std::vector<uint8_t> out;
    size_t t = 0;
    for (size_t k = 0; k < table.size(); ++k)
    {
        if (table[k].first > text.size() - t) throw std::runtime_error("spans-v1: a span is placed past the skeleton's text");
        out.insert(out.end(), text.begin() + std::ptrdiff_t(t), text.begin() + std::ptrdiff_t(t + table[k].first));
        t += size_t(table[k].first);
        if (members[k].empty()) throw std::runtime_error("spans-v1: an empty member");
        const std::vector<uint8_t> w = written(members[k], table[k].second);
        out.insert(out.end(), w.begin(), w.end());
    }
    out.insert(out.end(), text.begin() + std::ptrdiff_t(t), text.end());
    return out;
}

} // namespace sieve::cli::spans
