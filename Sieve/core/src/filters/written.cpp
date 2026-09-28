// Sieve — not-written-v1 (sieve/written.hpp): a unit of text passes unless some reading of it (the
// vault's decoders, less ascii85) is a file whose first bytes carry a signature: a PNG spelled out
// in hex, a ZIP in base64, a MIDI file in the letters a-p. Such a unit belongs on the binary line.
//
// Judged by decoding the unit outright; counted and ranked by the rule's automata (written.hpp),
// which are made once per alphabet and set of readings and kept for the process. With plugins in
// the same stack, the stack's ranker counts the units they keep that this one keeps too
// (FilterStack, sieve/filter.hpp).

#include "sieve/filekind.hpp"
#include "sieve/filter.hpp"
#include "sieve/written.hpp"

#include <stdexcept>

namespace sieve {

namespace {

class NotWritten : public Filter
{
public:
    NotWritten(const Alphabet& a, uint32_t mask, uint32_t length, std::string provenance)
        : alphabet_(&a), rule_(written_rule(a, mask)), mask_(mask), bytes_(holds_all_bytes(a))
    {
        provenance_ = std::move(provenance);
        std::string why;
        ranker_ = written_ranker(rule_, nullptr, length, why);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        if (bytes_) return !written_as_bytes(unit);
        std::u32string t;
        t.reserve(unit.size());
        for (uint32_t d : unit) t += alphabet_->symbol(d);
        return !written_as(t, mask_);
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    const std::shared_ptr<const WrittenRule>& rule() const { return rule_; }

private:
    const Alphabet* alphabet_;
    std::shared_ptr<const WrittenRule> rule_;
    uint32_t mask_;
    bool bytes_;
    std::unique_ptr<Ranker> ranker_;
};

} // namespace

std::shared_ptr<const WrittenRule> written_rule_of(const Filter& f)
{
    const auto* w = dynamic_cast<const NotWritten*>(&f);
    return w ? w->rule() : nullptr;
}

void add_written_filters(std::vector<FilterSpec>& out)
{
    FilterSpec w;
    w.id = "not-written";
    w.title = "not-written";
    w.description = "Not a file written out: fails a unit that some reading of it (its own bytes, hex, base64, base32, "
                    "decimal, the letters a-p, spelled-out digits, or any two symbols as bits) turns into a file whose "
                    "first bytes carry a signature (file-kinds-v1: PNG, ZIP, MID, PDF, ...). Such a unit belongs on "
                    "the binary line. On a line of every byte, a unit is read only as its own bytes. Exact.";
    std::vector<std::string> choices{"all"};
    for (const auto& r : written_decoders()) choices.push_back(r);
    w.params = {{"readings", "which readings to check: all, or one", FilterParam::Kind::Text, "all", 0, 0, 1, choices}};
    w.applies = [](const FilterLine& l) { return l.kind == "text" && l.alphabet != nullptr; };
    w.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& s = *find_filter("not-written-v1");
        const std::string readings = param_value(s, v, "readings");
        const uint32_t mask = written_mask_of(readings);
        const std::string prov = std::string(kWrittenVersion) + " readings=" + (holds_all_bytes(*l.alphabet) ? std::string("bytes") : readings) +
                                 " kinds=" + kFileKindsVersion;
        return std::make_unique<NotWritten>(*l.alphabet, mask, l.length, prov);
    };
    out.push_back(w);
}

} // namespace sieve
