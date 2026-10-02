// Sieve — the binary line's filters. binary-kind-v1 keeps the files of chosen kinds, read from
// their own first bytes by the file-kinds-v1 table (sieve/filekind.hpp): signed (every kind a
// signature gives), text, the two together, unknown, empty, any, or one kind (png, zip, mid, ...);
// or, with keep = exclude, every file but those. A file is judged from its head and size alone,
// and the survivors are counted and ranked exactly, so the hallway can close them up (compact).
//
// utf8-valid-v1 keeps the files that are well-formed UTF-8 throughout (Utf8Counter, filekind.hpp).
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

    FilterSpec n;
    n.id = "not-an-item";
    n.title = "not-an-item";
    n.description = "Not another line's item: fails a file that is exactly an item of another line, as F saves it (a page of "
                    "the pages line as its text, a picture as its PNG, a melody as its MIDI file, a model as its .obj). "
                    "Pages whose symbols are one byte each count exactly; melodies, pictures and models are judged file "
                    "by file, so with them the survivors cannot be counted (hide, not compact).";
    n.params = {{"items", "which items: all, pages, melodies, pictures or models", FilterParam::Kind::Text, "pages", 0, 0, 1,
                 {"pages", "all", "melodies", "pictures", "models"}}};
    n.applies = [](const FilterLine& l) { return l.kind == "binary"; };
    n.make = [](const FilterLine&, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        throw std::invalid_argument("not-an-item-v1 judges files on the binary line (a BinarySieve), not units of digits");
    };
    out.push_back(n);

    FilterSpec u;
    u.id = "utf8-valid";
    u.title = "utf8-valid";
    u.description = "Well-formed UTF-8 from the first byte to the last (no overlong forms, no surrogates, nothing past "
                    "U+10FFFF), and with controls = text no control characters but tab, line feed and carriage return: the "
                    "whole file is text, where binary-kind's TXT reads only its first 16 bytes. Exact: counted and ranked "
                    "on its own, at lengths whose table fits; with the kind filters it judges file by file.";
    u.params = {{"controls", "any: every well-formed character; text: no control characters but tab, line feed and "
                             "carriage return", FilterParam::Kind::Text, "text", 0, 0, 1, {"text", "any"}}};
    u.applies = [](const FilterLine& l) { return l.kind == "binary"; };
    u.counts_as = "own";
    u.make = [](const FilterLine&, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        throw std::invalid_argument("utf8-valid-v1 judges files on the binary line (a BinarySieve), not units of digits");
    };
    out.push_back(u);
}

} // namespace sieve
