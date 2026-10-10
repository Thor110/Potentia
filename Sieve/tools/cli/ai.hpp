// Sieve CLI — `sieve ai`: the AI line, every language model of one shape (sieve/aispace.hpp), each
// to read off its address and talk to.
#pragma once

#include "cli/args.hpp"
#include "sieve/aispace.hpp"

namespace sieve::cli {

// The line's shape from --ai-layers, --ai-width, --ai-heads and --ai-bits (the hallway's options).
AiShape ai_shape_of(const Args& a);
int cmd_ai(const Args& a);

} // namespace sieve::cli
