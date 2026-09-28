// Sieve hallway — the filter designer's screen (designer.hpp).
//
// The nodes and their fields are made afresh from the model every frame (build), as the main
// menu makes its rows: nothing on screen can drift from the filter it shows. A field is a label
// and a value with what to do when it is changed, typed into, or deleted; the keyboard walks the
// fields in order, the mouse picks one, and a node is moved by dragging its title (where it sits
// is saved in the file, as a comment the parser skips).

#include "designer.hpp"

#include "font.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/dictionaries.hpp"
#include "cli/plugins.hpp"
#include "cli/timings.hpp"
#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/plugin.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace hallway {

namespace {

namespace fs = std::filesystem;

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kGood{120, 230, 120, 255},
    kBad{255, 110, 110, 255}, kAccent{120, 200, 255, 255};

constexpr float kRow = 13, kTitle = 18, kNodeW = 250;

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }

void frame(SDL_Renderer* r, const SDL_FRect& f, SDL_Color c, Uint8 fill = 235)
{
    SDL_SetRenderDrawColor(r, 0, 0, 0, fill);
    SDL_RenderFillRect(r, &f);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
    SDL_RenderRect(r, &f);
}

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

std::string read_all(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    std::ostringstream o;
    o << in.rdbuf();
    return o.str();
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

template <typename T> void cycle(std::vector<T>& v, const T& cur, int dir, T& out)
{
    if (v.empty()) return;
    auto it = std::find(v.begin(), v.end(), cur);
    const int i = it == v.end() ? 0 : int(it - v.begin());
    out = v[size_t(((i + dir) % int(v.size()) + int(v.size())) % int(v.size()))];
}

std::string fmt_seconds(double s)
{
    char b[32];
    std::snprintf(b, sizeof b, s < 10 ? "%.1f s" : "%.0f s", s);
    return b;
}

std::vector<std::string> split_lines(const std::string& s)
{
    std::vector<std::string> out;
    std::istringstream in(s);
    std::string l;
    while (std::getline(in, l)) out.push_back(l);
    if (out.empty()) out.emplace_back();
    return out;
}

} // namespace

Designer::Designer(SDL_Window* window, SDL_Renderer* renderer, const std::string& open_file_path) : window_(window), r_(renderer)
{
    doc_ = design::Doc::new_tokens();
    if (!open_file_path.empty()) open_file(open_file_path);
    dirty_ = false; // nothing is compiled until asked for (F5) or needed (Save)
}

Designer::~Designer()
{
    // Never leave a worker running past the screen it reports to.
    if (job_.valid()) job_.wait();
    if (rel_job_.valid()) rel_job_.wait();
}

void Designer::open_file(const std::string& path)
{
    try
    {
        const fs::path p = from_u8(path);
        doc_ = design::Doc::from_text(read_all(p), p.parent_path());
        path_ = path;
        placed_.clear();
        for (const auto& entry : doc_.layout) placed_.insert(entry.first);
        status_ = trf("designer.opened", {path});
        dirty_ = false;
        open_pins_.clear();
        relations_.clear();
        sel_ = 0;
    }
    catch (const std::exception& e)
    {
        status_ = trf("designer.cannot_open", {path, e.what()});
    }
}

// ---------------------------------------------------------------- the nodes

void Designer::add(const std::string& node, FKind kind, const std::string& label, const std::string& value, std::function<void(int)> change,
                   std::function<void(const std::string&)> set, std::function<void()> remove)
{
    Field f;
    f.node = node;
    f.kind = kind;
    f.label = label;
    f.value = value;
    f.change = std::move(change);
    f.set = std::move(set);
    f.remove = std::move(remove);
    fields_.push_back(std::move(f));
    for (auto& n : nodes_)
        if (n.key == node) n.fields.push_back(int(fields_.size()) - 1);
}

std::vector<std::string> Designer::source_choices() const
{
    std::vector<std::string> out;
    for (const auto& p : doc_.params)
        if (p.kind == "dict") out.push_back("dict:{" + p.name + "}");
    try
    {
        for (const auto& e : sieve::cli::load_registry().entries) out.push_back("dict:" + e.id);
    }
    catch (const std::exception&)
    {
    }
    for (const auto& [file, words] : doc_.lists) out.push_back("list:" + file);
    // Tagged lists the filter already reads (written in the file; not made here), so cycling a
    // set's words can always come back to them.
    for (const auto& s : doc_.sets)
        if (s.source.rfind("tags:", 0) == 0 && std::find(out.begin(), out.end(), s.source) == out.end()) out.push_back(s.source);
    return out;
}

void Designer::build()
{
    fields_.clear();
    nodes_.clear();
    auto node = [&](const std::string& key, const std::string& title) { nodes_.push_back({key, title, {}, {}}); };
    auto mark = [this] { changed(); };

    // The entry node: prerequisites.
    node("requires", tr("designer.node.requires"));
    for (size_t i = 0; i < doc_.prerequisites.size(); ++i)
    {
        const std::string name = doc_.prerequisites[i].name;
        const bool open = std::find(open_pins_.begin(), open_pins_.end(), name) != open_pins_.end();
        add("requires", FKind::Button, name, open ? "[-]" : "[+]",
            [this, name, open](int) {
                if (open) open_pins_.erase(std::find(open_pins_.begin(), open_pins_.end(), name));
                else open_pins_.push_back(name);
            },
            {},
            [this, i, mark] {
                doc_.prerequisites.erase(doc_.prerequisites.begin() + long(i));
                mark();
            });
    }
    add("requires", FKind::Button, tr("designer.requires.add"), "[+]", [this](int) {
        picking_ = true;
        area_ = Area::List;
        status_ = tr("designer.pick");
    });

    // A settings node for each opened prerequisite: the settings it pins.
    for (const std::string& name : open_pins_)
    {
        auto it = std::find_if(doc_.prerequisites.begin(), doc_.prerequisites.end(), [&](const auto& r) { return r.name == name; });
        if (it == doc_.prerequisites.end()) continue;
        const size_t ri = size_t(it - doc_.prerequisites.begin());
        const std::string key = "pins:" + name;
        node(key, trf("designer.node.pins", {name}));
        const sieve::FilterSpec* spec = sieve::find_filter(name);
        if (!spec || spec->params.empty())
        {
            add(key, FKind::Info, tr("designer.pins.none"), "");
            continue;
        }
        for (const auto& p : spec->params)
        {
            auto& pins = doc_.prerequisites[ri].pins;
            auto pin = std::find_if(pins.begin(), pins.end(), [&](const auto& kv) { return kv.first == p.key; });
            const std::string value = pin == pins.end() ? tr("designer.pins.own") : pin->second;
            add(key, FKind::Choice, p.key, value,
                [this, ri, p, spec, mark](int dir) {
                    auto& ps = doc_.prerequisites[ri].pins;
                    auto at = std::find_if(ps.begin(), ps.end(), [&](const auto& kv) { return kv.first == p.key; });
                    std::string cur = at == ps.end() ? p.default_value : at->second;
                    std::string next = cur;
                    if (p.kind == sieve::FilterParam::Kind::Integer)
                    {
                        int64_t v = 0;
                        try { v = std::stoll(cur); } catch (...) { v = p.min; }
                        next = std::to_string(std::clamp<int64_t>(v + dir * p.step, p.min, p.max));
                    }
                    else
                    {
                        std::vector<std::string> choices = p.choices;
                        if (choices.empty())
                        {
                            choices.push_back("");
                            if (p.key == "dictionary" || p.registry == "dictionary")
                                for (const auto& e : sieve::cli::load_registry().entries) choices.push_back(e.id);
                        }
                        cycle(choices, cur, dir, next);
                    }
                    if (at == ps.end()) ps.emplace_back(p.key, next);
                    else at->second = next;
                    (void)spec;
                    mark();
                },
                {},
                [this, ri, p, mark] {
                    auto& ps = doc_.prerequisites[ri].pins;
                    ps.erase(std::remove_if(ps.begin(), ps.end(), [&](const auto& kv) { return kv.first == p.key; }), ps.end());
                    mark();
                });
        }
    }

    // The header.
    node("header", tr("designer.node.header"));
    add("header", FKind::Text, "id", doc_.id, {}, [this, mark](const std::string& v) { doc_.id = v; mark(); });
    add("header", FKind::Choice, "version", std::to_string(doc_.version), [this, mark](int dir) {
        doc_.version = uint32_t(std::max<int64_t>(1, int64_t(doc_.version) + dir));
        mark();
    });
    add("header", FKind::Text, "author", doc_.author, {}, [this, mark](const std::string& v) { doc_.author = v; mark(); });
    add("header", FKind::Choice, "origin", doc_.origin, [this, mark](int dir) {
        std::vector<std::string> o{"human", "ai-directed", "ai"};
        cycle(o, doc_.origin, dir, doc_.origin);
        mark();
    });
    {
        std::string ls;
        for (const auto& l : doc_.lines) ls += (ls.empty() ? "" : " ") + l;
        add("header", FKind::Choice, "lines", ls, [this, ls, mark](int dir) {
            std::vector<std::string> o{"text", "audio", "image", "video", "image video"};
            std::string n = ls;
            cycle(o, ls, dir, n);
            std::istringstream in(n);
            doc_.lines.clear();
            std::string w;
            while (in >> w) doc_.lines.push_back(w);
            mark();
        });
    }
    add("header", FKind::Choice, "symbols", doc_.symbols, [this, mark](int dir) {
        std::vector<std::string> o = sieve::alphabet_ids();
        o.push_back(sieve::kNotesSymbolsId);
        for (const auto& p : sieve::palette_ids()) o.push_back("palette:" + p);
        o.push_back("any");
        cycle(o, doc_.symbols, dir, doc_.symbols);
        mark();
    });
    add("header", FKind::Text, "describe", doc_.describe, {}, [this, mark](const std::string& v) { doc_.describe = v; mark(); });
    add("header", FKind::Choice, "form", doc_.form, [this, mark](int) {
        doc_.form = doc_.form == "tokens" ? "table" : "tokens";
        if (doc_.form == "table" && doc_.table.empty()) doc_.table = design::Doc::new_table().table;
        mark();
    });

    // Parameters.
    node("params", tr("designer.node.params"));
    for (size_t i = 0; i < doc_.params.size(); ++i)
    {
        auto remove = [this, i, mark] {
            doc_.params.erase(doc_.params.begin() + long(i));
            mark();
        };
        const auto& p = doc_.params[i];
        add("params", FKind::Text, "name", p.name, {}, [this, i, mark](const std::string& v) { doc_.params[i].name = v; mark(); }, remove);
        add("params", FKind::Choice, "  kind", p.kind, [this, i, mark](int) {
            auto& q = doc_.params[i];
            q.kind = q.kind == "int" ? "dict" : "int";
            q.def = q.kind == "int" ? "1" : "";
            mark();
        });
        add("params", FKind::Text, "  default", p.kind == "dict" && p.def.empty() ? "default" : p.def, {},
            [this, i, mark](const std::string& v) { doc_.params[i].def = v == "default" && doc_.params[i].kind == "dict" ? "" : v; mark(); });
        if (p.kind == "int")
        {
            add("params", FKind::Text, "  min", p.min, {}, [this, i, mark](const std::string& v) { doc_.params[i].min = v; mark(); });
            add("params", FKind::Text, "  max", p.max, {}, [this, i, mark](const std::string& v) { doc_.params[i].max = v; mark(); });
        }
        add("params", FKind::Text, "  about", p.text, {}, [this, i, mark](const std::string& v) { doc_.params[i].text = v; mark(); });
    }
    add("params", FKind::Button, tr("designer.params.add"), "[+]", [this, mark](int) {
        doc_.params.push_back({"p" + std::to_string(doc_.params.size() + 1), "int", "1", "1", "10", ""});
        mark();
    });

    // The rule.
    node("rule", tr(doc_.form == "tokens" ? "designer.node.tokens" : "designer.node.table"));
    if (doc_.form == "tokens")
    {
        add("rule", FKind::Text, "separator", doc_.separator, {}, [this, mark](const std::string& v) { doc_.separator = v; mark(); });
        add("rule", FKind::Choice, "edges", doc_.cut ? "cut" : "whole", [this, mark](int) {
            doc_.cut = !doc_.cut;
            mark();
        });
        add("rule", FKind::Button, tr("designer.sets.add"), "[+]", [this, mark](int) {
            const std::string name = "set" + std::to_string(doc_.sets.size() + 1);
            const std::string file = name + ".txt";
            doc_.lists[file] = "";
            doc_.sets.push_back({name, "list:" + file, true, true});
            mark();
        });
        for (size_t i = 0; i < doc_.sets.size(); ++i)
        {
            const std::string key = "set:" + doc_.sets[i].name;
            node(key, trf("designer.node.set", {doc_.sets[i].name}));
            const auto& s = doc_.sets[i];
            add(key, FKind::Text, "name", s.name, {}, [this, i, mark](const std::string& v) {
                const std::string old = doc_.sets[i].name;
                for (auto& [a, b] : doc_.follows)
                {
                    if (a == old) a = v;
                    if (b == old) b = v;
                }
                if (auto it = doc_.layout.find("set:" + old); it != doc_.layout.end())
                {
                    doc_.layout["set:" + v] = it->second;
                    doc_.layout.erase("set:" + old);
                }
                doc_.sets[i].name = v;
                mark();
            },
                [this, i, mark] {
                    const std::string old = doc_.sets[i].name;
                    doc_.follows.erase(std::remove_if(doc_.follows.begin(), doc_.follows.end(), [&](const auto& f) { return f.first == old || f.second == old; }),
                                       doc_.follows.end());
                    doc_.sets.erase(doc_.sets.begin() + long(i));
                    mark();
                });
            add(key, FKind::Choice, "words", s.source, [this, i, mark](int dir) {
                auto choices = source_choices();
                cycle(choices, doc_.sets[i].source, dir, doc_.sets[i].source);
                mark();
            });
            add(key, FKind::Toggle, "may start", s.first ? "yes" : "no", [this, i, mark](int) {
                doc_.sets[i].first = !doc_.sets[i].first;
                mark();
            });
            add(key, FKind::Toggle, "may end", s.last ? "yes" : "no", [this, i, mark](int) {
                doc_.sets[i].last = !doc_.sets[i].last;
                mark();
            });
            if (s.source.rfind("list:", 0) == 0)
            {
                const std::string file = s.source.substr(5);
                size_t n = 0;
                for (const auto& l : split_lines(doc_.lists[file]))
                    if (!l.empty()) ++n;
                add(key, FKind::Button, trf("designer.set.edit", {std::to_string(n)}), "...", [this, file](int) { open_editor("list:" + file); });
            }
            const std::string me = s.name;
            add(key, FKind::Button, link_from_.empty() ? tr("designer.set.link") : link_from_ == me ? tr("designer.set.linking") : trf("designer.set.link_to", {link_from_}),
                "->", [this, me, mark](int) {
                    if (link_from_.empty()) { link_from_ = me; status_ = trf("designer.linking", {me}); return; }
                    const std::pair<std::string, std::string> f{link_from_, me};
                    auto it = std::find(doc_.follows.begin(), doc_.follows.end(), f);
                    if (it == doc_.follows.end()) doc_.follows.push_back(f);
                    else doc_.follows.erase(it);
                    link_from_.clear();
                    mark();
                });
            for (size_t k = 0; k < doc_.follows.size(); ++k)
                if (doc_.follows[k].first == me)
                    add(key, FKind::Info, "  -> " + doc_.follows[k].second, "", {}, {}, [this, k, mark] {
                        doc_.follows.erase(doc_.follows.begin() + long(k));
                        mark();
                    });
        }
    }
    else
    {
        add("rule", FKind::Button, trf("designer.table.edit", {std::to_string(doc_.table.size())}), "...", [this](int) { open_editor("table"); });
        for (size_t i = 0; i < doc_.table.size() && i < 14; ++i) add("rule", FKind::Info, doc_.table[i], "");
        if (doc_.table.size() > 14) add("rule", FKind::Info, "...", "");
    }

    // Where a node sits: where it was put (dragged, or read from the file), or else under the
    // nodes before it in its column, worked out afresh as nodes grow (the entry node, the settings
    // its prerequisites pin, the header and the parameters in the first column; the rule and its
    // sets in the second).
    auto height = [&](const Node& n) { return kTitle + float(n.fields.size()) * kRow + 8; };
    auto column = [](const std::string& key) { return key == "requires" || key == "header" || key == "params" || key.rfind("pins:", 0) == 0 ? 0 : 1; };
    for (size_t i = 0; i < nodes_.size(); ++i)
    {
        Node& n = nodes_[i];
        if (placed_.count(n.key)) continue;
        const int c = column(n.key);
        float y = 10;
        for (size_t j = 0; j < i; ++j)
            if (column(nodes_[j].key) == c) y = std::max(y, doc_.layout[nodes_[j].key].y + height(nodes_[j]) + 12);
        doc_.layout[n.key] = {10 + float(c) * (kNodeW + 20), y};
    }
    // Nodes that are gone (a set deleted or renamed, a settings node closed) leave the layout.
    for (auto it = doc_.layout.begin(); it != doc_.layout.end();)
        it = std::any_of(nodes_.begin(), nodes_.end(), [&](const Node& n) { return n.key == it->first; }) ? std::next(it) : doc_.layout.erase(it);
    for (auto& n : nodes_)
    {
        design::Pos* p = &doc_.layout[n.key];
        const float h = height(n);
        n.rect = {canvas_.x + p->x + pan_x_, canvas_.y + p->y + pan_y_, kNodeW, h};
        for (size_t k = 0; k < n.fields.size(); ++k)
            fields_[size_t(n.fields[k])].rect = {n.rect.x + 4, n.rect.y + kTitle + float(k) * kRow, kNodeW - 8, kRow};
    }
    sel_ = std::clamp(sel_, 0, std::max(0, int(fields_.size()) - 1));
}

// ---------------------------------------------------------------- testing, on the worker

void Designer::changed()
{
    dirty_ = true;
    leave_warned_ = false;
    save_as_ = 0;
}

std::string Designer::current_key() const
{
    // What the file says, less where its nodes sit (moving a node changes nothing), and the length.
    design::Doc d = doc_;
    d.layout.clear();
    return d.to_text() + "|" + std::to_string(length_);
}

void Designer::request_test()
{
    if (stale()) retest_ = true;
}

void Designer::poll()
{
    if (job_running_ && job_.valid() && job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        result_ = job_.get();
        result_key_ = job_key_;
        job_running_ = false;
        verdict_ = design::judge(doc_, result_, judge_text_);
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - job_started_).count();
        done_ok_ = result_.error.empty();
        done_msg_ = done_ok_ ? trf("designer.popup.tested", {fmt_seconds(s)}) : trf("designer.popup.failed", {fmt_seconds(s)});
        done_at_ = std::chrono::steady_clock::now();
    }
    if (rel_running_ && rel_job_.valid() && rel_job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        relations_ = rel_job_.get();
        rel_running_ = false;
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - rel_started_).count();
        done_ok_ = std::none_of(relations_.begin(), relations_.end(), [](const std::string& r) { return r.find("duplicate") != std::string::npos; });
        done_msg_ = trf(done_ok_ ? "designer.popup.related" : "designer.popup.duplicate", {fmt_seconds(s)});
        done_at_ = std::chrono::steady_clock::now();
    }
    // What was waiting for a test: a save (only of a filter that compiles), and the relations.
    if (!job_running_ && !retest_ && (pending_save_ || pending_relations_))
    {
        if (stale()) retest_ = true;
        else if (!result_.error.empty())
        {
            if (pending_save_) status_ = tr("designer.not_saved.compile");
            pending_save_ = pending_relations_ = false;
        }
        else if (!rel_running_)
        {
            if (pending_save_)
            {
                pending_save_ = false;
                save_now();
            }
            else if (pending_relations_)
            {
                pending_relations_ = false;
                start_relations();
            }
        }
    }
    if (retest_ && !job_running_)
    {
        design::Doc d = doc_;
        d.layout.clear();
        retest_ = false;
        if (!stale()) return;
        job_key_ = current_key();
        job_running_ = true;
        popup_hidden_ = false;
        job_started_ = std::chrono::steady_clock::now();
        progress_ = std::make_shared<design::Progress>();
        progress_->set("starting");
        const uint32_t L = length_;
        auto pr = progress_;
        job_ = std::async(std::launch::async, [d, L, pr] {
            sieve::cli::timings::Scope timed("designer.test");
            return design::test(d, L, pr.get());
        });
    }
}

void Designer::settle()
{
    for (int i = 0; i < 2000; ++i)
    {
        poll();
        if (!job_running_ && !rel_running_ && !retest_ && !pending_save_ && !pending_relations_) return;
        SDL_Delay(5);
    }
}

void Designer::start_relations()
{
    if (rel_running_) return;
    if (stale() || !result_.dfa)
    {
        // Relations are of the filter as it is now: test it first.
        pending_relations_ = true;
        popup_hidden_ = false;
        request_test();
        return;
    }
    rel_running_ = true;
    relations_ = {tr("designer.relations.running")};
    design::Doc d = doc_;
    const design::TestResult t = result_;
    popup_hidden_ = false;
    rel_started_ = std::chrono::steady_clock::now();
    rel_progress_ = std::make_shared<design::Progress>();
    rel_progress_->set("starting");
    auto pr = rel_progress_;
    rel_job_ = std::async(std::launch::async, [d, t, pr] {
        sieve::cli::timings::Scope timed("designer.relations");
        return design::relations(d, t, pr.get());
    });
}

void Designer::do_save()
{
    // Nothing may be running over the registry while a filter is added to it: if the worker is
    // busy, the save waits for it (the progress window says so), and the window never freezes.
    // Only a filter that compiles is saved: test it first if it has changed since its last test.
    if (job_running_ || rel_running_ || retest_ || stale())
    {
        pending_save_ = true;
        popup_hidden_ = false;
        status_ = tr("designer.popup.save_waits");
        request_test();
        return;
    }
    if (!result_.error.empty())
    {
        status_ = tr("designer.not_saved.compile");
        return;
    }
    save_now();
}

void Designer::save_now()
{
    design::Doc d = doc_;
    if (save_as_) d.version = save_as_;
    uint32_t next = 0;
    status_ = design::save(d, next);
    if (next)
    {
        save_as_ = next;
        return;
    }
    if (status_.rfind("saved", 0) == 0 || status_.find("saved already") != std::string::npos)
    {
        doc_.version = d.version;
        save_as_ = 0;
        dirty_ = false;
        path_ = (design::filters_folder() / from_u8(doc_.file_name())).string();
        start_relations(); // duplicates are named straight away
    }
}

// ---------------------------------------------------------------- input

Designer::Result Designer::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
    music_mode(MusicMode::Menus);
    music_colours_default();
    while (!done_)
    {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 16))
        {
            handle(e);
            while (SDL_PollEvent(&e)) handle(e);
        }
        poll();
        render();
        present(r_);
    }
    SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_code_;
}

void Designer::press(SDL_Keycode k, SDL_Keymod mod)
{
    build();
    key(k, mod);
    poll();
}

void Designer::type(const std::string& utf8)
{
    if (editor_open_)
    {
        std::string& l = editor_lines_[size_t(ed_row_)];
        l.insert(size_t(std::min<int>(ed_col_, int(l.size()))), utf8);
        ed_col_ += int(utf8.size());
        return;
    }
    if (editing_) edit_ += utf8;
}

void Designer::handle(const SDL_Event& event)
{
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT)
    {
        result_code_ = Result::Quit;
        done_ = true;
        return;
    }
    if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        type(e.text.text);
        return;
    }
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        key(e.key.key, e.key.mod);
        return;
    }
    if (editor_open_) return;
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        const float mx = e.button.x, my = e.button.y;
        if (e.button.button != SDL_BUTTON_LEFT && inside(canvas_, mx, my))
        {
            panning_ = true;
            pan_mx_ = mx;
            pan_my_ = my;
            return;
        }
        for (const auto& [rect, i] : list_rects_)
            if (inside(rect, mx, my))
            {
                area_ = Area::List;
                list_row_ = i;
                return;
            }
        for (const auto& [rect, i] : test_rects_)
            if (inside(rect, mx, my))
            {
                area_ = Area::Test;
                test_sel_ = i;
                key(SDLK_RETURN, SDL_KMOD_NONE);
                return;
            }
        // A node's title: drag it; a field: choose it (and press a button).
        for (auto it = nodes_.rbegin(); it != nodes_.rend(); ++it)
        {
            const SDL_FRect title{it->rect.x, it->rect.y, it->rect.w, kTitle};
            if (inside(title, mx, my))
            {
                dragging_ = it->key;
                placed_.insert(it->key);
                drag_dx_ = mx - it->rect.x;
                drag_dy_ = my - it->rect.y;
                area_ = Area::Canvas;
                return;
            }
        }
        for (size_t i = 0; i < fields_.size(); ++i)
            if (inside(fields_[i].rect, mx, my))
            {
                if (editing_) finish_edit(true);
                area_ = Area::Canvas;
                sel_ = int(i);
                if (fields_[i].kind == FKind::Button || fields_[i].kind == FKind::Toggle) activate(1);
                return;
            }
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        if (!dragging_.empty())
        {
            auto& p = doc_.layout[dragging_];
            p.x = e.motion.x - drag_dx_ - canvas_.x - pan_x_;
            p.y = e.motion.y - drag_dy_ - canvas_.y - pan_y_;
            dirty_ = true;
        }
        if (panning_)
        {
            pan_x_ += e.motion.x - pan_mx_;
            pan_y_ += e.motion.y - pan_my_;
            pan_mx_ = e.motion.x;
            pan_my_ = e.motion.y;
        }
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        dragging_.clear();
        panning_ = false;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        if (area_ == Area::List) list_top_ = std::max(0, list_top_ - int(e.wheel.y));
        else pan_y_ += e.wheel.y * 30;
    }
}

void Designer::key(SDL_Keycode k, SDL_Keymod mod)
{
    const bool shift = (mod & SDL_KMOD_SHIFT) != 0, ctrl = (mod & SDL_KMOD_CTRL) != 0;
    if (editor_open_)
    {
        editor_key(k, mod);
        return;
    }
    if (editing_)
    {
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) finish_edit(true);
        else if (k == SDLK_ESCAPE) finish_edit(false);
        else if (k == SDLK_BACKSPACE && ctrl) edit_.clear();
        else if (k == SDLK_BACKSPACE && !edit_.empty())
        {
            // Back one character, not one byte.
            size_t n = edit_.size() - 1;
            while (n > 0 && (static_cast<unsigned char>(edit_[n]) & 0xC0) == 0x80) --n;
            edit_.resize(n);
        }
        return;
    }
    if (k == SDLK_TAB)
    {
        area_ = area_ == Area::List ? Area::Canvas : area_ == Area::Canvas ? Area::Test : Area::List;
        return;
    }
    if (ctrl && k == SDLK_S) { do_save(); return; }
    if (k == SDLK_F5) { request_test(); popup_hidden_ = false; return; }
    if (ctrl && k == SDLK_N) { doc_ = design::Doc::new_tokens(); path_.clear(); open_pins_.clear(); relations_.clear(); changed(); status_ = tr("designer.new"); return; }
    if (ctrl && k == SDLK_T) { doc_ = design::Doc::new_table(); path_.clear(); open_pins_.clear(); relations_.clear(); changed(); status_ = tr("designer.new"); return; }
    if (k == SDLK_ESCAPE)
    {
        if (popup_visible()) { popup_hidden_ = true; return; }
        if (picking_) { picking_ = false; area_ = Area::Canvas; status_.clear(); return; }
        if (!link_from_.empty()) { link_from_.clear(); status_.clear(); return; }
        if (dirty_ && !leave_warned_)
        {
            leave_warned_ = true;
            status_ = tr("designer.unsaved");
            return;
        }
        done_ = true;
        return;
    }
    if (area_ == Area::List) { list_key(k); return; }
    if (area_ == Area::Test)
    {
        constexpr int kItems = 5; // length, test now, text to judge, relations, save
        if (k == SDLK_UP) test_sel_ = (test_sel_ + kItems - 1) % kItems;
        else if (k == SDLK_DOWN) test_sel_ = (test_sel_ + 1) % kItems;
        else if (test_sel_ == 0 && (k == SDLK_LEFT || k == SDLK_RIGHT))
        {
            const int64_t step = shift ? 10 : 1;
            length_ = uint32_t(std::clamp<int64_t>(int64_t(length_) + (k == SDLK_RIGHT ? step : -step), 1, 100000));
        }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER)
        {
            if (test_sel_ == 1) { request_test(); popup_hidden_ = false; }
            else if (test_sel_ == 2) { editing_ = true; judge_edit_ = true; edit_ = judge_text_; }
            else if (test_sel_ == 3) start_relations();
            else if (test_sel_ == 4) do_save();
        }
        return;
    }
    // The canvas.
    const int n = int(fields_.size());
    if (n == 0) return;
    if (ctrl && (k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN))
    {
        pan_x_ += k == SDLK_LEFT ? 40.f : k == SDLK_RIGHT ? -40.f : 0.f;
        pan_y_ += k == SDLK_UP ? 40.f : k == SDLK_DOWN ? -40.f : 0.f;
        return;
    }
    switch (k)
    {
    case SDLK_UP: sel_ = (sel_ + n - 1) % n; break;
    case SDLK_DOWN: sel_ = (sel_ + 1) % n; break;
    case SDLK_LEFT: activate(-1); break;
    case SDLK_RIGHT: activate(1); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE: activate(1); break;
    case SDLK_DELETE:
        if (sel_ >= 0 && sel_ < n && fields_[size_t(sel_)].remove) fields_[size_t(sel_)].remove();
        break;
    default: break;
    }
}

void Designer::activate(int dir)
{
    if (sel_ < 0 || sel_ >= int(fields_.size())) return;
    Field& f = fields_[size_t(sel_)];
    switch (f.kind)
    {
    case FKind::Text: start_edit(); break;
    case FKind::Choice:
    case FKind::Toggle:
    case FKind::Button:
        if (f.change) f.change(dir);
        break;
    case FKind::Info: break;
    }
}

void Designer::start_edit()
{
    editing_ = true;
    edit_ = fields_[size_t(sel_)].value;
}

void Designer::finish_edit(bool keep)
{
    editing_ = false;
    const bool judge = judge_edit_;
    judge_edit_ = false;
    if (!keep) return;
    if (judge) // the text to judge
    {
        judge_text_ = edit_;
        if (stale()) request_test(); // judged against the filter as it is now, when the test is done
        else verdict_ = design::judge(doc_, result_, judge_text_);
        return;
    }
    if (sel_ < int(fields_.size()) && fields_[size_t(sel_)].set) fields_[size_t(sel_)].set(edit_);
}

std::vector<const sieve::FilterSpec*> Designer::list_rows() const
{
    std::vector<const sieve::FilterSpec*> out;
    try
    {
        for (const sieve::FilterSpec* f : sieve::filters_for(design::test_line(doc_.symbols, length_)))
            if (f->plugin_sha256.empty() == (tab_ == 0)) out.push_back(f);
    }
    catch (const std::exception&)
    {
    }
    return out;
}

void Designer::list_key(SDL_Keycode k)
{
    const auto rows = list_rows();
    const int n = int(rows.size());
    if (k == SDLK_LEFT || k == SDLK_RIGHT)
    {
        tab_ = 1 - tab_;
        list_row_ = 0;
        return;
    }
    if (n == 0) return;
    if (k == SDLK_UP) list_row_ = (list_row_ + n - 1) % n;
    if (k == SDLK_DOWN) list_row_ = (list_row_ + 1) % n;
    list_row_ = std::clamp(list_row_, 0, n - 1);
    const sieve::FilterSpec* f = rows[size_t(list_row_)];
    if (k == SDLK_R || (picking_ && (k == SDLK_RETURN || k == SDLK_KP_ENTER)))
    {
        // As a prerequisite of the filter being made.
        if (f->name() == doc_.name()) { status_ = tr("designer.self"); return; }
        if (std::none_of(doc_.prerequisites.begin(), doc_.prerequisites.end(), [&](const auto& r) { return r.name == f->name(); }))
        {
            doc_.prerequisites.push_back({f->name(), {}});
            changed();
        }
        status_ = trf("designer.required", {f->name()});
        picking_ = false;
        area_ = Area::Canvas;
        return;
    }
    if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !f->plugin_sha256.empty())
    {
        // Open a custom filter's file here.
        for (const auto& pf : sieve::cli::load_plugins())
            if (pf.name == f->name())
            {
                open_file(pf.path);
                changed();
                dirty_ = false;
                area_ = Area::Canvas;
                return;
            }
        const fs::path saved = design::filters_folder() / from_u8(f->name() + ".sfilter");
        std::error_code ec;
        if (fs::exists(saved, ec))
        {
            const std::u8string u = saved.u8string();
            open_file(std::string(u.begin(), u.end()));
            changed();
            dirty_ = false;
            area_ = Area::Canvas;
        }
    }
}

// ---------------------------------------------------------------- the multi-line editor

void Designer::open_editor(const std::string& target)
{
    editor_open_ = true;
    editor_target_ = target;
    if (target == "table") editor_lines_ = doc_.table.empty() ? std::vector<std::string>{""} : doc_.table;
    else editor_lines_ = split_lines(doc_.lists[target.substr(5)]);
    ed_row_ = ed_col_ = ed_top_ = 0;
}

void Designer::close_editor()
{
    editor_open_ = false;
    if (editor_target_ == "table") doc_.table = editor_lines_;
    else
    {
        std::string s;
        for (const auto& l : editor_lines_)
            if (!l.empty()) s += l + "\n";
        doc_.lists[editor_target_.substr(5)] = s;
    }
    changed();
}

void Designer::editor_key(SDL_Keycode k, SDL_Keymod mod)
{
    (void)mod;
    auto& lines = editor_lines_;
    std::string& l = lines[size_t(ed_row_)];
    ed_col_ = std::min<int>(ed_col_, int(l.size()));
    switch (k)
    {
    case SDLK_ESCAPE: close_editor(); return;
    case SDLK_UP: ed_row_ = std::max(0, ed_row_ - 1); break;
    case SDLK_DOWN: ed_row_ = std::min(int(lines.size()) - 1, ed_row_ + 1); break;
    case SDLK_LEFT:
        if (ed_col_ > 0) --ed_col_;
        else if (ed_row_ > 0) { --ed_row_; ed_col_ = int(lines[size_t(ed_row_)].size()); }
        break;
    case SDLK_RIGHT:
        if (ed_col_ < int(l.size())) ++ed_col_;
        else if (ed_row_ + 1 < int(lines.size())) { ++ed_row_; ed_col_ = 0; }
        break;
    case SDLK_HOME: ed_col_ = 0; break;
    case SDLK_END: ed_col_ = int(l.size()); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    {
        const std::string rest = l.substr(size_t(ed_col_));
        l.resize(size_t(ed_col_));
        lines.insert(lines.begin() + ed_row_ + 1, rest);
        ++ed_row_;
        ed_col_ = 0;
        break;
    }
    case SDLK_BACKSPACE:
        if (ed_col_ > 0)
        {
            l.erase(size_t(ed_col_ - 1), 1);
            --ed_col_;
        }
        else if (ed_row_ > 0)
        {
            const std::string cur = l;
            lines.erase(lines.begin() + ed_row_);
            --ed_row_;
            ed_col_ = int(lines[size_t(ed_row_)].size());
            lines[size_t(ed_row_)] += cur;
        }
        break;
    case SDLK_DELETE:
        if (ed_col_ < int(l.size())) l.erase(size_t(ed_col_), 1);
        else if (ed_row_ + 1 < int(lines.size()))
        {
            l += lines[size_t(ed_row_ + 1)];
            lines.erase(lines.begin() + ed_row_ + 1);
        }
        break;
    default: break;
    }
}

// ---------------------------------------------------------------- drawing

void Designer::render()
{
    constexpr int kMinW = 1240, kMinH = 720;
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    txt(r_, 16, 12, tr("designer.title"), kWhite, 2.5f);
    const std::string file = doc_.file_name() + (dirty_ ? "  " + tr("designer.unsaved_mark") : "");
    txt(r_, 16, 38, file, dirty_ ? kAccent : kGrey);
    const float top = 56, bottom = H - 54, left_w = 260, right_w = 360;
    canvas_ = {left_w + 12, top, W - left_w - right_w - 24, bottom - top};
    build();
    draw_list(8, top, left_w, bottom - top);
    draw_canvas(canvas_.x, canvas_.y, canvas_.w, canvas_.h);
    draw_test(W - right_w - 4, top, right_w, bottom - top);
    txt(r_, 16, H - 46, status_.substr(0, size_t((W - 32) / 8)), status_.find("not ") == 0 ? kBad : kWhite);
    txt(r_, 16, H - 30, tr(editor_open_ ? "designer.footer.editor" : "designer.footer"), kGrey);
    if (!editor_open_) txt(r_, 16, H - 16, tr("designer.footer2"), kGrey);
    if (editor_open_) draw_editor(W, H);
    draw_popup(W, H);
}

bool Designer::popup_visible() const
{
    if (popup_hidden_) return false;
    const auto now = std::chrono::steady_clock::now();
    // Not for work done in a blink: only once it has run a fifth of a second, so the window never
    // flickers; then for two seconds after, to say how it went.
    const bool busy = (job_running_ && now - job_started_ > std::chrono::milliseconds(200)) ||
                      (rel_running_ && now - rel_started_ > std::chrono::milliseconds(200)) || pending_save_;
    return busy || (!done_msg_.empty() && now - done_at_ < std::chrono::milliseconds(2000));
}

void Designer::draw_popup(float W, float H)
{
    if (!popup_visible()) return;
    const auto now = std::chrono::steady_clock::now();
    const bool busy = job_running_ || rel_running_ || pending_save_;
    const SDL_FRect box{W / 2 - 320, H / 2 - 55, 640, 110};
    frame(r_, box, busy ? kAccent : done_ok_ ? kGood : kBad, 250);
    const SDL_FRect inner{box.x + 3, box.y + 3, box.w - 6, box.h - 6};
    SDL_SetRenderDrawColor(r_, 60, 60, 60, 255);
    SDL_RenderRect(r_, &inner);
    const size_t cols = size_t((box.w - 32) / 8);
    if (!busy)
    {
        txt(r_, box.x + 16, box.y + 44, fit_cells(done_msg_, cols), done_ok_ ? kGood : kBad);
        return;
    }
    const bool rel = rel_running_ && !job_running_;
    const auto since = rel ? rel_started_ : job_started_;
    const double s = std::chrono::duration<double>(now - since).count();
    static const char spin[4] = {'|', '/', '-', '\\'};
    const std::string title = pending_save_ && !job_running_ && !rel_running_ ? tr("designer.popup.saving")
                              : job_running_                                   ? tr("designer.popup.testing")
                                                                               : tr("designer.popup.relations");
    txt(r_, box.x + 16, box.y + 14, title + "  " + std::string(1, spin[int(s * 8) % 4]), kWhite, 1.5f);
    const std::string step = rel ? (rel_progress_ ? rel_progress_->step() : "") : (progress_ ? progress_->step() : "");
    txt(r_, box.x + 16, box.y + 42, fit_cells(step, cols), kAccent);
    txt(r_, box.x + 16, box.y + 60, trf("designer.popup.elapsed", {fmt_seconds(s)}) + (pending_save_ ? "   " + tr("designer.popup.then_save") : ""), kGrey);
    txt(r_, box.x + 16, box.y + 84, fit_cells(tr("designer.popup.hint"), cols), kDim);
}

void Designer::draw_list(float x, float y, float w, float h)
{
    const SDL_FRect box{x, y, w, h};
    frame(r_, box, area_ == Area::List ? kWhite : kDim);
    const auto rows = list_rows();
    size_t built = 0, custom = 0;
    try
    {
        for (const sieve::FilterSpec* f : sieve::filters_for(design::test_line(doc_.symbols, length_))) ++(f->plugin_sha256.empty() ? built : custom);
    }
    catch (const std::exception&)
    {
    }
    txt(r_, x + 8, y + 8, tr(picking_ ? "designer.list.pick" : "designer.list.title"), picking_ ? kAccent : kWhite);
    const std::string a = trf("designer.tab.builtin", {std::to_string(built)}), b = trf("designer.tab.custom", {std::to_string(custom)});
    txt(r_, x + 8, y + 24, tab_ == 0 ? "[" + a + "] " + b : a + " [" + b + "]", kGrey);
    list_rects_.clear();
    const int visible = int((h - 180) / kRow);
    list_row_ = std::clamp(list_row_, 0, std::max(0, int(rows.size()) - 1));
    if (list_row_ < list_top_) list_top_ = list_row_;
    if (list_row_ >= list_top_ + visible) list_top_ = list_row_ - visible + 1;
    float yy = y + 44;
    for (int i = list_top_; i < int(rows.size()) && i < list_top_ + visible; ++i)
    {
        const bool on = i == list_row_;
        const SDL_FRect r{x + 4, yy - 2, w - 8, kRow};
        if (on && area_ == Area::List)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            SDL_RenderFillRect(r_, &r);
        }
        list_rects_.emplace_back(r, i);
        txt(r_, x + 8, yy, (on ? "> " : "  ") + rows[size_t(i)]->name(), on ? kWhite : kGrey);
        yy += kRow;
    }
    if (rows.empty()) txt(r_, x + 8, yy, tr("designer.list.none"), kGrey);
    // The chosen filter, described.
    if (!rows.empty())
    {
        const sieve::FilterSpec* f = rows[size_t(list_row_)];
        float dy = y + h - 130;
        std::string d = tr_or("filter." + f->name(), f->description);
        if (!f->plugin_sha256.empty()) d += "  (" + f->author + ", " + f->origin + ")";
        const size_t cols = size_t((w - 16) / 8);
        while (!d.empty() && dy < y + h - 40)
        {
            size_t cut = d.size() <= cols ? d.size() : d.rfind(' ', cols);
            if (cut == std::string::npos || cut == 0) cut = std::min(d.size(), cols);
            txt(r_, x + 8, dy, d.substr(0, cut), kGrey);
            d = d.substr(std::min(d.size(), cut + 1));
            dy += kRow;
        }
        txt(r_, x + 8, y + h - 28, tr(f->plugin_sha256.empty() ? "designer.list.keys.builtin" : "designer.list.keys.custom"), kDim);
    }
}

void Designer::draw_canvas(float x, float y, float w, float h)
{
    const SDL_FRect box{x, y, w, h};
    frame(r_, box, area_ == Area::Canvas ? kWhite : kDim, 255);
    SDL_Rect clip{int(x) + 1, int(y) + 1, int(w) - 2, int(h) - 2};
    SDL_SetRenderClipRect(r_, &clip);
    // follow arrows first, under the nodes
    auto rect_of = [&](const std::string& key) -> const SDL_FRect* {
        for (const auto& n : nodes_)
            if (n.key == key) return &n.rect;
        return nullptr;
    };
    // follow arrows: between side by side nodes, straight across; between nodes in one column,
    // round their right-hand side, each arrow in its own lane so they can be told apart.
    SDL_SetRenderDrawColor(r_, kAccent.r, kAccent.g, kAccent.b, 255);
    auto head = [&](float x2, float y2, float ux, float uy) {
        SDL_RenderLine(r_, x2, y2, x2 - 8 * ux + 4 * uy, y2 - 8 * uy - 4 * ux);
        SDL_RenderLine(r_, x2, y2, x2 - 8 * ux - 4 * uy, y2 - 8 * uy + 4 * ux);
    };
    int lane = 0;
    for (const auto& [a, b] : doc_.follows)
    {
        const SDL_FRect* ra = rect_of("set:" + a);
        const SDL_FRect* rb = rect_of("set:" + b);
        if (!ra || !rb) continue;
        const float ya = ra->y + kTitle / 2 + 2, yb = rb->y + kTitle / 2 + 6;
        if (std::abs(ra->x - rb->x) < kNodeW)
        {
            const float xr = std::max(ra->x + ra->w, rb->x + rb->w), xl = xr + 8 + 6 * float(lane++ % 6);
            SDL_RenderLine(r_, ra->x + ra->w, ya, xl, ya);
            SDL_RenderLine(r_, xl, ya, xl, yb);
            SDL_RenderLine(r_, xl, yb, rb->x + rb->w, yb);
            head(rb->x + rb->w, yb, -1, 0);
            continue;
        }
        const bool right = rb->x > ra->x;
        const float x1 = right ? ra->x + ra->w : ra->x, x2 = right ? rb->x : rb->x + rb->w;
        SDL_RenderLine(r_, x1, ya, x2, yb);
        const float dx = x2 - x1, dy = yb - ya, len = std::max(1.f, std::sqrt(dx * dx + dy * dy));
        head(x2, yb, dx / len, dy / len);
    }
    for (const auto& n : nodes_)
    {
        const bool has_sel = sel_ >= 0 && sel_ < int(fields_.size()) && fields_[size_t(sel_)].node == n.key;
        const bool entry = n.key == "requires";
        frame(r_, n.rect, has_sel && area_ == Area::Canvas ? kWhite : entry ? kAccent : kGrey, 245);
        txt(r_, n.rect.x + 6, n.rect.y + 5, n.title, entry ? kAccent : kWhite);
        for (int fi : n.fields)
        {
            const Field& f = fields_[size_t(fi)];
            const bool on = fi == sel_ && area_ == Area::Canvas && !judge_edit_;
            if (on)
            {
                SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
                SDL_RenderFillRect(r_, &f.rect);
            }
            const std::string value = on && editing_ ? edit_ + "_" : f.value;
            const size_t cols = size_t((kNodeW - 16) / 8);
            std::string line = f.label;
            if (!value.empty()) line += (f.kind == FKind::Button ? "  " : ": ") + value;
            if (f.kind == FKind::Button) line = value + " " + f.label;
            txt(r_, f.rect.x + 2, f.rect.y + 2, fit_cells(line, cols), f.kind == FKind::Info ? kGrey : on ? kWhite : SDL_Color{200, 200, 200, 255});
        }
    }
    SDL_SetRenderClipRect(r_, nullptr);
}

void Designer::draw_test(float x, float y, float w, float h)
{
    const SDL_FRect box{x, y, w, h};
    frame(r_, box, area_ == Area::Test ? kWhite : kDim);
    const size_t cols = size_t((w - 16) / 8);
    float yy = y + 8;
    auto line = [&](const std::string& s, SDL_Color c) {
        txt(r_, x + 8, yy, fit_cells(s, cols), c);
        yy += kRow;
    };
    auto wrap = [&](std::string s, SDL_Color c, int max_lines) {
        for (int n = 0; !s.empty() && n < max_lines; ++n)
        {
            size_t cut = s.size() <= cols ? s.size() : s.rfind(' ', cols);
            if (cut == std::string::npos || cut == 0) cut = std::min(s.size(), cols);
            line(s.substr(0, cut), c);
            s = s.substr(std::min(s.size(), cut + 1));
        }
    };
    line(tr("designer.test.title"), kWhite);
    yy += 4;
    test_rects_.clear();
    auto item = [&](int i, const std::string& s) {
        const SDL_FRect r{x + 4, yy - 2, w - 8, kRow};
        test_rects_.emplace_back(r, i);
        if (area_ == Area::Test && test_sel_ == i)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            SDL_RenderFillRect(r_, &r);
        }
        line((area_ == Area::Test && test_sel_ == i ? "> " : "  ") + s, kWhite);
    };
    item(0, trf("designer.test.length", {std::to_string(length_)}));
    item(1, tr("designer.test.now"));
    yy += 4;
    const bool old = !job_running_ && stale();
    if (old) wrap(tr(result_.dfa || !result_.error.empty() ? "designer.test.stale" : "designer.test.untested"), kAccent, 3);
    if (job_running_) line(tr("designer.test.compiling"), kAccent);
    else if (!result_.error.empty())
    {
        line(tr("designer.test.error"), kBad);
        wrap(result_.error, kBad, 6);
    }
    else if (result_.dfa)
    {
        line(trf(doc_.form == "tokens" ? "designer.test.words_states" : "designer.test.states", {std::to_string(result_.declared), std::to_string(result_.minimal)}), kGrey);
        if (result_.counted)
        {
            wrap(trf("designer.test.survivors", {result_.survivors}), kGood, 3);
            wrap(trf("designer.test.excluded", {result_.excluded}), kGrey, 3);
            line(tr("designer.test.samples"), kGrey);
            for (const auto& s : result_.samples) line("  " + s, SDL_Color{200, 200, 200, 255});
        }
        else line(tr("designer.test.judge_only"), kGrey);
    }
    yy += 6;
    const bool editing_judge = editing_ && judge_edit_;
    item(2, tr("designer.test.type"));
    line("  " + (editing_judge ? edit_ + "_" : "\"" + judge_text_ + "\""), editing_judge ? kWhite : SDL_Color{200, 200, 200, 255});
    wrap("  " + verdict_, verdict_ == "passes" ? kGood : kBad, 3);
    yy += 6;
    item(3, tr(rel_running_ ? "designer.test.relations_running" : "designer.test.relations"));
    for (const auto& r : relations_) wrap("  " + r, r.find("duplicate") != std::string::npos ? kBad : kGrey, 2);
    yy = std::max(yy + 6, y + h - 40);
    item(4, save_as_ ? trf("designer.test.save_as", {std::to_string(save_as_)}) : tr("designer.test.save"));
    line("  " + design::filters_folder().string(), kDim);
}

void Designer::draw_editor(float W, float H)
{
    const SDL_FRect box{80, 60, W - 160, H - 120};
    frame(r_, box, kWhite, 250);
    txt(r_, box.x + 10, box.y + 8, editor_target_ == "table" ? tr("designer.editor.table") : trf("designer.editor.list", {editor_target_.substr(5)}), kWhite);
    const int visible = int((box.h - 50) / kRow);
    if (ed_row_ < ed_top_) ed_top_ = ed_row_;
    if (ed_row_ >= ed_top_ + visible) ed_top_ = ed_row_ - visible + 1;
    float yy = box.y + 30;
    for (int i = ed_top_; i < int(editor_lines_.size()) && i < ed_top_ + visible; ++i)
    {
        const std::string& l = editor_lines_[size_t(i)];
        txt(r_, box.x + 10, yy, fit_cells(std::to_string(i + 1), 4), kDim);
        txt(r_, box.x + 50, yy, l, kWhite);
        if (i == ed_row_)
        {
            const float cx = box.x + 50 + text_width(l.substr(0, size_t(std::min<int>(ed_col_, int(l.size())))), 1);
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
            SDL_RenderLine(r_, cx, yy - 1, cx, yy + 9);
        }
        yy += kRow;
    }
}

} // namespace hallway
