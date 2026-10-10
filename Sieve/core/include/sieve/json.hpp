// Sieve — a small JSON reader (RFC 8259), for the files models are published with: a safetensors
// file's header and a model's config.json (sieve/safetensors.hpp).
//
// Strict: one value with only white space around it; no comments, no trailing commas. An object
// keeps its members in the order they were written, as the safetensors header's order matters.
// Numbers keep their text; integers that fit read exactly as unsigned 64-bit.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sieve::json {

class Value
{
public:
    enum class Kind { Null, Bool, Number, String, Array, Object };

    Kind kind() const { return kind_; }
    bool is_null() const { return kind_ == Kind::Null; }
    bool is_object() const { return kind_ == Kind::Object; }
    bool is_array() const { return kind_ == Kind::Array; }
    bool is_string() const { return kind_ == Kind::String; }
    bool is_number() const { return kind_ == Kind::Number; }
    bool is_bool() const { return kind_ == Kind::Bool; }

    // Each throws std::invalid_argument if the value is not of that kind.
    bool boolean() const;
    const std::string& string() const;
    double number() const;
    uint64_t u64() const; // a non-negative integer that fits, written without a fraction or exponent
    const std::string& number_text() const;
    const std::vector<Value>& array() const;
    const std::vector<std::pair<std::string, Value>>& object() const;

    // An object's member, or nullptr (the first, if a name is written twice).
    const Value* find(std::string_view name) const;
    // An object's member; throws if there is none.
    const Value& at(std::string_view name) const;

    static Value make_null() { return Value(); }
    static Value make_bool(bool b);
    static Value make_number(std::string text);
    static Value make_string(std::string s);
    static Value make_array(std::vector<Value> a);
    static Value make_object(std::vector<std::pair<std::string, Value>> o);

private:
    Kind kind_ = Kind::Null;
    bool b_ = false;
    std::string s_; // a string, or a number's text
    std::vector<Value> a_;
    std::vector<std::pair<std::string, Value>> o_;
};

// Parses one JSON text. Throws std::invalid_argument, naming the byte offset, on malformed input.
Value parse(std::string_view text);

// Writes a value compactly (no spaces), members in their order, strings escaped as serde_json
// does: \" \\ \b \f \n \r \t, other control characters as \u00XX, everything else as it is.
std::string write(const Value& v);

} // namespace sieve::json
