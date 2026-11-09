#pragma once

#include <cstdint>
#include <string>

namespace sieve::cli {

// The next manifest version in the making, a draft (Edward, 2 November 2026: "a dynamic next version
// draft setup system so that we just enable the draft mode and it sets the version number in the
// displayed string and the ini setting name"). Made only with the draft switch on: `--v33-draft` on
// the command line, `[locator] v33_draft = 1` in sieve-hallway.ini; off, every instructions-maker
// makes the newest frozen version as before. Its names all come from the number here, so the next
// draft is begun by changing it (and 0 says none is in the making). What it holds: docs/NEXT-VERSION.md.
// (v33 frozen 3 November 2026; v34 begun empty, and since 6 November 2026 holding the ways
// docs/NEXT-VERSION.md lists.)
inline constexpr uint32_t kDraftVersion = 34;

inline std::string draft_name() { return "v" + std::to_string(kDraftVersion); }              // "v33"
inline std::string draft_flag() { return draft_name() + "-draft"; }                           // "v33-draft" (--v33-draft)
inline std::string draft_ini_key() { return draft_name() + "_draft"; }                        // "v33_draft"
inline std::string draft_manifest() { return "sieve-manifest-" + draft_name(); }              // "sieve-manifest-v33"

// The switch, for the whole run.
void set_draft(bool on);
bool draft_on();

} // namespace sieve::cli
