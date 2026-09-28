// Sieve — the binary line's filters. binary-kind-v1 keeps the files of chosen kinds, read from
// their own first bytes by the file-kinds-v1 table (sieve/filekind.hpp): signed (every kind a
// signature gives), text, the two together, unknown, empty, any, or one kind (png, zip, mid, ...);
// or, with keep = exclude, every file but those. A file is judged from its head and size alone,
// and the survivors are counted and ranked exactly, so the hallway can close them up (compact).
//
// The binary line's files have lengths of their own, so its filters are not a FilterStack over
// digits: BinarySieve (filekind.hpp) judges and counts them. The spec here gives the filter its
// name, its settings and its place in every list.

#include "sieve/filekind.hpp"
#include "sieve/filter.hpp"

#include <stdexcept>

namespace sieve {

void add_binary_filters(std::vector<FilterSpec>& out)
{
    FilterSpec k;
    k.id = "binary-kind";
    k.title = "binary-kind";
    k.description = "Files of the chosen kinds, as their own first bytes say (file-kinds-v1: PNG, ZIP, MID, ... by "
                    "signature, TXT when every byte is readable, ? otherwise), or with keep = exclude every file but "
                    "those. Exact: the survivors are counted and ranked however long the line is.";
    k.params = {{"kinds", "which kinds: signed, text, signed-or-text, unknown, empty, any, or one kind", FilterParam::Kind::Text,
                 "signed", 0, 0, 1, kind_set_names()},
                {"keep", "keep those kinds, or exclude them", FilterParam::Kind::Text, "keep", 0, 0, 1, {"keep", "exclude"}}};
    k.applies = [](const FilterLine& l) { return l.kind == "binary"; };
    k.make = [](const FilterLine&, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        throw std::invalid_argument("binary-kind-v1 judges files on the binary line (a BinarySieve), not units of digits");
    };
    out.push_back(k);
}

} // namespace sieve
