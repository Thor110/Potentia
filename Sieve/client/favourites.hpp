// Sieve hallway — favourites: real items kept from the shelves, each dimension its own list, and one
// catalogue of them all (IDEAS 19.190; Edward, 13 November 2026: "users should also be able to add
// favourites into their personal dimensions, which could be real items picked from the shelf, this
// means each dimension needs a favourites system and overall a combined favourites system to catalog
// them all").
//
// A favourite is a reference, never a copy: the line it is from, the shape of that line (the space's
// id, which gives an address its meaning), the item itself as the unit's digits, and its title. It is
// named by the SHA-256 of the space's id, a line feed and the digits written as decimal numbers
// apart by commas, so the same item kept twice is one favourite. Kept beside the settings:
//   favourites/SHA.item     "sieve-favourite-v1", then "line ID", "space ID", "title TEXT" (the
//                           title percent-encoded: %25 for %, %0A for a line feed) and "digits D,D,..."
//   worlds/NAME.favourites  "sieve-favourites-v1", then one SHA a line: that dimension's favourites,
//                           in the order kept
// The combined catalogue is worked out from these and the Media Player's own favourites
// (sieve-favourites.sieve), never kept as a second copy, so the lists cannot drift apart.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

struct Favourite
{
    std::string sha;    // its name (above)
    std::string line;   // the line's id: pages, image, audio, ...
    std::string space;  // the line's shape (Hallway::space_id)
    std::string title;  // as the shelf shows it
    std::vector<uint32_t> digits;
};

std::string favourite_sha(const std::string& space, const std::vector<uint32_t>& digits);
// favourites/ beside the settings file (`base` is the folder the settings are in).
std::filesystem::path favourites_dir(const std::filesystem::path& base);
// Writes favourites/SHA.item; false and `why` if it cannot.
bool save_favourite(const std::filesystem::path& base, const Favourite& f, std::string& why);
std::optional<Favourite> read_favourite(const std::filesystem::path& base, const std::string& sha);
// A dimension's list (worlds/NAME.favourites beside its world file): read, and kept with one more.
std::vector<std::string> dimension_favourites(const std::filesystem::path& list_file);
// Adds `sha` to the list if it is not there; returns whether it was added (false: already there).
bool add_to_dimension(const std::filesystem::path& list_file, const std::string& sha, std::string& why);
bool remove_from_dimension(const std::filesystem::path& list_file, const std::string& sha);
// The list file beside a dimension's world file (worlds/personal.world: worlds/personal.favourites).
inline std::filesystem::path favourites_list_of(const std::filesystem::path& world_file)
{
    auto p = world_file;
    return p.replace_extension(".favourites");
}

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
