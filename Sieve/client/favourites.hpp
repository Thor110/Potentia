// Sieve hallway — favourites: the combined catalogue of them all (IDEAS 19.190). A favourite's own file
// and each dimension's list are read and written by the core (sieve/favourites.hpp, which says their
// formats); the catalogue is worked out from those and the Media Player's own favourites
// (sieve-favourites.sieve), never kept as a second copy, so the lists cannot drift apart.
#pragma once

#include "sieve/favourites.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace hallway {

using sieve::add_to_dimension;
using sieve::dimension_favourites;
using sieve::Favourite;
using sieve::favourite_sha;
using sieve::favourites_dir;
using sieve::favourites_list_of;
using sieve::read_favourite;
using sieve::remove_from_dimension;
using sieve::save_favourite;

// The combined catalogue: every favourite of every dimension in worlds/ (by name), then the Media
// Player's melodies, one row each: where it is kept, what it is, and its title.
struct FavouriteRow
{
    std::string where; // the dimension's name, or "media" for the Media Player
    std::string line;  // the line it is from ("audio" for a melody)
    std::string title;
    std::string sha;   // empty for a melody
};
std::vector<FavouriteRow> all_favourites(const std::filesystem::path& base);

} // namespace hallway
