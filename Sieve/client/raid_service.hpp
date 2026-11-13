// Sieve — the infinite RAID between machines from the menus (IDEAS 19.176 stage 3, 19.150's red boxes).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
//
// Both halves are opt in only, each behind a red box in Settings > RAID ([raid] share and serve):
// sharing sends your save, sealed under a key only you hold, to the machines you name each time
// Ctrl+S keeps it; serving keeps others' sealed saves on this machine while Sieve runs, as
// `sieve raid serve` does, answering several at once and within the limits set.
#pragma once

#include "app_settings.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hallway {
namespace raid_service {

// Where they are kept: the settings' own, else beside the settings file.
std::filesystem::path key_path(const AppSettings& s, const std::filesystem::path& settings_path);
std::filesystem::path held_path(const AppSettings& s, const std::filesystem::path& settings_path);

// Your key, made if it is not there yet (readable by its owner alone, where the file system says
// who may read). Returns an empty string, or why it could not be; *made says whether it was made now.
std::string ensure_key(const AppSettings& s, const std::filesystem::path& settings_path, bool* made = nullptr);

// Serving started or stopped to match [raid] serve (started again where its address, folder or
// limits changed). Never throws: what went wrong is in serving_status().
void apply(const AppSettings& s, const std::filesystem::path& settings_path);
void stop(); // as Sieve closes: no more accepted, and those answering finished
std::string serving_status(); // for Settings > RAID: "on 0.0.0.0:47100, 1,234 bytes kept", "off", or why not

// Sharing: these versions of the RAID on `drives` (your character, its animation, your save) sent to
// [raid] peers, sealed, on a thread of its own; what came of it is taken once by take_share_note().
void share(const AppSettings& s, const std::filesystem::path& settings_path, std::vector<std::filesystem::path> drives, std::vector<std::string> versions);
std::optional<std::string> take_share_note();
bool sharing(); // a share still under way

} // namespace raid_service
} // namespace hallway
