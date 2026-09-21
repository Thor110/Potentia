// Sieve — SHA-256 (FIPS 180-4). Used for content identity and the scramble round function.
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace sieve {

class Sha256
{
public:
    using Digest = std::array<uint8_t, 32>;

    Sha256();
    void update(std::span<const uint8_t> data);
    void update(std::string_view data);
    void update_u32le(uint32_t v);
    Digest finish();

    static Digest hash(std::string_view data);
    static std::string hex(const Digest& d);

private:
    void block(const uint8_t* p);
    std::array<uint32_t, 8> h_;
    std::array<uint8_t, 64> buf_{};
    size_t buf_len_ = 0;
    uint64_t total_len_ = 0;
};

} // namespace sieve
