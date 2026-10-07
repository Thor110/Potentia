// Sieve hallway -- COST's tailoring: the line's filters searched for those that keep the item in
// hand and name it shortest (cli/tailor.hpp), on a worker; K starts or stops it, Return applies it.
//
// Applying is the setup menu's work done from the item: this line's filters replaced by those found,
// in compact mode, saved to the settings file, and a new hallway built with them (the application
// does that, as it does for ENTER THE HALLWAY), with the item in hand again on its COST tab, so the
// balance shows what the new filters do for it.
#include "hallway.hpp"

#include <iostream>

namespace hallway::hall {

// The lines whose units the search can take: those with one stack of their own.
bool Hallway::can_tailor() const
{
    return !on_books() && !on_models() && !on_binary() && !on_composition() && !guided_on();
}

const Hallway::TailorJob* Hallway::tailor_of(const Book& bk) const
{
    return tailor_ && tailor_->line == li_ && tailor_->unit == bk.unit ? tailor_.get() : nullptr;
}

void Hallway::stop_tailoring()
{
    if (tailor_) tailor_->progress.cancel = true;
    if (tailor_thread_.joinable()) tailor_thread_.join();
}

// K: the search for the item in hand, shown on its COST tab; or, while one runs for it, stopped.
void Hallway::tailor_in_hand()
{
    if (!in_hand_) return;
    hand_tab_ = 1;
    if (!can_tailor())
    {
        message(tr("msg.tailor.line"));
        return;
    }
    if (const TailorJob* j = tailor_of(*in_hand_); j && !j->ready)
    {
        tailor_->progress.cancel = true;
        message(tr("msg.tailor.stopping"));
        return;
    }
    stop_tailoring();
    auto job = std::make_shared<TailorJob>();
    job->line = li_;
    job->unit = in_hand_->unit;
    tailor_ = job;
    const sieve::FilterLine fl = filter_line(line());
    const LineFilters current = filters_.lines[size_t(line().kind)];
    tailor_thread_ = std::thread([job, fl, current] {
        try
        {
            job->result = sieve::cli::tailor_filters(fl, job->unit, current, &job->progress);
        }
        catch (const std::exception& e)
        {
            job->error = e.what();
        }
        job->ready = true;
    });
}

// Return: the filters found, applied (Request::Tailored).
void Hallway::apply_tailored(bool& quit)
{
    if (!in_hand_ || hand_tab_ != 1) return;
    const TailorJob* j = tailor_of(*in_hand_);
    if (!j || !j->ready || !j->error.empty() || !j->result.finished) return;
    if (j->result.filters.enabled.empty())
    {
        message(tr("msg.tailor.nothing"));
        return;
    }
    FilterConfig next = filters_;
    LineFilters& lf = next.lines[size_t(line().kind)];
    const bool full = lf.mode == FilterMode::Full; // a line kept full stays full
    lf = j->result.filters;
    if (full) lf.mode = FilterMode::Full;
    tailored_ = std::move(next);
    tailored_unit_ = j->unit;
    request_ = Request::Tailored;
    quit = true;
}

void Hallway::hold_on_cost(const Space::Digits& unit)
{
    go_to_unit(unit, true);
    if (in_hand_) hand_tab_ = 1;
}

bool Hallway::tailor_now(const std::string& save_to)
{
    if (!in_hand_) return false;
    if (!tailor_of(*in_hand_)) tailor_in_hand(); // (one already running, from K in --press, is waited for)
    if (!tailor_of(*in_hand_)) return false;
    hand_tab_ = 1;
    if (tailor_thread_.joinable()) tailor_thread_.join();
    if (save_to.empty()) return true;
    bool quit = false;
    apply_tailored(quit);
    if (!tailored_) return false;
    tailored_->save(save_to);
    request_ = Request::None;
    return true;
}

// COST's tailoring section, under the rows: what K does, how far it has got, or what it found and
// what Return would apply. `unit_bits` is the item's address as the rows give it. Returns the y below.
float Hallway::draw_tailor(const Book& bk, double unit_bits, float x, float cy, float pw, float bottom)
{
    const SDL_Color ink = theme().edge, dim = mix(theme().edge, theme().bg, 0.45f);
    text(x + 14, cy, tr("cost.tailor.head"), 1, dim);
    cy += 14;
    if (!can_tailor())
    {
        text(x + 14, cy, tr("cost.tailor.line"), 1, dim);
        return cy + 20;
    }
    const TailorJob* j = tailor_of(bk);
    if (!j)
    {
        text(x + 14, cy, tr("cost.tailor.start"), 1, ink);
        return cy + 20;
    }
    if (!j->ready)
    {
        std::string current;
        {
            std::lock_guard<std::mutex> lock(tailor_->progress.mx);
            current = tailor_->progress.current;
        }
        const int done = j->progress.done, total = j->progress.total;
        text(x + 14, cy,
             fit(done >= total && total > 0 ? tr("cost.tailor.set") : trf("cost.tailor.running", {std::to_string(done), std::to_string(total), current}),
                 pw - 28, 1),
             1, ink);
        return cy + 20;
    }
    if (!j->error.empty())
    {
        text(x + 14, cy, fit(trf("cost.tailor.failed", {j->error}), pw - 28, 1), 1, ink);
        return cy + 20;
    }
    const sieve::cli::TailorResult& r = j->result;
    if (!r.finished)
    {
        text(x + 14, cy, tr("cost.tailor.stopped"), 1, dim);
        return cy + 20;
    }
    if (r.filters.enabled.empty())
    {
        text(x + 14, cy, tr("cost.tailor.none"), 1, dim);
        return cy + 20;
    }
    // The address the filters found would give it, against its address now: compact, or in a line
    // kept full, with the bits of its title and cover as well (Return keeps it full).
    const TitledSpace* titled = titled_[size_t(li_)].get();
    const bool full = filters_.lines[size_t(line().kind)].mode == FilterMode::Full && titled;
    const double bits = r.bits + (full ? (titled->size().log10_approx() - titled->content_size().log10_approx()) * 3.321928094887362 : 0.0);
    text(x + 14, cy, tr(full ? "cost.tailor.found.full" : "cost.tailor.found"), 1, ink);
    text(x + 260, cy, trf("cost.bits", {fixed(bits, 0)}), 1, ink);
    text(x + 380, cy, trf("cost.chars", {std::to_string(int(std::ceil(bits / 4)))}), 1, ink);
    if (unit_bits > 0) text(x + 500, cy, trf("cost.percent", {fixed(bits / unit_bits * 100.0, 1)}), 1, ink);
    text(x + 580, cy, fit(trf("cost.tailor.filters", {std::to_string(r.filters.enabled.size())}), pw - 594, 1), 1, dim);
    cy += 14;
    // The filters, each with the settings found.
    for (const sieve::cli::TailorChoice& c : r.choices)
    {
        if (!c.used) continue;
        if (cy + 14 > bottom - 40) break;
        std::string line = c.name;
        for (const auto& [k, v] : c.values) line += "  " + k + "=" + v;
        text(x + 28, cy, fit(line, pw - 42, 1), 1, dim);
        cy += 12;
    }
    text(x + 14, cy + 2, tr("cost.tailor.apply"), 1, ink);
    return cy + 22;
}

} // namespace hallway::hall
