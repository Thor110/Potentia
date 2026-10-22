// Sieve — a request to stop searching, and word of how a long piece of work is going.
//
// Long work (packing a file's Sieve instructions, the rope's search, tailoring, the weighing) runs on
// one thread, and is made of trials, each kept where it is the best so far. Whoever starts the work
// (the File Locator) may give the thread a flag to stop: every trial that is not needed for a result
// checks it, and the work ends with the best it has found, as a search's time running out ends it.
// It may also give the thread somewhere to say how it is going (what is being tried, the best so
// far). Both belong to the thread that set them (StopScope), so other threads are not touched.
#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace sieve {

// Whether this thread's work has been asked to stop (false when nobody asked, or gave no flag).
bool stop_requested();

// Says how the work on this thread is going ("the lzma2 stream: best so far ..."), where whoever
// started it asked to hear; nothing otherwise.
void progress(const std::string& what);

// For the length of a scope, this thread's flag to stop and where its progress goes (either may
// be empty); the ones before are put back after.
class StopScope
{
public:
    StopScope(const std::atomic<bool>* stop, std::function<void(const std::string&)> report);
    ~StopScope();
    StopScope(const StopScope&) = delete;
    StopScope& operator=(const StopScope&) = delete;

private:
    const std::atomic<bool>* old_stop_;
    std::function<void(const std::string&)> old_report_;
};

} // namespace sieve
