// Sieve — the filtration stack (SPECIFICATIONS §8, §9).
//
// A filter is a self-contained, versioned module that decides whether a unit passes. Filters are
// compiled in and listed in one place (core/src/filters/builtin.cpp); adding a filter is one new
// source file plus one line there. A changed filter is registered as a new version beside the
// old one (words-v1, words-v2, ...), so earlier results stay reproducible.
//
// Every pass/fail decision is exact: integer arithmetic only (see sieve/intlog.hpp), so every
// machine and the reference oracle agree bit for bit.
//
// Some filters can also count and rank the units that pass (a Ranker): the number of survivors
// at any unit length, and "the k-th survivor in address order" without visiting the others.
// The hallway uses that for its compact mode, where only survivors stand on the shelves.
//
// A stack is the filters ticked for one line; a unit must pass all of them. The stack's
// provenance (every filter's id, version, parameters and the hashes of its data) is recorded
// with any result, and hashed into a stack id.
#pragma once

#include "sieve/alphabet.hpp"
#include "sieve/biguint.hpp"
#include "sieve/model.hpp"
#include "sieve/sieve.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace sieve {

// The shape of the line a filter is asked to judge.
struct FilterLine
{
    std::string kind;          // "text", "image", "audio", "video"
    std::string symbols_id;    // alphabet id, palette-based id, or notes id
    uint32_t base = 0;         // symbols per position
    uint32_t length = 0;       // positions per unit
    const Alphabet* alphabet = nullptr; // text only
    uint32_t width = 0, height = 0, frames = 0; // image and video
};

// Pinned data a filter may need, provided by the application (registries, hash checks).
class FilterResources
{
public:
    virtual ~FilterResources() = default;
    // Registry id ("" = default). Throws if unknown or its hash does not match.
    virtual std::shared_ptr<const Dictionary> dictionary(const std::string& id) const = 0;
    // Registry id ("" = the alphabet's default). Throws if none.
    virtual std::shared_ptr<const CharModel> model(const std::string& id, const std::string& symbols) const = 0;
};

// One adjustable parameter of a filter.
struct FilterParam
{
    enum class Kind { Integer, Text };
    std::string key;
    std::string description;
    Kind kind = Kind::Integer;
    std::string default_value;
    int64_t min = 0, max = 0, step = 1; // Integer only
    std::vector<std::string> choices;   // Text only: the allowed values, if a fixed list
};
using FilterValues = std::map<std::string, std::string>; // key -> value; missing keys take defaults

// Counts and ranks the units that pass a filter, in address (positional) order.
//
// A ranker is a walk along a unit, one symbol at a time, through a finite set of states: from a
// state, next(state, symbol) is the state after that symbol (or kDead when no survivor can
// continue that way), and completions(state, r) is the number of ways to finish with r more
// symbols so that the whole unit passes. From these alone, the base class counts the survivors,
// ranks and unranks them, and the guided coder can restrict itself to the symbols that still
// lead to a survivor (sieve/guided.hpp).
class Ranker
{
public:
    using State = uint64_t;
    static constexpr State kDead = ~State(0);

    virtual ~Ranker() = default;
    virtual uint32_t length() const = 0;  // unit length
    virtual uint32_t base() const = 0;    // symbols per position
    virtual State start() const = 0;
    virtual State next(State s, uint32_t symbol) const = 0;
    virtual BigUint completions(State s, uint32_t remaining) const = 0;
    // completions(s, remaining) > 0; rankers override it when that is cheaper to decide.
    virtual bool alive(State s, uint32_t remaining) const { return !completions(s, remaining).is_zero(); }

    const BigUint& count() const { return count_; }                  // survivors at this length
    std::vector<uint32_t> unrank(const BigUint& k) const;             // k < count()
    BigUint rank(std::span<const uint32_t> unit) const;               // unit must pass
    // Whether the unit is a survivor, by walking it (agrees with the filter's own test).
    bool accepts(std::span<const uint32_t> unit) const;

protected:
    // Derived constructors call this last: count() = completions(start(), length()).
    void set_count() { count_ = completions(start(), length()); }

private:
    BigUint count_;
};

class Filter
{
public:
    virtual ~Filter() = default;
    virtual bool passes(std::span<const uint32_t> unit) const = 0;
    virtual const Ranker* ranker() const { return nullptr; }
    // Parameters and data hashes, for provenance ("dictionary=scowl-en-60 sha256=...").
    const std::string& provenance() const { return provenance_; }

protected:
    std::string provenance_;
};

// A registered filter: identity, parameters, where it applies, and how to build it.
struct FilterSpec
{
    std::string id;       // "words"
    uint32_t version = 1; // -> "words-v1"
    std::string title;    // short, for lists
    std::string description;
    std::vector<FilterParam> params;
    std::vector<std::string> implies; // full names of filters every survivor of this one also passes
    std::function<bool(const FilterLine&)> applies;
    std::function<std::unique_ptr<Filter>(const FilterLine&, const FilterValues&, const FilterResources&)> make;

    std::string name() const { return id + "-v" + std::to_string(version); }
};

// All compiled-in filters, in list order.
const std::vector<FilterSpec>& filter_registry();
const FilterSpec* find_filter(const std::string& name); // "words-v1", or "words" for the newest version
std::vector<const FilterSpec*> filters_for(const FilterLine& line);

// The value of a parameter, or its default. Throws if an integer is malformed or out of range.
std::string param_value(const FilterSpec& spec, const FilterValues& values, const std::string& key);
int64_t param_int(const FilterSpec& spec, const FilterValues& values, const std::string& key);

// The ticked filters of one line.
class FilterStack
{
public:
    struct Entry
    {
        const FilterSpec* spec;
        FilterValues values;
    };
    FilterStack() = default;
    FilterStack(const FilterLine& line, const std::vector<Entry>& entries, const FilterResources& resources);

    bool empty() const { return filters_.empty(); }
    size_t size() const { return filters_.size(); }
    // Index of the first filter the unit fails, or -1 if it passes them all.
    int first_failure(std::span<const uint32_t> unit) const;
    bool passes(std::span<const uint32_t> unit) const { return first_failure(unit) < 0; }
    const std::string& filter_name(size_t i) const { return names_[i]; }

    // A ranker for the whole stack, if one filter has a ranker and implies every other ticked
    // filter (so its survivors are exactly the stack's survivors). Otherwise nullptr.
    const Ranker* ranker() const { return compact_; }
    std::string compact_blocker() const; // why there is no ranker ("" if there is one)

    // "words-v1{dictionary=scowl-en-60 sha256=...}; max-run-v1{max_run=2}" and its SHA-256.
    const std::string& provenance() const { return provenance_; }
    const std::string& id() const { return id_; }

private:
    std::vector<std::unique_ptr<Filter>> filters_;
    std::vector<std::string> names_;
    const Ranker* compact_ = nullptr;
    std::string blocker_;
    std::string provenance_, id_;
};

} // namespace sieve
