#include "sieve/stop.hpp"

#include <utility>

namespace sieve {

namespace {

thread_local const std::atomic<bool>* t_stop = nullptr;
thread_local std::function<void(const std::string&)> t_report;

} // namespace

bool stop_requested() { return t_stop && t_stop->load(std::memory_order_relaxed); }

void progress(const std::string& what)
{
    if (t_report) t_report(what);
}

StopScope::StopScope(const std::atomic<bool>* stop, std::function<void(const std::string&)> report) : old_stop_(t_stop), old_report_(std::move(t_report))
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
