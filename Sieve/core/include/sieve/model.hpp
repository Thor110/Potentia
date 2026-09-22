// Sieve — pinned symbol models for entropy-ordered ("guided") addressing (SPECIFICATIONS §4.2).
//
// A model gives, for every history of symbols, an integer frequency table over the next symbol:
// every symbol gets at least 1, and the table always sums to kTotal = 2^16. Nothing here is
// floating point, so every machine derives exactly the same tables from the same file.
//
// "sieve-charmodel-v1" is a variable-order context model:
//   * The file stores, for each stored context (the up-to-ORDER symbols before a position), how
//     often each symbol followed it in the training stream. Only contexts seen at least
//     MIN_COUNT times are stored, and a context is only stored if its parent (the context minus
//     its oldest symbol) is. The empty context is always stored.
//   * Tables ("witten-bell-v1"), parents before children:
//       empty context:  a_s = n_s + 1,                   D = n + N
//       context c:      a_s = n_s * T + u * parent_s,   D = (n + u) * T
//     where n_s are the stored counts, n their sum, u the number of symbols with n_s > 0, and
//     parent_s the parent's finished table. Then quantise(a, D):
//       f_s = 1 + floor((T - N) * a_s / D), and the T - sum(f) units left over go one each to
//       the symbols with the largest remainders (T - N) * a_s mod D, ties to the lowest symbol.
//   * Padding: if the header names a padding symbol p (SPACE for text), the context "p p" has
//     the fixed table f_p = T - (N - 1), every other symbol 1. Canonical text never contains
//     two SPACEs in a row, so they only appear as the padding of a last unit, which then
//     costs almost nothing.
//   * Lookup: the table for a history is that of its longest suffix (at most ORDER symbols)
//     that is stored. At the start of a unit the history is empty.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sieve {

inline constexpr const char* kCharModelFormat = "sieve-charmodel-v1";
inline constexpr const char* kSmoothing = "witten-bell-v1";
inline constexpr uint32_t kModelTotalBits = 16;
inline constexpr uint32_t kModelTotal = 1u << kModelTotalBits;

struct ModelParams
{
    std::string symbols_id;           // e.g. "lower27"
    uint32_t base = 0;                // number of symbols
    uint32_t order = 5;               // longest context
    uint32_t min_count = 8;           // contexts seen fewer times are not stored
    std::optional<uint32_t> padding;  // padding symbol (text: 0 = SPACE)
    std::string corpus;               // free-text description of the training data
};

class CharModel
{
public:
    // Counts every context of up to `order` symbols in `stream` and keeps those that qualify.
    static CharModel train(std::span<const uint32_t> stream, const ModelParams& params);
    // Parses a model file (text, see serialise()). The SHA-256 of `text` becomes the model's id.
    static CharModel parse(std::string_view text);
    static CharModel load_file(const std::string& path);

    // The model file: a header, then one line per stored context (see SPECIFICATIONS §4.2).
    std::string serialise() const;

    const ModelParams& params() const { return params_; }
    uint32_t base() const { return params_.base; }
    uint32_t order() const { return params_.order; }
    size_t context_count() const { return contexts_.size(); }
    uint64_t trained_symbols() const { return trained_; } // symbols in the training stream
    const std::string& sha256() const { return sha256_; }

    // Cumulative table for the next symbol after `history` (oldest first): base()+1 entries,
    // cum[0] = 0, cum[base()] = kModelTotal; symbol s has frequency cum[s+1] - cum[s].
    const uint32_t* cumulative(std::span<const uint32_t> history) const;

    // Information content of a symbol stream in bits, coded as one long history (context
    // carries across, capped at ORDER). DISPLAY/measurement only: uses floating point.
    double stream_bits(std::span<const uint32_t> stream) const;

private:
    struct Context
    {
        std::vector<uint32_t> symbols;                     // oldest first
        std::vector<std::pair<uint32_t, uint64_t>> counts; // (symbol, count), ascending symbol
    };
    void finish(); // validates, builds tables and the lookup index
    uint64_t key(const uint32_t* symbols, size_t k) const;

    ModelParams params_;
    std::vector<Context> contexts_; // sorted by (length, symbols)
    std::vector<uint32_t> cum_;     // (base+1) entries per table
    std::unordered_map<uint64_t, uint32_t> index_; // context key -> table number
    std::optional<uint32_t> padding_table_;
    uint64_t trained_ = 0;
    std::string sha256_;
};

} // namespace sieve
