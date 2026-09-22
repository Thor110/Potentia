#include "sieve/filter.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

std::vector<FilterSpec> builtin_filters(); // core/src/filters/builtin.cpp

const std::vector<FilterSpec>& filter_registry()
{
    static const std::vector<FilterSpec> list = [] {
        auto v = builtin_filters();
        for (size_t i = 0; i < v.size(); ++i)
            for (size_t j = 0; j < i; ++j)
                if (v[i].name() == v[j].name()) throw std::logic_error("filter registered twice: " + v[i].name());
        return v;
    }();
    return list;
}

const FilterSpec* find_filter(const std::string& name)
{
    const FilterSpec* newest = nullptr;
    for (const auto& f : filter_registry())
    {
        if (f.name() == name) return &f;
        if (f.id == name && (!newest || f.version > newest->version)) newest = &f;
    }
    return newest;
}

std::vector<const FilterSpec*> filters_for(const FilterLine& line)
{
    std::vector<const FilterSpec*> out;
    for (const auto& f : filter_registry())
        if (f.applies(line)) out.push_back(&f);
    return out;
}

std::string param_value(const FilterSpec& spec, const FilterValues& values, const std::string& key)
{
    for (const auto& p : spec.params)
        if (p.key == key)
        {
            const auto it = values.find(key);
            return it == values.end() ? p.default_value : it->second;
        }
    throw std::logic_error(spec.name() + " has no parameter '" + key + "'");
}

int64_t param_int(const FilterSpec& spec, const FilterValues& values, const std::string& key)
{
    const std::string v = param_value(spec, values, key);
    size_t used = 0;
    int64_t n = 0;
    try { n = std::stoll(v, &used); } catch (const std::exception&) { used = 0; }
    if (v.empty() || used != v.size()) throw std::invalid_argument(spec.name() + ": " + key + " must be a whole number, got '" + v + "'");
    for (const auto& p : spec.params)
        if (p.key == key && (n < p.min || n > p.max))
            throw std::invalid_argument(spec.name() + ": " + key + " must be " + std::to_string(p.min) + ".." + std::to_string(p.max));
    return n;
}

FilterStack::FilterStack(const FilterLine& line, const std::vector<Entry>& entries, const FilterResources& resources)
{
    provenance_ = line.kind + "/" + line.symbols_id + "/L" + std::to_string(line.length);
    for (const auto& e : entries)
    {
        if (!e.spec->applies(line)) throw std::invalid_argument(e.spec->name() + " does not apply to the " + line.kind + " line");
        filters_.push_back(e.spec->make(line, e.values, resources));
        names_.push_back(e.spec->name());
        provenance_ += "; " + e.spec->name() + "{" + filters_.back()->provenance() + "}";
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));

    // Compact: one filter with a ranker whose survivors are exactly the stack's.
    for (size_t i = 0; i < filters_.size() && !compact_; ++i)
    {
        if (!filters_[i]->ranker()) continue;
        bool covers = true;
        for (size_t j = 0; j < entries.size(); ++j)
        {
            if (j == i) continue;
            const auto& imp = entries[i].spec->implies;
            if (std::find(imp.begin(), imp.end(), names_[j]) == imp.end()) covers = false;
        }
        if (covers) compact_ = filters_[i]->ranker();
    }
    if (filters_.empty()) blocker_ = "no filters ticked";
    else if (!compact_)
    {
        // Name the first ranking filter and the ticked filters it does not imply.
        size_t r = filters_.size();
        for (size_t i = 0; i < filters_.size() && r == filters_.size(); ++i)
            if (filters_[i]->ranker()) r = i;
        if (r == filters_.size()) blocker_ = "none of the ticked filters can rank its survivors at this length";
        else
        {
            std::string extra;
            const auto& imp = entries[r].spec->implies;
            for (size_t j = 0; j < names_.size(); ++j)
                if (j != r && std::find(imp.begin(), imp.end(), names_[j]) == imp.end()) extra += (extra.empty() ? "" : ", ") + names_[j];
            blocker_ = names_[r] + " can rank, but not together with " + extra;
        }
    }
}

int FilterStack::first_failure(std::span<const uint32_t> unit) const
{
    for (size_t i = 0; i < filters_.size(); ++i)
        if (!filters_[i]->passes(unit)) return int(i);
    return -1;
}

std::string FilterStack::compact_blocker() const { return blocker_; }

} // namespace sieve
