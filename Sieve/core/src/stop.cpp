#include "sieve/stop.hpp"

#include <utility>

namespace sieve {

namespace {

thread_local const std::atomic<int>* t_stop = nullptr;
thread_local ProgressSink t_report;

} // namespace

bool stop_requested() { return t_stop && t_stop->load(std::memory_order_relaxed) >= kStop; }

bool force_requested() { return t_stop && t_stop->load(std::memory_order_relaxed) >= kForce; }

const std::atomic<int>* current_stop() { return t_stop; }

void progress(const std::string& what, double best, double of)
{
    if (t_report) t_report(what, best, of);
}

StopScope::StopScope(const std::atomic<int>* stop, ProgressSink report) : old_stop_(t_stop), old_report_(std::move(t_report))
{
    t_stop = stop;
    t_report = std::move(report);
}

StopScope::~StopScope()
{
    t_stop = old_stop_;
    t_report = std::move(old_report_);
}

} // namespace sieve
