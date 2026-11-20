// Sieve — what the filters that build their own automata share (review 4: these were four copies): the bound on an
// automaton's size, and the automata (and answers about them) kept once worked out for a line's shape.
#pragma once

#include "sieve/dfa.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace sieve {

// An automaton is built where its states times its symbols stay under this (max-run-v1's bound).
inline constexpr uint64_t kMaxDfaCells = uint64_t(1) << 24;
// Whether `states` states over `base` symbols are small enough (worked out without overflow).
inline bool cells_fit(uint64_t states, uint64_t base) { return base > 0 && states <= kMaxDfaCells / base; }

// Each built once a line's shape and kept (the filters' lists ask whether one applies often): the automaton, or
// nothing where it does not fit, so a filter is offered only where it counts.
inline std::shared_ptr<const std::optional<Dfa>> kept_dfa(const std::string& key, const std::function<std::optional<Dfa>()>& make)
{
    static std::mutex m;
    static std::map<std::string, std::shared_ptr<const std::optional<Dfa>>> kept;
    {
        std::lock_guard<std::mutex> lock(m);
        if (const auto it = kept.find(key); it != kept.end()) return it->second;
    }
    auto d = std::make_shared<const std::optional<Dfa>>(make());
    std::lock_guard<std::mutex> lock(m);
    return kept.emplace(key, std::move(d)).first->second;
}

// An answer worked out from an automaton once a line's shape and settings and kept, without the automaton (a
// filter's counts_as_on, which the setup menu asks every frame a filter's row is chosen).
inline std::string kept_answer(const std::string& key, const std::function<std::string()>& work)
{
    static std::mutex m;
    static std::map<std::string, std::string> kept;
    {
        std::lock_guard<std::mutex> lock(m);
        if (const auto it = kept.find(key); it != kept.end()) return it->second;
    }
    std::string a = work();
    std::lock_guard<std::mutex> lock(m);
    return kept.emplace(key, std::move(a)).first->second;
}

} // namespace sieve
