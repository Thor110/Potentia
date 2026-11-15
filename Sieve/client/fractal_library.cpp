// Sieve hallway — the Fractal Dimension (fractal_library.hpp; IDEAS 19.191, 19.235).
#include "fractal_library.hpp"

#include "font.hpp"
#include "footer.hpp"
#include "gpu_memory.hpp"
#include "keys.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/dictionaries.hpp" // find_data_folder

#include <algorithm>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace hallway {

namespace {

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kPurple{205, 180, 255, 255};
constexpr int kMinW = 1024, kMinH = 640;
constexpr float kMargin = 24, kListW = 300, kTop = 96, kLine = 14;
// The deepest drawn on the screen: a word past this many symbols takes too long to draw a frame (a
// limit chosen, not measured; the command line draws any depth the ruleset reaches).
constexpr size_t kDrawnMost = size_t(1) << 18;

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }
bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

// The deepest a fractal is drawn here: within its own limit, and within kDrawnMost symbols.
uint32_t drawn_deepest(const sieve::Fractal& f)
{
    uint32_t d = 0;
    while (d < f.deepest())
    {
        const auto w = f.grow(d + 1);
        if (!w || w->size() > kDrawnMost) break;
        ++d;
    }
    return d;
}

} // namespace

std::filesystem::path FractalLibrary::folder() { return sieve::cli::find_data_folder("fractals", "chart.txt"); }

FractalLibrary::FractalLibrary(SDL_Window* window, SDL_Renderer* renderer, const std::filesystem::path& folder, const std::string& id)
    : window_(window), r_(renderer)
{
    try
    {
        chart_ = sieve::read_fractal_chart(folder);
    }
    catch (const std::exception& e)
    {
        status_ = trf("fractals.unreadable", {e.what()});
    }
    for (const auto& c : chart_) depth_.push_back(std::min<uint32_t>(drawn_deepest(c.fractal), 5));
    for (size_t i = 0; i < chart_.size(); ++i)
        if (chart_[i].id == id) chosen_ = int(i);
    if (!id.empty() && (chart_.empty() || chart_[size_t(chosen_)].id != id)) status_ = trf("fractals.no_such", {id});
}

FractalLibrary::~FractalLibrary()
{
    if (tex_) SDL_DestroyTexture(tex_);
}

FractalLibrary::Result FractalLibrary::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    music_mode(MusicMode::Menus);
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
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void FractalLibrary::handle(const SDL_Event& event)
{
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT) result_ = Result::Quit, done_ = true;
    else if (e.type == SDL_EVENT_KEY_DOWN) key(e.key.key, e.key.mod);
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
        for (const auto& [box, i] : row_boxes_)
            if (inside(box, e.button.x, e.button.y)) choose(i);
}

void FractalLibrary::press(SDL_Keycode key_code, SDL_Keymod mod) { key(key_code, mod); }

void FractalLibrary::choose(int i)
{
    if (chart_.empty()) return;
    chosen_ = std::clamp(i, 0, int(chart_.size()) - 1);
    status_.clear();
}

void FractalLibrary::key(SDL_Keycode k, SDL_Keymod mod)
{
    namespace mk = menu_keys;
    namespace fk = fractal_keys;
    const std::string a = Keys::get().which({mk::kBack, mk::kUp, mk::kDown, mk::kLeft, mk::kRight, fk::kDeeper, fk::kShallower, fk::kFirst, fk::kDeepest}, k, mod);
    if (a == mk::kBack)
    {
        done_ = true;
        return;
    }
    if (chart_.empty()) return;
    uint32_t& d = depth_[size_t(chosen_)];
    const uint32_t most = drawn_deepest(chart_[size_t(chosen_)].fractal);
    if (a == mk::kUp) choose(chosen_ - 1);
    else if (a == mk::kDown) choose(chosen_ + 1);
    else if (a == mk::kRight || a == fk::kDeeper)
    {
        if (d < most) ++d, status_.clear();
        else status_ = trf("fractals.deepest", {std::to_string(most)});
    }
    else if (a == mk::kLeft || a == fk::kShallower) d = d > 0 ? d - 1 : 0, status_.clear();
    else if (a == fk::kFirst) d = 0, status_.clear();
    else if (a == fk::kDeepest) d = most, status_.clear();
}

std::string FractalLibrary::summary() const
{
    if (chart_.empty()) return trf("fractals.summary", {"-", "0", "0", "0"});
    const auto& c = chart_[size_t(chosen_)];
    const auto w = c.fractal.grow(depth_[size_t(chosen_)]);
    return trf("fractals.summary", {c.id, std::to_string(depth_[size_t(chosen_)]), std::to_string(w ? w->size() : 0), std::to_string(chart_.size())});
}

void FractalLibrary::render()
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW), h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    const std::vector<FooterLine> keys{{trk("fractals.footer"), kGrey}};
    const float keys_up = footer_extra(keys, W - 2 * kMargin, 16);
    const float bottom = H - 48 - keys_up;
    txt(r_, kMargin, 16, tr("fractals.title"), kPurple, 2.5f);
    txt(r_, kMargin, 44, fit_cells(tr("fractals.subtitle"), size_t((W - 2 * kMargin) / 8)), kGrey);

    // The shelves: each charted fractal, its family beneath its name.
    row_boxes_.clear();
    txt(r_, kMargin, kTop - 22, trf("fractals.shelves", {std::to_string(chart_.size())}), kDim);
    float y = kTop;
    for (size_t i = 0; i < chart_.size() && y + 2 * kLine < bottom; ++i)
    {
        const auto& c = chart_[i];
        const SDL_FRect box{kMargin - 4, y - 3, kListW, 2 * kLine + 4};
        if (int(i) == chosen_)
        {
            SDL_SetRenderDrawColor(r_, kPurple.r, kPurple.g, kPurple.b, 60);
            SDL_RenderFillRect(r_, &box);
        }
        row_boxes_.emplace_back(box, int(i));
        txt(r_, kMargin, y, fit_cells(c.title, size_t(kListW / 8) - 1), int(i) == chosen_ ? kWhite : kGrey);
        txt(r_, kMargin + 16, y + kLine, tr(c.fractal.family == sieve::Fractal::Family::Substitution ? "fractals.substitution" : "fractals.lsystem"), kDim);
        y += 2 * kLine + 8;
    }

    // The one chosen: drawn, then its ruleset and depth beneath.
    if (!chart_.empty())
    {
        const auto& c = chart_[size_t(chosen_)];
        const uint32_t d = depth_[size_t(chosen_)];
        const float px = kMargin + kListW + 24, pw = W - px - kMargin;
        const float text_rows = 4 + float(std::count(c.text.begin(), c.text.end(), '\n'));
        const uint32_t size = uint32_t(std::max(64.0f, std::min(pw, bottom - kTop - text_rows * kLine - 30)));
        if (!tex_ || tex_for_ != chosen_ || tex_depth_ != d || tex_size_ != size)
        {
            if (tex_) SDL_DestroyTexture(tex_), tex_ = nullptr;
            if (const auto img = sieve::draw_fractal(c.fractal, d, size, size))
            {
                tex_ = gpu::create(r_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, int(img->width), int(img->height));
                if (tex_) SDL_UpdateTexture(tex_, nullptr, img->rgba.data(), int(img->width) * 4);
            }
            tex_for_ = chosen_, tex_depth_ = d, tex_size_ = size;
        }
        txt(r_, px, kTop - 22, fit_cells(c.title, size_t(pw / 8)), kWhite);
        if (tex_)
        {
            const SDL_FRect at{px, kTop, float(size), float(size)};
            SDL_RenderTexture(r_, tex_, nullptr, &at);
        }
        float ty = kTop + float(size) + 12;
        const auto word = c.fractal.grow(d);
        txt(r_, px, ty, trf("fractals.depth", {std::to_string(d), std::to_string(drawn_deepest(c.fractal)), std::to_string(word ? word->size() : 0)}), kPurple);
        ty += kLine + 4;
        size_t at = 0;
        while (at < c.text.size() && ty < bottom)
        {
            const size_t lf = c.text.find('\n', at);
            txt(r_, px, ty, fit_cells(c.text.substr(at, lf - at), size_t(pw / 8)), kDim);
            ty += kLine;
            at = lf + 1;
        }
    }
    else txt(r_, kMargin + kListW + 24, kTop, tr("fractals.empty"), kDim);

    txt(r_, kMargin, H - 38 - keys_up, fit_cells(status_.empty() ? summary() : status_, size_t((W - 2 * kMargin) / 8)), status_.empty() ? kDim : kPurple);
    draw_footer(r_, "fractals", kMargin, H - 24, W - 2 * kMargin, keys, 16);
}

} // namespace hallway
