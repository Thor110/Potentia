#include "cli/preflate_glue.hpp"

#include "preflate.h"

namespace sieve::cli {
bool preflate_split(const std::vector<uint8_t>& deflate, std::vector<uint8_t>& unpacked, std::vector<uint8_t>& diff)
{
    std::vector<unsigned char> un, d;
    if (!preflate_decode(un, d, deflate)) return false;
    unpacked.assign(un.begin(), un.end());
    diff.assign(d.begin(), d.end());
    return true;
}

bool preflate_join(const std::vector<uint8_t>& unpacked, const std::vector<uint8_t>& diff, std::vector<uint8_t>& deflate)
{
    std::vector<unsigned char> raw;
    if (!preflate_reencode(raw, diff, unpacked)) return false;
    deflate.assign(raw.begin(), raw.end());
    return true;
}
} // namespace sieve::cli
