// Sieve — model files in the safetensors format, taken apart (IDEAS §15, "The first model, taken
// apart"; SPECIFICATIONS §12.0a). The first step towards the AI dimension: a model file is read as
// its three parts, and each is weighed.
//
// A safetensors file is
//   the start    an 8-byte little-endian length N, then N bytes of JSON: an object whose members
//                are the tensors, each {"dtype", "shape", "data_offsets": [begin, end)} with the
//                offsets counted from the end of the start, and an optional "__metadata__" object
//                of strings; padded with spaces;
//   the weights  the tensors' bytes, end to end, little-endian, row-major.
// The tokenizer and the configuration are files of their own (tokenizer.json, config.json).
//
// "safetensors-layout-v1": the start as the safetensors library (0.4) writes it, from nothing
// but the metadata and each tensor's name, type and shape. The tensors are ordered by type,
// latest in the list below first, then by name in byte order; each one's offsets follow the last;
// the JSON is written compactly, "__metadata__" first, then each tensor as
// {"dtype":..,"shape":[..],"data_offsets":[b,e]}, and padded with spaces to a multiple of 8. A
// file whose start is this one needs no start of its own: it is rebuilt. (Checked on files of one
// type; the order between types follows the library's list of types, which is pinned here.)
//
// From a config.json (llama_tensors): the names, types and shapes a Llama model of that shape
// has, as the transformers library names them. With the layout above that is the whole start, so
// for such a model the start is the config.
//
// Statistics (TensorStats), for the 16-bit float types (BF16, F16), each tensor's values taken as
// 16-bit symbols: how many, how many distinct, the order-0 entropy of the symbols, of their high
// bytes and of their low bytes (bits a value), and the mean and standard deviation of the numbers.
// Entropy sums -p log2 p over the symbols in increasing order; the mean and the sums of squares
// add the values in file order, in double precision. reference/sieve_ref.py computes the same.
#pragma once

#include "sieve/json.hpp"

#include <array>
#include <cstdint>
#include <istream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace sieve::safetensors {

inline constexpr const char* kLayoutVersion = "safetensors-layout-v1";

// The safetensors library's types, in its order (which the layout sorts by, latest first).
inline constexpr std::array<const char*, 15> kDTypes = {"BOOL", "U8",  "I8",  "F8_E5M2", "F8_E4M3", "I16", "U16", "F16",
                                                         "BF16", "I32", "U32", "F32",     "F64",     "I64", "U64"};
// Bytes an element of a type takes; 0 for a type not in kDTypes.
size_t dtype_bytes(const std::string& dtype);
// Its place in kDTypes, or -1.
int dtype_rank(const std::string& dtype);

struct Tensor
{
    std::string name, dtype;
    std::vector<uint64_t> shape;
    uint64_t begin = 0, end = 0; // byte offsets from the end of the start
    uint64_t elements() const;
    uint64_t bytes() const { return end - begin; }
    bool operator==(const Tensor&) const = default;
};

struct Header
{
    uint64_t json_bytes = 0; // N
    std::optional<std::vector<std::pair<std::string, std::string>>> metadata; // in the order written
    std::vector<Tensor> tensors;                                              // in the order written
    std::string json;                                                         // the N bytes as read
    uint64_t start_bytes() const { return 8 + json_bytes; }
    uint64_t data_bytes() const; // the end of the last tensor
};

// Reads the start of a safetensors file. Throws std::invalid_argument if it is not one: a length
// past `max_json` bytes (100 MB by default), bad JSON, a tensor whose offsets do not hold its
// shape, or tensors that overlap or leave gaps.
Header read_header(std::istream& in, uint64_t max_json = 100u << 20);
Header parse_header(std::span<const uint8_t> start); // the same, from bytes in memory

// The start safetensors-layout-v1 writes for these tensors (their offsets ignored and laid out
// anew) and this metadata: the 8-byte length and the JSON. `laid` receives the tensors with their
// new offsets, in the layout's order.
std::string layout_start(const std::optional<std::vector<std::pair<std::string, std::string>>>& metadata, std::vector<Tensor> tensors,
                         std::vector<Tensor>* laid = nullptr);
// True if the file's start is exactly the one the layout writes for its own tensors and metadata.
bool is_canonical(const Header& h);

// The tensors of a Llama model (config.json's model_type "llama"), as transformers names them,
// with config.json's torch_dtype as their type and no offsets. Throws std::invalid_argument for a
// configuration this does not cover (biases, another model type, a missing size).
std::vector<Tensor> llama_tensors(const json::Value& config);
// A one-line description of such a configuration: "llama, 32 layers, hidden 960, ...".
std::string llama_summary(const json::Value& config);

// Statistics of one tensor's values (16-bit float types only).
struct TensorStats
{
    uint64_t values = 0;
    uint32_t distinct = 0;
    double h_value = 0, h_high = 0, h_low = 0; // bits a value
    double mean = 0, sd = 0;
};
class StatsBuilder
{
public:
    explicit StatsBuilder(const std::string& dtype); // BF16 or F16; throws otherwise
    void add(std::span<const uint8_t> bytes);        // whole elements, little-endian
    TensorStats finish() const;

private:
    bool bf16_;
    std::vector<uint64_t> counts_;
    uint64_t n_ = 0;
    double sum_ = 0, sum2_ = 0;
};
bool has_stats(const std::string& dtype); // BF16 and F16
double half_to_double(uint16_t bits, bool bf16);

} // namespace sieve::safetensors
