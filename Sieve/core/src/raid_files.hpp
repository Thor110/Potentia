// Sieve — the RAID's files read and written whole, and a folder's tag file (raid.cpp and raid_net.cpp
// share this one copy).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#pragma once

#include "sieve/hex.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

namespace sieve::raid_files {

namespace fs = std::filesystem;

inline std::optional<std::string> read_file(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return s;
}

// Written whole or not at all: to a file beside it, then renamed over it.
inline bool write_file(const fs::path& p, const std::string& bytes)
{
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    const fs::path tmp = p.string() + ".part";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), std::streamsize(bytes.size()));
        if (!out) return false;
    }
    fs::rename(tmp, p, ec);
    if (ec)
    {
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

// A folder made one if it is not yet, and the 32 hex digits of its tag file: the file `file`, reading
// "VERSION WORD=HEX". A RAID's folder is tagged so (sieve-raid, its id) and so is a held folder
// (sieve-raid-held, its salt). A folder without the file has it written once, the digits from
// `fresh`: a held folder's salt from the system's random source (as a key is made), a RAID folder's
// id from new_folder_id (raid.cpp). Throws std::invalid_argument for a folder that cannot be made, a
// tag file of another version, or digits that are not hex (`what` names them in that message, as each
// caller always has).
inline std::string folder_tag(const fs::path& folder, const char* file, const char* version, const char* word, const char* what,
                              const std::function<std::string()>& fresh)
{
    std::error_code ec;
    fs::create_directories(folder, ec);
    if (!fs::is_directory(folder, ec)) throw std::invalid_argument("not a folder, and cannot be made one: " + folder.string());
    const fs::path tag = folder / file;
    const std::string want = std::string(version) + " " + word + "=";
    std::string value;
    if (const auto t = read_file(tag))
    {
        if (t->rfind(want, 0) != 0 || t->size() < want.size() + 32) throw std::invalid_argument(folder.string() + "'s " + file + " file is not " + version);
        value = t->substr(want.size(), 32);
        if (!is_lower_hex(value, 32)) throw std::invalid_argument(folder.string() + "'s " + what + " is not hex");
    }
    else
    {
        value = fresh();
        if (!write_file(tag, want + value + "\n")) throw std::invalid_argument("cannot write " + tag.string());
    }
    return value;
}

} // namespace sieve::raid_files
