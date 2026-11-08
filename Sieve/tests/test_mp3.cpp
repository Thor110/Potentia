// packMP3's decoder against damaged input (8 November 2026; results/mp3-pmp-2026-11-08.md):
// tests/mp3/tone.mp3 coded by packMP3 and given back exactly; then its packMP3 form changed, 2,000
// times (bytes flipped, bytes set to edges, cut short), each decoded: refused, or decoded to some MP3,
// but never read outside its buffers. Run under AddressSanitizer this checks that; in any build it
// checks that none ends the program.
//
//   sieve_mp3_check TESTS_FOLDER        (prints "N damaged forms: R refused, D decoded, 0 failures")

#include "cli/mp3_glue.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    std::ifstream f(std::string(argv[1]) + "/mp3/tone.mp3", std::ios::binary);
    const std::vector<uint8_t> mp3{std::istreambuf_iterator<char>(f), {}};
    const auto pmp = sieve::cli::mp3_to_pmp(mp3);
    if (!pmp || sieve::cli::pmp_to_mp3(*pmp) != mp3)
    {
        std::printf("tone.mp3 not given back by packMP3: 1 failure\n");
        return 1;
    }
    std::mt19937 rng(34);
    int refused = 0, decoded = 0;
    const int n = 2000;
    for (int k = 0; k < n; ++k)
    {
        std::vector<uint8_t> c = *pmp;
        const int changes = 1 + int(rng() % 8);
        for (int j = 0; j < changes; ++j)
        {
            const size_t at = 2 + (rng() % 4 == 0 ? rng() % 40 : rng() % (c.size() - 2));
            c[at] = rng() % 3 == 0 ? uint8_t(rng() % 2 ? 0xFF : 0x00) : uint8_t(c[at] ^ (1u + rng() % 255));
        }
        if (rng() % 6 == 0) c.resize(2 + rng() % (c.size() - 2));
        try
        {
            sieve::cli::pmp_to_mp3(c);
            ++decoded;
        }
        catch (const std::exception&)
        {
            ++refused;
        }
    }
    std::printf("%d damaged forms: %d refused, %d decoded, 0 failures\n", n, refused, decoded);
    return 0;
}
