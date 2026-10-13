// Sieve hallway — the Encryption Engine's screen (cipher_screen.hpp).

#include "cipher_screen.hpp"

#include "font.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "sieve/cipher.hpp"

#include <algorithm>
#include <stdexcept>

namespace hallway {

namespace {

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kGood{120, 230, 120, 255},
    kBad{255, 110, 110, 255}, kAccent{120, 200, 255, 255};

constexpr float kLine = 14;    // one line of text, at scale 1 (8-pixel cells and a gap)
constexpr int kMinW = 1024, kMinH = 640;
constexpr size_t kBreakTop = 5;  // the guesses shown, as `sieve cipher --break` shows them
constexpr size_t kPreview = 48;  // the bytes of a guess shown beside it

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

std::string hex_of(const std::vector<uint8_t>& b)
{
    static const char k[] = "0123456789abcdef";
    std::string s;
    for (const uint8_t c : b) s += {k[c >> 4], k[c & 15]};
    return s;
}

int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Hex as bytes; spaces are skipped, so hex can be pasted in groups.
std::vector<uint8_t> bytes_of_hex(const std::string& h)
{
    std::string d;
    for (const char c : h)
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') d += c;
    if (d.size() % 2) throw std::invalid_argument(tr("cipher.bad_hex_odd"));
    std::vector<uint8_t> b;
    for (size_t i = 0; i < d.size(); i += 2)
    {
        const int hi = hex_digit(d[i]), lo = hex_digit(d[i + 1]);
        if (hi < 0 || lo < 0) throw std::invalid_argument(tr("cipher.bad_hex"));
        b.push_back(uint8_t(hi * 16 + lo));
    }
    return b;
}

// Bytes as text when every one is printable ASCII (a newline or tab shown as \n or \t).
bool printable(const std::vector<uint8_t>& b, size_t n, std::string& out)
{
    out.clear();
    for (size_t i = 0; i < std::min(n, b.size()); ++i)
    {
        const uint8_t c = b[i];
        if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c >= 0x20 && c < 0x7F) out += char(c);
        else return false;
    }
    return true;
}

// A long string cut into lines of `cells` characters.
std::vector<std::string> wrap(const std::string& s, size_t cells)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size(); i += cells) out.push_back(s.substr(i, cells));
    if (out.empty()) out.push_back("");
    return out;
}

// The last UTF-8 character of s removed.
void pop_char(std::string& s)
{
    while (!s.empty() && (uint8_t(s.back()) & 0xC0) == 0x80) s.pop_back();
    if (!s.empty()) s.pop_back();
}

} // namespace

CipherScreen::CipherScreen(SDL_Window* window, SDL_Renderer* renderer) : window_(window), r_(renderer) { run_recipe(); }

std::string CipherScreen::output_hex() const { return have_out_ ? hex_of(out_) : std::string(); }

CipherScreen::Result CipherScreen::run()
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

void CipherScreen::press(SDL_Keycode k, SDL_Keymod mod) { key(k, mod); }

std::string* CipherScreen::text_row()
{
    if (row_ == kInput) return &input_;
    if (row_ == kRecipe) return &recipe_;
    return nullptr;
}

void CipherScreen::type(const std::string& utf8)
{
    if (std::string* t = text_row()) *t += utf8;
}

void CipherScreen::handle(const SDL_Event& event)
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
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        for (int i = 0; i < int(row_rects_.size()); ++i)
            if (inside(row_rects_[size_t(i)], e.button.x, e.button.y))
            {
                if (row_ == i && (i == kInputKind || i == kAction)) key(e.button.button == SDL_BUTTON_LEFT ? SDLK_RIGHT : SDLK_LEFT, SDL_KMOD_NONE);
                row_ = i;
            }
    }
}

void CipherScreen::key(SDL_Keycode k, SDL_Keymod mod)
{
    const bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
    if (k == SDLK_ESCAPE)
    {
        done_ = true;
        result_ = Result::Back;
    }
    else if (k == SDLK_UP) row_ = (row_ + kRows - 1) % kRows;
    else if (k == SDLK_DOWN) row_ = (row_ + 1) % kRows;
    else if (k == SDLK_LEFT || k == SDLK_RIGHT)
    {
        const int dir = k == SDLK_RIGHT ? 1 : -1;
        if (row_ == kInputKind) input_hex_ = !input_hex_;
        else if (row_ == kAction) action_ = Action((int(action_) + 3 + dir) % 3);
        else return;
        run_recipe();
    }
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) run_recipe();
    else if (k == SDLK_TAB)
    {
        // The output in as the input, in hex: decrypt what was encrypted, or go another round.
        if (!have_out_) return;
        input_ = hex_of(out_);
        input_hex_ = true;
        if (action_ == Action::Encrypt) action_ = Action::Decrypt;
        else if (action_ == Action::Decrypt) action_ = Action::Encrypt;
        run_recipe();
    }
    else if (k == SDLK_BACKSPACE)
    {
        if (std::string* t = text_row())
        {
            if (ctrl) t->clear();
            else pop_char(*t);
        }
    }
    else if (ctrl && k == SDLK_V)
    {
        if (std::string* t = text_row())
            if (char* clip = SDL_GetClipboardText())
            {
                *t += clip;
                SDL_free(clip);
            }
    }
    else if (ctrl && k == SDLK_C)
    {
        if (have_out_) SDL_SetClipboardText(hex_of(out_).c_str());
    }
}

void CipherScreen::run_recipe()
{
    lines_.clear();
    have_out_ = false;
    out_.clear();
    try
    {
        const std::vector<uint8_t> in = input_hex_ ? bytes_of_hex(input_) : std::vector<uint8_t>(input_.begin(), input_.end());
        if (action_ == Action::Break)
        {
            const auto guesses = sieve::cipher::break_weak(in, 16, kBreakTop);
            lines_.push_back(trf("cipher.break_head", {std::to_string(in.size()), std::to_string(sieve::cipher::english_score(in))}));
            for (size_t i = 0; i < guesses.size(); ++i)
            {
                std::string shown;
                if (!printable(guesses[i].plain, kPreview, shown)) shown = hex_of(std::vector<uint8_t>(guesses[i].plain.begin(),
                                                                                guesses[i].plain.begin() + std::ptrdiff_t(std::min(guesses[i].plain.size(), kPreview / 2))));
                else shown = "\"" + shown + "\"";
                lines_.push_back(trf("cipher.guess", {std::to_string(i + 1), guesses[i].recipe.layers.front().text(),
                                                       std::to_string(guesses[i].rank), shown}));
            }
            const bool none = guesses.empty() || guesses.front().rank <= 0;
            status_ = none ? tr("cipher.break_none") : trf("cipher.break_best", {guesses.front().recipe.layers.front().text()});
            return;
        }
        const sieve::cipher::Recipe recipe = sieve::cipher::Recipe::parse(recipe_);
        out_ = action_ == Action::Encrypt ? sieve::cipher::encrypt(recipe, in) : sieve::cipher::decrypt(recipe, in);
        have_out_ = true;
        std::string layers;
        for (const auto& l : recipe.layers) layers += (layers.empty() ? "" : "; ") + l.text();
        lines_.push_back(trf("cipher.layers", {layers}));
        lines_.push_back(trf(action_ == Action::Encrypt ? "cipher.encrypted" : "cipher.decrypted", {std::to_string(out_.size())}));
        status_ = tr("cipher.ok");
    }
    catch (const std::exception& ex)
    {
        status_ = ex.what();
        have_out_ = false;
    }
}

void CipherScreen::render()
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

    const float x = 24, label_w = 150;
    const size_t cells = size_t(std::max(20.0f, (W - x - label_w - 24) / 8));
    txt(r_, x, 16, tr("cipher.title"), kWhite, 2.5f);
    txt(r_, x, 44, tr("cipher.subtitle"), kGrey);

    // The rows.
    const char* labels[kRows] = {"cipher.row.input", "cipher.row.input_kind", "cipher.row.recipe", "cipher.row.action"};
    const char* actions[3] = {"cipher.action.encrypt", "cipher.action.decrypt", "cipher.action.break"};
    float y = 76;
    row_rects_.assign(kRows, SDL_FRect{});
    for (int i = 0; i < kRows; ++i)
    {
        std::vector<std::string> value;
        if (i == kInput) value = wrap(input_, cells);
        else if (i == kRecipe) value = wrap(recipe_, cells);
        else if (i == kInputKind) value = {"< " + tr(input_hex_ ? "cipher.kind.hex" : "cipher.kind.text") + " >"};
        else value = {"< " + tr(actions[int(action_)]) + " >"};
        const bool chosen = row_ == i;
        if (chosen && (i == kInput || i == kRecipe)) value.back() += "_"; // the caret, where typing goes
        const float rh = kLine * float(value.size()) + 6;
        row_rects_[size_t(i)] = {x - 6, y - 3, W - 2 * x + 12, rh};
        if (chosen)
        {
            SDL_SetRenderDrawColor(r_, 40, 60, 80, 255);
            SDL_RenderFillRect(r_, &row_rects_[size_t(i)]);
        }
        txt(r_, x, y, tr(labels[i]), chosen ? kAccent : kGrey);
        for (size_t l = 0; l < value.size(); ++l) txt(r_, x + label_w, y + kLine * float(l), value[l], kWhite);
        y += rh + 6;
    }

    // The output.
    y += 8;
    txt(r_, x, y, tr("cipher.output"), kGrey);
    y += kLine + 4;
    for (const std::string& l : lines_)
        for (const std::string& part : wrap(l, cells + 18))
        {
            txt(r_, x, y, part, kWhite);
            y += kLine;
        }
    if (have_out_)
    {
        y += 4;
        txt(r_, x, y, tr("cipher.hex"), kGrey);
        // As many lines of hex as fit above the footer; the rest is marked, and Ctrl+C copies all.
        const auto hex_lines = wrap(hex_of(out_), cells);
        const size_t room = size_t(std::max(1.0f, (H - 120 - y) / kLine));
        for (size_t l = 0; l < hex_lines.size() && l < room; ++l)
            txt(r_, x + label_w, y + kLine * float(l), hex_lines[l] + (l + 1 == room && hex_lines.size() > room ? " .." : ""), kWhite);
        y += kLine * float(std::min(room, hex_lines.size())) + 4;
        std::string as_text;
        if (printable(out_, out_.size(), as_text))
        {
            txt(r_, x, y, tr("cipher.as_text"), kGrey);
            txt(r_, x + label_w, y, "\"" + fit_cells(as_text, cells - 2) + "\"", kGood);
        }
        else txt(r_, x, y, tr("cipher.not_text"), kDim);
    }

    const bool bad = !have_out_ && action_ != Action::Break;
    txt(r_, x, H - 58, fit_cells(status_, size_t((W - 2 * x) / 8)), bad ? kBad : kGood);
    txt(r_, x, H - 40, tr("cipher.footer"), kGrey);
    txt(r_, x, H - 24, tr("cipher.footer2"), kDim);
}

} // namespace hallway
