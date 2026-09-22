// Sieve — SHA-256 (FIPS 180-4). Used for content identity and the scramble round function.
//
// On x86-64 CPUs with the SHA extensions (SHA-NI: Intel Ice Lake / Goldmont and later, AMD Zen
// and later) the compression function runs on those instructions, chosen at runtime. Both paths
// produce identical digests; the test suite runs every vector through both.
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

    // True if this CPU has the SHA instructions.
    static bool hardware_available();
    // Selects the hardware path (if available) or the portable one. Returns whether the hardware
    // path is now in use. Hardware is selected automatically at start-up when available.
    static bool use_hardware(bool enable);
    static bool using_hardware();

private:
    std::array<uint32_t, 8> h_;
    std::array<uint8_t, 64> buf_{};
    size_t buf_len_ = 0;
    uint64_t total_len_ = 0;
};

} // namespace sieve
