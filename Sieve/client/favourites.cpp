// Sieve hallway — favourites' combined catalogue (see favourites.hpp).
#include "favourites.hpp"

#include "cli/sieve_cache.hpp"
#include "sieve/paths.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <sstream>

namespace hallway {

namespace fs = std::filesystem;

std::vector<FavouriteRow> all_favourites(const fs::path& base)
{
    std::vector<FavouriteRow> out;
    std::vector<fs::path> lists;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(base / "worlds", ec))
        if (e.path().extension() == ".favourites") lists.push_back(e.path());
    std::sort(lists.begin(), lists.end());
    for (const fs::path& l : lists)
        for (const std::string& sha : dimension_favourites(l))
        {
            const auto f = read_favourite(base, sha);
            out.push_back({sieve::u8(l.stem()), f ? f->line : std::string("?"), f ? f->title : std::string(), sha});
        }
    // The Media Player's melodies: their names, from the index in its .sieve (read in memory, checked).
    try
    {
        const fs::path music = base / "sieve-favourites.sieve";
        if (fs::exists(music, ec))
            for (const auto& [name, bytes] : sieve::cli::files_in_sieve(music, uint64_t(64) << 20))
                if (fs::path(sieve::from_u8(name)).filename() == "favourites.tsv")
                {
                    std::istringstream in(std::string(bytes.begin(), bytes.end()));
                    std::string l;
                    while (std::getline(in, l))
                        if (!l.empty()) out.push_back({"media", "audio", l.substr(0, l.find('\t')), {}});
                }
    }
    catch (const std::exception&)
    {
        // (a Media Player file that does not read is the Media Player's to say; it is left out here)
    }
    return out;
}

} // namespace hallway
