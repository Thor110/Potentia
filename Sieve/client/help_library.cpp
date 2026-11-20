// The Documentation (help_library.hpp): Sieve's help library, read from help/ and shown as contents,
// index and search beside the page chosen.

#include "help_library.hpp"

#include "font.hpp"
#include "footer.hpp"
#include "keys.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/dictionaries.hpp" // executable_dir
#include "sieve/text.hpp"
#include "sieve/paths.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hallway {

namespace {

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kAccent{120, 200, 255, 255},
    kLinkColour{120, 230, 160, 255};

constexpr float kLine = 14; // one line of text at scale 1 (8-pixel cells and a gap)
constexpr int kMinW = 1024, kMinH = 640;
constexpr float kMargin = 24, kListW = 300, kTop = 96;

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

// (sieve/text.hpp's, so this file keeps no copies of its own; review 4)
std::string lower(std::string s) { return sieve::ascii_lower(std::move(s)); }
using sieve::trim;

// The parts of a line between `sep`, each trimmed.
std::vector<std::string> split(const std::string& s, char sep)
{
    std::vector<std::string> out = sieve::split_fields(s, sep);
    for (std::string& f : out) f = sieve::trim(f);
    return out;
}

// The last UTF-8 character of s removed.
void pop_char(std::string& s)
{
    while (!s.empty() && (uint8_t(s.back()) & 0xC0) == 0x80) s.pop_back();
    if (!s.empty()) s.pop_back();
}

} // namespace

// Beside the programs it is help-library (CMakeLists.txt says why); in the repository, data/help. A help
// folder beside the programs, as earlier builds and releases left it, is still read.
std::filesystem::path HelpLibrary::folder()
{
    namespace fs = std::filesystem;
    const fs::path exe = sieve::cli::executable_dir();
    std::vector<fs::path> folders = {fs::path("data") / "help"}; // (run from the repository: the pages as they are now)
    if (!exe.empty())
        for (const fs::path& f : {exe / "help-library", exe / "help", exe / ".." / "data" / "help"}) folders.push_back(f);
    for (const fs::path& f : folders)
    {
        std::error_code ec;
        if (fs::is_regular_file(f / "contents.txt", ec)) return f;
    }
    return folders.front();
}

HelpLibrary::HelpLibrary(SDL_Window* window, SDL_Renderer* renderer, const std::filesystem::path& folder, const std::string& page)
    : window_(window), r_(renderer)
{
    load(folder);
    build_rows();
    const int asked = page.empty() ? -1 : find_page(page);
    if (!page.empty() && asked < 0) status_ = trf("docs.no_page", {page});
    open(asked >= 0 ? asked : pages_.empty() ? -1 : 0, false);
    for (size_t i = 0; i < rows_.size(); ++i)
        if (rows_[i].page == page_)
        {
            row_ = int(i);
            break;
        }
}

// contents.txt, then each page it names; a page whose file is missing is said, not shown.
void HelpLibrary::load(const std::filesystem::path& dir)
{
    std::ifstream in(dir / "contents.txt");
    if (!in)
    {
        status_ = trf("docs.no_library", {sieve::u8(dir / "contents.txt")});
        return;
    }
    std::string l;
    std::vector<std::string> missing;
    while (std::getline(in, l))
    {
        l = trim(l);
        if (l.empty() || l[0] == '#') continue;
        const size_t a = l.find(' '), b = a == std::string::npos ? std::string::npos : l.find(' ', a + 1);
        if (b == std::string::npos) continue;
        const std::string kind = l.substr(0, a), id = l.substr(a + 1, b - a - 1), title = trim(l.substr(b + 1));
        if (kind == "category") categories_.push_back({id, title, {}});
        if (kind != "page") continue;
        std::ifstream p(dir / (id + ".txt"));
        if (!p)
        {
            missing.push_back(id);
            continue;
        }
        Page pg{id, title, categories_.empty() ? std::string() : categories_.back().id, {}, {}};
        std::string pl;
        for (bool first = true; std::getline(p, pl); first = false)
        {
            if (!pl.empty() && pl.back() == '\r') pl.pop_back();
            if (first && pl.rfind("index:", 0) == 0)
            {
                for (const std::string& t : split(pl.substr(6), ','))
                    if (!t.empty()) pg.terms.push_back(t);
                continue;
            }
            pg.lines.push_back(pl);
        }
        while (!pg.lines.empty() && trim(pg.lines.front()).empty()) pg.lines.erase(pg.lines.begin());
        if (categories_.empty()) categories_.push_back({"", "", {}});
        categories_.back().pages.push_back(pages_.size());
        pages_.push_back(std::move(pg));
    }
    if (!missing.empty())
    {
        std::string names;
        for (const std::string& m : missing) names += (names.empty() ? "" : ", ") + m;
        status_ = trf("docs.missing", {names});
    }
}

int HelpLibrary::find_page(const std::string& id) const
{
    for (size_t i = 0; i < pages_.size(); ++i)
        if (pages_[i].id == id) return int(i);
    return -1;
}

std::string HelpLibrary::page_id() const { return page_ >= 0 ? pages_[size_t(page_)].id : std::string(); }

// The list's rows for the tab shown: the contents (each category, its pages under it), the index (every
// term, in order, under its first letter, those beginning otherwise first, under #), or the search's results (every page holding every word typed,
// with the first line holding the first word under it).
void HelpLibrary::build_rows()
{
    rows_.clear();
    if (tab_ == Tab::Contents)
        for (const Category& c : categories_)
        {
            if (!c.title.empty()) rows_.push_back({c.title, -1, 0});
            for (const size_t p : c.pages) rows_.push_back({pages_[p].title, int(p), 1});
        }
    else if (tab_ == Tab::Index)
    {
        std::vector<std::pair<std::string, int>> terms;
        for (size_t p = 0; p < pages_.size(); ++p)
            for (const std::string& t : pages_[p].terms) terms.emplace_back(t, int(p));
        std::stable_sort(terms.begin(), terms.end(), [](const auto& a, const auto& b) { return lower(a.first) < lower(b.first); });
        char letter = 0;
        for (const auto& [t, p] : terms)
        {
            // (terms beginning with a digit or a sign, ".sieve" or "[", together under #)
            const unsigned char c0 = static_cast<unsigned char>(t[0]);
            const char first = std::isalpha(c0) ? char(std::toupper(c0)) : '#';
            if (first != letter)
            {
                letter = first;
                rows_.push_back({std::string(1, letter), -1, 0});
            }
            rows_.push_back({t + "  (" + pages_[size_t(p)].title + ")", p, 1});
        }
    }
    else
    {
        std::vector<std::string> want;
        for (const std::string& w : split(lower(query_), ' '))
            if (!w.empty()) want.push_back(w);
        if (!want.empty())
            for (size_t p = 0; p < pages_.size(); ++p)
            {
                std::string all = lower(pages_[p].title);
                for (const std::string& l : pages_[p].lines) all += "\n" + lower(l);
                bool every = true;
                for (const std::string& w : want) every = every && all.find(w) != std::string::npos;
                if (!every) continue;
                rows_.push_back({pages_[p].title, int(p), 0});
                for (const std::string& l : pages_[p].lines)
                    if (lower(l).find(want[0]) != std::string::npos)
                    {
                        rows_.push_back({trim(with_keys(l)), int(p), 1});
                        break;
                    }
            }
    }
    row_ = 0;
    list_top_ = 0;
    while (row_ < int(rows_.size()) && rows_[size_t(row_)].page < 0) ++row_;
    if (row_ >= int(rows_.size())) row_ = rows_.empty() ? 0 : int(rows_.size()) - 1;
}

void HelpLibrary::open(int page, bool remember)
{
    if (page < 0 || page >= int(pages_.size())) return;
    if (remember && page_ >= 0 && page != page_) history_.push_back(page_);
    page_ = page;
    scroll_ = 0;
    link_ = -1;
    laid_page_ = -1; // (laid out again when next drawn)
}

// The keys a line names, as they are set.
std::string HelpLibrary::with_keys(const std::string& line) const { return Keys::get().keyed(line); }

// The page laid out `cells` characters wide: paragraphs joined and wrapped at spaces, "## " a heading,
// "- " a list item and "1. " a numbered step (their lines after the first indented under their words), links shown as their words
// (or the page's title) and kept with the columns they take, a link wrapped over two lines still one.
void HelpLibrary::lay_out(size_t cells)
{
    laid_.clear();
    link_targets_.clear();
    laid_cells_ = cells;
    laid_page_ = page_;
    if (page_ < 0) return;
    const Page& pg = pages_[size_t(page_)];
    auto emit = [&](const std::string& source, size_t hang, bool heading) {
        // Its characters, and for each the link it is part of (-1: none).
        std::u32string s;
        std::vector<int> of;
        const std::string line = with_keys(source);
        for (size_t at = 0; at < line.size();)
        {
            const size_t open_at = line.find("[[", at);
            const size_t close_at = open_at == std::string::npos ? std::string::npos : line.find("]]", open_at + 2);
            const std::u32string before = sieve::utf8_decode(line.substr(at, open_at == std::string::npos || close_at == std::string::npos ? std::string::npos : open_at - at));
            s += before;
            of.insert(of.end(), before.size(), -1);
            if (open_at == std::string::npos || close_at == std::string::npos) break;
            const std::string inner = line.substr(open_at + 2, close_at - open_at - 2);
            const size_t bar = inner.find('|');
            const std::string id = trim(inner.substr(0, bar));
            const int p = find_page(id);
            const std::u32string shown = sieve::utf8_decode(bar != std::string::npos ? inner.substr(bar + 1) : p >= 0 ? pages_[size_t(p)].title : id);
            s += shown;
            of.insert(of.end(), shown.size(), p >= 0 ? int(link_targets_.size()) : -1);
            if (p >= 0) link_targets_.push_back(p);
            at = close_at + 2;
        }
        // Wrapped at spaces; a word longer than a line is cut.
        size_t at = 0;
        for (bool first = true; at < s.size() || first; first = false)
        {
            const size_t indent = first ? 0 : hang, room = cells > indent + 1 ? cells - indent : 1;
            size_t end = std::min(s.size(), at + room);
            if (end < s.size())
            {
                const size_t sp = s.rfind(U' ', end);
                if (sp != std::u32string::npos && sp > at) end = sp;
            }
            Laid l;
            l.heading = heading;
            l.text = std::string(indent, ' ') + sieve::utf8_encode(s.substr(at, end - at));
            for (size_t i = at; i < end;)
            {
                size_t j = i + 1;
                while (j < end && of[j] == of[i]) ++j;
                if (of[i] >= 0) l.links.push_back({indent + i - at, indent + j - at, of[i]});
                i = j;
            }
            laid_.push_back(std::move(l));
            at = end;
            while (at < s.size() && s[at] == U' ') ++at;
        }
    };
    std::string para;
    auto flush = [&] {
        if (!para.empty()) emit(para, 0, false);
        para.clear();
    };
    for (const std::string& raw : pg.lines)
    {
        const std::string l = trim(raw);
        if (l.empty())
        {
            flush();
            if (!laid_.empty() && !laid_.back().text.empty()) laid_.push_back({});
        }
        else if (l.rfind("## ", 0) == 0)
        {
            flush();
            emit(l.substr(3), 0, true);
        }
        else if (l.rfind("- ", 0) == 0)
        {
            flush();
            emit(l, 2, false);
        }
        else if (const size_t dot = l.find(". "); dot != std::string::npos && dot > 0 && dot <= 3 &&
                                                  std::all_of(l.begin(), l.begin() + std::ptrdiff_t(dot), [](char c) { return c >= '0' && c <= '9'; }))
        {
            // A numbered step, its lines after the first under its words.
            flush();
            emit(l, dot + 2, false);
        }
        else para += (para.empty() ? "" : " ") + l;
    }
    flush();
    while (!laid_.empty() && laid_.back().text.empty()) laid_.pop_back();
}

// Up or Down on the list: the next row with a page, the headings passed over.
void HelpLibrary::choose_row(int dir)
{
    if (rows_.empty())
    {
        row_ = 0; // (Home on an empty list left it at -1, which Enter then read; review 4)
        return;
    }
    int r = row_;
    for (size_t n = 0; n < rows_.size(); ++n)
    {
        r = std::clamp(r + dir, 0, int(rows_.size()) - 1);
        if (rows_[size_t(r)].page >= 0)
        {
            row_ = r;
            return;
        }
        if (r == 0 || r == int(rows_.size()) - 1) return;
    }
}

// Tab on the page: the next link (or the one before), the page scrolled to show it.
void HelpLibrary::step_link(int dir)
{
    const int n = int(link_targets_.size());
    if (n == 0) return;
    link_ = link_ < 0 ? (dir > 0 ? 0 : n - 1) : (link_ + dir + n) % n;
    for (size_t i = 0; i < laid_.size(); ++i)
        for (const Link& k : laid_[i].links)
            if (k.number == link_)
            {
                if (i < scroll_) scroll_ = i;
                if (i >= scroll_ + page_rows_) scroll_ = i + 1 - page_rows_;
                return;
            }
}

void HelpLibrary::key(SDL_Keycode k, SDL_Keymod mod)
{
    namespace mk = menu_keys;
    namespace hk = help_keys;
    // Typing a search: Backspace takes a letter back before it goes back a page.
    const bool typing = tab_ == Tab::Search && !on_page_;
    if (typing && k == SDLK_BACKSPACE && !query_.empty())
    {
        pop_char(query_);
        build_rows();
        return;
    }
    const std::string a = Keys::get().which({mk::kBack, mk::kUp, mk::kDown, mk::kLeft, mk::kRight, mk::kChoose, mk::kPageUp, mk::kPageDown,
                                             mk::kNext, hk::kPrevious, hk::kSearch, hk::kHistory, hk::kFirst, hk::kLast},
                                            k, mod);
    const size_t last_line = laid_.size() > page_rows_ ? laid_.size() - page_rows_ : 0;
    if (a == mk::kBack) done_ = true;
    else if (a == hk::kSearch)
    {
        tab_ = Tab::Search;
        on_page_ = false;
        build_rows();
    }
    else if (a == hk::kHistory)
    {
        if (history_.empty()) return;
        const int back = history_.back();
        history_.pop_back();
        open(back, false);
    }
    else if (a == mk::kNext || a == hk::kPrevious)
    {
        const int dir = a == mk::kNext ? 1 : -1;
        if (on_page_) step_link(dir);
        else
        {
            tab_ = Tab((int(tab_) + 3 + dir) % 3);
            build_rows();
        }
    }
    else if (a == mk::kLeft) on_page_ = false;
    else if (a == mk::kRight)
    {
        if (!on_page_ && row_ >= 0 && row_ < int(rows_.size()) && rows_[size_t(row_)].page >= 0) open(rows_[size_t(row_)].page, true);
        on_page_ = page_ >= 0;
    }
    else if (a == mk::kChoose)
    {
        if (on_page_)
        {
            if (link_ >= 0 && link_ < int(link_targets_.size())) open(link_targets_[size_t(link_)], true);
        }
        else if (row_ >= 0 && row_ < int(rows_.size()) && rows_[size_t(row_)].page >= 0)
        {
            open(rows_[size_t(row_)].page, true);
            on_page_ = true;
        }
    }
    else if (a == mk::kUp || a == mk::kDown)
    {
        const int dir = a == mk::kDown ? 1 : -1;
        if (on_page_) scroll_ = size_t(std::clamp<long long>((long long)scroll_ + dir, 0, (long long)last_line));
        else choose_row(dir);
    }
    else if (a == mk::kPageUp || a == mk::kPageDown)
    {
        const long long step = (long long)std::max<size_t>(1, page_rows_ - 1) * (a == mk::kPageDown ? 1 : -1);
        scroll_ = size_t(std::clamp<long long>((long long)scroll_ + step, 0, (long long)last_line));
    }
    else if (a == hk::kFirst || a == hk::kLast)
    {
        const bool to_end = a == hk::kLast;
        if (on_page_) scroll_ = to_end ? last_line : 0;
        else
        {
            row_ = to_end ? int(rows_.size()) : -1;
            choose_row(to_end ? -1 : 1);
        }
    }
}

void HelpLibrary::press(SDL_Keycode k, SDL_Keymod mod) { key(k, mod); }

// Typed letters are the search, from anywhere on the list (the SEARCH tab shown); on the page they are not.
void HelpLibrary::type(const std::string& utf8)
{
    if (on_page_) return;
    tab_ = Tab::Search;
    query_ += utf8;
    build_rows();
}

void HelpLibrary::handle(const SDL_Event& event)
{
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT)
    {
        result_ = Result::Quit;
        done_ = true;
    }
    else if (e.type == SDL_EVENT_TEXT_INPUT) type(e.text.text);
    else if (e.type == SDL_EVENT_KEY_DOWN) key(e.key.key, e.key.mod);
    else if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        const long long last = laid_.size() > page_rows_ ? (long long)(laid_.size() - page_rows_) : 0;
        scroll_ = size_t(std::clamp<long long>((long long)scroll_ - (long long)(e.wheel.y * 3), 0, last));
    }
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (const auto& [box, t] : tab_boxes_)
            if (inside(box, e.button.x, e.button.y))
            {
                tab_ = Tab(t);
                on_page_ = false;
                build_rows();
                return;
            }
        for (const auto& [box, r] : row_boxes_)
            if (inside(box, e.button.x, e.button.y) && size_t(r) < rows_.size() && rows_[size_t(r)].page >= 0) // (boxes from the last frame drawn)
            {
                row_ = r;
                open(rows_[size_t(r)].page, true);
                return;
            }
        for (const auto& [box, n] : link_boxes_)
            if (inside(box, e.button.x, e.button.y))
            {
                on_page_ = true;
                open(link_targets_[size_t(n)], true);
                return;
            }
    }
}

HelpLibrary::Result HelpLibrary::run()
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
        render();
        present(r_);
    }
    SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void HelpLibrary::render()
{
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

    const std::vector<FooterLine> keys{{trk("docs.footer"), kGrey}};
    const float keys_up = footer_extra(keys, W - 2 * kMargin, 16);
    const float bottom = H - 48 - keys_up; // the foot of the list and the page
    txt(r_, kMargin, 16, tr("docs.title"), kWhite, 2.5f);
    txt(r_, kMargin, 44, tr("docs.subtitle"), kGrey);

    // The tabs over the list: CONTENTS, INDEX, SEARCH.
    tab_boxes_.clear();
    {
        const char* names[3] = {"docs.tab.contents", "docs.tab.index", "docs.tab.search"};
        float bx = kMargin;
        for (int t = 0; t < 3; ++t)
        {
            const std::string label = tr(names[t]);
            const SDL_FRect box{bx, 64, text_width(label, 1) + 20, 20};
            const bool lit = int(tab_) == t;
            SDL_SetRenderDrawColor(r_, kAccent.r, kAccent.g, kAccent.b, lit ? 90 : 0);
            if (lit) SDL_RenderFillRect(r_, &box);
            SDL_SetRenderDrawColor(r_, kAccent.r, kAccent.g, kAccent.b, 255);
            SDL_RenderRect(r_, &box);
            txt(r_, box.x + 10, box.y + 6, label, lit ? kWhite : kGrey);
            tab_boxes_.emplace_back(box, t);
            bx += box.w + 6;
        }
    }

    // The list: the search typed first where it is shown, then the rows, scrolled to keep the one
    // chosen in sight. Its frame lit while the keys move on it.
    const size_t list_cells = size_t((kListW - 16) / 8);
    float y = kTop;
    if (tab_ == Tab::Search)
    {
        txt(r_, kMargin + 6, y, fit_cells(trf("docs.search", {query_}) + (on_page_ ? "" : "_"), list_cells), kWhite);
        y += kLine + 6;
        if (rows_.empty())
            txt(r_, kMargin + 6, y, fit_cells(tr(query_.empty() ? "docs.search.type" : "docs.search.none"), list_cells), kDim);
    }
    {
        const SDL_FRect frame{kMargin - 4, kTop - 6, kListW, bottom - kTop + 6};
        const SDL_Color c = on_page_ ? kDim : kAccent;
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        SDL_RenderRect(r_, &frame);
    }
    const size_t fit = size_t(std::max(1.0f, (bottom - y) / kLine));
    if (row_ >= 0 && size_t(row_) < list_top_) list_top_ = size_t(row_);
    if (row_ >= 0 && size_t(row_) >= list_top_ + fit) list_top_ = size_t(row_) + 1 - fit;
    row_boxes_.clear();
    for (size_t i = list_top_; i < rows_.size() && i < list_top_ + fit; ++i)
    {
        const Row& rw = rows_[i];
        const SDL_FRect box{kMargin, y - 2, kListW - 8, kLine};
        const bool chosen = int(i) == row_ && rw.page >= 0;
        if (chosen)
        {
            SDL_SetRenderDrawColor(r_, 40, 60, 80, 255);
            SDL_RenderFillRect(r_, &box);
        }
        const SDL_Color c = rw.page < 0 ? kAccent : rw.page == page_ ? kWhite : kGrey;
        txt(r_, kMargin + 6 + 12 * float(rw.indent), y, fit_cells(rw.text, list_cells - 2 * size_t(rw.indent)), c);
        row_boxes_.emplace_back(box, int(i));
        y += kLine;
    }

    // The page: its title, then its lines from where it is scrolled, its links lit (the one chosen
    // boxed); a bar at the right shows where in the page you are.
    const float px = kMargin + kListW + 20, pw = W - px - kMargin;
    const size_t cells = size_t(std::max(20.0f, (pw - 12) / 8));
    if (laid_page_ != page_ || laid_cells_ != cells) lay_out(cells);
    {
        const SDL_FRect frame{px - 8, kTop - 6, pw + 8, bottom - kTop + 6};
        const SDL_Color c = on_page_ ? kAccent : kDim;
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        SDL_RenderRect(r_, &frame);
    }
    link_boxes_.clear();
    if (page_ >= 0)
    {
        const Page& pg = pages_[size_t(page_)];
        std::string where;
        for (const Category& c : categories_)
            if (c.id == pg.category) where = c.title;
        txt(r_, px, kTop, fit_cells(pg.title, size_t(pw / 16)), kWhite, 2);
        if (!where.empty()) txt(r_, px, kTop + 22, fit_cells(where, cells), kDim);
        float ly = kTop + 42;
        page_rows_ = size_t(std::max(1.0f, (bottom - ly) / kLine));
        scroll_ = std::min(scroll_, laid_.size() > page_rows_ ? laid_.size() - page_rows_ : size_t(0));
        for (size_t i = scroll_; i < laid_.size() && i < scroll_ + page_rows_; ++i)
        {
            const Laid& l = laid_[i];
            txt(r_, px, ly, l.text, l.heading ? kAccent : kWhite);
            for (const Link& k : l.links)
            {
                // (the link's words drawn over in its colour, underlined; the one chosen boxed)
                const std::u32string all = sieve::utf8_decode(l.text);
                const std::string words = sieve::utf8_encode(all.substr(k.from, k.to - k.from));
                const float lx = px + 8 * float(k.from), lw = 8 * float(k.to - k.from);
                txt(r_, lx, ly, words, kLinkColour);
                SDL_SetRenderDrawColor(r_, kLinkColour.r, kLinkColour.g, kLinkColour.b, 255);
                SDL_RenderLine(r_, lx, ly + 10, lx + lw, ly + 10);
                const SDL_FRect box{lx - 2, ly - 3, lw + 4, kLine};
                if (k.number == link_ && on_page_) SDL_RenderRect(r_, &box);
                link_boxes_.emplace_back(box, k.number);
            }
            ly += kLine;
        }
        if (laid_.size() > page_rows_)
        {
            const float track = bottom - (kTop + 42);
            const SDL_FRect bar{px + pw - 4, kTop + 42 + track * float(scroll_) / float(laid_.size()), 3,
                                std::max(6.0f, track * float(page_rows_) / float(laid_.size()))};
            SDL_SetRenderDrawColor(r_, kDim.r, kDim.g, kDim.b, 255);
            SDL_RenderFillRect(r_, &bar);
        }
    }
    else txt(r_, px, kTop, fit_cells(tr("docs.empty"), cells), kDim);

    txt(r_, kMargin, H - 38 - keys_up, fit_cells(status_.empty() ? summary() : status_, size_t((W - 2 * kMargin) / 8)), status_.empty() ? kDim : kAccent);
    draw_footer(r_, "docs", kMargin, H - 24, W - 2 * kMargin, keys, 16);
}

// For scripted runs and the line above the keys: where you are.
std::string HelpLibrary::summary() const
{
    const char* tabs[3] = {"docs.tab.contents", "docs.tab.index", "docs.tab.search"};
    return trf("docs.summary", {page_id(), std::to_string(pages_.size()), std::to_string(categories_.size()), tr(tabs[int(tab_)]),
                                std::to_string(rows_.size()), std::to_string(link_targets_.size()), std::to_string(history_.size())});
}

std::string HelpLibrary::list_text() const
{
    std::string out;
    for (const Row& r : rows_) out += std::string(size_t(2 * r.indent), ' ') + r.text + "\n";
    return out;
}

std::string HelpLibrary::page_text() const
{
    std::string out;
    for (const Laid& l : laid_)
    {
        std::u32string t = sieve::utf8_decode(l.text);
        // (the links bracketed, from the last on the line back, so the columns stay true)
        for (auto it = l.links.rbegin(); it != l.links.rend(); ++it)
        {
            t.insert(it->to, U"]");
            t.insert(it->from, U"[");
        }
        out += sieve::utf8_encode(t) + "\n";
    }
    return out;
}

} // namespace hallway
