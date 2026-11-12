// Sieve hallway — the character store and your personal dimension (IDEAS 19.152, 19.154).
//
// The store is a folder, `characters` beside the settings file, holding every character saved from
// the CHARACTER page as <SHA-256>.character, so the same character saved twice is one file and any
// copy can be checked against its name. Your personal dimension is the store read back: a short line
// of your own characters, in the order of their names, each checked as it is read (a file whose
// name is not its SHA-256, or that is refused, is left out and counted).
#pragma once

#include "app_settings.hpp"
#include "sieve/character.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace hallway {

inline std::filesystem::path character_store_dir(const std::filesystem::path& app_path)
{
    return (app_path.empty() ? AppSettings::default_path() : app_path).parent_path() / "characters";
}

struct PersonalDimension
{
    std::vector<sieve::Character> characters; // in the order of their names
    size_t left_out = 0;                     // files refused, or not named by their SHA-256
};

// At most `limit` characters, the first by name.
inline PersonalDimension read_personal_dimension(const std::filesystem::path& dir, size_t limit = 64)
{
    PersonalDimension d;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return d;
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec))
        if (e.path().extension() == ".character") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files)
    {
        if (d.characters.size() >= limit) break;
        std::ifstream in(f, std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        try
        {
            sieve::Character c = sieve::read_character(text);
            if (f.stem().string() != c.id())
            {
                ++d.left_out;
                continue;
            }
            d.characters.push_back(std::move(c));
        }
        catch (const std::exception&)
        {
            ++d.left_out;
        }
    }
    return d;
}

} // namespace hallway
