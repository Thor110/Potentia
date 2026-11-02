// Sieve CLI -- binary written as text, carried as what it is (spans-v1). See text_spans.hpp.

#include "cli/text_spans.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace sieve::cli::spans {

namespace {

constexpr char kStandard[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char kUrlSafe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

bool letter_or_digit(uint8_t c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); }
bool in_either(uint8_t c) { return letter_or_digit(c) || c == '+' || c == '/' || c == '-' || c == '_'; }
int value_of(uint8_t c, bool url)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
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

// What a span would be, read from `p`: its lines (each a run of the two alphabets' characters), a
// line end between each two of the same kind, "=" at most twice at its end. Returns its length in the
// file (0 for none), with `width` and `crlf` as read; the checks of what it holds come after.
size_t read_span(std::span<const uint8_t> f, size_t p, uint32_t& width, bool& crlf)
{
    auto run = [&](size_t q) {
        size_t e = q;
        while (e < f.size() && in_either(f[e])) ++e;
        return e - q;
    };
    auto pads = [&](size_t q) {
        size_t n = 0;
        while (n < 2 && q + n < f.size() && f[q + n] == '=') ++n;
        return n;
    };
    auto line_end = [&](size_t q) -> size_t { // its length, 0 for none
        if (q < f.size() && f[q] == '\n') return 1;
        if (q + 1 < f.size() && f[q] == '\r' && f[q + 1] == '\n') return 2;
        return 0;
    };
    // What may follow a span's last line: the file's end, a line end, or what closes a quotation,
    // a tag or a bracket, or separates a field (not a space: so a word of text after a block is not
    // read as its last line).
    auto closes = [&](size_t q) {
        if (q >= f.size() || line_end(q)) return true;
        const uint8_t c = f[q];
        return c == '"' || c == '\'' || c == '<' || c == '>' || c == ')' || c == ']' || c == '}' || c == ',' || c == ';' || c == ':' || c == '`';
    };
    const size_t first = run(p);
    if (first == 0) return 0;
    width = 0;
    crlf = false;
    size_t end = p + first + pads(p + first);
    const size_t eol = line_end(p + first);
    if (pads(p + first) || !eol) return end - p;
    // In lines: every line the first's width, and the last no wider, then something that closes it.
    crlf = eol == 2;
    const uint32_t w = uint32_t(first);
    size_t q = p + first; // the end of the last line taken
    size_t lines = 1;
    for (;;)
    {
        const size_t e = line_end(q);
        if (e != eol) break;
        const size_t s = q + e;
        const size_t r = run(s);
        if (r == 0 || r > w) break;
        const size_t pd = pads(s + r);
        if (r == w && !pd && line_end(s + r) == eol)
        {
            q = s + r;
            ++lines;
            continue;
        }
        if (closes(s + r + pd))
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

} // namespace

std::vector<uint8_t> base64_written(std::span<const uint8_t> bytes, uint8_t how, uint32_t width)
{
    const char* const a = (how & 1) ? kUrlSafe : kStandard;
    std::string chars;
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
        if (how & 2) chars.append(3 - left, '=');
    }
    std::vector<uint8_t> out;
    out.reserve(chars.size() + (width ? chars.size() / width * 2 : 0));
    for (size_t k = 0; k < chars.size(); ++k)
    {
        if (width && k && k % width == 0)
        {
            if (how & 4) out.push_back('\r');
            out.push_back('\n');
        }
        out.push_back(uint8_t(chars[k]));
    }
    return out;
}

std::vector<Span> base64_spans(std::span<const uint8_t> file, size_t smallest)
{
    std::vector<Span> spans;
    for (size_t p = 0; p < file.size();)
    {
        if (!in_either(file[p]) || (p > 0 && in_either(file[p - 1])))
        {
            ++p;
            continue;
        }
        uint32_t width = 0;
        bool crlf = false;
        const size_t length = read_span(file, p, width, crlf);
        if (length == 0)
        {
            ++p;
            continue;
        }
        const size_t next = p + length;
        // What it holds: the alphabet, the padding, the value of each character; at least
        // `smallest` bytes, and some of each of capitals, small letters and digits (a long word,
        // a path or a name is rarely all three).
        bool standard = false, url = false, upper = false, lower = false, digit = false;
        size_t chars = 0, padded = 0;
        for (size_t k = p; k < next; ++k)
        {
            const uint8_t c = file[k];
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
            digit |= c >= '0' && c <= '9';
        }
        const size_t bytes = chars * 3 / 4;
        if ((standard && url) || !upper || !lower || !digit || chars % 4 == 1 || bytes < smallest || (padded && (chars + padded) % 4 != 0))
        {
            p = next;
            continue;
        }
        Span s;
        s.at = p;
        s.length = length;
        s.width = width;
        s.how = uint8_t((url ? 1 : 0) | (padded || chars % 4 == 0 ? 2 : 0) | (crlf ? 4 : 0));
        s.bytes.reserve(bytes);
        uint32_t acc = 0;
        int bits = 0;
        for (size_t k = p; k < next; ++k)
        {
            const int v = value_of(file[k], url);
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
        // Checked: written again, the same bytes (which also refuses a last character with bits
        // set past the bytes, which decoding drops).
        const std::vector<uint8_t> again = base64_written(s.bytes, s.how, s.width);
        if (again.size() == length && std::equal(again.begin(), again.end(), file.begin() + std::ptrdiff_t(p))) spans.push_back(std::move(s));
        p = next;
    }
    return spans;
}

std::vector<uint8_t> skeleton_of(std::span<const uint8_t> file, const std::vector<Span>& spans)
{
    std::vector<uint8_t> out;
    put_varint(out, spans.size());
    size_t at = 0;
    for (const Span& s : spans)
    {
        put_varint(out, s.at - at);
        out.push_back(s.how);
        put_varint(out, s.width);
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
    struct Where
    {
        uint64_t gap;
        uint8_t how;
        uint32_t width;
    };
    std::vector<Where> table;
    table.reserve(members.size());
    for (uint64_t k = 0; k < n; ++k)
    {
        Where w{};
        w.gap = get_varint(skeleton, at);
        if (at >= skeleton.size()) throw std::runtime_error("spans-v1: the skeleton ends inside its table");
        w.how = skeleton[at++];
        const uint64_t width = get_varint(skeleton, at);
        if (w.how > 7 || width > (uint64_t(1) << 30)) throw std::runtime_error("spans-v1: a span is not written as Sieve writes it");
        w.width = uint32_t(width);
        table.push_back(w);
    }
    const std::span<const uint8_t> text = skeleton.subspan(at);
    std::vector<uint8_t> out;
    size_t t = 0;
    for (size_t k = 0; k < table.size(); ++k)
    {
        if (table[k].gap > text.size() - t) throw std::runtime_error("spans-v1: a span is placed past the skeleton's text");
        out.insert(out.end(), text.begin() + std::ptrdiff_t(t), text.begin() + std::ptrdiff_t(t + table[k].gap));
        t += size_t(table[k].gap);
        if (members[k].empty()) throw std::runtime_error("spans-v1: an empty member");
        const std::vector<uint8_t> w = base64_written(members[k], table[k].how, table[k].width);
        out.insert(out.end(), w.begin(), w.end());
    }
    out.insert(out.end(), text.begin() + std::ptrdiff_t(t), text.end());
    return out;
}

} // namespace sieve::cli::spans
