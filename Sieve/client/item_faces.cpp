// Sieve hallway -- the pictures on the items' fronts. Every line's items show what they are: a
// page its text, an image its picture, a video its first frame, a book its title over its cover,
// a model its mesh. Each is drawn small for your room and the rooms either side, by worker
// threads so that no frame waits for one, and rooms further off wear the picture of the same
// slot in your room until you reach them. Where on the item the picture goes is faces.ini, beside
// the meshes. Titled lines carry their title across the top, and audio and video show their cover
// under it, as books do.

#include "hallway.hpp"
#include "gpu_memory.hpp"
#include "cli/timings.hpp"

namespace hallway::hall {

namespace {

void fill_rect(std::vector<uint32_t>& px, int w, int h, int x0, int y0, int x1, int y1, uint32_t colour)
{
    x0 = std::max(x0, 0), y0 = std::max(y0, 0), x1 = std::min(x1, w), y1 = std::min(y1, h);
    for (int y = y0; y < y1; ++y)
        std::fill(px.begin() + std::ptrdiff_t(size_t(y) * size_t(w) + size_t(x0)),
                  px.begin() + std::ptrdiff_t(size_t(y) * size_t(w) + size_t(x1)), colour);
}

// A title without the blank padding at either end, which is what a short title is padded with.
std::u32string trimmed(std::u32string t)
{
    while (!t.empty() && (t.back() == U' ' || t.back() == 0)) t.pop_back();
    size_t i = 0;
    while (i < t.size() && (t[i] == U' ' || t[i] == 0)) ++i;
    return t.substr(i);
}

// One frame of a picture unit, as large as fits the rectangle while keeping its shape, centred,
// each of its pixels a block (nearest neighbour): a picture of a few dozen pixels a side should
// look like one, not like a blur.
// `down` places it in the rectangle's height when it is wider than tall: 0.5 centred, 1 at the
// foot, which is where a cover sits under its title.
void paint_picture(std::vector<uint32_t>& px, int w, int h, const Space::Digits& unit, const ImageFormat& f, int x0, int y0,
                   int x1, int y1, float down = 0.5f)
{
    if (f.width == 0 || f.height == 0 || unit.size() < size_t(f.width) * f.height) return;
    const auto rgb = render_image(unit, f); // every frame; the first is at the start
    const float s = std::min(float(x1 - x0) / float(f.width), float(y1 - y0) / float(f.height));
    const float pw = float(f.width) * s, ph = float(f.height) * s;
    const float ox = float(x0) + (float(x1 - x0) - pw) * 0.5f, oy = float(y0) + (float(y1 - y0) - ph) * down;
    for (int y = std::max(0, int(oy)); y < std::min(h, int(oy + ph)); ++y)
    {
        const uint32_t sy = std::min(f.height - 1, uint32_t((float(y) + 0.5f - oy) / s));
        for (int x = std::max(0, int(ox)); x < std::min(w, int(ox + pw)); ++x)
        {
            const uint32_t sx = std::min(f.width - 1, uint32_t((float(x) + 0.5f - ox) / s));
            const Rgb& c = rgb[size_t(sy) * f.width + sx];
            px[size_t(y) * size_t(w) + size_t(x)] = 0xFF000000u | uint32_t(c.r) << 16 | uint32_t(c.g) << 8 | uint32_t(c.b);
        }
    }
}

// Text laid out in the rectangle a character to a square cell, row after row, in cells as large
// as fit all of it. Characters are drawn where the cells are large enough to read, and as a short
// bar where they are not, so a long page still looks like a page of writing: the words' shapes
// are there even when the letters cannot be.
void paint_text(std::vector<uint32_t>& px, int w, int h, const std::u32string& text, int x0, int y0, int x1, int y1, uint32_t ink,
                float letters)
{
    size_t n = text.size();
    while (n > 0 && (text[n - 1] == U' ' || text[n - 1] == 0)) --n; // the padding is not writing
    if (n == 0 || x1 <= x0 || y1 <= y0) return;
    const float area = float(x1 - x0) * float(y1 - y0);
    float cell = std::max(0.5f, std::floor(std::sqrt(area / float(n)) * 4.0f) / 4.0f);
    while (cell > 0.5f && std::floor(float(x1 - x0) / cell) * std::floor(float(y1 - y0) / cell) < float(n)) cell -= 0.25f;
    const int cols = std::max(1, int(float(x1 - x0) / cell));
    int col = 0, row = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const char32_t ch = text[i];
        if (ch == U'\n')
        {
            col = 0;
            ++row;
            continue;
        }
        const float x = float(x0) + float(col) * cell, y = float(y0) + float(row) * cell;
        if (y + cell > float(y1) + 0.01f) break;
        if (ch != U' ' && ch != 0)
        {
            if (cell >= letters) paint_glyph(px.data(), w, h, x, y, cell, cell, ch, ink);
            else
                fill_rect(px, w, h, int(x), int(y + cell * 0.35f), std::max(int(x) + 1, int(x + cell * 0.8f)),
                          std::max(int(y + cell * 0.35f) + 1, int(y + cell * 0.7f)), ink);
        }
        if (++col >= cols)
        {
            col = 0;
            ++row;
        }
    }
}

// A title across the top of an item, on a band of its own: returns where the rest begins.
// The band is drawn whenever the line has titles, even when this title is blank (all spaces, as
// the first titles of a line in positional order are), so every item on a titled line shows
// where its title goes; a blank one reads [Null Title], white brackets and red words, as it does
// in the item's window. The title is set as large as it can be in
// at most a third of the item's height, choosing how many rows to break it into for that, so a
// title of a few dozen characters stays readable on a small picture rather than shrinking to
// the short bars paint_text falls back to.
int paint_title_band(std::vector<uint32_t>& px, int w, int h, const std::u32string& given, bool titled, uint32_t band,
                     uint32_t ink, const std::u32string& null_words, float letters)
{
    if (!titled) return 0;
    const bool blank = given.empty();
    const std::u32string title = blank ? U"[" + null_words + U"]" : given;
    if (blank) band = 0xFF141414u; // [Null Title] stays on one row, on a dark band, so its colours read on any item
    const int m = std::max(1, w / 24);
    const float room_w = float(w - 2 * m);
    const size_t n = std::max<size_t>(1, title.size());
    // The largest cell over every way of breaking the title into rows, in `room_h` of height.
    size_t rows = 1;
    auto best = [&](float room_h) {
        float c = 0;
        for (size_t r = 1; r <= (blank ? 1 : n); ++r)
        {
            const size_t cols = (n + r - 1) / r;
            const float cr = std::min(room_w / float(cols), room_h / float(r));
            if (cr > c)
            {
                c = cr;
                rows = r;
            }
            if (room_h / float(r) < c) break; // more rows only make the cells smaller
        }
        return c;
    };
    // A third of the item, or up to half when a third is too little for letters of the smallest
    // size (a short item such as an audio track's front, at a small picture size).
    float cell = best(float(h / 3 - 2 * m));
    if (cell < letters) cell = best(float(h / 2 - 2 * m));
    // No larger than a sixteenth of the width, or a short title on a large display would fill
    // the top of it with a few huge letters and leave the page below too little room.
    if (cell > room_w / 16.0f)
    {
        cell = std::max(letters, room_w / 16.0f);
        rows = (n + size_t(room_w / cell) - 1) / std::max<size_t>(1, size_t(room_w / cell));
    }
    cell = std::max(1.0f, std::floor(cell));
    const int bh = std::max(2 * m + 2, int(float(rows) * cell) + 2 * m);
    fill_rect(px, w, h, 0, 0, w, bh, band);
    {
        const size_t cols = (n + rows - 1) / rows;
        const float x0 = float(m) + (room_w - float(cols) * cell) * 0.5f; // centred, like a title
        for (size_t i = 0; i < title.size(); ++i)
        {
            const float x = x0 + float(i % cols) * cell, y = float(m) + float(i / cols) * cell;
            if (title[i] == U' ' || title[i] == 0) continue;
            const uint32_t c = !blank ? ink : (i == 0 || i + 1 == title.size()) ? 0xFFFFFFFFu : 0xFFFF4646u;
            if (cell >= letters) paint_glyph(px.data(), w, h, x, y, cell, cell, title[i], c);
            else fill_rect(px, w, h, int(x), int(y + cell * 0.35f), std::max(int(x) + 1, int(x + cell * 0.8f)),
                           std::max(int(y + cell * 0.35f) + 1, int(y + cell * 0.7f)), c);
        }
    }
    return bh;
}

// The front of anything with a cover -- a book, an audio track, a film: its title across the top
// and its cover in a dark frame below, sitting at the foot, on the line's own colour.
void paint_cover_front(std::vector<uint32_t>& px, int w, int h, const std::u32string& title, const Space::Digits& cover,
                       const ImageFormat& cf, uint32_t ground, uint32_t ink, const std::u32string& null_words, float letters)
{
    px.assign(size_t(w) * size_t(h), ground);
    const int top = paint_title_band(px, w, h, title, true, ground, ink, null_words, letters);
    const int m = std::max(1, w / 24);
    // The frame is the cover's own shape, as wide as the item allows, at the foot.
    const float s = cf.width && cf.height ? std::min(float(w - 2 * m) / float(cf.width), float(h - top - 2 * m) / float(cf.height)) : 0;
    const int fw = int(float(cf.width) * s), fh = int(float(cf.height) * s);
    const int x0 = (w - fw) / 2, y1 = h - m, y0 = std::max(top + m, y1 - fh);
    fill_rect(px, w, h, x0 - 1, y0 - 1, x0 + fw + 1, y1 + 1, 0xFF141414u);
    paint_picture(px, w, h, cover, cf, x0, y0, x0 + fw, y1, 1.0f);
}

} // namespace

const FaceRect& Hallway::face_rect()
{
    const std::string medium = kDimensions[li_].id; // its section of faces.ini
    auto it = face_rects_.find(medium);
    if (it == face_rects_.end()) it = face_rects_.emplace(medium, load_face_rect(medium)).first;
    return it->second;
}

// What to draw on the item in a slot, captured by value: the frame goes on while it is drawn.
Hallway::Painter Hallway::face_painter(const Book& b) const
{
    if (b.empty) return {};
    const std::u32string nw = utf8_decode(tr("title.null")); // looked up here: the painters run off the main thread
    const float lp = float(letters_px_);
    if (on_models())
    {
        if (!b.model) return {};
        const ModelSpace* space = model_space_.get();
        const ModelSpace::Parts parts = *b.model;
        const SDL_Color edge = theme_of(kModelsLine).edge;
        const std::u32string title = utf8_decode(title_text(b));
        const bool titled = has_titles();
        // The title, then the mesh, drawn square, as wide as the picture, centred in what is left.
        return [space, parts, edge, title, titled, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            std::vector<uint32_t> sq;
            render_model_face(*space, parts, w, kModelSpin, kModelTilt, edge, sq);
            px.assign(size_t(w) * size_t(h), 0u);
            const int top = paint_title_band(px, w, h, title, titled, 0xFF1C1C1Cu, argb(edge), nw, lp);
            const int oy = top + (h - top - w) / 2;
            for (int y = 0; y < w; ++y)
                if (y + oy >= top && y + oy < h)
                    std::copy(sq.begin() + std::ptrdiff_t(size_t(y) * size_t(w)), sq.begin() + std::ptrdiff_t(size_t(y + 1) * size_t(w)),
                              px.begin() + std::ptrdiff_t(size_t(y + oy) * size_t(w)));
        };
    }
    if (on_books())
    {
        if (!b.parts) return {};
        // A book's front: its title on a band across the top, its cover below.
        const Space::Digits cover = b.parts->cover;
        const ImageFormat cf = unit_line(LineKind::Image).image;
        const std::u32string title = trimmed(unit_line(LineKind::Text).space.text_of(b.parts->title));
        return [cover, cf, title, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            paint_cover_front(px, w, h, title, cover, cf, argb({150, 26, 26, 255}), argb({235, 225, 205, 255}), nw, lp);
        };
    }
    if (b.is_file)
    {
        // A file: its title across the top, and below it, large, what kind of file it is, read
        // from its own first bytes (ZIP, PNG, TXT, ... or "?"), so the label is right whatever its
        // title says. Its size beneath, small. Nothing here is part of its address.
        const std::u32string kind = utf8_decode(file_type(b.head, b.file_size));
        const std::u32string size = utf8_decode(std::to_string(b.file_size) + (b.file_size == 1 ? " byte" : " bytes"));
        const std::u32string title = utf8_decode(title_text(b));
        const bool titled = has_titles();
        const SDL_Color c = theme_of(li_).edge;
        const uint32_t ground = argb({Uint8(c.r / 3), Uint8(c.g / 3), Uint8(c.b / 3), 255}), ink = argb({235, 225, 205, 255});
        return [kind, size, title, titled, ground, ink, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            px.assign(size_t(w) * size_t(h), ground);
            const int top = paint_title_band(px, w, h, title, titled, 0xFF1C1C1Cu, ink, nw, lp);
            // One line of square cells, as wide as fits in four fifths of the width and half the
            // height left, centred. Always letters, never the bars paint_text falls back to: the
            // label is a few characters and drawn large.
            const float room_w = float(w) * 0.8f, room_h = float(h - top);
            const float cell = std::max(1.0f, std::min(room_w / float(std::max<size_t>(3, kind.size())), room_h * 0.5f));
            const float x0 = (float(w) - cell * float(kind.size())) * 0.5f, y0 = float(top) + (room_h - cell) * 0.45f;
            for (size_t i = 0; i < kind.size(); ++i) paint_glyph(px.data(), w, h, x0 + float(i) * cell, y0, cell, cell, kind[i], ink);
            const float small = std::max(1.0f, std::min(cell / 4.0f, float(w) * 0.9f / float(size.size())));
            if (small >= lp)
            {
                const float sx = (float(w) - small * float(size.size())) * 0.5f, sy = y0 + cell * 1.15f;
                for (size_t i = 0; i < size.size(); ++i)
                    if (size[i] != U' ') paint_glyph(px.data(), w, h, sx + float(i) * small, sy, small, small, size[i], ink);
            }
        };
    }
    if (b.unit.empty()) return {};
    const std::u32string title = utf8_decode(title_text(b));
    const bool titled = has_titles();
    // Audio and video show their cover as a book does, on a darker shade of the line's colour; the
    // rest show themselves.
    if (!b.cover.empty())
    {
        const Space::Digits cover = b.cover;
        const ImageFormat cf = unit_line(LineKind::Image).image;
        const SDL_Color c = theme_of(li_).edge;
        const uint32_t ground = argb({Uint8(c.r / 3), Uint8(c.g / 3), Uint8(c.b / 3), 255});
        return [cover, cf, title, ground, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            paint_cover_front(px, w, h, title, cover, cf, ground, argb({235, 225, 205, 255}), nw, lp);
        };
    }
    switch (line().kind)
    {
    case LineKind::Text:
    {
        // A page: paper, its title across the top, and its text.
        const std::u32string text = line().space.text_of(b.unit);
        return [text, title, titled, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            px.assign(size_t(w) * size_t(h), argb({230, 224, 204, 255}));
            const uint32_t ink = argb({40, 36, 32, 255});
            const int top = paint_title_band(px, w, h, title, titled, argb({196, 184, 150, 255}), ink, nw, lp);
            const int m = std::max(1, w / 16);
            paint_text(px, w, h, text, m, top + m, w - m, h - m, ink, lp);
        };
    }
    case LineKind::Image:
    case LineKind::Video:
    {
        // A canvas: its title across the top, and the picture (a film without a cover, the first frame).
        const Space::Digits unit = b.unit;
        const ImageFormat f = line().image;
        return [unit, f, title, titled, nw, lp](std::vector<uint32_t>& px, int w, int h) {
            px.assign(size_t(w) * size_t(h), 0xFF000000u);
            const int top = paint_title_band(px, w, h, title, titled, 0xFF1C1C1Cu, argb({235, 225, 205, 255}), nw, lp);
            paint_picture(px, w, h, unit, f, 0, top, w, h);
        };
    }
    default: return {};
    }
}

// The model of one crate, drawn small: faces back to front, shaded by depth, on nothing, and
// every vertex as a dot on top. The dots are what make a model with no face of its own -- one
// whose vertices have all come out in the same place, as they do near the start of a long
// line in positional order -- show as a point rather than as a crate nobody has rendered.
// Everything it needs is passed in, because it runs on the render workers, not the frame.
void Hallway::render_model_face(const ModelSpace& space, const ModelSpace::Parts& p, int n, float spin, float tilt,
                              SDL_Color edge, std::vector<uint32_t>& px)
{
    px.assign(size_t(n) * size_t(n), 0u);
    const auto verts = space.mesh_of(p);
    const auto faces = space.faces_of(p);
    const float ca = std::cos(spin), sa = std::sin(spin);
    const float ct = std::cos(tilt), st = std::sin(tilt);
    const float half = float(n) * 0.5f, r = float(n) * 0.30f;
    auto project = [&](const ModelSpace::Vertex& v) {
        const float x = v.x * ca + v.z * sa, z = -v.x * sa + v.z * ca;
        const float y = v.y * ct - z * st, depth = v.y * st + z * ct;
        const float k = 1.0f / (2.4f - depth * 0.45f);
        return std::array<float, 3>{half + x * r * k * 2.4f, half - y * r * k * 2.4f, depth};
    };
    auto shade = [&](float t) {
        return 0xFF000000u | (uint32_t(float(edge.r) * t) << 16) | (uint32_t(float(edge.g) * t) << 8) | uint32_t(float(edge.b) * t);
    };
    std::vector<std::array<float, 3>> at(verts.size());
    for (size_t v = 0; v < verts.size(); ++v) at[v] = project(verts[v]);
    std::vector<std::pair<float, size_t>> order;
    order.reserve(faces.size());
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const auto& f = faces[i];
        if (f.a == f.b || f.b == f.c || f.a == f.c) continue; // a degenerate face has no face
        order.emplace_back((at[f.a][2] + at[f.b][2] + at[f.c][2]) / 3.0f, i);
    }
    std::sort(order.begin(), order.end());
    for (const auto& [depth, i] : order)
    {
        const auto& f = faces[i];
        fill_triangle(px, n, at[f.a], at[f.b], at[f.c], shade(std::clamp(0.30f + depth * 0.55f, 0.10f, 1.0f)));
    }
    // The dots, a pixel or two across whatever the size of the picture.
    const int dot = std::max(1, n / 128);
    for (const auto& a : at)
    {
        const int cx = int(a[0]), cy = int(a[1]);
        for (int y = cy - dot / 2; y < cy - dot / 2 + dot; ++y)
            for (int x = cx - dot / 2; x < cx - dot / 2 + dot; ++x)
                if (x >= 0 && y >= 0 && x < n && y < n) px[size_t(y) * size_t(n) + size_t(x)] = shade(1.0f);
    }
}

// A flat triangle into the crate's little image, one row at a time: each row is filled
// between where its centre line crosses the triangle's edges, so the cost is the pixels the
// triangle covers rather than the whole box around it, which at 512 pixels is most of the
// difference.
void Hallway::fill_triangle(std::vector<uint32_t>& px, int n, const std::array<float, 3>& a, const std::array<float, 3>& b,
                          const std::array<float, 3>& c, uint32_t argb)
{
    const float miny = std::min({a[1], b[1], c[1]}), maxy = std::max({a[1], b[1], c[1]});
    const int y0 = std::max(0, int(std::floor(miny))), y1 = std::min(n - 1, int(std::ceil(maxy)));
    const std::array<float, 3>* e[3][2] = {{&a, &b}, {&b, &c}, {&c, &a}};
    for (int y = y0; y <= y1; ++y)
    {
        const float fy = float(y) + 0.5f;
        float xl = 1e30f, xr = -1e30f;
        for (const auto& edge : e)
        {
            const auto& p = *edge[0];
            const auto& q = *edge[1];
            if ((p[1] <= fy && q[1] > fy) || (q[1] <= fy && p[1] > fy))
            {
                const float x = p[0] + (fy - p[1]) * (q[0] - p[0]) / (q[1] - p[1]);
                xl = std::min(xl, x);
                xr = std::max(xr, x);
            }
        }
        if (xl > xr) continue;
        const int x0 = std::max(0, int(std::ceil(xl - 0.5f))), x1 = std::min(n - 1, int(std::floor(xr - 0.5f)));
        uint32_t* row = px.data() + size_t(y) * size_t(n);
        for (int x = x0; x <= x1; ++x) row[x] = argb;
    }
}

double Hallway::face_ms_per_frame() const
{
    float hz = 0; // (60 where SDL cannot tell)
    if (SDL_Window* w = SDL_GetRenderWindow(r_))
        if (const SDL_DisplayMode* m = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(w))) hz = m->refresh_rate;
    return 1000.0 / double(hz > 0 ? hz : 60.0f) / 4.0;
}

void Hallway::start_face_workers()
{
    if (!face_workers_.empty()) return;
    // One a core but the one that draws (at least one; 0 when the system cannot tell).
    const unsigned n = std::max(2u, std::thread::hardware_concurrency()) - 1;
    for (unsigned i = 0; i < n; ++i)
        face_workers_.emplace_back([this] {
            std::vector<uint32_t> px;
            for (;;)
            {
                FaceJob job;
                {
                    std::unique_lock<std::mutex> lock(face_mx_);
                    face_cv_.wait(lock, [this] { return face_stop_ || !face_jobs_.empty(); });
                    if (face_stop_) return;
                    job = std::move(face_jobs_.front());
                    face_jobs_.pop_front();
                }
                try
                {
                    sieve::cli::timings::Scope timed("hallway.face"); // one item's picture, on a worker
                    job.paint(px, job.w, job.h);
                }
                catch (const std::exception&)
                {
                    px.clear();
                }
                if (px.size() != size_t(job.w) * size_t(job.h)) px.assign(size_t(job.w) * size_t(job.h), 0u); // an empty face rather than none
                std::lock_guard<std::mutex> lock(face_mx_);
                face_done_.push_back({job.place, job.generation, job.w, job.h, std::move(px), job.sharp});
            }
        });
}

void Hallway::stop_face_workers()
{
    {
        std::lock_guard<std::mutex> lock(face_mx_);
        face_stop_ = true;
        face_jobs_.clear();
    }
    face_cv_.notify_all();
    for (auto& t : face_workers_) t.join();
    face_workers_.clear();
}

void Hallway::clear_faces()
{
    for (auto& [key, cf] : faces_)
        if (cf.tex) gpu::destroy(cf.tex);
    faces_.clear();
    for (auto& [key, cf] : sharp_)
        if (cf.tex) gpu::destroy(cf.tex);
    sharp_.clear();
    sharp_pending_.clear();
    // Anything asked for or finished before now is for a field that no longer exists.
    std::lock_guard<std::mutex> lock(face_mx_);
    ++face_generation_;
    face_jobs_.clear();
    face_done_.clear();
    face_pending_.clear();
}

// How many images are kept: the rooms that get faces, or fewer if the budget in Settings >
// Graphics (megabytes, which is what costs) will not hold that many.
size_t Hallway::face_capacity() const
{
    const size_t rooms = size_t(2 * face_rooms() + 1) * sieve::books_per_tile();
    return std::min(rooms, std::max<size_t>(8, size_t(face_budget_mb_) * 1024 * 1024 / face_bytes()));
}

// Room for one more face: a face left behind in a room outside the window goes first, then
// the one seen longest ago.
void Hallway::make_face_room()
{
    const int64_t per = int64_t(sieve::books_per_tile());
    const int rooms = face_rooms();
    const auto outside = [per, rooms](int64_t k) {
        const int64_t t = k >= 0 ? k / per : -((-k + per - 1) / per);
        return t < -rooms || t > rooms;
    };
    while (!faces_.empty() && faces_.size() >= face_capacity())
    {
        auto oldest = faces_.begin();
        for (auto it = faces_.begin(); it != faces_.end(); ++it)
        {
            const bool out_it = outside(it->first), out_old = outside(oldest->first);
            if (out_it != out_old ? out_it : it->second.used < oldest->second.used) oldest = it;
        }
        if (oldest->second.tex) gpu::destroy(oldest->second.tex);
        faces_.erase(oldest);
    }
}

// Uploads what the workers have finished, for as long as this frame allows.
void Hallway::collect_faces(Uint64 until)
{
    std::vector<FaceDone> done;
    {
        std::lock_guard<std::mutex> lock(face_mx_);
        done.swap(face_done_);
    }
    const int64_t per = int64_t(sieve::books_per_tile());
    size_t k = 0;
    for (; k < done.size() && (k == 0 || SDL_GetTicksNS() < until); ++k)
    {
        FaceDone& d = done[k];
        if (d.sharp)
        {
            // A close-up replaces the one it was asked to improve on, if any.
            sharp_pending_.erase(d.place);
            if (d.generation != face_generation_) continue;
            SDL_Texture* tex = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, d.w, d.h, gpu::Use::Pictures);
            if (!tex) continue;
            SDL_UpdateTexture(tex, nullptr, d.pixels.data(), d.w * 4);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
            if (auto it = sharp_.find(d.place); it != sharp_.end() && it->second.tex) gpu::destroy(it->second.tex);
            sharp_[d.place] = Face{tex, portal_frame_, d.w};
            continue;
        }
        face_pending_.erase(d.place);
        if (d.generation != face_generation_ || d.w != face_w() || d.h != face_h()) continue;
        const int64_t key = d.place - face_shift_ * per;
        if (key < -int64_t(face_rooms()) * per || key >= int64_t(face_rooms() + 1) * per || faces_.count(key)) continue;
        SDL_Texture* tex = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, d.w, d.h, gpu::Use::Pictures);
        if (!tex) continue;
        SDL_UpdateTexture(tex, nullptr, d.pixels.data(), d.w * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        make_face_room();
        faces_.emplace(key, Face{tex, portal_frame_});
    }
    if (k < done.size())
    {
        // The rest wait for the next frame.
        std::lock_guard<std::mutex> lock(face_mx_);
        face_done_.insert(face_done_.end(), std::make_move_iterator(done.begin() + std::ptrdiff_t(k)),
                           std::make_move_iterator(done.end()));
    }
}

// The picture for one item, if it is drawn. If not, and `ask` is true, the item is worked out here
// (that part is the frame's) and handed to the workers as a painter. Returns whether it asked, so
// the caller can keep to its allowance.
SDL_Texture* Hallway::item_face(int64_t dt, uint32_t slot, bool ask, bool& asked)
{
    asked = false;
    const int64_t per = int64_t(sieve::books_per_tile());
    const int64_t key = dt * per + slot;
    if (auto it = faces_.find(key); it != faces_.end())
    {
        it->second.used = portal_frame_;
        return it->second.tex;
    }
    const int64_t place = key + face_shift_ * per;
    if (!ask || face_pending_.count(place)) return nullptr;
    const Book& b = book(dt, slot);
    if (effective_mode() == FilterMode::Hide && !b.passes) return nullptr; // it is not on the shelf
    if (effective_mode() == FilterMode::Excluded && b.passes) return nullptr; // nor, here, is a survivor
    if (b.withheld) return nullptr; // the vault: never drawn
    Painter paint = face_painter(b);
    if (!paint) return nullptr;
    start_face_workers();
    {
        std::lock_guard<std::mutex> lock(face_mx_);
        face_jobs_.push_back({place, face_generation_, face_w(), face_h(), std::move(paint), false});
    }
    face_pending_.insert(place);
    face_cv_.notify_one();
    asked = true;
    return nullptr;
}

// Prints the pictures on the items in view, and has more drawn for as long as this frame allows,
// spreading out from where you stand. Items past the rendered rooms wear their stand-in.
void Hallway::draw_item_faces(const bool* visible, int back, int ahead)
{
    const FaceRect rect = face_rect();
    if (rect.aspect() != face_aspect_)
    {
        face_aspect_ = rect.aspect(); // a new shape of picture: the old ones are the wrong shape
        clear_faces();
    }
    if (const int px = display_px_here(); px != line_px_)
    {
        line_px_ = px; // a line whose letters need a different width (a new line, or new settings)
        clear_faces();
    }
    // Every crate in the rendered rooms, in view before out of view, and then nearest you
    // first. Nearest you, not nearest the crosshair: when the cache is too small for three
    // rooms, which crates get faces must not change as you look around, or rows of faces
    // come and go with every glance.
    struct Want { int64_t order; int64_t dt; uint32_t slot; };
    std::vector<Want> want;
    for (int t = -face_rooms(); t <= face_rooms(); ++t)
    {
        const uint32_t books = books_in_tile(t);
        for (uint32_t k = 0; k < books; ++k)
        {
            const BookSlot bs = BookSlot::of(t, k);
            Vec3 f[4];
            picture_face(float(t) * kTile, bs.side, bs.row, bs.col, rect.bottom, rect.top, rect.half_width, f, sizes_vary());
            const Vec3 d = (f[0] + f[2]) * 0.5f - cam_.pos;
            const bool seen = t >= -back && t <= ahead && visible[t + back];
            want.push_back({(seen ? 0 : int64_t(1) << 40) + int64_t(dot(d, d) * 1000.0f), t, k});
        }
    }
    std::sort(want.begin(), want.end(), [](const Want& a, const Want& b) { return a.order < b.order; });
    if (want.size() > face_capacity())
    {
        // Say so once for each setting, rather than leave it to look like a fault.
        const uint64_t setting = uint64_t(line_px_) << 32 | face_budget_mb_;
        if (setting != face_warned_)
        {
            face_warned_ = setting;
            const size_t need_mb = (want.size() * face_bytes() + (1u << 20) - 1) >> 20;
            message(trf("msg.crate_cache_short", {std::to_string(face_capacity()), std::to_string(want.size()),
                                                  std::to_string(line_px_), std::to_string(need_mb)}));
        }
        want.resize(face_capacity());
    }

    const Uint64 start = SDL_GetTicksNS();
    const Uint64 until = start + Uint64(face_ms_per_frame() * 1e6);
    collect_faces(until);
    // The workers are kept a little ahead, not given the whole field at once, so that what
    // they draw next is still what is nearest when they get to it.
    const size_t keep_ahead = 2 * std::max<size_t>(1, face_workers_.size());
    std::vector<SDL_Vertex> verts;
    auto draw = [&](SDL_Texture* tex, int64_t dt, uint32_t slot) {
        const BookSlot bs = BookSlot::of(dt, slot);
        Vec3 f[4];
        picture_face(float(dt) * kTile, bs.side, bs.row, bs.col, rect.bottom, rect.top, rect.half_width, f, sizes_vary());
        draw_face_image(tex, f, verts);
    };
    bool asked_one = false;
    const int64_t per = int64_t(sieve::books_per_tile());
    size_t near = 0; // items close enough for a close-up, this frame
    const int closeup = closeup_px();
    const size_t sharp_max = this->sharp_max();
    for (const Want& w : want)
    {
        // At least one item a frame is worked out and handed on, however long it takes, so
        // the field always fills; then more while the allowance lasts and the workers are
        // not already far enough ahead.
        const bool ask = (!asked_one || SDL_GetTicksNS() < until) && face_pending_.size() < keep_ahead;
        bool asked = false;
        SDL_Texture* tex = item_face(w.dt, w.slot, ask, asked);
        asked_one = asked_one || asked;
        const bool seen = w.dt >= -back && w.dt <= ahead && visible[w.dt + back];
        if (!seen) continue;
        // A close-up, where the display is drawn wider on screen than it has pixels.
        if (closeup >= 2 * line_px_ && near < sharp_max)
        {
            const BookSlot bs = BookSlot::of(w.dt, w.slot);
            Vec3 f[4];
            picture_face(float(w.dt) * kTile, bs.side, bs.row, bs.col, rect.bottom, rect.top, rect.half_width, f, sizes_vary());
            const float sw = screen_width(f);
            if (sw > 1.25f * float(line_px_))
            {
                ++near;
                int level = 2 * line_px_;
                while (float(level) < sw && level < closeup) level *= 2;
                level = std::min(level, closeup);
                const int64_t place = w.dt * per + w.slot + face_shift_ * per;
                auto it = sharp_.find(place);
                if (it != sharp_.end())
                {
                    it->second.used = portal_frame_;
                    tex = it->second.tex; // better than the ordinary one, even at another level
                }
                // Up a level as you close in; down only when it is more than twice what is needed,
                // so standing still at a boundary does not redraw it back and forth.
                const bool want_new = it == sharp_.end() || level > it->second.w || it->second.w > 2 * level;
                if (want_new && !sharp_pending_.count(place) && sharp_pending_.size() < 2)
                    if (Painter paint = face_painter(book(w.dt, w.slot)))
                    {
                        start_face_workers();
                        {
                            std::lock_guard<std::mutex> lock(face_mx_);
                            // To the front: what is at your nose matters more than the far end of the field.
                            face_jobs_.push_front({place, face_generation_, level,
                                                   std::max(1, int(std::lround(float(level) * face_aspect_))), std::move(paint), true});
                        }
                        sharp_pending_.insert(place);
                        face_cv_.notify_one();
                    }
            }
        }
        if (tex) draw(tex, w.dt, w.slot);
    }
    drop_stale_sharp();
    // The stand-ins: the rooms in view past the rendered ones borrow the face of the same
    // slot in your room, once it has one.
    for (int t = -back; t <= ahead; ++t)
    {
        if ((t >= -face_rooms() && t <= face_rooms()) || !visible[t + back]) continue;
        const uint32_t books = books_in_tile(t);
        for (uint32_t k = 0; k < books; ++k)
            if (auto it = faces_.find(int64_t(k)); it != faces_.end() && it->second.tex) draw(it->second.tex, t, k);
    }
}

// How wide a display comes out on screen, in pixels: the longer of its top and bottom edges.
// 0 when any corner is behind you, which draw_face_image does not draw either.
float Hallway::screen_width(const Vec3 f[4]) const
{
    Point2 p[4];
    for (int i = 0; i < 4; ++i)
    {
        const Vec3 c = cam_.to_camera(f[i]);
        if (c.z < cam_.near_z + 0.01f) return 0;
        p[i] = cam_.project_camera(c);
    }
    const auto len = [](Point2 a, Point2 b) { return std::hypot(a.x - b.x, a.y - b.y); };
    return std::max(len(p[0], p[1]), len(p[3], p[2]));
}

// The widest a close-up is drawn now: the setting, or the screen's width rounded up to a power of
// two (display.hpp closeup_width).
int Hallway::closeup_px() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    return closeup_width(closeup_setting_, w, texture_px_);
}

// How many close-ups are kept: as many at their widest as the graphics memory holds beside the
// world (the renderer's three frames and what gpu_memory counts for it) and the display cache.
size_t Hallway::sharp_max() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    const double world = 3.0 * double(w) * double(h) * 4.0 + gpu::bytes(gpu::Use::World);
    const double room = graphics_bytes_ - world - double(face_budget_mb_) * 1048576.0;
    return closeup_count(room, closeup_px(), double(face_aspect_), sieve::books_per_tile());
}

// Close-ups unused for two seconds go, and then the least recently used while there are more
// than sharp_max().
void Hallway::drop_stale_sharp()
{
    const size_t keep = sharp_max();
    for (auto it = sharp_.begin(); it != sharp_.end();)
    {
        if (portal_frame_ - it->second.used > 120)
        {
            if (it->second.tex) gpu::destroy(it->second.tex);
            it = sharp_.erase(it);
        }
        else ++it;
    }
    while (sharp_.size() > keep)
    {
        auto oldest = sharp_.begin();
        for (auto it = sharp_.begin(); it != sharp_.end(); ++it)
            if (it->second.used < oldest->second.used) oldest = it;
        if (oldest->second.tex) gpu::destroy(oldest->second.tex);
        sharp_.erase(oldest);
    }
}

// The width the current line's displays are drawn at (display.hpp): the display size setting,
// or wider for the letters its items carry.
DisplayText Hallway::display_text_here() const
{
    const TitledSpace* ts = titled_here();
    const std::optional<Space>* ttl = on_composition() ? &comp().space->title_space() : ts ? &ts->title_space() : nullptr;
    const double title = ttl && *ttl ? double((*ttl)->unit_length()) : 0.0;
    if (on_books()) return display_text_books(double(unit_line(LineKind::Text).space.unit_length()));
    if (!on_models() && !on_binary() && line().kind == LineKind::Text) return display_text_pages(double(line().space.unit_length()), title);
    return display_text_titled(title);
}

int Hallway::display_px_here() const
{
    const DisplayText t = display_text_here();
    // As wide as the letters need, while every picture of the rooms with pictures still fits the
    // display cache (display.hpp widest_display_px).
    const double pictures = double(2 * face_rooms() + 1) * double(sieve::books_per_tile());
    const int widest = widest_display_px(double(face_budget_mb_) * 1048576.0, pictures, double(face_aspect_), texture_px_);
    return display_px(face_px_, letters_px_, double(face_aspect_), t, widest);
}

} // namespace hallway::hall
