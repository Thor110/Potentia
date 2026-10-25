#pragma once
// preflate (third_party_licenses/preflate) behind two functions: its own headers are C++17 (they use
// std::result_of, which C++20 removed and MSVC no longer has), so they are seen only by
// preflate_glue.cpp, built with preflate as C++17.
#include <cstdint>
#include <vector>

namespace sieve::cli {
// A raw deflate stream split into what it holds and its difference from preflate's own deflate;
// false if preflate cannot read it.
bool preflate_split(const std::vector<uint8_t>& deflate, std::vector<uint8_t>& unpacked, std::vector<uint8_t>& diff);
// The stream made again from the two; false if it cannot be.
bool preflate_join(const std::vector<uint8_t>& unpacked, const std::vector<uint8_t>& diff, std::vector<uint8_t>& deflate);
} // namespace sieve::cli
