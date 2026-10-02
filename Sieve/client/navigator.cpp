// The address navigator: the whole of an address on one screen, one hex digit at a time.
//
// Walking, the wheel and PgUp/PgDn and [ ] move by tiles, which reaches the right-hand end of
// an address and nothing else: a pages line of 32 characters has addresses of 70-odd hex digits,
// and even a million tiles a press changes only the last few. The navigator opens on the address
// of the item you are looking at (the first item of the tile when you are looking at none), lays
// all of its digits out in rows, and lets you pick any digit and turn it. Turning digit p moves
// 16^p units along the line, carrying into the digits to its left as counting does, and wrapping
// round the end of the line to its start, as the corridor itself does. ENTER goes there.
//
// The number being turned is the item's position in the corridor's loop -- its address in
// positional and scrambled order, its compact address in compact order, its point at the current
// zoom in guided order -- which is the number place() takes, so the navigator needs no parsing
// of its own and works in every ordering. On the binary line it is the file's place on the line,
// which place() turns into its place on the one wall.
//
// At the foot, the same position as a bearing round the loop, written to the setup menu's Angle
// Precision, can be typed instead (Tab, or a click on it): 0 degrees is where the loop starts, as
// on the compass. A bearing A with d decimal places names the first unit at or past it,
// ceil(A * units / (360 * 10^d)), worked out exactly; ENTER goes there as it does from the digits.
// It is a way of choosing where to look, not a shorter address: the bearing carries only the
// leading part of the position (docs/IDEAS.md §3.6).

#include "hallway.hpp"

#include <algorithm>

namespace hallway::hall {

namespace {

constexpr float kCellW = 18;   // one digit, drawn at scale 2
constexpr float kGroupGap = 8; // after every fourth digit
constexpr float kRowH = 52;    // a row of digits with room for an arrow above and below
constexpr float kLabelW = 80;  // the digit-number labels at the left of each row

} // namespace

void Hallway::open_navigator()
{
    const Book* ref = reference_book();
    nav_value_ = ref ? ref->index : BigUint();
    if (nav_value_ >= line_units()) nav_value_ = BigUint();
    BigUint top = line_units();
    top -= BigUint(1);
    nav_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
    nav_hex_ = nav_value_.to_hex(nav_width_);
    nav_sel_ = nav_width_ - 1; // the last digit: a step of one unit, where the wheel already works
    nav_hover_.reset();
    nav_scroll_ = -1; // scroll to the selection on the first draw
    nav_note_.clear();
    nav_angle_focus_ = false;
    nav_angle_ = nav_angle_of(nav_value_);
    nav_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false); // the digits are picked with the pointer
    nav_open_ = true;
}

void Hallway::close_navigator(bool go)
{
    nav_open_ = false;
    SDL_SetWindowRelativeMouseMode(window_, nav_had_mouse_);
    if (!go) return; // back to where it was opened from: the hallway, or the pause menu
    if (pause_open_) close_pause(); // going somewhere leaves the pause too
    trail_.clear();
    place(nav_value_, false);
    message(trf("nav.went", {short_address(nav_hex_)}));
}

// Digit `d` (counted from the left) up or down by one, with carry, round the loop.
void Hallway::nav_step(size_t d, int dir)
{
    const BigUint& n = line_units();
    BigUint step = BigUint::pow(16, uint64_t(nav_width_ - 1 - d));
    if (step >= n) step = n; // cannot happen with a width from n - 1, but a loop of one unit has no step
    if (dir > 0)
    {
        nav_value_ += step;
        if (nav_value_ >= n) nav_value_ -= n;
    }
    else if (nav_value_ >= step) nav_value_ -= step;
    else
    {
        BigUint wrapped = n;
        wrapped -= step;
        wrapped += nav_value_;
        nav_value_ = std::move(wrapped);
    }
    nav_hex_ = nav_value_.to_hex(nav_width_);
    nav_note_.clear();
    nav_angle_ = nav_angle_of(nav_value_);
}

// Digit `d` set outright, as typed: refused if it would name a place past the end of the line.
void Hallway::nav_set(size_t d, uint32_t v)
{
    std::string hex = nav_hex_;
    hex[d] = "0123456789abcdef"[v & 15];
    const BigUint to = BigUint::from_hex(hex);
    if (to >= line_units())
    {
        nav_note_ = tr("nav.past_end");
        return;
    }
    nav_value_ = to;
    nav_hex_ = std::move(hex);
    nav_note_.clear();
    nav_angle_ = nav_angle_of(nav_value_);
    if (nav_sel_ + 1 < nav_width_) ++nav_sel_;
}

// A position's bearing, exactly: floor(v * 360 * 10^d / units), written with d decimal places.
std::string Hallway::nav_angle_of(const BigUint& v) const
{
    const int d = angle_decimals_;
    const BigUint units = line_units();
    if (units.is_zero()) return "0";
    BigUint scale = BigUint::pow(10, uint64_t(d));
    scale.mul_small(360);
    BigUint q, r;
    BigUint::divmod(BigUint::mul(v, scale), units, q, r);
    std::string digits = q.to_decimal();
    if (d == 0) return digits;
    if (digits.size() <= size_t(d)) digits.insert(0, size_t(d) + 1 - digits.size(), '0');
    return digits.substr(0, digits.size() - size_t(d)) + "." + digits.substr(digits.size() - size_t(d));
}

// The typed bearing, checked against the Angle Precision, as a position: the first unit at or past
// it, round the loop to the start when it falls past the last unit.
void Hallway::nav_angle_apply()
{
    const int d = angle_decimals_;
    const std::string& t = nav_angle_;
    const size_t dot = t.find('.');
    const std::string whole = t.substr(0, dot), frac = dot == std::string::npos ? "" : t.substr(dot + 1);
    if (whole.empty() || whole.size() > 3 || frac.find('.') != std::string::npos)
    {
        nav_note_ = tr("nav.angle_bad");
        return;
    }
    if (frac.size() > size_t(d))
    {
        nav_note_ = trf("nav.angle_places", {std::to_string(d)});
        return;
    }
    const std::string scaled = whole + frac + std::string(size_t(d) - frac.size(), '0');
    const BigUint a = BigUint::from_decimal(scaled);
    BigUint scale = BigUint::pow(10, uint64_t(d));
    scale.mul_small(360);
    if (a >= scale)
    {
        nav_note_ = tr("nav.angle_range");
        return;
    }
    const BigUint units = line_units();
    BigUint q, r;
    BigUint::divmod(BigUint::mul(a, units), scale, q, r);
    if (!r.is_zero()) q.add_small(1);
    if (q >= units) q = BigUint();
    nav_value_ = q;
    nav_hex_ = nav_value_.to_hex(nav_width_);
    nav_note_.clear();
}

// A key while the bearing field has focus: digits and one decimal point, Backspace; the first key
// after entering the field replaces what it showed.
void Hallway::nav_angle_key(SDL_Keycode k)
{
    char c = 0;
    if (k >= SDLK_0 && k <= SDLK_9) c = char('0' + (k - SDLK_0));
    else if (k >= SDLK_KP_1 && k <= SDLK_KP_9) c = char('1' + (k - SDLK_KP_1));
    else if (k == SDLK_KP_0) c = '0';
    else if (k == SDLK_PERIOD || k == SDLK_KP_PERIOD) c = '.';
    if (c)
    {
        if (nav_angle_fresh_) nav_angle_.clear();
        nav_angle_fresh_ = false;
        if (nav_angle_.size() < 16) nav_angle_ += c;
    }
    else if (k == SDLK_BACKSPACE)
    {
        if (nav_angle_fresh_) nav_angle_.clear();
        nav_angle_fresh_ = false;
        if (!nav_angle_.empty()) nav_angle_.pop_back();
    }
    else return;
    if (nav_angle_.empty() || nav_angle_ == ".") nav_note_.clear();
    else nav_angle_apply();
}

// Where digit d is drawn: its row and its x.
std::pair<size_t, float> Hallway::nav_cell(size_t d) const
{
    const size_t row = d / nav_cols_, col = d % nav_cols_;
    return {row, nav_x0_ + float(col) * kCellW + float(col / 4) * kGroupGap};
}

// The digit under a point on the screen, and whether the point is on its arrow above (+1), on
// its arrow below (-1) or on the digit itself (0).
std::optional<std::pair<size_t, int>> Hallway::nav_hit(float x, float y) const
{
    if (nav_cols_ == 0 || y < nav_y0_ || x < nav_x0_) return std::nullopt;
    const size_t row = size_t((y - nav_y0_) / kRowH) + size_t(std::max(0, nav_scroll_));
    if (int(row) - std::max(0, nav_scroll_) >= nav_rows_shown_) return std::nullopt;
    const float in_row = std::fmod(y - nav_y0_, kRowH);
    const float gx = x - nav_x0_;
    const size_t group = size_t(gx / (4 * kCellW + kGroupGap));
    const float in_group = gx - float(group) * (4 * kCellW + kGroupGap);
    if (in_group >= 4 * kCellW) return std::nullopt; // the gap between groups
    const size_t col = group * 4 + size_t(in_group / kCellW);
    if (col >= nav_cols_) return std::nullopt;
    const size_t d = row * nav_cols_ + col;
    if (d >= nav_width_) return std::nullopt;
    const int part = in_row < 16 ? 1 : in_row > 36 ? -1 : 0;
    return std::make_pair(d, part);
}

void Hallway::navigator_event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        float x = e.motion.x, y = e.motion.y;
        SDL_RenderCoordinatesFromWindow(r_, e.motion.x, e.motion.y, &x, &y);
        const auto hit = nav_hit(x, y);
        nav_hover_ = hit ? std::optional<size_t>(hit->first) : std::nullopt;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        float x = e.button.x, y = e.button.y;
        SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &x, &y);
        const SDL_FPoint pt{x, y};
        if (SDL_PointInRectFloat(&pt, &nav_angle_box_))
        {
            nav_angle_focus_ = true;
            nav_angle_fresh_ = true;
        }
        else if (const auto hit = nav_hit(x, y))
        {
            nav_angle_focus_ = false;
            nav_sel_ = hit->first;
            if (hit->second != 0) nav_step(hit->first, hit->second);
        }
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        // The wheel turns the digit under the pointer, or the chosen one when it is over none.
        nav_wheel_ += e.wheel.y;
        const int notches = int(nav_wheel_);
        nav_wheel_ -= float(notches);
        const size_t d = nav_hover_.value_or(nav_sel_);
        for (int i = 0; i < std::abs(notches); ++i) nav_step(d, notches > 0 ? 1 : -1);
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    const bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0;
    const SDL_Keycode k = e.key.key;
    // Tab moves between the digits and the bearing field. In the field, Esc leaves it (a second
    // Esc closes the navigator) and ENTER goes to the bearing typed.
    if (k == SDLK_TAB)
    {
        nav_angle_focus_ = !nav_angle_focus_;
        nav_angle_fresh_ = nav_angle_focus_;
        if (!nav_angle_focus_) nav_angle_ = nav_angle_of(nav_value_);
        return;
    }
    if (nav_angle_focus_)
    {
        if (k == SDLK_ESCAPE)
        {
            nav_angle_focus_ = false;
            nav_angle_ = nav_angle_of(nav_value_);
            nav_note_.clear();
        }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER)
        {
            if (nav_note_.empty()) close_navigator(true);
        }
        else nav_angle_key(k);
        return;
    }
    if (k >= SDLK_0 && k <= SDLK_9) { nav_set(nav_sel_, uint32_t(k - SDLK_0)); return; }
    if (k >= SDLK_A && k <= SDLK_F) { nav_set(nav_sel_, uint32_t(10 + k - SDLK_A)); return; }
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) { nav_set(nav_sel_, uint32_t(1 + k - SDLK_KP_1)); return; }
    if (k == SDLK_KP_0) { nav_set(nav_sel_, 0); return; }
    switch (k)
    {
    case SDLK_ESCAPE:
    case SDLK_X: close_navigator(false); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: close_navigator(true); break;
    case SDLK_LEFT: nav_sel_ = shift ? (nav_sel_ >= nav_cols_ ? nav_sel_ - nav_cols_ : 0) : (nav_sel_ > 0 ? nav_sel_ - 1 : 0); break;
    case SDLK_RIGHT: nav_sel_ = std::min(nav_width_ - 1, nav_sel_ + (shift ? nav_cols_ : 1)); break;
    case SDLK_HOME: nav_sel_ = 0; break;
    case SDLK_END: nav_sel_ = nav_width_ - 1; break;
    case SDLK_UP: nav_step(nav_sel_, shift ? 8 : 1); break;
    case SDLK_DOWN: nav_step(nav_sel_, shift ? -8 : -1); break;
    // A page of rows at a time. They move the chosen digit, not just the view: the view follows
    // the chosen digit, so scrolling it alone would snap straight back.
    case SDLK_PAGEUP:
    {
        const size_t by = size_t(std::max(1, nav_rows_shown_ - 1)) * std::max<size_t>(1, nav_cols_);
        nav_sel_ = nav_sel_ >= by ? nav_sel_ - by : nav_sel_ % std::max<size_t>(1, nav_cols_);
        break;
    }
    case SDLK_PAGEDOWN:
    {
        const size_t by = size_t(std::max(1, nav_rows_shown_ - 1)) * std::max<size_t>(1, nav_cols_);
        nav_sel_ = std::min(nav_width_ - 1, nav_sel_ + by);
        break;
    }
    default: break;
    }
    // Up and down with Shift step eight at a time, which is half a turn of a hex digit.
    if ((k == SDLK_UP || k == SDLK_DOWN) && shift)
        for (int i = 1; i < 8; ++i) nav_step(nav_sel_, k == SDLK_UP ? 1 : -1);
}

void Hallway::draw_navigator(float W, float H)
{
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{255, 255, 255, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);

    text(20, 16, tr("nav.title"), 3, ink);
    text(20, 48, fit(trf("nav.where", {tr(theme().key), ordering_name(), std::to_string(nav_width_)}), W - 40, 1), 1, grey);

    // Layout: as many groups of four digits to a row as fit, every row the same length.
    const float usable = W - 40 - kLabelW;
    const size_t groups = std::max<size_t>(1, size_t((usable + kGroupGap) / (4 * kCellW + kGroupGap)));
    nav_cols_ = std::min(groups * 4, ((nav_width_ + 3) / 4) * 4);
    nav_x0_ = 20 + kLabelW;
    nav_y0_ = 110;
    const size_t rows = (nav_width_ + nav_cols_ - 1) / nav_cols_;
    nav_rows_shown_ = std::max(1, int((H - nav_y0_ - 150) / kRowH));
    // Keep the chosen digit in view; PgUp/PgDn scroll freely within the rows there are.
    const int sel_row = int(nav_sel_ / nav_cols_);
    if (nav_scroll_ < 0) nav_scroll_ = std::max(0, sel_row - nav_rows_shown_ / 2);
    nav_scroll_ = std::clamp(nav_scroll_, 0, std::max(0, int(rows) - nav_rows_shown_));
    if (sel_row < nav_scroll_ || sel_row >= nav_scroll_ + nav_rows_shown_)
        nav_scroll_ = std::clamp(sel_row - nav_rows_shown_ / 2, 0, std::max(0, int(rows) - nav_rows_shown_));

    // An arrow: a small filled triangle pointing up (dir 1) or down (dir -1).
    auto arrow = [&](float cx, float y, int dir, SDL_Color c) {
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        for (int i = 0; i < 6; ++i)
        {
            const float yy = dir > 0 ? y + float(i) : y + 5 - float(i);
            SDL_RenderLine(r_, cx - float(i), yy, cx + float(i), yy);
        }
    };
    for (int r = nav_scroll_; r < std::min(int(rows), nav_scroll_ + nav_rows_shown_); ++r)
    {
        const float y = nav_y0_ + float(r - nav_scroll_) * kRowH;
        // Each row is labelled with the power of 16 its first digit stands for.
        text(20, y + 22, "16^" + std::to_string(nav_width_ - 1 - size_t(r) * nav_cols_), 1, grey);
        for (size_t c = 0; c < nav_cols_; ++c)
        {
            const size_t d = size_t(r) * nav_cols_ + c;
            if (d >= nav_width_) break;
            const float x = nav_cell(d).second;
            const bool sel = d == nav_sel_, hov = nav_hover_ && *nav_hover_ == d;
            if (sel)
            {
                SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 70);
                const SDL_FRect box{x - 1, y + 17, kCellW, 20};
                SDL_RenderFillRect(r_, &box);
            }
            text(x, y + 19, std::string(1, nav_hex_[d]), 2, sel ? white : hov ? ink : grey);
            if (sel || hov)
            {
                const SDL_Color ac = sel ? white : ink;
                arrow(x + kCellW / 2 - 1, y + 6, 1, ac);
                arrow(x + kCellW / 2 - 1, y + 41, -1, ac);
            }
        }
    }
    if (nav_scroll_ > 0) text(W - 180, nav_y0_ - 18, tr("nav.more_above"), 1, grey);
    if (nav_scroll_ + nav_rows_shown_ < int(rows)) text(W - 180, nav_y0_ + float(nav_rows_shown_) * kRowH, tr("nav.more_below"), 1, grey);

    // The bearing: the position as an angle round the loop, which can be typed instead.
    {
        const float ay = H - 136;
        const std::string label = trf("nav.angle", {std::to_string(angle_decimals_)});
        text(20, ay + 4, label, 1, nav_angle_focus_ ? white : grey);
        const float bx = 20 + text_width(label, 1) + 12;
        const std::string shown = nav_angle_ + (nav_angle_focus_ && (SDL_GetTicks() / 500) % 2 ? "_" : "") + "\xc2\xb0";
        nav_angle_box_ = SDL_FRect{bx - 6, ay - 4, std::max(160.0f, text_width(shown, 2) + 12), 26};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, nav_angle_focus_ ? 70 : 25);
        SDL_RenderFillRect(r_, &nav_angle_box_);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &nav_angle_box_);
        text(bx, ay, shown, 2, nav_angle_focus_ ? white : ink);
        text(nav_angle_box_.x + nav_angle_box_.w + 12, ay + 4, fit(tr(nav_angle_focus_ ? "nav.angle_typing" : "nav.angle_hint"),
                                                                  W - (nav_angle_box_.x + nav_angle_box_.w + 32), 1), 1, grey);
    }

    // What the chosen digit does, and where the address stands.
    const size_t p = nav_width_ - 1 - nav_sel_;
    const double frac = nav_value_.is_zero() ? 0.0 : std::pow(10.0, nav_value_.log10_approx() - line_units().log10_approx());
    float y = H - 92;
    text(20, y, fit(trf("nav.digit", {std::to_string(nav_sel_ + 1), std::to_string(nav_width_), std::to_string(p)}), W - 40, 1), 1, ink);
    text(20, y + 14, fit(trf("nav.along", {percent(frac)}), W - 40, 1), 1, ink);
    if (!nav_note_.empty()) text(20, y + 28, nav_note_, 1, SDL_Color{255, 80, 80, 255});
    // Which page of rows is in view, of how many: bottom right, where a reader looks for it.
    {
        const int per = std::max(1, nav_rows_shown_);
        const int pages = std::max(1, (int(rows) + per - 1) / per);
        const int page = std::min(pages, (nav_scroll_ + per - 1) / per + 1);
        const std::string pg = trf("nav.page", {std::to_string(page), std::to_string(pages)});
        text(W - 20 - text_width(pg, 2), y, pg, 2, ink);
    }
    text(20, H - 40, fit(tr("nav.keys1"), W - 40, 1), 1, grey);
    text(20, H - 26, fit(tr("nav.keys2"), W - 40, 1), 1, grey);
}

} // namespace hallway::hall
