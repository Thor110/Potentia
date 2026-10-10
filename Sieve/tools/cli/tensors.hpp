// Sieve CLI — `sieve tensors`: a model file in the safetensors format, taken apart into its start
// and its weights, and weighed (sieve/safetensors.hpp; IDEAS §15).
#pragma once

#include "cli/args.hpp"

namespace sieve::cli {

int cmd_tensors(const Args& a);

} // namespace sieve::cli
