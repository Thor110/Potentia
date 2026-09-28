// Sieve — timings (--timings): how long each phase of the work takes, for finding what is slow and
// for benchmarking one build or machine against another.
//
// Off by default, when a phase costs one relaxed atomic read. Turned on by --timings (the command
// line and the hallway both take it), every timed phase adds its duration to a table kept per
// phase name: how many times it ran, its total, its mean and its slowest. The table is written when
// the program exits: to standard error, and, where a file was given, to that file too (the hallway
// writes sieve-timings.txt beside its settings, since a program started from Explorer has no
// console). Phases nest (an item's vault check is inside the building of that item), so totals
// overlap and do not add up to the run's length. Safe from any thread: the hallway builds on a
// worker, and paints faces on several.
#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace sieve::cli::timings {

// Turns timing on for the rest of the run; the table is written at exit (to `file` as well, when
// it is not empty). Calling it again only changes the file.
void enable(const std::filesystem::path& file = {});
bool enabled();

// Adds one run of a phase.
void add(const std::string& phase, double ms);

// The table as text, slowest total first.
std::string report();

// Times the enclosing block as one run of `phase` (a string literal, or any string that outlives
// the scope).
class Scope
{
public:
    explicit Scope(const char* phase) : phase_(phase), on_(enabled())
    {
        if (on_) t0_ = std::chrono::steady_clock::now();
    }
    ~Scope()
    {
        if (on_) add(phase_, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0_).count());
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    const char* phase_;
    bool on_;
    std::chrono::steady_clock::time_point t0_{};
};

} // namespace sieve::cli::timings
