// Sieve — a request to stop searching, and word of how a long piece of work is going.
//
// Long work (packing a file's Sieve instructions, the rope's search, tailoring, the weighing) runs on
// one thread, and is made of trials, each kept where it is the best so far. Whoever starts the work
// (the File Locator) may give the thread a flag to stop: every trial that is not needed for a result
// checks it, and the work ends with the best it has found, as a search's time running out ends it;
// at its second level (force) the trial under way is abandoned too.
// It may also give the thread somewhere to say how it is going (what is being tried, the best so
// far). Both belong to the thread that set them (StopScope), so other threads are not touched.
#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace sieve {

// How far this thread's work has been asked to stop: 0 not at all (or no flag given); 1 stop, the
// trial under way finished and the best kept (a result needs one finished pass of each stream);
// 2 force, the trial under way abandoned too, and the last finished kept.
inline constexpr int kStop = 1, kForce = 2;
bool stop_requested();  // 1 or more
bool force_requested(); // 2

// This thread's flag (or none), for the threads it starts to take on as their own (StopScope), so
// that a stop reaches the work they do for it too.
const std::atomic<int>* current_stop();

// Says how the work on this thread is going ("the lzma2 stream: best so far ..."), where whoever
// started it asked to hear; nothing otherwise. With `best` and `of` (bytes, both 0 or more), a
// new best of a step: so many bytes for the `of` it packs, for a graph of the best so far.
void progress(const std::string& what, double best = -1, double of = -1);

// Where a thread's progress goes: what it is doing, and a new best (-1, -1 when there is none).
using ProgressSink = std::function<void(const std::string& what, double best, double of)>;

// This thread's sink (empty where none was given), for threads it starts that should be heard too.
ProgressSink current_progress();

// For the length of a scope, this thread's flag to stop and where its progress goes (either may
// be empty); the ones before are put back after.
class StopScope
{
public:
    StopScope(const std::atomic<int>* stop, ProgressSink report);
    ~StopScope();
    StopScope(const StopScope&) = delete;
    StopScope& operator=(const StopScope&) = delete;

private:
    const std::atomic<int>* old_stop_;
    ProgressSink old_report_;
};

} // namespace sieve
