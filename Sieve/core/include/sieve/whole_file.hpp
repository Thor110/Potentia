// Sieve — a whole file's bytes, read at once: the one copy the command line's readers share.
#pragma once

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>

namespace sieve {

// The file's bytes; throws std::runtime_error naming the path where it cannot be opened.
inline std::string read_whole_file(const std::string& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    return std::string(std::istreambuf_iterator<char>(in), {});
}

} // namespace sieve
