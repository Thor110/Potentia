// Sieve — the catalogue of dimensions (IDEAS 19.162): every dimension in one place, built or not.
//
// A dimension is a state space with its own addresses: a line of the hallway, a family of
// characters, a sub state space such as a game's generator (19.161). Each entry says what one
// address is, the dimension's roof (the most it holds), whether it is built, and which other
// dimensions it links to, so that a new dimension is added here, in one place, and its links are
// written down from the start. The links are the lattice's highways (19.145): DNA to research,
// characters to species to DNA, every line to the binary line beneath it.
//
// This is a record of what exists and what is planned, not a registry the programs dispatch on;
// `sieve dimensions` prints it and docs/DIMENSIONS.md explains it.
#pragma once

#include <string_view>
#include <vector>

namespace sieve {

enum class DimensionState { Built, Partly, Planned };

struct Dimension
{
    std::string_view id;      // "pages", "dna", "research"
    std::string_view name;    // "Pages"
    DimensionState state;
    std::string_view address; // what one address is
    std::string_view roof;    // the most it holds, as built (or as planned)
    std::string_view links;   // ids of the dimensions it links to, space-separated
    std::string_view ideas;   // where it is written about (IDEAS sections, SPECIFICATIONS)
};

const std::vector<Dimension>& dimensions();
const Dimension* dimension_by_id(std::string_view id); // null if there is none
const char* dimension_state_name(DimensionState s);    // "built", "partly built", "planned"

} // namespace sieve
