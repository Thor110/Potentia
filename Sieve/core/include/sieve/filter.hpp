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
#include <mutex>
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
    std::string registry = {};          // Text only: "dictionary" to choose from the registered dictionaries
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
    // The generic walks below work for every ranker; a ranker may override them with a faster
    // method that gives the same answers (key-v1 ranks as a plain base-a number).
    virtual std::vector<uint32_t> unrank(const BigUint& k) const;     // k < count()
    virtual BigUint rank(std::span<const uint32_t> unit) const;       // unit must pass
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
    // Whether ranker() would give a ranker, without building it (a large automaton's table takes
    // seconds and hundreds of megabytes, so it is built only when a stack will use it).
    virtual bool can_rank() const { return ranker() != nullptr; }
    // Parameters and data hashes, for provenance ("dictionary=scowl-en-60 sha256=...").
    const std::string& provenance() const { return provenance_; }
    // The memory its ranker's tables take, or would take where they pass the filter memory
    // (plugin.hpp): what counting it needs. 0 where that is small or there is no table.
    double table_bytes() const { return table_bytes_; }
    void set_table_bytes(double bytes) { table_bytes_ = bytes; }

protected:
    std::string provenance_;
    double table_bytes_ = 0;
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
    // How the filter counts, for which filters can be counted together (docs/FILTERS-CONFLICTS.md):
    //   "automaton"   an automaton over the symbols: combines with other automata (plugins,
    //                 not-other-line, not-packed) and with not-written
    //   "written"     not-written: combines with automata
    //   "own"         its own ranker (the word filters, neighbour-agreement, key): an automaton
    //                 underneath, not yet merged with others ("filters need merging")
    //   "arithmetic"  counted by arithmetic on the whole unit (not-a-file, not-a-pattern): combines
    //                 with nothing ("conflicting filters")
    //   "model-rule"  the models line's own rules: combine with each other
    //   ""            judges only, or has no rule about combining (never unticks anything)
    std::string counts_as;
    // Where how it counts depends on the line (a filter that is an automaton on small palettes and
    // counts its own way on large ones): what it counts as there, at the largest settings the line
    // allows, so it is never said to merge where it cannot. Empty: counts_as everywhere.
    std::function<std::string(const FilterLine&)> counts_as_on;
    // Retired: kept, and loadable, so earlier stacks still reproduce, but no longer offered beside
    // the others: it judges only and would stop a line compacting (its condensed address space), or
    // another filter does the same as an automaton, which counts and merges (`replaced_by`). The
    // menus list retired filters on a tab of their own, tick them only by hand, and leave them out
    // of "tick all".
    bool retired = false;
    std::string replaced_by; // the filter that does the same, when that is why it is retired
    // Hard or soft (docs/FILTER-PLUGINS.md §17): "hard" sets aside only noise, with no collateral
    // (structural and exclusion rules: content of another line, a re-encoding of the same mesh, a
    // pattern); "soft" may set aside things a person would keep (a dictionary, a model, a key, a
    // rule of style). A label for the lists and for ticking a category at once, not part of the
    // rule: it changes no version. "" for a filter not yet labelled (plugins, for now).
    std::string category;
    std::string author, origin, plugin_sha256; // plugins only (sieve/plugin.hpp): who made it, and its file
    // Filters this one needs switched on with it (a plugin's `requires` lines), each by its full
    // name and, where the plugin pins them, settings. Not `implies`: this says what must be ticked
    // alongside, not what its survivors are known to pass.
    struct Prerequisite
    {
        std::string name;
        FilterValues values;
    };
    std::vector<Prerequisite> prerequisites;
    std::function<bool(const FilterLine&)> applies;
    std::function<std::unique_ptr<Filter>(const FilterLine&, const FilterValues&, const FilterResources&)> make;

    std::string name() const { return id + "-v" + std::to_string(version); }
};

// All compiled-in filters, in list order.
const std::vector<FilterSpec>& filter_registry();
const FilterSpec* find_filter(const std::string& name); // "words-v1", or "words" for the newest version
std::vector<const FilterSpec*> filters_for(const FilterLine& line);

// Whether two filters for the same line cannot be counted together, and why: "" (they can, or one
// judges only), "merge" (both are automata underneath but are not merged yet: "filters need
// merging") or "conflict" (one is counted by arithmetic: "conflicting filters"). A filter implied
// by the other never conflicts with it. See docs/FILTERS-CONFLICTS.md.
// With a line, a filter's counts_as_on decides how it counts there; without one, counts_as does.
std::string filter_conflict(const FilterSpec& a, const FilterSpec& b, const FilterLine* line = nullptr);

// The value of a parameter, or its default. Throws if an integer is malformed or out of range.
std::string param_value(const FilterSpec& spec, const FilterValues& values, const std::string& key);
int64_t param_int(const FilterSpec& spec, const FilterValues& values, const std::string& key);

// V voices' ranker from one voice's (the one kept alive by the caller); see FilterStack.
std::unique_ptr<Ranker> voices_ranker(const Ranker& one, uint32_t voices);
// How many voices a line's filters judge one at a time: a note set's voices, a sound's channels
// (sieve/sound.hpp), else 1.
uint32_t line_voices(const FilterLine& l);

// The ticked filters of one line.
//
// On a note line of several voices (notes2, sieve/audio.hpp) the filters are made for one voice's
// line (its length the line's divided by the voices) and judge each voice on its own: a unit
// passes when every voice does. Its survivors are then those of one voice to the power of the
// voices, and a ranker for one voice ranks the whole (VoicesRanker): the unit's rank is its
// voices' ranks read as one number in base (one voice's count), voice 1 first.
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
    // Worked out on first use (see filter.cpp), so a stack that only judges costs nothing for it.
    const Ranker* ranker() const;
    std::string compact_blocker() const; // why there is no ranker ("" if there is one)
    // The memory the stack's count needs for its tables (the plugins' combined automaton's, not-written
    // with them, or the one filter that ranks it): what the filter memory must hold for it to count.
    double table_bytes() const;

    // "words-v1{dictionary=scowl-en-60 sha256=...}; max-run-v1{max_run=2}" and its SHA-256.
    const std::string& provenance() const { return provenance_; }
    const std::string& id() const { return id_; }

    uint32_t voices() const { return voices_; }

private:
    std::vector<std::unique_ptr<Filter>> filters_;
    std::vector<std::string> names_;
    uint32_t length_ = 0, voices_ = 1;
    // Compact, worked out once, when first asked for (settle()).
    std::vector<Entry> entries_;
    uint32_t unit_length_ = 0;
    std::unique_ptr<std::once_flag> lazy_;
    void settle() const;
    void settle_now() const;
    mutable const Ranker* compact_ = nullptr;
    mutable std::unique_ptr<Ranker> own_ranker_; // a stack of plugins with not-written: its ranker
    mutable std::shared_ptr<const Ranker> shared_ranker_; // plugins alone: their combined table, perhaps shared
    mutable std::unique_ptr<Ranker> voices_ranker_; // several voices: one voice's ranker, for them all
    mutable std::string blocker_;
    mutable double table_bytes_ = 0;
    std::string provenance_, id_;
    std::string symbols_id_; // the line's kind and symbols, without its length: what merges are kept by
};

} // namespace sieve
