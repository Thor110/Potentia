// Sieve — a small JSON reader (see json.hpp).
#include "sieve/json.hpp"

#include <cstdlib>
#include <stdexcept>

namespace sieve::json {

namespace {

[[noreturn]] void fail(size_t at, const std::string& what)
{
    throw std::invalid_argument("JSON: " + what + " at byte " + std::to_string(at));
}

void append_utf8(std::string& out, uint32_t cp)
{
    if (cp < 0x80) out.push_back(char(cp));
    else if (cp < 0x800)
    {
        out.push_back(char(0xC0 | (cp >> 6)));
        out.push_back(char(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
        out.push_back(char(0xE0 | (cp >> 12)));
        out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(char(0x80 | (cp & 0x3F)));
    }
    else
    {
        out.push_back(char(0xF0 | (cp >> 18)));
        out.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(char(0x80 | (cp & 0x3F)));
    }
}

class Parser
{
public:
    explicit Parser(std::string_view t) : t_(t) {}

    Value document()
    {
        space();
        Value v = value(0);
        space();
        if (i_ != t_.size()) fail(i_, "text after the value");
        return v;
    }

private:
    std::string_view t_;
    size_t i_ = 0;
    static constexpr int kMaxDepth = 512;

    void space()
    {
        while (i_ < t_.size() && (t_[i_] == ' ' || t_[i_] == '\t' || t_[i_] == '\n' || t_[i_] == '\r')) ++i_;
    }
    bool take(char c)
    {
        if (i_ < t_.size() && t_[i_] == c)
        {
            ++i_;
            return true;
        }
        return false;
    }
    void expect(char c)
    {
        if (!take(c)) fail(i_, std::string("expected '") + c + "'");
    }
    void word(std::string_view w)
    {
        if (t_.substr(i_, w.size()) != w) fail(i_, "expected " + std::string(w));
        i_ += w.size();
    }

    Value value(int depth)
    {
        if (depth > kMaxDepth) fail(i_, "nested too deeply");
        if (i_ >= t_.size()) fail(i_, "expected a value");
        switch (t_[i_])
        {
        case '{': return object(depth);
        case '[': return array(depth);
        case '"': return Value::make_string(string());
        case 't': word("true"); return Value::make_bool(true);
        case 'f': word("false"); return Value::make_bool(false);
        case 'n': word("null"); return Value::make_null();
        default: return number();
        }
    }

    Value object(int depth)
    {
        expect('{');
        std::vector<std::pair<std::string, Value>> members;
        space();
        if (take('}')) return Value::make_object(std::move(members));
        for (;;)
        {
            space();
            if (i_ >= t_.size() || t_[i_] != '"') fail(i_, "expected a member name");
            std::string name = string();
            space();
            expect(':');
            space();
            Value v = value(depth + 1);
            members.emplace_back(std::move(name), std::move(v));
            space();
            if (take('}')) return Value::make_object(std::move(members));
            expect(',');
        }
    }

    Value array(int depth)
    {
        expect('[');
        std::vector<Value> items;
        space();
        if (take(']')) return Value::make_array(std::move(items));
        for (;;)
        {
            space();
            items.push_back(value(depth + 1));
            space();
            if (take(']')) return Value::make_array(std::move(items));
            expect(',');
        }
    }

    uint32_t hex4()
    {
        if (i_ + 4 > t_.size()) fail(i_, "a short \\u escape");
        uint32_t v = 0;
        for (int k = 0; k < 4; ++k)
        {
            const char c = t_[i_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= uint32_t(c - '0');
            else if (c >= 'a' && c <= 'f') v |= uint32_t(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= uint32_t(c - 'A' + 10);
            else fail(i_ - 1, "a bad hex digit");
        }
        return v;
    }

    std::string string()
    {
        expect('"');
        std::string out;
        for (;;)
        {
            if (i_ >= t_.size()) fail(i_, "an unterminated string");
            const unsigned char c = static_cast<unsigned char>(t_[i_++]);
            if (c == '"') return out;
            if (c < 0x20) fail(i_ - 1, "a control character in a string");
            if (c != '\\')
            {
                out.push_back(char(c));
                continue;
            }
            if (i_ >= t_.size()) fail(i_, "an unterminated escape");
            switch (t_[i_++])
            {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u':
            {
                uint32_t cp = hex4();
                if (cp >= 0xD800 && cp < 0xDC00)
                {
                    if (t_.substr(i_, 2) != "\\u") fail(i_, "a lone high surrogate");
                    i_ += 2;
                    const uint32_t lo = hex4();
                    if (lo < 0xDC00 || lo >= 0xE000) fail(i_, "a bad low surrogate");
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                }
                else if (cp >= 0xDC00 && cp < 0xE000) fail(i_, "a lone low surrogate");
                append_utf8(out, cp);
                break;
            }
            default: fail(i_ - 1, "a bad escape");
            }
        }
    }

    Value number()
    {
        const size_t start = i_;
        take('-');
        auto digits = [&] {
            const size_t d = i_;
            while (i_ < t_.size() && t_[i_] >= '0' && t_[i_] <= '9') ++i_;
            if (i_ == d) fail(i_, "expected a digit");
        };
        if (take('0')) {}
        else digits();
        if (take('.')) digits();
        if (i_ < t_.size() && (t_[i_] == 'e' || t_[i_] == 'E'))
        {
            ++i_;
            if (!take('+')) take('-');
            digits();
        }
        return Value::make_number(std::string(t_.substr(start, i_ - start)));
    }
};

void write_string(std::string& out, const std::string& s)
{
    static const char* hex = "0123456789abcdef";
    out.push_back('"');
    for (const char ch : s)
    {
        const unsigned char c = static_cast<unsigned char>(ch);
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20)
            {
                out += "\\u00";
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 15]);
            }
            else out.push_back(ch);
        }
    }
    out.push_back('"');
}

void write_value(std::string& out, const Value& v)
{
    switch (v.kind())
    {
    case Value::Kind::Null: out += "null"; break;
    case Value::Kind::Bool: out += v.boolean() ? "true" : "false"; break;
    case Value::Kind::Number: out += v.number_text(); break;
    case Value::Kind::String: write_string(out, v.string()); break;
    case Value::Kind::Array:
    {
        out.push_back('[');
        bool first = true;
        for (const Value& x : v.array())
        {
            if (!first) out.push_back(',');
            first = false;
            write_value(out, x);
        }
        out.push_back(']');
        break;
    }
    case Value::Kind::Object:
    {
        out.push_back('{');
        bool first = true;
        for (const auto& [k, x] : v.object())
        {
            if (!first) out.push_back(',');
            first = false;
            write_string(out, k);
            out.push_back(':');
            write_value(out, x);
        }
        out.push_back('}');
        break;
    }
    }
}

[[noreturn]] void wrong(const char* want)
{
    throw std::invalid_argument(std::string("JSON: expected ") + want);
}

} // namespace

bool Value::boolean() const
{
    if (kind_ != Kind::Bool) wrong("true or false");
    return b_;
}
const std::string& Value::string() const
{
    if (kind_ != Kind::String) wrong("a string");
    return s_;
}
const std::string& Value::number_text() const
{
    if (kind_ != Kind::Number) wrong("a number");
    return s_;
}
double Value::number() const { return std::strtod(number_text().c_str(), nullptr); }
uint64_t Value::u64() const
{
    const std::string& t = number_text();
    if (t.empty() || t.size() > 20) wrong("a whole number that fits in 64 bits");
    uint64_t v = 0;
    for (const char c : t)
    {
        if (c < '0' || c > '9') wrong("a whole number that fits in 64 bits");
        const uint64_t d = uint64_t(c - '0');
        if (v > (UINT64_MAX - d) / 10) wrong("a whole number that fits in 64 bits");
        v = v * 10 + d;
    }
    return v;
}
const std::vector<Value>& Value::array() const
{
    if (kind_ != Kind::Array) wrong("an array");
    return a_;
}
const std::vector<std::pair<std::string, Value>>& Value::object() const
{
    if (kind_ != Kind::Object) wrong("an object");
    return o_;
}
const Value* Value::find(std::string_view name) const
{
    for (const auto& [k, v] : object())
        if (k == name) return &v;
    return nullptr;
}
const Value& Value::at(std::string_view name) const
{
    if (const Value* v = find(name)) return *v;
    throw std::invalid_argument("JSON: no member \"" + std::string(name) + "\"");
}

Value Value::make_bool(bool b)
{
    Value v;
    v.kind_ = Kind::Bool;
    v.b_ = b;
    return v;
}
Value Value::make_number(std::string text)
{
    Value v;
    v.kind_ = Kind::Number;
    v.s_ = std::move(text);
    return v;
}
Value Value::make_string(std::string s)
{
    Value v;
    v.kind_ = Kind::String;
    v.s_ = std::move(s);
    return v;
}
Value Value::make_array(std::vector<Value> a)
{
    Value v;
    v.kind_ = Kind::Array;
    v.a_ = std::move(a);
    return v;
}
Value Value::make_object(std::vector<std::pair<std::string, Value>> o)
{
    Value v;
    v.kind_ = Kind::Object;
    v.o_ = std::move(o);
    return v;
}

Value parse(std::string_view text) { return Parser(text).document(); }

std::string write(const Value& v)
{
    std::string out;
    write_value(out, v);
    return out;
}

} // namespace sieve::json
