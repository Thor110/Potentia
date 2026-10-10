// Sieve CLI — `sieve chat`: a language model run by Sieve itself (sieve/llm.hpp), to talk to, and its
// tokenizer to look into (IDEAS §15).
#pragma once

#include "cli/args.hpp"

namespace sieve::cli {

int cmd_chat(const Args& a);

} // namespace sieve::cli
