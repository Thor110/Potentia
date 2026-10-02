// Sieve hallway -- everything drawn in screen space over the corridor: the compass, the panels
// and the readout, the preview of the unit under the crosshair, and the item page with its ITEM
// and COST tabs.

#include "hallway.hpp"

namespace hallway::hall {

// Text cut to fit `width` pixels at `scale` (8 pixels per character at scale 1).
std::string Hallway::fit(const std::string& s, float width, float scale)
{
    const size_t cols = size_t(std::max(4.0f, width / (8 * scale)));
    return text_cells(s) <= cols ? s : fit_cells(s, cols - 2) + "..";
}

// The compass. Every line is a loop, so the corridor is a set of concentric circles: binary
// outermost, then the six it bounds, then binary again innermost, because binary wraps around
// the outside of the others and is met from either end. A needle runs from the middle out to
// the ring you are standing on, at the angle you stand at; every other ring carries a mark at
// its own angle, because each line loops at its own rate and they only agree at zero. Zero is
// at the top, where every loop starts and finishes.
//
// This reads the same corridor position the rest of the hallway does, so nothing here decides
// anything: it is the readout drawn round instead of along.
SDL_Color Hallway::ring_ink(const Theme& t, SDL_Color on)
{
    auto lum = [](SDL_Color c) { return (c.r * 3 + c.g * 6 + c.b) / 10; };
    const int b = lum(on);
    const SDL_Color pick = std::abs(lum(t.edge) - b) >= std::abs(lum(t.bg) - b) ? t.edge : t.bg;
    if (std::abs(lum(pick) - b) >= 40) return pick;
    // Both of a line's colours sit too close to the panel behind it: lift it until it reads.
    const float k = b < 128 ? 1.0f : -1.0f;
    auto f = [k](Uint8 v) { return Uint8(std::clamp(float(v) + k * 90.0f, 0.0f, 255.0f)); };
    return {f(pick.r), f(pick.g), f(pick.b), 255};
}

double Hallway::compute_line_fraction(int i) const
{
    const BigUint& at = all_loop_tiles_[size_t(i)];
    if (at.is_zero()) return 0.0;
    const BigUint units = units_of(i);
    if (units.is_zero()) return 0.0;
    BigUint first = at; // the first unit of the tile you are in
    first <<= sieve::books_per_tile_bits();
    const double f = std::pow(10.0, first.log10_approx() - units.log10_approx());
    return std::clamp(f, 0.0, 1.0);
}

void Hallway::draw_compass(float W, float H)
{
    const Theme& th = theme();
    const float r_out = 92, pad = 15, row = 20;
    const float bw = 2 * r_out + 2 * pad, bh = bw + row;
    const float bx = W - bw - 10, by = H - bh - 30;
    panel(bx, by, bw, bh);
    const float cx = bx + bw / 2, cy = by + pad + r_out;
    // Rings from the outside in: binary, the six in door order, binary again. The picture is
    // the corridor read from one edge to the other, bent into circles.
    const int kRings = kLines + 1;
    const float step = (r_out - 12) / float(kRings - 1);
    for (int k = 0; k < kRings; ++k)
    {
        const int line = k == 0 || k == kRings - 1 ? kBinaryLine : k - 1;
        const float rad = r_out - float(k) * step;
        const bool here = line == li_ && (line != kBinaryLine || k == 0);
        const SDL_Color c = ring_ink(theme_of(line), th.bg);
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, here ? 255 : 190);
        const int segs = std::max(32, int(rad * 1.4f));
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = float(j) / segs * 6.2831853f, a1 = float(j + 1) / segs * 6.2831853f;
            auto arc = [&](float d) {
                SDL_RenderLine(r_, cx + (rad + d) * std::sin(a0), cy - (rad + d) * std::cos(a0),
                               cx + (rad + d) * std::sin(a1), cy - (rad + d) * std::cos(a1));
            };
            arc(0);
            if (here) { arc(-1); arc(1); }
        }
        // Where that line stands in its own loop. They only agree at zero.
        if (line == kBinaryLine) continue;
        const float a = float(line_fraction(line)) * 6.2831853f;
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        const SDL_FRect dot{cx + rad * std::sin(a) - 2, cy - rad * std::cos(a) - 2, 5, 5};
        SDL_RenderFillRect(r_, &dot);
    }
    const SDL_Color ink = th.edge;
    // Zero at the top, where every loop starts and finishes.
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    SDL_RenderLine(r_, cx, cy - r_out - 2, cx, cy - r_out - 11);
    text(cx + 6, cy - r_out - 15, tr("hud.zero"), 1, ink);
    // The needle. It runs the whole radius so the bearing is easy to read off, and is drawn
    // bright as far as the ring you are standing on, faint beyond it.
    const int mine = on_binary() ? 0 : li_ + 1;
    const float rad = r_out - float(mine) * step;
    const float a = float(line_fraction(li_)) * 6.2831853f;
    const float sn = std::sin(a), cs = std::cos(a);
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 80);
    SDL_RenderLine(r_, cx + rad * sn, cy - rad * cs, cx + (r_out + 2) * sn, cy - (r_out + 2) * cs);
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    for (float d : {-0.5f, 0.5f}) SDL_RenderLine(r_, cx + d, cy, cx + d + rad * sn, cy - rad * cs);
    const SDL_FRect at{cx + rad * sn - 3, cy - rad * cs - 3, 7, 7};
    SDL_RenderFillRect(r_, &at);
    // And the same bearing written out, to whatever precision Settings > Graphics asks for.
    const std::string deg = degrees(line_fraction(li_), angle_decimals_);
    text(cx - text_width(deg, 1) / 2, by + bh - row + 4, deg, 1, ink);
}

// `alpha`: the readout's panels let a little of the corridor through, but the item page is
// something you stop and read, so it is drawn solid.
void Hallway::panel(float x, float y, float w, float h, Uint8 alpha)
{
    const Theme& th = theme();
    SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, alpha);
    const SDL_FRect box{x, y, w, h};
    SDL_RenderFillRect(r_, &box);
    SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
    SDL_RenderRect(r_, &box);
}

void Hallway::draw_pixels(const Space::Digits& unit, float x, float y, float size, int frame)
{
    draw_pixels(unit, on_books() ? lines_[1].image : line().image, x, y, size, frame);
}

void Hallway::draw_pixels(const Space::Digits& unit, const ImageFormat& f, float x, float y, float size, int frame)
{
    const float cell = size / float(std::max(f.width, f.height));
    const size_t base = size_t(frame) * f.width * f.height;
    // One texture, scaled with nearest-neighbour sampling (one rectangle per pixel costs a draw
    // call per pixel); per-pixel rectangles only if the texture cannot be made. The picture in
    // front of you and the one in hand rarely change, so their textures are kept between frames.
    static_assert(sizeof(Rgb) == 3, "Rgb must be packed RGB24");
    // The image line's format drawn anywhere but the image line is a cover (a book's, a record's,
    // a tape's), and gets a texture of its own so it does not fight the picture beside it.
    PictureCache& c = picture_[&f == &lines_[1].image && li_ != 1 ? 1 : 0];
    const bool same = c.tex && c.unit == unit && c.frame == frame && c.w == f.width && c.h == f.height;
    if (!same)
    {
        if (c.tex && (c.w != f.width || c.h != f.height))
        {
            SDL_DestroyTexture(c.tex);
            c.tex = nullptr;
        }
        if (!c.tex) c.tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, int(f.width), int(f.height));
        if (c.tex)
        {
            const auto px = render_image(unit, f);
            SDL_UpdateTexture(c.tex, nullptr, px.data() + base, int(f.width * 3));
            SDL_SetTextureScaleMode(c.tex, SDL_SCALEMODE_NEAREST);
            c.unit = unit;
            c.frame = frame;
            c.w = f.width;
            c.h = f.height;
        }
    }
    if (c.tex)
    {
        const SDL_FRect dst{x, y, f.width * cell, f.height * cell};
        SDL_RenderTexture(r_, c.tex, nullptr, &dst);
    }
    else
    {
    const auto px = render_image(unit, f);
    for (uint32_t py = 0; py < f.height; ++py)
        for (uint32_t pxi = 0; pxi < f.width; ++pxi)
        {
            const Rgb pc = px[base + size_t(py) * f.width + pxi];
            SDL_SetRenderDrawColor(r_, pc.r, pc.g, pc.b, 255);
            const SDL_FRect cellr{x + pxi * cell, y + py * cell, cell, cell};
            SDL_RenderFillRect(r_, &cellr);
        }
    }
    const Theme& th = theme();
    SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
    const SDL_FRect border{x - 1, y - 1, f.width * cell + 2, f.height * cell + 2};
    SDL_RenderRect(r_, &border);
}

// A file, as a line of text: how long it is and its first bytes in hex. A hex dump rather than
// the bytes as characters, because most files are not text and a byte is not a character.
std::string Hallway::binary_preview(const BinarySpace::Bytes& f, size_t most, uint64_t size)
{
    if (size == UINT64_MAX) size = f.size();
    std::string s = trf("hud.binary.bytes", {std::to_string(size)});
    char b[4];
    for (size_t i = 0; i < f.size() && i < most; ++i)
    {
        std::snprintf(b, sizeof b, " %02x", f[i]);
        s += b;
    }
    if (size > std::min<uint64_t>(most, f.size())) s += " ...";
    return s;
}

std::string Hallway::one_line_preview(const Space::Digits& u)
{
    if (on_binary()) return ""; // a file is written out by binary_preview
    if (on_models()) return ""; // a model is drawn, not written out: see draw_model
    if (line().kind == LineKind::Text) return "\"" + ascii(utf8_encode(line().space.text_of(u))) + "\"";
    if (line().kind == LineKind::Audio) return notes_to_notation(note_set_of(line().space.symbols_id()), u);
    return "";
}

// A blank title (all spaces, as the first titles of a line in positional order are) is shown as
// [Null Title]: white brackets, red words, so an empty title reads as a fact about the item rather
// than as something that failed to draw. Every place a title is shown uses this.
static bool is_blank_title(const std::string& t)
{
    return t.find_first_not_of(std::string(" \0", 2)) == std::string::npos;
}

float Hallway::draw_null_title(float x, float y, float scale)
{
    const SDL_Color white{255, 255, 255, 255}, red{255, 70, 70, 255};
    const std::string words = tr("title.null");
    // On a dark plate of its own, so the red and the white read on every line's colour (the books
    // line is grey and the models line olive, where neither would on its own).
    const float full = text_width("[" + words + "]", scale);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 200);
    const SDL_FRect plate{x - 3 * scale, y - 2 * scale, full + 6 * scale, 8 * scale + 4 * scale};
    SDL_RenderFillRect(r_, &plate);
    text(x, y, "[", scale, white);
    const float a = text_width("[", scale);
    text(x + a, y, words, scale, red);
    const float b = text_width(words, scale);
    text(x + a + b, y, "]", scale, white);
    return a + b + text_width("]", scale);
}

void Hallway::draw_hud(int w, int h)
{
    const Theme& th = theme();
    const SDL_Color ink = th.edge;
    const float W = float(w), H = float(h);

    // FPS counter: the average over the last half second, and its slowest frame.
    if (fps_counter_)
    {
        const Uint64 now = SDL_GetTicksNS();
        if (fps_last_)
        {
            ++fps_frames_;
            fps_worst_now_ = std::max(fps_worst_now_, double(now - fps_last_) / 1e6);
        }
        else fps_since_ = now;
        fps_last_ = now;
        if (now - fps_since_ >= 500000000ull && fps_frames_ > 0)
        {
            fps_ms_ = double(now - fps_since_) / 1e6 / fps_frames_;
            fps_shown_ = 1000.0 / fps_ms_;
            fps_worst_ = fps_worst_now_;
            fps_frames_ = 0;
            fps_worst_now_ = 0;
            fps_since_ = now;
        }
        char a[32], b[32], c[32];
        std::snprintf(a, sizeof a, "%.0f", fps_shown_);
        std::snprintf(b, sizeof b, "%.1f", fps_ms_);
        std::snprintf(c, sizeof c, "%.1f", fps_worst_);
        std::string line = trf("hud.fps", {a, b, c});
        if (fps_tris_) line += "   " + trf("hud.fps.triangles", {std::to_string(fps_tris_)});
        const float tw = text_width(line, 1);
        panel(W - tw - 22, 48, tw + 16, 18);
        text(W - tw - 14, 53, line, 1, ink);
    }

    // Crosshair.
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    SDL_RenderLine(r_, W / 2 - 8, H / 2, W / 2 + 8, H / 2);
    SDL_RenderLine(r_, W / 2, H / 2 - 8, W / 2, H / 2 + 8);

    // Top bar: where you are.
    panel(-1, -1, W + 2, 44);
    const Book& first = book(0, 0);
    const std::string where =
        trf("hud.line", {tr(th.key)}) + "   " + ordering_name() +
                          (guided_on() ? "  " + trf("hud.zoom", {std::to_string(zoom_)}) : std::string()) + "   " + tr("hud.tile") + " " +
                          tile_label_ +
                          (first.empty ? "   " + tr("hud.padding") : "   " + trf("hud.along", {percent(first.fraction)}));
    text(10, 7, fit(where, W - 20, 2), 2, ink);
    const std::string per_tile = "   " + trf("hud.per_tile", {std::to_string(sieve::books_per_tile())});
    const std::string loop = trf("hud.loop", {loop_label_}) + (loop_.fills_whole_tiles()
                                                                                 ? std::string()
                                                                                 : " " + trf("hud.loop.padding", {std::to_string(loop_.padding())}));
    text(10, 28, fit((on_books() ? books_->id() : titled_here() ? titled_here()->id() : line().space.id()) + (guided_on() ? "   " + trf("hud.model", {line().model_id}) : std::string()) + "   " +
                     loop + per_tile + "   " + filter_status() + "   " +
                     (on_binary() ? trf("hud.door_one", {tr(theme_of(binary_from_).key)})
                                  : trf("hud.doors", {tr(theme_of((li_ + 1) % kLines).key), tr(theme_of((li_ + kLines - 1) % kLines).key)})),
                     W - 20, 1),
         1, ink);

    // The book you are looking at.
    if (hover_ && !in_hand_)
    {
        const Book& bk = book(hover_->tile, hover_->slot());
        // One size for every line: tall enough for a picture of 60 px beside the words, or a
        // title and the first line of a page under it.
        const float ph = 150;
        panel(10, H - ph - 44, std::min(W - 20, 900.0f), ph);
        const std::string label = trf(hover_->side == Side::Left ? "hud.slot.left" : "hud.slot.right",
                                      {std::to_string(hover_->row + 1), std::to_string(hover_->col + 1), std::to_string(hover_->slot())});
        float y = H - ph - 36;
        text(20, y, label, 2, ink);
        y += 22;
        if (bk.empty)
            text(20, y, tr("hud.empty_slot"), 1, ink);
        else if (withheld(bk))
            text(20, y, tr("vault.withheld"), 1, ink); // the vault: nothing of it, not even its address
        else
        {
            const Space::Digits& u = bk.unit;
            if (bk.guided)
            {
                char b1[16], b2[16];
                std::snprintf(b1, sizeof b1, "%.2f", double(bk.bits) / double(u.size()));
                std::snprintf(b2, sizeof b2, "%.2f", std::log2(double(line().space.base())));
                text(20, y, trf("hud.point", {short_address(hex_of(bk)), std::to_string(bk.bits), b1, b2}), 1, ink);
            }
            else text(20, y, trf(bk.survivor ? "hud.compact_address" : "hud.address", {short_hex_of(bk)}), 1, ink);
            y += 12;
            std::string verdict;
            if (bk.survivor) verdict = trf("hud.survivor", {bk.survivor_label}) + "   ";
            else if (has_filters()) verdict = (bk.passes ? tr("hud.passes") : trf("hud.fails", {bk.failed_by})) + "   ";
            text(20, y, verdict + trf("hud.along_loop", {percent(bk.fraction)}), 1, ink);
            y += 16;
            if (bk.parts)
            {
                // A book: its cover and its title.
                draw_pixels(bk.parts->cover, 20, y, 60, 0);
                const std::string title = utf8_encode(line().space.text_of(bk.parts->title));
                if (is_blank_title(title)) draw_null_title(96, y, 2);
                const auto rows = wrap_words(title, size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 110) / 16)));
                for (size_t r = 0; r < rows.size() && r < 3 && !is_blank_title(title); ++r) text(96, y + float(r) * 20, rows[r], 2, ink);
            }
            else if (const std::string title = title_text(bk); has_titles() || !bk.cover.empty())
            {
                // A titled unit. With a cover it is laid out as a book is: the cover on the left,
                // the title beside it, and what the thing is (its notes, its bytes) under the title,
                // everything inside the panel. Without one, the title and then the thing itself.
                const float panel_w = std::min(W - 20, 900.0f);
                const bool covered = !bk.cover.empty();
                const float tx = covered ? 96.0f : 20.0f;
                if (covered) draw_pixels(bk.cover, lines_[1].image, 20, y, 60, 0);
                if (!title.empty()) text(tx, y, fit("\"" + title + "\"", panel_w - tx, 2), 2, ink);
                else if (has_titles()) draw_null_title(tx, y, 2);
                const float below = y + 26;
                const float room = panel_w - tx;
                if (bk.is_file) // its kind and first bytes, known without working out the rest of it
                    text(tx, below, fit(file_type(bk.head, bk.file_size) + "  " + binary_preview(bk.head, 16, bk.file_size), room, 1), 1, ink);
                else if (bk.model) text(tx, below, fit(model_line_summary(*bk.model), room, 2), 2, ink);
                else if (covered) text(tx, below, fit(one_line_preview(u), room, 1), 1, ink); // a video's cover stands for it
                else if (line().kind == LineKind::Image || line().kind == LineKind::Video) draw_pixels(u, 20, below, 60, 0);
                else text(20, below, wrap(one_line_preview(u), size_t(std::max(20.0f, (panel_w - 40) / 16)))[0], 2, ink);
            }
            else if (bk.model) text(20, y, model_line_summary(*bk.model), 2, ink);
            else if (line().kind == LineKind::Image || line().kind == LineKind::Video) draw_pixels(u, 20, y, 60, 0);
            else text(20, y, wrap(one_line_preview(u), size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 40) / 16)))[0], 2, ink);
        }
    }

    if (in_hand_) draw_in_hand(W, H);

    // Messages, input box and help.
    if (!message_.empty() && SDL_GetTicks() < message_until_) text(12, H - 36, message_, 1, ink);
    if (input_ != Input::None)
    {
        const std::string prompt = input_ == Input::Warp
                                       ? tr(on_books() ? "prompt.warp.book" : on_binary() ? "prompt.warp.file"
                                            : line().kind == LineKind::Image || line().kind == LineKind::Video ? "prompt.warp.picture"
                                            : line().kind == LineKind::Audio                                   ? "prompt.warp.notes"
                                                                                                               : "prompt.warp.text")
                                       : tr("prompt.goto");
        panel(10, H / 2 + 40, W - 20, 34);
        text(20, H / 2 + 49, prompt + text_ + ((SDL_GetTicks() / 400) % 2 ? "_" : " "), 2, ink);
    }
    text(12, H - 18,
         tr("hud.keys1") + (guided_on() ? tr("hud.keys.zoom") : std::string()) + tr("hud.keys2") +
             tr(in_hand_ && in_hand_->parts ? "hud.keys.page" : "hud.keys.trail") + tr("hud.keys3"),
         1, ink);
    // Last, so nothing else in the readout is drawn over it.
    draw_compass(float(W), float(H));
    if (pause_open_) draw_pause(W, H);
    if (nav_open_) draw_navigator(W, H); // over everything, the pause menu too: a screen of its own
    if (loc_open_) draw_locator(W, H);
    if (media_open_) draw_media_player(W, H);
    if (graph_open_) draw_graph_view(W, H);
}

// A model in hand: its wireframe, turned by the mouse or by A and D, and its .obj text beside
// it. The mesh is small enough (a few dozen triangles) to draw as lines with the painter's
// algorithm; the shelf copy is a crate, which is what the render cache fills in.
float Hallway::draw_model(const ModelSpace::Parts& p, float x, float y, float pw, float bottom)
{
    const Theme& th = theme();
    const auto verts = model_space_->mesh_of(p);
    const auto faces = model_space_->faces_of(p);
    const float box = std::min(pw * 0.5f, bottom - y - 20);
    if (box < 40) return y;
    const float cx = x + 14 + box * 0.5f, cy = y + box * 0.5f, r = box * 0.34f;
    // Turn about the upright axis, and tip a little so the shape reads as solid.
    const float ca = std::cos(model_spin_), sa = std::sin(model_spin_);
    const float ct = std::cos(model_tilt_), st = std::sin(model_tilt_);
    auto project = [&](const ModelSpace::Vertex& v) {
        const float px = v.x * ca + v.z * sa;
        const float pz = -v.x * sa + v.z * ca;
        const float py = v.y * ct - pz * st;
        const float depth = v.y * st + pz * ct;
        // A gentle perspective, so turning it reads as turning.
        const float k = 1.0f / (2.4f - depth * 0.45f);
        return std::array<float, 3>{cx + px * r * k * 2.4f, cy - py * r * k * 2.4f, depth};
    };
    // Faces back to front, drawn as filled triangles under their own edges.
    std::vector<std::pair<float, size_t>> order;
    order.reserve(faces.size());
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const auto& f = faces[i];
        order.emplace_back((project(verts[f.a])[2] + project(verts[f.b])[2] + project(verts[f.c])[2]) / 3.0f, i);
    }
    std::sort(order.begin(), order.end());
    std::vector<SDL_Vertex> fill;
    for (const auto& [depth, i] : order)
    {
        const auto& f = faces[i];
        if (f.a == f.b || f.b == f.c || f.a == f.c) continue; // a degenerate face has no face
        const auto a3 = project(verts[f.a]), b3 = project(verts[f.b]), c3 = project(verts[f.c]);
        // Nearer faces a little brighter, so the shape has depth without a light.
        const float t = std::clamp(0.35f + depth * 0.5f, 0.12f, 0.85f);
        const SDL_FColor fc{th.edge.r / 255.0f * t, th.edge.g / 255.0f * t, th.edge.b / 255.0f * t, 0.55f};
        fill.clear();
        for (const auto& v3 : {a3, b3, c3}) fill.push_back({{v3[0], v3[1]}, fc, {0, 0}});
        SDL_RenderGeometry(r_, nullptr, fill.data(), 3, nullptr, 0);
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 200);
        SDL_RenderLine(r_, a3[0], a3[1], b3[0], b3[1]);
        SDL_RenderLine(r_, b3[0], b3[1], c3[0], c3[1]);
        SDL_RenderLine(r_, c3[0], c3[1], a3[0], a3[1]);
    }
    // The .obj text of the very same model, beside it: the two lines this object lives on.
    const float tx = x + 14 + box + 20;
    const size_t cols = size_t(std::max(12.0f, (pw - 48 - box) / 8));
    float ty = y;
    uint32_t shown = 0;
    for (const std::string& l : split_lines(model_space_->to_obj(p)))
    {
        if (ty > bottom - 14) { text(tx, ty, "...", 1, th.edge); break; }
        text(tx, ty, fit(l, float(cols) * 8, 1), 1, ++shown <= model_space_->vertices() ? th.edge : mix(th.edge, th.bg, 0.35f));
        ty += 11;
    }
    return std::max(y + box, ty) + 6;
}

// One line about a model, for the shelf row and the readout.
std::string Hallway::model_line_summary(const ModelSpace::Parts& p) const
{
    const auto fs = model_space_->faces_of(p);
    std::vector<bool> seen(model_space_->vertices(), false);
    uint32_t used = 0, degenerate = 0;
    for (const auto& f : fs)
    {
        seen[f.a] = seen[f.b] = seen[f.c] = true;
        if (f.a == f.b || f.b == f.c || f.a == f.c) ++degenerate;
    }
    for (bool b : seen)
        if (b) ++used;
    return trf("model.summary", {std::to_string(used), std::to_string(model_space_->vertices()),
                                 std::to_string(model_space_->face_count() - degenerate),
                                 std::to_string(model_space_->face_count())});
}

std::vector<std::string> Hallway::split_lines(const std::string& s)
{
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i)
        if (i == s.size() || s[i] == '\n')
        {
            if (i > start) out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    return out;
}

void Hallway::draw_in_hand(float W, float H)
{
    const Theme& th = theme();
    const SDL_Color ink = th.edge;
    const Book& bk = *in_hand_;
    const Space::Digits& u = bk.unit;
    // A titled item's cover and title take room above the thing itself: the page grows by as
    // much, where the window has it, so the thing keeps its size and the address stays clear.
    const bool titled = has_titles() || !bk.cover.empty();
    const float head = !titled ? 0.0f : bk.cover.empty() ? 44.0f : 100.0f;
    const float pw = std::min(W - 40, 1000.0f), ph = std::min(H - 120, 640.0f + head);
    const float x = (W - pw) / 2, y = 50;
    panel(x, y, pw, ph, 255);
    float cy = y + 12;
    text(x + 14, cy, fit(tr("hand.title") + "   " + in_hand_where_ + "   " + trf("hud.along", {percent(bk.fraction)}), pw - 28, 2), 2, ink);
    cy += 28;
    // Three tabs: the thing itself, what it costs to name it, and where it stands in a map. C moves
    // between them. On every tab, at the right, the chosen map and V to add the item to it.
    {
        if (const GraphMap* g = graph_current())
        {
            const bool has = !g->read_only() && sort_node(bk) > 0;
            const std::string m = g->read_only() ? trf("hand.anchor.read_only", {g->title})
                                  : has          ? trf("hand.anchor.remove", {g->title})
                                                 : trf("hand.anchor", {g->title});
            const float mw = text_width(m, 1);
            text(x + pw - 14 - mw, cy, m, 1, g->read_only() ? SDL_Color{150, 150, 150, 255} : ink);
        }
        const int tabs = hand_tabs();
        if (hand_tab_ >= tabs) hand_tab_ = 0; // META, gone with a change of map
        float tx = x + 14;
        for (int t = 0; t < tabs; ++t)
        {
            const std::string label = tr(t == 0 ? "hand.tab.item" : t == 1 ? "hand.tab.cost" : t == 2 ? "hand.tab.sort" : "hand.tab.meta");
            const float tw = text_width(label, 1) + 16;
            if (t == hand_tab_)
            {
                SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
                const SDL_FRect box{tx, cy - 3, tw, 16};
                SDL_RenderFillRect(r_, &box);
            }
            text(tx + 8, cy, label, 1, ink);
            tx += tw + 6;
        }
        cy += 22;
    }
    if (hand_tab_ == 1) { draw_cost(bk, x, cy, pw, y + ph); return; }
    if (hand_tab_ == 2) { draw_sort(bk, x, cy, pw, y + ph - 16); return; }
    if (hand_tab_ == 3) { draw_meta(bk, x, cy, pw, y + ph - 16); return; }
    // A titled unit's cover and title, above the thing itself.
    if (const std::string title = title_text(bk); titled)
    {
        float tx = x + 14;
        if (!bk.cover.empty())
        {
            draw_pixels(bk.cover, lines_[1].image, x + 14, cy, 90, 0);
            tx += 104;
        }
        text(tx, cy, tr("hand.title_label"), 1, ink);
        if (title.empty()) draw_null_title(tx, cy + 14, 2);
        else text(tx, cy + 14, fit(title, x + pw - 14 - tx, 2), 2, ink);
        cy += bk.cover.empty() ? 44 : 100;
    }
    const size_t cols2 = size_t((pw - 28) / 16), cols1 = size_t((pw - 28) / 8);
    if (bk.model) cy = draw_model(*bk.model, x, cy, pw, y + ph - 110);
    else if (bk.parts) cy = draw_book(*bk.parts, x, cy, pw, y + ph - 110);
    else if (bk.is_file)
    {
        text(x + 14, cy, trf("hand.binary.kind", {file_type(bk.head, bk.file_size)}), 2, theme().edge);
        cy = draw_file(file_of(bk), x, cy + 24, pw, y + ph - 110);
    }
    else switch (line().kind)
    {
    case LineKind::Text:
        for (const auto& l : wrap(utf8_encode(line().space.text_of(u)), cols2))
        {
            text(x + 14, cy, l, 2, ink);
            cy += 20;
            if (cy > y + ph - 120) break;
        }
        break;
    case LineKind::Audio:
        for (const auto& l : wrap(notes_to_notation(note_set_of(line().space.symbols_id()), u), cols2))
        {
            if (cy > y + ph - 150) { text(x + 14, cy, "...", 2, ink); cy += 20; break; }
            text(x + 14, cy, l, 2, ink);
            cy += 20;
        }
        text(x + 14, cy + 6, tr("hand.play"), 1, ink);
        cy += 20;
        break;
    case LineKind::Image:
    case LineKind::Video:
    {
        const int frames = int(line().image.frames);
        const int frame = frames > 1 ? int((SDL_GetTicks() / 250) % Uint64(frames)) : 0;
        // What is left above the address and the keys (the cover and title may have taken some).
        const float size = std::max(40.0f, std::min(y + ph - 120 - cy, pw - 28));
        draw_pixels(u, x + 14, cy, size, frame);
        if (frames > 1)
            text(x + 24 + size, cy, trf("hand.frame", {std::to_string(frame + 1), std::to_string(frames)}), 1, ink);
        cy += size + 10;
        break;
    }
    }
    cy = std::max(cy, y + ph - 110);
    const std::string kind = bk.survivor ? "hand.compact_address" : "hand.address";
    text(x + 14, cy,
         bk.guided ? trf(kind + ".guided", {std::to_string(bk.bits)}) : trf(kind, {tr(std::string("ordering.") + to_string(mode_))}), 1, ink);
    cy += 12;
    int shown = 0;
    // (At most six lines of it are shown, so a file's address, as long as the file, is cut short.)
    const std::string whole = bk.guided ? bk.own_hex : hex_of(bk);
    for (const auto& l : wrap(whole.size() > 6 * cols1 + 1 ? whole.substr(0, 6 * cols1 + 1) : whole, cols1))
    {
        if (++shown > 6) { text(x + 14, cy, "...", 1, ink); break; }
        text(x + 14, cy, l, 1, ink);
        cy += 10;
    }
    text(x + 14, y + ph - 16, tr(bk.parts ? "hand.keys.book" : bk.model ? "hand.keys.model" : bk.is_file ? "hand.keys.file" : "hand.keys"), 1, ink);
}

// COST: what it costs to name the thing in your hand.
//
// There is one number underneath all of this -- the unit's index, somewhere in [0, N) -- and
// every row is that same number written a different way. The point of showing them together
// is that the first three are the same length: an address in a bijection is the content, not
// a handle on it. Only the guided ordering is shorter, and only when the content is likely
// under the model, which is why its percentage doubles as a measure of how text-like the
// thing in your hand is.
void Hallway::draw_cost(const Book& bk, float x, float cy, float pw, float bottom)
{
    const Theme& th = theme();
    const SDL_Color ink = th.edge, dim = mix(th.edge, th.bg, 0.45f);
    const BigUint& n = on_books() ? books_->size() : titled_here() ? titled_here()->size() : line().space.size();
    const double bits = n.log10_approx() * 3.321928094887362;
    auto row = [&](const std::string& what, double b, const std::string& written, const std::string& note) {
        text(x + 14, cy, what, 1, ink);
        text(x + 260, cy, trf("cost.bits", {fixed(b, 0)}), 1, ink);
        text(x + 380, cy, written, 1, ink);
        if (bits > 0) text(x + 500, cy, trf("cost.percent", {fixed(b / bits * 100.0, 1)}), 1, b < bits * 0.995 ? ink : dim);
        if (!note.empty()) text(x + 580, cy, note, 1, dim);
        cy += 14;
    };
    auto digits_in = [](double b, double per) { return std::to_string(int(std::ceil(b / per))); };

    text(x + 14, cy, tr("cost.head"), 1, dim);
    cy += 18;
    row(tr("cost.unit"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.unit.note"));
    cy += 6;
    row(tr("ordering.positional"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.same"));
    row(tr("ordering.scrambled"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.shuffled"));
    // The same number stored as raw bytes, eight bits to a byte: half the hex, and what an
    // installer file holds (SPECIFICATIONS §12.2).
    row(tr("cost.raw"), bits, trf("cost.bytes", {digits_in(bits, 8)}), tr("cost.raw.note"));
    // The guided length for this unit, whatever ordering you are walking in. It exists only
    // on a text line with a model behind it.
    if (!on_books() && !on_models() && !on_binary() && line().guided)
    {
        try
        {
            const auto code = line().guided->code(bk.unit);
            row(tr("ordering.guided"), double(code.bits), trf("cost.chars", {std::to_string(code.hex.size())}),
                double(code.bits) < bits * 0.9 ? tr("cost.likely") : tr("cost.unlikely"));
        }
        catch (const std::exception&)
        {
        }
    }
    cy += 10;
    // The other half of a written-down key: the shape that gives the address its meaning.
    const std::string spec = (on_books() ? books_->id() : titled_here() ? titled_here()->id() : line().space.id());
    text(x + 14, cy, tr("cost.spec"), 1, dim);
    cy += 14;
    for (const auto& l : wrap(spec, size_t((pw - 28) / 8)))
    {
        text(x + 14, cy, l, 1, ink);
        cy += 12;
    }
    text(x + 14, cy, trf("cost.spec.len", {std::to_string(spec.size())}), 1, dim);
    cy += 20;
    // How long the address is in each of the ways it could be written. Sieve writes hex; the
    // others are here because the choice is open and this is the measurement that settles it.
    text(x + 14, cy, tr("cost.written"), 1, dim);
    cy += 14;
    for (const auto& [name, per] : {std::pair<const char*, double>{"hex", 4.0}, {"base32", 5.0}, {"base64", 6.0},
                                    {"base85", 6.409390936137702}})
    {
        text(x + 14, cy, name, 1, ink);
        text(x + 260, cy, trf("cost.chars", {digits_in(bits, per)}), 1, ink);
        cy += 12;
    }
    text(x + 14, bottom - 16, tr("hand.keys.cost"), 1, ink);
}

// A book open in hand: the cover beside the title, then the current page. Returns the height used.
float Hallway::draw_book(const BookSpace::Parts& p, float x, float cy, float pw, float bottom)
{
    const SDL_Color ink = theme().edge;
    const float cover = 170;
    draw_pixels(p.cover, lines_[1].image, x + 14, cy, cover, 0);
    const float tx = x + 14 + cover + 20;
    const size_t tcols = size_t(std::max(10.0f, (x + pw - 14 - tx) / 16));
    text(tx, cy, tr("hand.title_label"), 1, ink);
    float ty = cy + 14;
    const std::string whole = utf8_encode(line().space.text_of(p.title));
    const auto title = is_blank_title(whole) ? std::vector<std::string>{} : wrap_words(whole, tcols);
    if (title.empty()) draw_null_title(tx, ty, 2);
    for (size_t r = 0; r < title.size() && r < 6; ++r, ty += 20) text(tx, ty, title[r], 2, ink);
    if (title.size() > 6) text(tx, ty, "...", 2, ink);
    cy += cover + 14;
    const uint32_t n = books_->pages();
    if (n == 0)
    {
        text(x + 14, cy, tr("hand.no_pages"), 1, ink);
        return cy + 14;
    }
    text(x + 14, cy, trf("hand.page", {std::to_string(book_page_ + 1), std::to_string(n)}), 1, ink);
    cy += 14;
    // The page at the largest scale that fits.
    const std::string page = utf8_encode(line().space.text_of(p.pages[size_t(book_page_)]));
    for (float scale : {2.0f, 1.0f})
    {
        const size_t cols = size_t((pw - 28) / (8 * scale));
        const auto rows = wrap_words(page, cols);
        const float lh = 10 * scale;
        if (scale > 1 && cy + rows.size() * lh > bottom) continue;
        for (const auto& r : rows)
        {
            if (cy + lh > bottom) { text(x + 14, cy, "...", scale, ink); cy += lh; break; }
            text(x + 14, cy, r, scale, ink);
            cy += lh;
        }
        break;
    }
    return cy;
}

// A file in hand: its length, then a hex dump of as much of it as fits, sixteen bytes to a row
// with the offset first and the printable bytes beside, as any hex viewer shows a file. Returns
// the height used.
float Hallway::draw_file(const BinarySpace::Bytes& f, float x, float cy, float pw, float bottom)
{
    const SDL_Color ink = theme().edge;
    text(x + 14, cy, trf("hand.binary.bytes", {std::to_string(f.size())}), 2, ink);
    cy += 24;
    if (f.empty())
    {
        text(x + 14, cy, tr("hand.binary.empty"), 1, ink);
        return cy + 14;
    }
    const size_t per = pw >= 620 ? 16 : 8;
    char b[24];
    for (size_t at = 0; at < f.size(); at += per)
    {
        if (cy + 12 > bottom)
        {
            text(x + 14, cy, trf("hand.binary.more", {std::to_string(f.size() - at)}), 1, ink);
            cy += 12;
            break;
        }
        std::snprintf(b, sizeof b, "%06zx", at);
        std::string row = std::string(b) + "  ";
        std::string chars;
        for (size_t i = at; i < at + per; ++i)
        {
            if (i < f.size())
            {
                std::snprintf(b, sizeof b, "%02x ", f[i]);
                row += b;
                chars += f[i] >= 0x20 && f[i] < 0x7F ? char(f[i]) : '.';
            }
            else row += "   ";
        }
        text(x + 14, cy, row + " " + chars, 1, ink);
        cy += 12;
    }
    return cy + 6;
}

} // namespace hallway::hall
