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
#include "sieve/plugin.hpp"
#include "sieve/written.hpp"

#include <functional>
#include <map>
#include <mutex>
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
    w.counts_as = "written";
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

namespace {

uint32_t bits_of(uint32_t base)
{
    uint32_t b = 0;
    while ((1u << b) < base) ++b;
    return (1u << b) == base ? b : 0;
}

// Kept for the process, per alphabet (or base) and form: the automata of what passes.
std::shared_ptr<const std::optional<Dfa>> kept_dfa(const std::string& key, const std::function<std::optional<Dfa>()>& make)
{
    static std::mutex mx;
    // With the filter memory each was made under: one too large then is made again with more.
    static std::map<std::string, std::pair<double, std::shared_ptr<const std::optional<Dfa>>>> cache;
    {
        std::lock_guard<std::mutex> lock(mx);
        if (auto it = cache.find(key); it != cache.end())
            if (*it->second.second || filter_memory() <= it->second.first) return it->second.second;
    }
    const double memory = filter_memory();
    std::optional<Dfa> d = make();
    auto made = std::make_shared<const std::optional<Dfa>>(d ? std::optional<Dfa>(complement(*d)) : std::nullopt);
    std::lock_guard<std::mutex> lock(mx);
    return cache.insert_or_assign(key, std::pair{memory, made}).first->second.second;
}

class JudgeOther : public Filter
{
public:
    JudgeOther(const Alphabet& a, uint32_t mask, std::string prov) : a_(&a), mask_(mask) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        std::u32string t;
        for (uint32_t d : unit) t += a_->symbol(d);
        return !other_line_as(t, mask_);
    }

private:
    const Alphabet* a_;
    uint32_t mask_;
};

} // namespace

void add_other_line_filters(std::vector<FilterSpec>& out)
{
    FilterSpec o;
    o.id = "not-other-line";
    o.title = "not-other-line";
    o.description = "Not another line's content written as text: fails a page that is melody notation (notes, rests and "
                    "durations, as the audio line reads them) or a model's .obj text (v and f lines). Such a page belongs on "
                    "the audio or the models line. Only alphabets with digits can hold either. Exact.";
    o.params = {{"forms", "which forms: all, notes or obj", FilterParam::Kind::Text, "all", 0, 0, 1, {"all", "notes", "obj"}}};
    o.applies = [](const FilterLine& l) { return l.kind == "text" && l.alphabet != nullptr && !holds_all_bytes(*l.alphabet); };
    o.counts_as = "automaton";
    o.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const std::string forms = param_value(*find_filter("not-other-line-v1"), v, "forms");
        const uint32_t mask = other_line_mask_of(forms);
        const std::string prov = "not-other-line-v1 forms=" + forms;
        const Alphabet& a = *l.alphabet;
        const auto dfa = kept_dfa("other/" + a.id() + "/" + forms, [&] { return other_line_dfa(a, mask); });
        if (*dfa) return make_dfa_filter(**dfa, l.length, prov);
        return std::make_unique<JudgeOther>(a, mask, prov);
    };
    out.push_back(o);

    FilterSpec p;
    p.id = "not-packed";
    p.title = "not-packed";
    p.description = "Not a file in its pixels: fails a picture whose colours, packed as bits (a palette of 2^b colours gives b "
                    "bits a pixel, in address order), make a file whose first bytes carry a signature (file-kinds-v1). That "
                    "picture is a file stored as pixels, and belongs on the binary line. Palettes of 2, 4, 16 or 256 colours. Exact.";
    p.applies = [](const FilterLine& l) { return (l.kind == "image" || l.kind == "video") && bits_of(l.base) > 0 && l.base <= 256; };
    p.counts_as = "automaton";
    p.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t base = l.base;
        const auto dfa = kept_dfa("packed/" + std::to_string(base), [&] { return std::optional<Dfa>(packed_dfa(base, bits_of(base))); });
        return make_dfa_filter(**dfa, l.length, "not-packed-v1 bits=" + std::to_string(bits_of(base)) + " kinds=file-kinds-v1");
    };
    out.push_back(p);
}

} // namespace sieve
