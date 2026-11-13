// Sieve — the RAID's protocol over TCP (sieve raid serve, share, fetch; IDEAS 19.176 stage 3).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#pragma once

#include "sieve/raid_net.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace sieve_cli {

// A connection to HOST:PORT; reads and writes give up after `timeout_s` seconds of silence. Throws
// std::runtime_error if it cannot be made.
std::unique_ptr<sieve::RaidStream> raid_connect(const std::string& address, int timeout_s = 30);

// Listens on HOST:PORT and hands each connection to `on` with the address it came from (the IP
// address alone, as text). Connections are answered at once, each on a thread of its own (32 at
// most; more wait to be accepted), so `on` must be safe to call from several threads. Stops after `connections` (0: never). `ready` is called once
// listening has begun. Throws std::runtime_error if it cannot listen.
// `stop`, where given, is looked at about once a second: set, no more are accepted, and it returns once
// the connections already answering are done (the hallway's serving, stopped as it closes).
void raid_listen(const std::string& address, size_t connections, int timeout_s, const std::function<void()>& ready,
                 const std::function<void(sieve::RaidStream&, const std::string&)>& on, const std::atomic<bool>* stop = nullptr);

} // namespace sieve_cli
