// Sieve — sealing bytes for others to keep (the infinite RAID, stage 3; IDEAS 19.176).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits.
//
// When a save is kept on other people's machines, they must not be able to read it or change it
// unseen. "sieve-seal-v1" seals bytes under a 32-byte key that only the save's owner holds, from two
// published primitives rather than a cipher of Sieve's own (unlike sieve/cipher.hpp, which is for
// study): ChaCha20 (RFC 8439) to hide the bytes and HMAC-SHA-256 (RFC 2104) to check them, encrypt
// then MAC. Both are checked against their RFCs' test vectors in the unit tests.
//
//     ke = HMAC(key, "sieve-seal-v1 encrypt")      km = HMAC(key, "sieve-seal-v1 mac")
//     nonce = the first 12 bytes of HMAC(km, "sieve-seal-v1 nonce" | plain)
//     sealed = nonce | ChaCha20(ke, nonce, counter from 0) of plain | HMAC(km, nonce | that)
//
// The nonce is made from the bytes themselves (as in SIV), so the same bytes sealed under the same
// key are always the same sealed bytes: a chunk two versions of a save share is sent and kept once.
// The cost, accepted for that: whoever holds two sealed chunks can tell whether they are the same
// bytes, never what the bytes are. Opening checks the MAC before anything else (in constant time),
// then that the nonce is the one the bytes give, so a sealed chunk changed in any bit is refused.
//
// Lose the key and what others keep for you cannot be read by anyone, you included.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sieve {

inline constexpr const char* kSealVersion = "sieve-seal-v1";
inline constexpr size_t kSealOverhead = 12 + 32; // nonce and MAC

using SealKey = std::array<uint8_t, 32>;

// HMAC-SHA-256 (RFC 2104), any key length.
std::array<uint8_t, 32> hmac_sha256(std::string_view key, std::string_view message);
// ChaCha20 (RFC 8439): bytes XOR the key stream from the given block counter.
std::string chacha20(const std::array<uint8_t, 32>& key, const std::array<uint8_t, 12>& nonce, uint32_t counter, std::string_view bytes);

std::string seal(const SealKey& key, std::string_view plain);
// The plain bytes, or nothing if the sealed bytes are not what this key sealed (changed, cut short,
// or another key's).
std::optional<std::string> unseal(const SealKey& key, std::string_view sealed);

// A key as kept in a file: "sieve-raid-key-v1 HEX\n" (64 hex digits). new_seal_key() takes 32 bytes
// from the system's random source (BCryptGenRandom on Windows; std::random_device elsewhere, which
// reads /dev/urandom or the CPU's generator with libstdc++ and libc++). Throws std::runtime_error if
// it does not answer.
SealKey new_seal_key();
std::string seal_key_text(const SealKey& key);
// Throws std::invalid_argument if the text is not a key.
SealKey read_seal_key(std::string_view text);

} // namespace sieve
