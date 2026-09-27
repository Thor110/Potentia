// Sieve hallway -- the models line's crate faces: each crate's model rendered small and printed
// on its front, for your room and the rooms either side, by worker threads so that no frame waits
// for one; rooms further off wear the face of the same slot in your room.

#include "hallway.hpp"

namespace hallway::hall {

// The model of one crate, drawn small: faces back to front, shaded by depth, on nothing, and
// every vertex as a dot on top. The dots are what make a model with no face of its own -- one
// whose vertices have all come out in the same place, as they do near the start of a long
// line in positional order -- show as a point rather than as a crate nobody has rendered.
// Everything it needs is passed in, because it runs on the render workers, not the frame.
void Hallway::render_crate_face(const ModelSpace& space, const ModelSpace::Parts& p, int n, float spin, float tilt,
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

void Hallway::start_crate_workers()
{
    if (!crate_workers_.empty()) return;
    const unsigned n = std::clamp(std::thread::hardware_concurrency(), 2u, 6u) - 1;
    for (unsigned i = 0; i < n; ++i)
        crate_workers_.emplace_back([this] {
            std::vector<uint32_t> px;
            for (;;)
            {
                CrateJob job;
                {
                    std::unique_lock<std::mutex> lock(crate_mx_);
                    crate_cv_.wait(lock, [this] { return crate_stop_ || !crate_jobs_.empty(); });
                    if (crate_stop_) return;
                    job = std::move(crate_jobs_.front());
                    crate_jobs_.pop_front();
                }
                try
                {
                    render_crate_face(*model_space_, job.parts, job.px, job.spin, job.tilt, job.edge, px);
                }
                catch (const std::exception&)
                {
                    px.assign(size_t(job.px) * size_t(job.px), 0u); // an empty face rather than none
                }
                std::lock_guard<std::mutex> lock(crate_mx_);
                crate_done_.push_back({job.place, job.generation, job.px, std::move(px)});
            }
        });
}

void Hallway::stop_crate_workers()
{
    {
        std::lock_guard<std::mutex> lock(crate_mx_);
        crate_stop_ = true;
        crate_jobs_.clear();
    }
    crate_cv_.notify_all();
    for (auto& t : crate_workers_) t.join();
    crate_workers_.clear();
}

void Hallway::clear_crate_faces()
{
    for (auto& [key, cf] : crate_)
        if (cf.tex) SDL_DestroyTexture(cf.tex);
    crate_.clear();
    // Anything asked for or finished before now is for a field that no longer exists.
    std::lock_guard<std::mutex> lock(crate_mx_);
    ++crate_generation_;
    crate_jobs_.clear();
    crate_done_.clear();
    crate_pending_.clear();
}

// How many images are kept: the rooms that get faces, or fewer if the budget in Settings >
// Graphics (megabytes, which is what costs) will not hold that many.
size_t Hallway::crate_capacity() const
{
    const size_t rooms = size_t(2 * kCrateRooms + 1) * sieve::books_per_tile();
    return std::min(rooms, std::max<size_t>(8, size_t(crate_budget_mb_) * 1024 * 1024 / crate_bytes()));
}

// Room for one more face: a face left behind in a room outside the window goes first, then
// the one seen longest ago.
void Hallway::make_crate_room()
{
    const int64_t per = int64_t(sieve::books_per_tile());
    const auto outside = [per](int64_t k) {
        const int64_t t = k >= 0 ? k / per : -((-k + per - 1) / per);
        return t < -kCrateRooms || t > kCrateRooms;
    };
    while (!crate_.empty() && crate_.size() >= crate_capacity())
    {
        auto oldest = crate_.begin();
        for (auto it = crate_.begin(); it != crate_.end(); ++it)
        {
            const bool out_it = outside(it->first), out_old = outside(oldest->first);
            if (out_it != out_old ? out_it : it->second.used < oldest->second.used) oldest = it;
        }
        if (oldest->second.tex) SDL_DestroyTexture(oldest->second.tex);
        crate_.erase(oldest);
    }
}

// Uploads what the workers have finished, for as long as this frame allows.
void Hallway::collect_crate_faces(Uint64 until)
{
    std::vector<CrateDone> done;
    {
        std::lock_guard<std::mutex> lock(crate_mx_);
        done.swap(crate_done_);
    }
    const int64_t per = int64_t(sieve::books_per_tile());
    size_t k = 0;
    for (; k < done.size() && (k == 0 || SDL_GetTicksNS() < until); ++k)
    {
        CrateDone& d = done[k];
        crate_pending_.erase(d.place);
        if (d.generation != crate_generation_ || d.px != crate_px_) continue;
        const int64_t key = d.place - crate_shift_ * per;
        if (key < -int64_t(kCrateRooms) * per || key >= int64_t(kCrateRooms + 1) * per || crate_.count(key)) continue;
        SDL_Texture* tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, d.px, d.px);
        if (!tex) continue;
        SDL_UpdateTexture(tex, nullptr, d.pixels.data(), d.px * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        make_crate_room();
        crate_.emplace(key, CrateFace{tex, portal_frame_});
    }
    if (k < done.size())
    {
        // The rest wait for the next frame.
        std::lock_guard<std::mutex> lock(crate_mx_);
        crate_done_.insert(crate_done_.end(), std::make_move_iterator(done.begin() + std::ptrdiff_t(k)),
                           std::make_move_iterator(done.end()));
    }
}

// The image for one crate, if it is rendered. If not, and `ask` is true, the crate's model is
// worked out here (that part is the frame's) and handed to the workers to draw. Returns
// whether it asked, so the caller can keep to its allowance.
SDL_Texture* Hallway::crate_face(int64_t dt, uint32_t slot, bool ask, bool& asked)
{
    asked = false;
    const int64_t per = int64_t(sieve::books_per_tile());
    const int64_t key = dt * per + slot;
    if (auto it = crate_.find(key); it != crate_.end())
    {
        it->second.used = portal_frame_;
        return it->second.tex;
    }
    const int64_t place = key + crate_shift_ * per;
    if (!ask || crate_pending_.count(place)) return nullptr;
    const Book& b = book(dt, slot);
    if (b.empty || !b.model) return nullptr;
    start_crate_workers();
    {
        std::lock_guard<std::mutex> lock(crate_mx_);
        crate_jobs_.push_back({place, crate_generation_, *b.model, crate_px_, model_spin_, model_tilt_, theme_of(kModelsLine).edge});
    }
    crate_pending_.insert(place);
    crate_cv_.notify_one();
    asked = true;
    return nullptr;
}

// Prints the rendered faces on the crates in view, and renders more for as long as this
// frame allows, spreading out from where you stand. Crates past the rendered rooms wear
// their stand-in.
void Hallway::draw_crate_faces(const bool* visible, int back, int ahead)
{
    // Every crate in the rendered rooms, in view before out of view, and then nearest you
    // first. Nearest you, not nearest the crosshair: when the cache is too small for three
    // rooms, which crates get faces must not change as you look around, or rows of faces
    // come and go with every glance.
    struct Want { int64_t order; int64_t dt; uint32_t slot; };
    std::vector<Want> want;
    for (int t = -kCrateRooms; t <= kCrateRooms; ++t)
    {
        const uint32_t books = books_in_tile(t);
        for (uint32_t k = 0; k < books; ++k)
        {
            const BookSlot bs = BookSlot::of(t, k);
            Vec3 f[4];
            book_face(float(t) * kTile, bs.side, bs.row, bs.col, f, sizes_vary());
            const Vec3 d = (f[0] + f[2]) * 0.5f - cam_.pos;
            const bool seen = t >= -back && t <= ahead && visible[t + back];
            want.push_back({(seen ? 0 : int64_t(1) << 40) + int64_t(dot(d, d) * 1000.0f), t, k});
        }
    }
    std::sort(want.begin(), want.end(), [](const Want& a, const Want& b) { return a.order < b.order; });
    if (want.size() > crate_capacity())
    {
        // Say so once for each setting, rather than leave it to look like a fault.
        const uint64_t setting = uint64_t(crate_px_) << 32 | crate_budget_mb_;
        if (setting != crate_warned_)
        {
            crate_warned_ = setting;
            const size_t need_mb = (want.size() * crate_bytes() + (1u << 20) - 1) >> 20;
            message(trf("msg.crate_cache_short", {std::to_string(crate_capacity()), std::to_string(want.size()),
                                                  std::to_string(crate_px_), std::to_string(need_mb)}));
        }
        want.resize(crate_capacity());
    }

    const Uint64 start = SDL_GetTicksNS();
    const Uint64 until = start + Uint64(kCrateMsPerFrame * 1e6);
    collect_crate_faces(until);
    // The workers are kept a little ahead, not given the whole field at once, so that what
    // they draw next is still what is nearest when they get to it.
    const size_t keep_ahead = 2 * std::max<size_t>(1, crate_workers_.size());
    std::vector<SDL_Vertex> verts;
    auto draw = [&](SDL_Texture* tex, int64_t dt, uint32_t slot) {
        const BookSlot bs = BookSlot::of(dt, slot);
        Vec3 f[4];
        book_face(float(dt) * kTile, bs.side, bs.row, bs.col, f, sizes_vary());
        draw_face_image(tex, f, verts);
    };
    bool asked_one = false;
    for (const Want& w : want)
    {
        // At least one model a frame is worked out and handed on, however long it takes, so
        // the field always fills; then more while the allowance lasts and the workers are
        // not already far enough ahead.
        const bool ask = (!asked_one || SDL_GetTicksNS() < until) && crate_pending_.size() < keep_ahead;
        bool asked = false;
        SDL_Texture* tex = crate_face(w.dt, w.slot, ask, asked);
        asked_one = asked_one || asked;
        const bool seen = w.dt >= -back && w.dt <= ahead && visible[w.dt + back];
        if (tex && seen) draw(tex, w.dt, w.slot);
    }
    // The stand-ins: the rooms in view past the rendered ones borrow the face of the same
    // slot in your room, once it has one.
    for (int t = -back; t <= ahead; ++t)
    {
        if ((t >= -kCrateRooms && t <= kCrateRooms) || !visible[t + back]) continue;
        const uint32_t books = books_in_tile(t);
        for (uint32_t k = 0; k < books; ++k)
            if (auto it = crate_.find(int64_t(k)); it != crate_.end() && it->second.tex) draw(it->second.tex, t, k);
    }
}

} // namespace hallway::hall
