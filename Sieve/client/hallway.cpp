// Sieve hallway -- the corridor itself: the lines and where you stand in them, the filters,
// finding and placing units, movement and doors, input, the frame, and Real Graphics. The parts
// drawn over it live in files of their own (hallway.hpp says which).

#include "hallway.hpp"

#include "cli/vault.hpp"
#include "cli/vault_decode.hpp"
#include "cli/timings.hpp"

#include <cstring>

namespace hallway::hall {

Hallway::Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines, const FilterConfig& filters, uint32_t book_pages,
        ModelShape shape)
: window_(window), r_(renderer), lines_(std::move(lines)), tile_geometry_(build_tile()), hall_geometry_(build_tile(true, false)), case_geometry_(build_tile(false, true)),
      bin_tile_{build_tile(true, true, -1), build_tile(true, true, 1)}, bin_hall_{build_tile(true, false, -1), build_tile(true, false, 1)},
      bin_case_{build_tile(false, true, -1), build_tile(false, true, 1)}, edge_geometry_{build_edge(1), build_edge(-1)},
      book_geometry_{build_books(false), build_books(true)}
{
    // The books line: a cover from the image line, a title and book_pages pages from the pages line.
    books_ = std::make_unique<BookSpace>(lines_[1].space, lines_[0].space, book_pages);
    // The models line: V vertices and F triangles on a grid of C steps (SPECIFICATIONS §12).
    model_space_ = std::make_unique<ModelSpace>(shape.vertices, shape.faces, shape.coords, lines_[0].space.key());
    // Every line but books and binary carries a title, and audio and video a cover from the image
    // line (SPECIFICATIONS §11, "Titled lines"). A title is written in the pages line's alphabet.
    {
        const std::string key = lines_[0].space.key();
        std::optional<Space> title; // a title length of 0 is no title
        if (shape.title_length > 0)
            title = lines_[0].alphabet ? Space(*lines_[0].alphabet, shape.title_length, key)
                                       : Space(lines_[0].space.symbols_id(), lines_[0].space.base(), shape.title_length, key);
        for (int i = 0; i < 4; ++i)
        {
            const Space& content = lines_[size_t(i)].space;
            std::optional<Space> cover;
            if (lines_[size_t(i)].kind == LineKind::Audio || lines_[size_t(i)].kind == LineKind::Video) cover = lines_[1].space;
            titled_[size_t(i)] = std::make_unique<TitledSpace>(title, cover, content.size(),
                                                               content.symbols_id() + "/L" + std::to_string(content.unit_length()), key);
        }
        titled_[kModelsLine] = std::make_unique<TitledSpace>(
            title, std::nullopt, model_space_->size(),
            "models/V" + std::to_string(shape.vertices) + "/F" + std::to_string(shape.faces) + "/C" + std::to_string(shape.coords), key);
        // The binary line: every file up to binary_bytes, titled, with a cover as audio and video have.
        binary_space_ = std::make_unique<BinarySpace>(std::max<uint64_t>(1, shape.binary_bytes), key);
        // (No cover: a file's kind is read from its own first bytes instead, and shown on its front.)
        titled_[kBinaryLine] = std::make_unique<TitledSpace>(title, std::nullopt, binary_space_->size(), binary_space_->shape(), key);
    }
    // The binary line's filters: by the files' own kinds (binary-kind-v1).
    binary_filters_ = filters.binary;
    modes_[kBinaryLine] = filters.binary.mode;
    rebuild_binary_sieve();
    // Each line's filter stack and mode (sieve-filters.ini, edited in the setup menu).
    for (int i = 0; i < 4; ++i)
    {
        modes_[i] = filters.lines[i].mode;
        try
        {
            stacks_[i] = build_stack(lines_[size_t(i)], filters.lines[i]);
        }
        catch (const std::exception& e)
        {
            std::cerr << "filters for the " << to_string(lines_[size_t(i)].kind) << " line: " << e.what() << "\n";
            modes_[i] = FilterMode::Off;
            continue;
        }
        try
        {
            // Compact needs the survivors in every ordering; if that fails, the line hides instead.
            const Ranker* rk = stacks_[i].ranker();
            if (rk && !rk->count().is_zero())
            {
                const Line& ln = lines_[size_t(i)];
                compact_[i] = std::make_unique<CompactLine>(*rk, ln.space.key(), stacks_[i].id(),
                                                            ln.guided ? ln.guided->model_ptr() : nullptr);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "compact for the " << to_string(lines_[size_t(i)].kind) << " line: " << e.what() << " (hiding instead)\n";
        }
    }
    // The books line: a stack per part (cover, title, and all pages as one text), one mode.
    modes_[kBooksLine] = filters.books.mode;
    try
    {
        book_stacks_ = build_book_stacks(lines_[1], lines_[0], book_pages, filters.books);
        book_sieve_ = std::make_unique<BookSieve>(*books_, book_stacks_.cover, book_stacks_.title, book_stacks_.pages);
    }
    catch (const std::exception& e)
    {
        std::cerr << "filters for the books line: " << e.what() << "\n";
        book_sieve_.reset();
        modes_[kBooksLine] = FilterMode::Off;
    }
    // Bounds of one tile's geometry (plus its doors), for skipping tiles out of view.
    tile_lo_ = {-kHalfWidth, 0, -0.5f};
    tile_hi_ = {kHalfWidth, kHeight, kTile};
    for (const Segment& g : tile_geometry_)
        for (const Vec3& v : {g.a, g.b})
        {
            tile_lo_ = {std::min(tile_lo_.x, v.x), std::min(tile_lo_.y, v.y), std::min(tile_lo_.z, v.z)};
            tile_hi_ = {std::max(tile_hi_.x, v.x), std::max(tile_hi_.y, v.y), std::max(tile_hi_.z, v.z)};
        }
    rebase();
}

// The current line's medium, and whether its books vary in size: pages, pictures and books do;
// records (audio) and tapes (video) are all one size, as the real things are (world.hpp).
Media Hallway::media() const
{
    if (on_books()) return Media::Books;
    if (on_models()) return Media::Models;
    if (on_binary()) return Media::Binary;
    switch (line().kind)
    {
    case LineKind::Image: return Media::Image;
    case LineKind::Audio: return Media::Audio;
    case LineKind::Video: return Media::Video;
    default: return Media::Pages;
    }
}

// The guided line in use: in compact mode, the one restricted to survivors.
const GuidedLine& Hallway::guided() const
{
    return effective_mode() == FilterMode::Compact ? *compact_[li_]->guided() : *line().guided;
}

void Hallway::set_line(int li)
{
    li_ = li;
    rebase();
    music_line();
}

void Hallway::music_line()
{
    music_colours(theme().bg, theme().edge);
    if (music()) music()->set_line(li_);
}

void Hallway::set_mode(AddressMode m)
{
    mode_ = m;
    rebase();
}

void Hallway::enable_guided()
{
    guided_ = true;
    rebase();
}

// Moves along the corridor by whole tiles (walking, jumping). Only the line you are on: every
// other line keeps where you last stood on it (the minimap's dots), and a door takes you to the
// same angle on the next (cross).
void Hallway::move_tiles(int64_t d)
{
    if (d == 0) return;
    tile_ += d;
    face_shift_ += d; // a crate's place stays the same number while its key moves
    loop_tile_ = offset_loop_tile(d);
    all_loop_tiles_[li_] = loop_tile_;
    door_back_.clear(); // moved: the way back is by angle again
    // Keep the books already worked out that are still near enough to be drawn: they are the
    // same books, d tiles closer. (Books left behind are dropped, so the cache stays small.)
    std::unordered_map<int64_t, Book> shifted;
    if (d > -16 && d < 16)
    {
        const int64_t by = d * int64_t(sieve::books_per_tile());
        const int64_t lo = -int64_t(kCacheBack) * int64_t(sieve::books_per_tile()), hi = int64_t(kCacheAhead + 1) * int64_t(sieve::books_per_tile());
        for (auto& [key, b] : cache_)
            if (key - by >= lo && key - by < hi) shifted.emplace(key - by, std::move(b));
        // The rendered crate faces move with them: the same models, d tiles closer.
        std::unordered_map<int64_t, Face> moved;
        for (auto& [key, cf] : faces_)
        {
            if (key - by >= lo && key - by < hi) moved.emplace(key - by, cf);
            else if (cf.tex) SDL_DestroyTexture(cf.tex);
        }
        faces_ = std::move(moved);
    }
    else clear_faces(); // too far to be the same shelves: start the field again
    cache_ = std::move(shifted);
    refresh_labels();
}

// ---- filters
//
// Each line has a stack and a mode: off, mark (failing books dimmed), hide (failing books
// left out, every address where it was), excluded (the other way round: passing books left out,
// so what the stack sets aside can be walked and checked) or compact (only survivors, closed up, in every
// ordering: positional by survivor number, scrambled by a keyed shuffle of those numbers,
// guided on the guided line restricted to survivors). Compact needs a stack that can rank its
// survivors; otherwise the line hides instead.
FilterMode Hallway::effective_mode(int i) const
{
    if (i == kBinaryLine)
    {
        if (!binary_sieve_ || binary_sieve_->empty()) return FilterMode::Off;
        if (modes_[i] != FilterMode::Compact) return modes_[i];
        return binary_sieve_->count().is_zero() ? FilterMode::Hide : FilterMode::Compact;
    }
    if (i == kBooksLine)
    {
        if (!book_sieve_ || book_sieve_->empty()) return FilterMode::Off;
        if (modes_[i] != FilterMode::Compact) return modes_[i];
        return books_compact() ? FilterMode::Compact : FilterMode::Hide;
    }
    const FilterStack& st = stacks_[i];
    if (st.empty()) return FilterMode::Off;
    const FilterMode m = modes_[i];
    if (m != FilterMode::Compact) return m;
    return compact_[i] ? FilterMode::Compact : FilterMode::Hide;
}

// The models line has no filters yet (SPECIFICATIONS §12 sets out the three tiers to come).
bool Hallway::has_filters() const
{
    if (on_models()) return false; // no filters there yet
    if (on_binary()) return binary_sieve_ && !binary_sieve_->empty();
    return on_books() ? book_sieve_ && !book_sieve_->empty() : !stack().empty();
}

std::string Hallway::compute_filter_status() const
{
    if (on_books())
    {
        if (!has_filters()) return tr("hud.filters.none");
        const FilterMode m = effective_mode();
        const auto parts = std::to_string(book_stacks_.cover.size()) + "+" + std::to_string(book_stacks_.title.size()) + "+" +
                           std::to_string(book_stacks_.pages.size());
        std::string s = trf("hud.filters", {parts, tr(std::string("mode.") + to_string(m))});
        if (m == FilterMode::Compact) s += trf("hud.filters.books", {short_big(book_sieve_->count())});
        else if (m == FilterMode::Excluded && book_sieve_->can_rank())
            s += trf("hud.filters.excluded", {short_big(BigUint(books_->size()) -= book_sieve_->count())});
        else if (modes_[li_] == FilterMode::Compact)
            s += book_sieve_->can_rank() ? tr("hud.filters.no_book") : trf("hud.filters.blocked", {book_sieve_->blocker()});
        return s;
    }
    if (on_binary())
    {
        if (!has_filters()) return tr("hud.filters.none");
        const FilterMode m = effective_mode();
        std::string st = trf("hud.filters", {std::to_string(binary_sieve_->size()), tr(std::string("mode.") + to_string(m))});
        if (m == FilterMode::Compact) st += trf("hud.filters.units", {short_big(binary_sieve_->count())});
        else if (m == FilterMode::Excluded)
            st += trf("hud.filters.excluded", {short_big(BigUint(binary_space_->size()) -= binary_sieve_->count())});
        else if (modes_[li_] == FilterMode::Compact) st += tr("hud.filters.no_unit");
        return st;
    }
    const FilterStack& st = stack();
    if (st.empty()) return tr("hud.filters.none");
    const FilterMode m = effective_mode();
    std::string s = trf("hud.filters", {std::to_string(st.size()), tr(std::string("mode.") + to_string(m))});
    if (m == FilterMode::Compact) s += trf("hud.filters.units", {short_big(st.ranker()->count())});
    else if (m == FilterMode::Excluded && st.ranker())
        s += trf("hud.filters.excluded", {short_big(BigUint(line().space.size()) -= st.ranker()->count())});
    else if (modes_[li_] == FilterMode::Compact)
        s += st.ranker() ? tr("hud.filters.no_unit") : trf("hud.filters.blocked", {st.compact_blocker()});
    return s;
}

// How many units line i has in its loop, in the current ordering and filter mode.
BigUint Hallway::units_of(int i) const
{
    // Binary's files stand on one wall, so its loop counts twice as many slots, the right wall's
    // empty (loop_pos()).
    if (i == kBinaryLine)
    {
        // Compact: only the surviving files, closed up (their titles blank, as a compact line's are).
        BigUint m = effective_mode(i) == FilterMode::Compact ? binary_sieve_->count() : titled_[kBinaryLine]->size();
        m -= BigUint(1);
        return loop_pos(m) += BigUint(1);
    }
    if (i == kModelsLine) return titled_[kModelsLine]->size();
    if (i == kBooksLine) return effective_mode(i) == FilterMode::Compact ? book_sieve_->count() : books_->size();
    if (guided_ && lines_[size_t(i)].guided) return BigUint::pow(2, zoom_);
    if (effective_mode(i) == FilterMode::Compact) return compact_[i]->count();
    return titled_[size_t(i)] ? titled_[size_t(i)]->size() : lines_[size_t(i)].space.size();
}

// The loop of the current line in the current ordering, and which of its tiles you are in.
// A full modulo is only needed when the line, ordering or zoom changes.
// The readout's tile and loop labels, worked out when they change rather than every frame.
// The compass's dots go with them: each one is a big-integer division of where you stand by
// how long the line is, and they only move when you do, so doing them here keeps them out of
// the draw path, where they would otherwise be repeated for all seven lines sixty times a
// second for an answer that had not changed.
void Hallway::refresh_labels()
{
    tile_label_ = short_big(tile_.magnitude, tile_.negative);
    loop_label_ = short_big(loop_.tiles());
    for (int i = 0; i < kLines; ++i) line_fraction_[i] = compute_line_fraction(i);
}

void Hallway::rebase()
{
    // The guided zoom can never be finer than the line's own precision (16 bits per character).
    if (lines_[0].guided) zoom_ = std::clamp<uint32_t>(zoom_, 1, uint32_t(std::min<size_t>(lines_[0].guided->scale_bits(), UINT32_MAX)));
    loop_ = LineLoop(units_of(li_));
    loop_tile_ = loop_.loop_tile(tile_);
    // Every line's loop too, and where you last stood on each: the line you are on is where you
    // are; the others keep their place (0 until you have been there), brought inside their loop
    // if a change of ordering or zoom has made it shorter.
    for (int i = 0; i < kLines; ++i)
    {
        all_loops_[i] = LineLoop(units_of(i));
        if (i == li_) all_loop_tiles_[i] = loop_tile_;
        else if (all_loop_tiles_[i] >= all_loops_[i].tiles()) all_loop_tiles_[i] = BigUint::mod(all_loop_tiles_[i], all_loops_[i].tiles());
    }
    cache_.clear();
    clear_faces();
    filter_status_ = compute_filter_status();
    refresh_labels();
}

// The first bytes of the file at a positional binary-v1 address, and its length, without
// converting the whole file. The file of L bytes is v - 0101...01 (L ones), so its first n bytes
// are the top of that difference: the top of v less n ones, less one more if the part of v below
// them is smaller than the ones below them (a borrow).
static std::vector<uint8_t> file_head(const BigUint& v, size_t most, uint64_t& size)
{
    auto ones = [](size_t n) {
        std::string h;
        h.reserve(2 * n);
        for (size_t i = 0; i < n; ++i) h += "01";
        return n ? BigUint::from_hex(h) : BigUint();
    };
    BigUint t = v;
    t.mul_small(255);
    t.add_small(1);
    const size_t length = (t.bit_length() - 1) / 8;
    size = length;
    if (length == 0) return {};
    const size_t n = std::min(most, length), shift = 8 * (length - n);
    BigUint top = v;
    top >>= shift;
    BigUint below = top;
    below <<= shift;
    BigUint low = v;
    low -= below;
    BigUint head = top;
    head -= ones(n);
    if (low < ones(length - n)) head -= BigUint(1);
    const std::string h = head.to_hex(2 * n);
    std::vector<uint8_t> out(n);
    auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
    for (size_t i = 0; i < n; ++i) out[i] = uint8_t(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
    return out;
}


const Hallway::Book& Hallway::book(int64_t dt, uint32_t slot)
{
    const int64_t key = dt * int64_t(sieve::books_per_tile()) + slot;
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    sieve::cli::timings::Scope timed("hallway.item"); // one item worked out: content, filters, vault
    if (cache_.size() > 4096) cache_.clear(); // more than a screenful (14 tiles of 128 books)
    // Thin: only your own room's books are kept, so a line of huge units keeps a room of them.
    if (thin_ && dt != 0)
        for (auto ci = cache_.begin(); ci != cache_.end();)
            ci = ci->first / int64_t(sieve::books_per_tile()) != 0 || ci->first < 0 ? cache_.erase(ci) : std::next(ci);
    Book b;
    const auto idx = loop_.unit_index(offset_loop_tile(dt), slot);
    if (!idx) b.empty = true;
    else try
    {
        b.index = *idx;
        const Space& sp = line().space;
        const bool compact_here = effective_mode() == FilterMode::Compact;
        if (on_books())
        {
            if (compact_here)
            {
                // Only surviving books stand here, closed up, like a compact line.
                b.parts = book_sieve_->parts_at(b.index, mode_);
                b.hex = book_sieve_->hex_of(b.index);
                b.survivor = true;
                b.survivor_number = mode_ == AddressMode::Positional ? b.index : book_sieve_->rank(*b.parts);
                b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - book_sieve_->count().log10_approx());
            }
            else
            {
                b.parts = books_->parts_at(b.index, mode_);
                b.hex = books_->hex_of(b.index);
                b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - books_->size().log10_approx());
                if (book_sieve_ && !book_sieve_->empty())
                {
                    b.failed_by = book_sieve_->first_failure(*b.parts);
                    b.passes = b.failed_by.empty();
                }
            }
        }
        else if (on_binary())
        {
            // A file: its title, its cover, and the file itself by its own positional index. Only
            // the left wall's slots hold files; the right wall is the edge.
            const uint32_t half = uint32_t(sieve::books_per_tile() / 2);
            if (slot >= half) b.empty = true;
            else if (compact_here)
            {
                // Only the surviving files stand here, closed up: slot i holds the survivor whose
                // compact address is i. Its bytes come from its survivor number (filekind.hpp).
                b.index = unit_of_pos(b.index);
                const BinarySpace::Bytes f = binary_sieve_->file_at(b.index, mode_);
                b.title = titled_[kBinaryLine]->blank_title();
                b.is_file = true;
                b.file_size = f.size();
                b.head.assign(f.begin(), f.begin() + std::ptrdiff_t(std::min<size_t>(16, f.size())));
                b.content = binary_space_->index_of(f, AddressMode::Positional);
                b.survivor = true;
                b.survivor_number = binary_sieve_->rank_of_index(b.index, mode_);
                b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - binary_sieve_->count().log10_approx());
            }
            else
            {
                b.index = unit_of_pos(b.index);
                const TitledSpace& ts = *titled_[kBinaryLine];
                const TitledSpace::Parts tp = ts.parts_at(b.index, mode_);
                b.title = tp.title;
                b.is_file = true; // its bytes and hex on demand: file_of(), hex_of()
                b.head = file_head(tp.content, 16, b.file_size);
                b.content = tp.content;
                b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - ts.size().log10_approx());
                if (binary_sieve_ && !binary_sieve_->empty())
                {
                    b.failed_by = binary_sieve_->first_failure(b.head, b.file_size);
                    b.passes = b.failed_by.empty();
                }
            }
        }
        else if (on_models())
        {
            // A titled model: its title, and the model itself by its own positional index.
            const TitledSpace& ts = *titled_[kModelsLine];
            const TitledSpace::Parts tp = ts.parts_at(b.index, mode_);
            b.title = tp.title;
            b.model = model_space_->parts_at(tp.content, AddressMode::Positional);
            b.hex = ts.hex_of(b.index);
            b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - ts.size().log10_approx());
        }
        else if (compact_here && !guided_on())
        {
            // Only survivors stand here: slot i holds the survivor whose compact address is i
            // (its survivor number, or in scrambled order a keyed shuffle of it).
            const CompactLine& cl = compact();
            b.unit = cl.unit_at(b.index, mode_);
            b.survivor = true;
            b.survivor_number = mode_ == AddressMode::Positional ? b.index : cl.ranker().rank(b.unit);
            b.hex = cl.hex_of(b.index);
            b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - cl.count().log10_approx());
        }
        else if (guided_on())
        {
            const GuidedLine& g = guided();
            BigUint point = b.index;
            point <<= g.scale_bits() - zoom_;
            b.guided = true;
            b.unit = g.unit_at(point);
            b.hex = g.hex_of(point, zoom_);
            b.fraction = g.fraction(point);
            const auto c = g.code(b.unit);
            b.bits = c.bits;
            b.own_hex = c.hex;
            if (compact_here)
            {
                b.survivor = true;
                b.survivor_number = compact().ranker().rank(b.unit);
            }
        }
        else if (const TitledSpace* ts = titled_here())
        {
            // A titled unit: its cover (audio and video), its title, and its content, which is the
            // unit of the line as it was, by its own positional index (its digits read as a number).
            const TitledSpace::Parts tp = ts->parts_at(b.index, mode_);
            b.cover = tp.cover;
            b.title = tp.title;
            b.unit = tp.content.to_digits(sp.base(), sp.unit_length());
            b.hex = ts->hex_of(b.index);
            b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - ts->size().log10_approx());
        }
        else
        {
            const auto address = b.index.to_digits(sp.base(), sp.unit_length());
            b.unit = sp.unit_of_address(address, mode_);
            b.hex = sp.hex_of(address);
            b.fraction = sp.fraction_of(address);
        }
        if (!on_books() && !on_models() && !b.survivor && !stack().empty())
        {
            sieve::cli::timings::Scope timed_filters("hallway.item.filters");
            const int fail = stack().first_failure(b.unit);
            b.passes = fail < 0;
            if (!b.passes) b.failed_by = stack().filter_name(size_t(fail));
        }
        if (b.survivor) b.survivor_label = short_big(b.survivor_number);
        sieve::cli::timings::Scope timed_vault("hallway.item.vault");
        b.withheld = vault_withholds(b); // the vault: kept in its place, never shown
    }
    catch (const std::exception& e)
    {
        // Never let one book stop the frame: show the slot as empty and say why.
        b = Book{};
        b.empty = true;
        message(trf("msg.cannot_show", {e.what()}));
    }
    return cache_.emplace(key, std::move(b)).first->second;
}

// The unit's position in its loop under the current ordering. In guided order the zoom is
// set to the length of the unit's address, so the unit sits exactly on a book.
const TitledSpace* Hallway::titled_of(int i) const
{
    if (i < 0 || i >= kLines || !titled_[size_t(i)]) return nullptr;
    if (i == kModelsLine || i == kBinaryLine) return titled_[size_t(i)].get();
    if (guided_ && lines_[size_t(i)].guided) return nullptr; // guided order: the content alone, for now
    if (effective_mode(i) == FilterMode::Compact) return nullptr; // compact: the content's survivors, for now
    return titled_[size_t(i)].get();
}

const TitledSpace* Hallway::titled_here() const { return titled_of(li_); }

std::string Hallway::title_text(const Book& b) const
{
    const TitledSpace* ts = titled_of(li_);
    if (!ts || b.title.empty() || !ts->title_space()) return {};
    std::u32string t = ts->title_space()->text_of(b.title);
    while (!t.empty() && (t.back() == U' ' || t.back() == 0)) t.pop_back();
    while (!t.empty() && (t.front() == U' ' || t.front() == 0)) t.erase(t.begin());
    return utf8_encode(t);
}

BigUint Hallway::index_of(const Space::Digits& unit)
{
    if (guided_on())
    {
        const GuidedLine& g = guided();
        const auto c = g.code(unit);
        zoom_ = uint32_t(std::max<size_t>(1, c.bits));
        BigUint i = c.point;
        i >>= g.scale_bits() - zoom_;
        return i;
    }
    if (effective_mode() == FilterMode::Compact) return compact().index_of(unit, mode_);
    // On a titled line a unit warped in on its own carries a blank title and a blank cover.
    if (const TitledSpace* ts = titled_here())
        return ts->index_of({ts->blank_cover(), ts->blank_title(), BigUint::from_digits(unit, line().space.base())}, mode_);
    return BigUint::from_digits(line().space.address_digits(unit, mode_), line().space.base());
}

// Go to a unit: its slot in the first copy of the loop, facing it. In compact mode a unit
// that fails the stack has no shelf; it is shown in hand instead (SPECIFICATIONS 6.4).
void Hallway::go_to_unit(const Space::Digits& unit, bool open)
{
    if (effective_mode() == FilterMode::Compact)
    {
        const int fail = stack().first_failure(unit);
        if (fail >= 0)
        {
            Book b;
            b.unit = unit;
            const auto address = line().space.address_digits(unit, AddressMode::Positional);
            b.hex = line().space.hex_of(address);
            b.fraction = line().space.fraction_of(address);
            b.passes = false;
            b.failed_by = stack().filter_name(size_t(fail));
            b.withheld = vault_withholds(b);
            in_hand_ = b;
            hand_tab_ = 0;
            in_hand_where_ = trf("hand.not_shelved", {b.failed_by});
            refuse_if_withheld();
            return;
        }
    }
    place(index_of(unit), open);
}

// The books line: go to a book, face it, and optionally open it.
// A file on the binary line, with a blank title and cover, as a unit warped in on its own has.
void Hallway::go_to_file(const BinarySpace::Bytes& f, bool open, const Space::Digits* title, const Space::Digits* cover)
{
    const TitledSpace& ts = *titled_[kBinaryLine];
    const BigUint content = binary_space_->index_of(f, AddressMode::Positional);
    if (effective_mode() == FilterMode::Compact)
    {
        // Compact: a surviving file by its compact address (its title is not kept there); one the
        // filters set aside has no shelf, and is shown in hand, as on the other lines.
        const std::string fail = binary_sieve_->first_failure(f, f.size());
        if (fail.empty())
        {
            place(binary_sieve_->index_of(f, mode_), open);
            return;
        }
        Book b;
        b.is_file = true;
        b.index = ts.index_of({ts.blank_cover(), title ? *title : ts.blank_title(), content}, AddressMode::Positional);
        b.title = title ? *title : ts.blank_title();
        b.content = content;
        b.file_size = f.size();
        b.head.assign(f.begin(), f.begin() + std::ptrdiff_t(std::min<size_t>(16, f.size())));
        b.passes = false;
        b.failed_by = fail;
        drop_in_hand();
        in_hand_ = b;
        hand_tab_ = 0;
        in_hand_where_ = trf("hand.not_shelved", {fail});
        refuse_if_withheld();
        return;
    }
    place(ts.index_of({cover ? *cover : ts.blank_cover(), title ? *title : ts.blank_title(), content}, mode_), open);
}

void Hallway::go_to_book(const BookSpace::Parts& p, bool open)
{
    if (effective_mode() == FilterMode::Compact)
    {
        const std::string fail = book_sieve_->first_failure(p);
        if (!fail.empty())
        {
            // Not on the shelves: shown in hand, with its full (uncompacted) address.
            Book b;
            b.parts = p;
            const BigUint index = books_->index_of(p, AddressMode::Positional);
            b.index = index;
            b.hex = books_->hex_of(index);
            b.fraction = index.is_zero() ? 0.0 : std::pow(10.0, index.log10_approx() - books_->size().log10_approx());
            b.passes = false;
            b.failed_by = fail;
            drop_in_hand();
            in_hand_ = b;
            hand_tab_ = 0;
            in_hand_where_ = trf("hand.not_shelved", {fail});
            book_page_ = 0;
            return;
        }
        place(book_sieve_->index_of(p, mode_), open);
    }
    else place(books_->index_of(p, mode_), open);
    book_page_ = 0;
}

// A book record (sieve bind) as a book of this line: its title, cover and pages must have the
// line's shape. A missing cover or title is shown blank, and missing pages are blank pages.
BookSpace::Parts Hallway::parts_of_record(const std::string& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    const std::string text((std::istreambuf_iterator<char>(in)), {});
    const sieve::cli::Book book = parse_book(text);
    const auto decoded = decode_book(book);
    if (book_id(decoded) != book.id) throw std::runtime_error("the book's content does not match its id");
    try
    {
        return record_parts(decoded, *books_);
    }
    catch (const std::invalid_argument& e)
    {
        throw std::runtime_error(trf("msg.book_shape", {e.what()}));
    }
}

// Teleport to a unit's slot in the first copy of the loop (where position = address), face
// it, and optionally open it.
void Hallway::place(const BigUint& unit, bool open)
{
    const BigUint index = on_binary() ? loop_pos(unit) : unit; // binary's files stand on one wall
    tile_ = LineLoop::tile_of(index);
    rebase();
    const uint32_t slot = LineLoop::slot_of(index);
    face(slot);
    drop_in_hand();
    book_page_ = 0;
    if (open)
    {
        in_hand_ = book(0, slot);
        hand_tab_ = 0;
        in_hand_where_ = trf("hand.where", {tile_label_, std::to_string(slot)});
        refuse_if_withheld();
    }
}

// Stand in the corridor facing the book in `slot` of the current tile.
void Hallway::face(uint32_t slot)
{
    const BookSlot b = BookSlot::of(0, slot);
    Vec3 f[4];
    book_face(0, b.side, b.row, b.col, f, sizes_vary());
    const Vec3 centre = (f[0] + f[2]) * 0.5f;
    const float sx = b.side == Side::Left ? 1.0f : -1.0f;
    cam_.pos = {0.3f * sx, 1.6f, centre.z + 1.2f};
    const Vec3 d = centre - cam_.pos;
    cam_.yaw = std::atan2(d.x, d.z);
    cam_.pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
}

void Hallway::message(const std::string& m)
{
    message_ = m;
    message_until_ = SDL_GetTicks() + 6000;
}

// The book the camera is looking at, or else the first book of the tile: the reference for
// changing ordering or zoom.
const Hallway::Book* Hallway::reference_book()
{
    if (hover_)
    {
        const Book& b = book(hover_->tile, hover_->slot());
        if (!b.empty) return &b;
    }
    const Book& b = book(0, 0);
    return b.empty ? nullptr : &b;
}

// positional -> scrambled -> guided (if this line has a model) -> positional, keeping the book
// you are looking at in front of you.
void Hallway::cycle_ordering()
{
    if (on_binary())
    {
        // Positional and scrambled only, keeping the file you are looking at, title, cover and all.
        const Book* ref = reference_book();
        const Book keep = ref ? *ref : Book{};
        if (keep.is_file) (void)file_of(keep); // worked out in the old ordering, before it changes
        mode_ = mode_ == AddressMode::Positional ? AddressMode::Scrambled : AddressMode::Positional;
        rebase();
        if (keep.is_file) go_to_file(file_of(keep), false, &keep.title, &keep.cover);
        message(trf(mode_ == AddressMode::Positional ? "msg.ordering.positional" : "msg.ordering.scrambled", {ordering_name()}));
        return;
    }
    if (on_books())
    {
        const Book* ref = reference_book();
        const auto parts = ref ? ref->parts : std::nullopt;
        mode_ = mode_ == AddressMode::Positional ? AddressMode::Scrambled : AddressMode::Positional;
        rebase();
        if (parts) go_to_book(*parts, false);
        message(trf(mode_ == AddressMode::Positional ? "msg.ordering.books_positional" : "msg.ordering.books_scrambled", {ordering_name()}));
        return;
    }
    const Book* ref = reference_book();
    const Space::Digits unit = ref ? ref->unit : Space::Digits{};
    if (guided_on()) { guided_ = false; mode_ = AddressMode::Positional; }
    else if (mode_ == AddressMode::Positional) mode_ = AddressMode::Scrambled;
    else if (line().guided) guided_ = true;
    else mode_ = AddressMode::Positional;
    rebase();
    if (ref) go_to_unit(unit, false);
    const char* what = guided_on() ? "msg.ordering.guided" : mode_ == AddressMode::Positional ? "msg.ordering.positional" : "msg.ordering.scrambled";
    message(trf(what, {ordering_name()}));
}

// Guided order: change the spacing of the books, keeping the point you are looking at.
void Hallway::zoom_by(int delta)
{
    if (!guided_on())
    {
        message(tr(line().guided && !on_books() ? "msg.zoom.press_m" : "msg.zoom.no_model"));
        return;
    }
    const Book* ref = reference_book();
    BigUint point;
    if (ref)
    {
        point = ref->index;
        point <<= guided().scale_bits() - zoom_;
    }
    zoom_ = uint32_t(std::clamp<int64_t>(int64_t(zoom_) + delta, 1, int64_t(guided().scale_bits())));
    point >>= guided().scale_bits() - zoom_;
    place(point, false);
    message(trf(delta < 0 ? "msg.zoom.out" : "msg.zoom.in",
                {std::to_string(zoom_), zoom_ < 64 ? "1/" + std::to_string(uint64_t(1) << zoom_) : "2^-" + std::to_string(zoom_)}));
}

bool Hallway::warp(const std::string& input)
{
    try
    {
        if (on_books())
        {
            // A book record, bound with sieve bind.
            go_to_book(parts_of_record(input), true);
            trail_.clear();
            message(trf("msg.opened_book", {input}));
            return true;
        }
        if (on_binary())
        {
            // A file, if the input names one; otherwise the text itself, as its UTF-8 bytes.
            BinarySpace::Bytes bytes;
            std::error_code ec;
            // The typed text is UTF-8; the path is built from it as UTF-8, so Windows finds the file too.
            const std::filesystem::path path(std::u8string(input.begin(), input.end()));
            Space::Digits title = titled_[kBinaryLine]->blank_title();
            if (std::filesystem::is_regular_file(path, ec))
            {
                std::ifstream in(path, std::ios::binary);
                bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
                const std::u8string name = path.filename().u8string();
                title = title_for_name(std::string(name.begin(), name.end())); // its name is its title
                walked_names_[cli::sha256_hex(bytes)] = std::string(name.begin(), name.end());
            }
            else bytes.assign(input.begin(), input.end());
            cli::vault::check_bytes(bytes, "the file"); // the vault: refused before it has a place
            go_to_file(bytes, true, &title);
            trail_.clear();
            message(trf("msg.warped.file", {std::to_string(bytes.size())}));
            return true;
        }
        Args a;
        a.opts["line"] = to_string(line().kind);
        if (line().kind == LineKind::Image || line().kind == LineKind::Video) a.opts["file"] = input;
        else a.positional = {input};
        const WarpInput w = read_warp_input(line(), a);
        if (w.units.empty()) throw std::invalid_argument(tr("msg.warp.empty"));
        trail_ = w.units;
        trail_index_ = 0;
        go_to_unit(trail_[0], true);
        message(trf(trail_.size() > 1 ? "msg.warped.trail" : "msg.warped", {w.report.front()}));
        return true;
    }
    catch (const std::exception& e)
    {
        message(trf("msg.warp.failed", {e.what()}));
        return false;
    }
}

// An address (hex), a percentage (P%), or a corridor tile (@T).
bool Hallway::go_to(std::string input)
{
    try
    {
        input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
        if (input.empty()) throw std::invalid_argument(tr("msg.goto.empty"));
        if (input[0] == '@')
        {
            tile_ = TileIndex::parse(input.substr(1));
            rebase();
            face(0);
            drop_in_hand();
            trail_.clear();
            message(trf("msg.goto.tile", {tile_label_}));
            return true;
        }
        BigUint index;
        if (input.back() == '%')
        {
            // "36.5%" -> 365 / 10^3
            const std::string num = input.substr(0, input.size() - 1);
            const size_t dot = num.find('.');
            std::string digits = num;
            uint32_t decimals = 2;
            if (dot != std::string::npos)
            {
                digits = num.substr(0, dot) + num.substr(dot + 1);
                decimals += uint32_t(num.size() - dot - 1);
            }
            if (digits.empty() || digits.size() > 9 || digits.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument(tr("msg.goto.percent"));
            // floor(p / 10^decimals * loop units)
            index = line_units();
            index.mul_small(uint32_t(std::stoul(digits)));
            for (uint32_t i = 0; i < decimals; ++i) index.divmod_small(10);
            if (index >= line_units()) throw std::invalid_argument(tr("msg.goto.below100"));
        }
        else if (guided_on())
        {
            const GuidedLine& g = guided();
            const BigUint point = g.point_of(input);
            zoom_ = uint32_t(std::clamp<size_t>(4 * input.size(), 1, g.scale_bits()));
            index = point;
            index >>= g.scale_bits() - zoom_;
        }
        else if (on_models()) index = titled_[kModelsLine]->parse(input);
        else if (on_binary()) index = effective_mode() == FilterMode::Compact ? binary_sieve_->parse(input) : titled_[kBinaryLine]->parse(input);
        else if (on_books()) index = effective_mode() == FilterMode::Compact ? book_sieve_->parse(input) : books_->parse(input);
        else if (effective_mode() == FilterMode::Compact) index = compact().parse(input); // a compact address, as the books show
        else if (const TitledSpace* ts = titled_here()) index = ts->parse(input);
        else index = BigUint::from_digits(line().space.parse_address(input), line().space.base());
        trail_.clear();
        refused_ = false;
        place(index, true);
        if (!refused_) message(trf("msg.goto.done", {input})); // else "withheld" stands
        return true;
    }
    catch (const std::exception& e)
    {
        message(trf("msg.goto.failed", {e.what()}));
        return false;
    }
}

void Hallway::step_trail(int dir)
{
    if (trail_.size() < 2) return;
    trail_index_ = (trail_index_ + trail_.size() + size_t(dir)) % trail_.size();
    go_to_unit(trail_[trail_index_], true);
    message(trf("msg.trail", {std::to_string(trail_index_ + 1), std::to_string(trail_.size())}));
}

// ---- movement and doors
void Hallway::update(float dt, const bool* keys)
{
    // Holding a book, you stand still: walking or turning under an open book is disorienting.
    if (input_ != Input::None || in_hand_ || nav_open_ || pause_open_ || loc_open_) return;
    const float speed = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 9.0f : 3.0f;
    const Vec3 fwd = {std::sin(cam_.yaw), 0, std::cos(cam_.yaw)};
    const Vec3 right = {std::cos(cam_.yaw), 0, -std::sin(cam_.yaw)};
    Vec3 move{};
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) move = move + fwd;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) move = move - fwd;
    if (keys[SDL_SCANCODE_D]) move = move + right;
    if (keys[SDL_SCANCODE_A]) move = move - right;
    if (keys[SDL_SCANCODE_LEFT]) cam_.yaw -= 1.8f * dt;
    if (keys[SDL_SCANCODE_RIGHT]) cam_.yaw += 1.8f * dt;
    if (dot(move, move) > 0) move_by(normalize(move) * (speed * dt));
}

void Hallway::move_by(Vec3 d)
{
    // A doorway lets you through the wall; the binary line's open side has no wall and no
    // door, so out there the short wall stops you wherever you are along the tile.
    auto in_door = [this](float x, float z) {
        if (on_binary() && x * drop_sign() > 0) return false;
        return z > kDoorStart + 0.15f && z < kDoorEnd - 0.15f;
    };
    const float lo = walk_lo(), hi = walk_hi();
    Vec3 p = cam_.pos + d;
    // Walls stop you except in a doorway; once in a doorway you cannot slide along inside the wall.
    if ((p.x < lo || p.x > hi) && !in_door(p.x, p.z))
    {
        if (cam_.pos.x < lo || cam_.pos.x > hi) p.z = cam_.pos.z;
        else p.x = std::clamp(p.x, lo, hi);
    }
    cam_.pos = p;
    while (cam_.pos.z >= kTile) { cam_.pos.z -= kTile; move_tiles(1); }
    while (cam_.pos.z < 0) { cam_.pos.z += kTile; move_tiles(-1); }
    // The door is in the wall, and on the binary line there is only one wall.
    const bool left_door = !on_binary() || binary_shelf_ == 0, right_door = !on_binary() || binary_shelf_ == 1;
    if (cam_.pos.x < -(kHalfWidth + 0.05f) && left_door) cross(Side::Left);
    else if (cam_.pos.x > kHalfWidth + 0.05f && right_door) cross(Side::Right);
}

void Hallway::jump_tiles(int64_t n)
{
    if (in_hand_)
    {
        message(tr("msg.holding"));
        return;
    }
    move_tiles(n);
    message(trf("msg.jumped", {std::to_string(n), std::to_string(n * int64_t(sieve::books_per_tile()))}));
}

// A door keeps your corridor position and changes the line reading it: left wall to the next
// line, right wall to the previous one. You come in through the opposite wall's door.
//
// The binary line is the exception. It has one wall, the left, and one door in it, and that door
// leads back to the line you came from. From pages you walk out of pages' right wall and into
// binary's left, as between any two lines. From models you walk out of models' left wall, and the
// only door in binary is also in its left wall: so you are turned round, and come out of that
// door facing into the room, with the line running the other way from you. Going back to models
// turns you round again. The room itself never turns: it is one line, met from either end.
void Hallway::cross(Side side)
{
    int to = side == Side::Left ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines;
    bool turn = false;
    if (on_binary())
    {
        to = binary_from_;
        turn = to != (li_ + 1) % kLines; // back to models, through its left wall
    }
    else if (to == kBinaryLine)
    {
        binary_from_ = li_;
        turn = side == Side::Left; // from models' left wall into binary's left wall
    }
    const float out = (kHalfWidth - 0.1f) * (side == Side::Left ? 1.0f : -1.0f);
    cam_.pos.x = turn ? -out : out;
    if (turn) cam_.yaw += 3.14159265f;
    drop_in_hand();
    trail_.clear(); // a warped trail belongs to the line it was warped on
    // The same angle on the next line: tile t of A's T_A tiles is tile floor(t * T_B / T_A) of B's
    // T_B, exactly, so it is the same place round the circle to the whole precision of the shorter
    // line. A shorter line cannot hold every place of a longer one, so going straight back (not
    // having moved) returns you to exactly the tile you left rather than to the nearest one, and
    // through several doors in a row, back through each of them in turn (a stack of doors, which
    // walking along the corridor clears).
    const BigUint here = loop_tile_;
    BigUint there;
    if (!door_back_.empty() && door_back_.back().on_line == li_ && door_back_.back().to_line == to && door_back_.back().on_tile == here)
    {
        there = door_back_.back().to_tile;
        door_back_.pop_back();
    }
    else
    {
        BigUint q, r;
        BigUint::divmod(BigUint::mul(here, all_loops_[to].tiles()), loop_.tiles(), q, r);
        there = q;
        door_back_.push_back(DoorBack{li_, here, to, there});
        if (door_back_.size() > 64) door_back_.erase(door_back_.begin());
    }
    tile_ = TileIndex{false, there};
    set_line(to);
    message(trf("msg.door", {tr(theme().key), tile_label_}));
}

// ---- input events

// Every key and click: anything that goes wrong becomes a message, never the end of the app.
void Hallway::handle(const SDL_Event& e, bool& quit)
{
    try
    {
        handle_event(e, quit);
    }
    catch (const std::exception& ex)
    {
        message(trf("status.error", {ex.what()}));
    }
}

void Hallway::handle_event(const SDL_Event& e, bool& quit)
{
    if (e.type == SDL_EVENT_QUIT) quit = true;
    if (graph_open_)
    {
        graph_event(e);
        return;
    }
    if (loc_open_)
    {
        locator_event(e);
        return;
    }
    if (media_open_) // over the pause menu, like the other tools
    {
        media_event(e, quit);
        return;
    }
    if (nav_open_) // before the pause menu: it may be open over it
    {
        navigator_event(e);
        return;
    }
    if (pause_open_)
    {
        pause_event(e, quit);
        return;
    }
    if (input_ != Input::None)
    {
        if (e.type == SDL_EVENT_TEXT_INPUT) text_ += e.text.text;
        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            if (e.key.key == SDLK_ESCAPE) close_input();
            else if (e.key.key == SDLK_BACKSPACE && !text_.empty())
            {
                // Remove one UTF-8 character.
                do text_.pop_back();
                while (!text_.empty() && (static_cast<unsigned char>(text_.back()) & 0xC0) == 0x80);
            }
            else if (e.key.key == SDLK_V && (e.key.mod & SDL_KMOD_CTRL))
            {
                if (char* clip = SDL_GetClipboardText()) { text_ += clip; SDL_free(clip); }
            }
            else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
            {
                const Input which = input_;
                const std::string t = text_;
                close_input();
                if (!t.empty()) which == Input::Warp ? warp(t) : go_to(t);
            }
        }
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode(window_))
    {
        // Holding a model, the mouse turns the model instead of you: it is in your hands. On the
        // SORT tab it turns the graph.
        if (in_hand_ && hand_tab_ == 2)
        {
            sort_yaw_ += e.motion.xrel * 0.008f;
            sort_pitch_ = std::clamp(sort_pitch_ + e.motion.yrel * 0.008f * (invert_y_ ? -1.0f : 1.0f), -1.5f, 1.5f);
        }
        else if (in_hand_ && in_hand_->model)
        {
            model_spin_ += e.motion.xrel * 0.008f;
            model_tilt_ = std::clamp(model_tilt_ + e.motion.yrel * 0.008f * (invert_y_ ? -1.0f : 1.0f), -1.5f, 1.5f);
        }
        else if (!in_hand_)
        {
            cam_.yaw += e.motion.xrel * look_;
            cam_.pitch = std::clamp(cam_.pitch - e.motion.yrel * look_ * (invert_y_ ? -1.0f : 1.0f), -1.45f, 1.45f);
        }
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (!SDL_GetWindowRelativeMouseMode(window_)) SDL_SetWindowRelativeMouseMode(window_, true);
        else if (e.button.button == SDL_BUTTON_LEFT) take_or_return();
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        // Touchpads send fractions of a notch: a tile per whole notch, however it arrives.
        wheel_ += e.wheel.y;
        const int notches = int(wheel_);
        if (notches != 0)
        {
            wheel_ -= float(notches);
            jump_tiles(notches);
        }
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    // Held keys repeat only for what is meant to repeat (jumps, zoom, pages); a held E would
    // otherwise take a book and put it back again and again.
    if (e.key.repeat)
        switch (e.key.key)
        {
        case SDLK_PAGEUP: case SDLK_PAGEDOWN: case SDLK_LEFTBRACKET: case SDLK_RIGHTBRACKET:
        case SDLK_MINUS: case SDLK_KP_MINUS: case SDLK_EQUALS: case SDLK_KP_PLUS: case SDLK_N: case SDLK_B: break;
        default: return;
        }
    switch (e.key.key)
    {
    case SDLK_ESCAPE:
        // Put down what is in your hands first; with nothing in them, pause.
        if (in_hand_) drop_in_hand();
        else open_pause();
        break;
    case SDLK_TAB: SDL_SetWindowRelativeMouseMode(window_, !SDL_GetWindowRelativeMouseMode(window_)); break;
    case SDLK_Q:
        if (e.key.mod & SDL_KMOD_CTRL) quit = true;
        break;
    case SDLK_E: take_or_return(); break;
    case SDLK_T: open_input(Input::Warp); break;
    case SDLK_G: open_input(Input::Goto); break;
    case SDLK_X: open_navigator(); break;
    case SDLK_O: open_graph(); break;
    case SDLK_M: cycle_ordering(); break;
    case SDLK_F1:
        menu_requested_ = true;
        quit = true;
        break;
    case SDLK_MINUS:
    case SDLK_KP_MINUS: zoom_by((e.key.mod & SDL_KMOD_SHIFT) ? -8 : -1); break;
    case SDLK_EQUALS:
    case SDLK_KP_PLUS: zoom_by((e.key.mod & SDL_KMOD_SHIFT) ? 8 : 1); break;
    case SDLK_N:
        if (in_hand_ && in_hand_->parts) turn_page(1);
        else step_trail(1);
        break;
    case SDLK_B:
        if (in_hand_ && in_hand_->parts) turn_page(-1);
        else step_trail(-1);
        break;
    case SDLK_A:
        if (in_hand_ && hand_tab_ == 2) sort_yaw_ -= 0.15f;
        else if (in_hand_ && in_hand_->model) model_spin_ -= 0.15f;
        break;
    case SDLK_D:
        if (in_hand_ && hand_tab_ == 2) sort_yaw_ += 0.15f;
        else if (in_hand_ && in_hand_->model) model_spin_ += 0.15f;
        break;
    case SDLK_C:
        // The item page's tabs: the thing itself, what it costs to name it, and where it stands
        // in a map (SORT).
        if (in_hand_)
        {
            hand_tab_ = (hand_tab_ + 1) % hand_tabs();
            if (hand_tab_ == 2)
            {
                const int node = sort_node(*in_hand_);
                GraphMap* g = graph_current();
                if (node >= 0 && g) message(trf("msg.sort.anchor", {std::to_string(node), g->map.nodes[size_t(node)].path, g->title}));
            }
        }
        break;
    case SDLK_R:
        if (in_hand_ && in_hand_->model) { model_spin_ = 0.6f; model_tilt_ = 0.35f; }
        break;
    case SDLK_P:
        if (in_hand_ && !on_books() && line().kind == LineKind::Audio)
        {
            // Through the music player when there is one, which fades the music out under it.
            const sieve::NoteSet set = note_set_of(line().space.symbols_id());
            const std::string err = music() ? music()->play_item(set, in_hand_->unit) : synth_.play(set, in_hand_->unit);
            message(err.empty() ? tr("msg.playing") : err);
        }
        break;
    case SDLK_PAGEUP: jump_tiles(1000); break;
    case SDLK_PAGEDOWN: jump_tiles(-1000); break;
    case SDLK_RIGHTBRACKET:
        if (in_hand_) graph_select(graph_sel_ + 1); // holding something: the map it is shown against, and V adds to
        else jump_tiles(1000000);
        break;
    case SDLK_LEFTBRACKET:
        if (in_hand_) graph_select(graph_sel_ - 1);
        else jump_tiles(-1000000);
        break;
    case SDLK_V:
        if (in_hand_) graph_add_anchor(*in_hand_);
        break;
    case SDLK_F:
        if (in_hand_) save_in_hand();
        break;
    case SDLK_J:
        if (in_hand_) jump_kind();
        break;
    case SDLK_HOME:
        tile_ = TileIndex{};
        rebase();
        face(0);
        drop_in_hand();
        message(tr("msg.home"));
        break;
    default: break;
    }
}

void Hallway::turn_page(int dir)
{
    const int n = int(books_->pages());
    if (n == 0) return;
    book_page_ = std::clamp(book_page_ + dir, 0, n - 1);
}

void Hallway::drop_in_hand()
{
    in_hand_.reset();
    synth_.stop();
    if (music()) music()->stop_item(); // and the music comes back
}

void Hallway::take_or_return()
{
    if (in_hand_) { drop_in_hand(); return; }
    take_hovered();
}

void Hallway::take_hovered()
{
    if (!hover_) return;
    const Book& b = book(hover_->tile, hover_->slot());
    if (b.empty) return;
    if (withheld(b))
    {
        message(tr("vault.withheld"));
        return;
    }
    in_hand_ = b;
    hand_tab_ = 0;
    book_page_ = 0;
    TileIndex t = tile_;
    t += hover_->tile;
    in_hand_where_ = trf("hand.where", {short_big(t.magnitude, t.negative), std::to_string(hover_->slot())});
}

void Hallway::open_input(Input which)
{
    input_ = which;
    text_.clear();
    SDL_StartTextInput(window_);
}

void Hallway::close_input()
{
    input_ = Input::None;
    SDL_StopTextInput(window_);
}

// One-line summary of where you are (for scripted walks and testing).
std::string Hallway::status()
{
    const Book& first = book(0, 0);
    std::string where = std::string(theme().name) + " line, " + ordering_name();
    if (guided_on()) where += " zoom " + std::to_string(zoom_);
    where += ", " + filter_status() + ", tile " + tile_label_ + ", x " + std::to_string(cam_.pos.x) + ", ";
    if (first.empty) where += "first slot empty (padding)";
    else
    {
        where += percent(first.fraction) + " along, first book " + short_address(hex_of(first));
        if (first.parts) where += " \"" + ascii(utf8_encode(line().space.text_of(first.parts->title))) + "\"";
        else if (first.model)
        {
            const auto fs = model_space_->faces_of(*first.model);
            std::vector<bool> seen(model_space_->vertices(), false);
            uint32_t used = 0, degenerate = 0;
            for (const auto& f : fs)
            {
                seen[f.a] = seen[f.b] = seen[f.c] = true;
                if (f.a == f.b || f.b == f.c || f.a == f.c) ++degenerate;
            }
            for (bool b : seen)
                if (b) ++used;
            where += " " + std::to_string(used) + "/" + std::to_string(model_space_->vertices()) + " vertices used, " +
                     std::to_string(degenerate) + " degenerate faces";
        }
        else if (first.is_file) where += " " + file_type(first.head, first.file_size) + " " + binary_preview(first.head, 16, first.file_size);
        else if (line().kind == LineKind::Text) where += " \"" + ascii(utf8_encode(line().space.text_of(first.unit))) + "\"";
        if (first.guided) where += " (" + std::to_string(first.bits) + " bits)";
    }
    return where + (message_.empty() ? "" : "  | " + message_);
}

// ---- drawing
void Hallway::render()
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r_, &w, &h);
    cam_.update(w, h);
    ++portal_frame_;
    item_save_poll(); // an item F asked to save, once the dialog has said where
    const Theme& th = theme();
    SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    // Nothing stands on the binary line's shelves, so there is nothing to look at or take.
    {
        const FaceRect& rect = face_rect(); // the item's front as its model has it
        hover_ = pick_book(cam_.pos, cam_.forward(), 0, 5.0f, sizes_vary(), rect.bottom, rect.top);
    }
    if (on_binary() && hover_ && hover_->side == Side::Right) hover_.reset(); // the edge: no shelves there
    if (hover_ && effective_mode() == FilterMode::Hide && !book(hover_->tile, hover_->slot()).passes) hover_.reset();
    if (hover_ && effective_mode() == FilterMode::Excluded && book(hover_->tile, hover_->slot()).passes) hover_.reset();

    constexpr int kBack = kCacheBack, kAhead = kCacheAhead;
    // Only tiles that can appear on screen are drawn (usually about half of them).
    bool visible[kBack + kAhead + 1];
    for (int t = -kBack; t <= kAhead; ++t)
    {
        const Vec3 shift{0, 0, t * kTile};
        visible[t + kBack] = cam_.box_visible(tile_lo_ + shift, tile_hi_ + shift);
    }
    // Real Graphics: the line's models, where it has them; the rest stays wireframe.
    fps_tris_ = 0;
    const Models* md = real_graphics_ ? &models() : nullptr;
    const bool real_hall = md && md->hallway, real_cases = md && md->bookshelf, real_books = md && md->book,
               real_marker = md && md->marker;
    bool models_drawn = false;
    if (md && (real_hall || real_cases || real_books || real_marker))
    {
        draw_models(*md, visible, kBack, kAhead, w, h);
        models_drawn = true;
    }
    // The portals are drawn over that image, so they test against its depth (see draw_portal).
    const float* depth = models_drawn ? models_batch_.depth() : nullptr;
    // Doors: solid black. Start lines: checkered, where a loop of this line begins.
    // Far tile first, so a nearer door's black face covers the one behind it.
    std::vector<SDL_Vertex> sign_verts;
    for (int t = kAhead; t >= -kBack; --t)
    {
        if (!visible[t + kBack]) continue;
        const float z0 = t * kTile;
        for (float sx : {-1.0f, 1.0f})
        {
            // The binary line has one wall, so it has one door: nothing to draw on the side
            // the floor ends at.
            if (on_binary() && sx == drop_sign()) continue;
            const float x = sx * kHalfWidth;
            const std::vector<Vec3> door = {{x, 0, z0 + kDoorStart}, {x, 0, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorStart}};
            if (!real_hall) fill(door, SDL_Color{0, 0, 0, 255});
            // A door leads to the next line on the left wall and the previous one on the
            // right (cross()), so each portal takes the colour of the line behind it.
            if (door_portals_)
                draw_portal(door, theme_of(door_to(sx)),
                            depth, models_batch_.width(), models_batch_.height());
            draw_door_sign(sx, z0, sign_verts);
        }
        if (!real_marker && offset_loop_tile(t).is_zero())
        {
            draw_start_line(z0);
            if (all_start(t)) draw_start_line(z0 + 1.0f); // every line starts here: a double flag
        }
    }
    // Every edge, faded towards the background with distance. Padding slots have no book.
    // (The buckets are kept between frames, so their memory is reused.)
    std::vector<std::vector<SDL_FPoint>>& buckets = edge_buckets_;
    buckets.resize(kBuckets);
    for (auto& b : buckets) b.clear();
    // `faint` is the least fade an edge gets (0 = full strength, 1 = background).
    auto add = [&](const Segment& s, float z0, float faint = 0) {
        const auto p = cam_.project_segment({s.a.x, s.a.y, s.a.z + z0}, {s.b.x, s.b.y, s.b.z + z0});
        if (!p) return;
        const float fade = std::clamp(std::max((p->second - 10.0f) / 45.0f, faint), 0.0f, 1.0f);
        auto& b = buckets[size_t(std::min(kBuckets - 1, int(fade * kBuckets)))];
        b.push_back({p->first.first.x, p->first.first.y});
        b.push_back({p->first.second.x, p->first.second.y});
    };
    for (int t = -kBack; t <= kAhead; ++t)
    {
        if (!visible[t + kBack]) continue;
        const float z0 = t * kTile;
        static const std::vector<Segment> none;
        const int b = binary_shelf_;
        const std::vector<Segment>& edges =
            on_binary() ? (real_hall ? (real_cases ? none : bin_case_[b]) : (real_cases ? bin_hall_[b] : bin_tile_[b]))
                        : (real_hall ? (real_cases ? none : case_geometry_) : (real_cases ? hall_geometry_ : tile_geometry_));
        for (const Segment& s : edges) add(s, z0);
        // The binary line's open side: the short wall on the last edge of the floor. Under
        // Real Graphics it is a solid model instead (draw_models), so it is not drawn twice.
        if (on_binary() && !real_hall)
            for (const Segment& s : edge_geometry_[b]) add(s, z0);
        const uint32_t books = real_books ? 0 : books_in_tile(t);
        const FilterMode fm = effective_mode();
        for (uint32_t k = 0; k < books; ++k)
        {
            // Mark: books that fail the filters are drawn faint, so survivors stand out.
            // Hide: they are left out.
            float dim = 0;
            if (fm == FilterMode::Mark || fm == FilterMode::Hide)
                if (!book(t, k).passes)
                {
                    if (fm == FilterMode::Hide) continue;
                    dim = 0.8f;
                }
            if (fm == FilterMode::Excluded && book(t, k).passes) continue; // only what the stack excludes
            if (book(t, k).withheld) // the vault: blank, and never in the excluded view
            {
                if (fm == FilterMode::Excluded) continue;
                dim = 0.8f;
            }
            for (const Segment& s : book_geometry_[sizes_vary()][k]) add(s, z0, dim);
        }
    }
    // The binary line: the rain falling off its edge. Only there -- the other six lines are
    // bounded by it, they do not contain it.
    if (on_binary()) draw_binary_edge(visible, kBack, kAhead, real_hall);
    // The pictures on the items' fronts, on every line that has them (item_faces.cpp).
    draw_item_faces(visible, kBack, kAhead);
    // The item you are looking at, tinted in the line's edge colour. After the pictures, or a
    // picture covering the whole front hides it (every line but models, whose picture leaves
    // the front uncovered), and over the picture's own rectangle (faces.ini), which is the
    // item's front as its model has it: an audio item is shorter than the slot.
    if (hover_ && !book(hover_->tile, hover_->slot()).empty)
    {
        const FaceRect& rect = face_rect();
        Vec3 f[4];
        picture_face(float(hover_->tile) * kTile, hover_->side, hover_->row, hover_->col, rect.bottom, rect.top,
                     rect.half_width, f, sizes_vary());
        // A line with black edges (books) would only darken it, which hardly shows on a dark
        // cover: there the tint is white.
        SDL_Color c = th.edge;
        if (int(c.r) * 3 + int(c.g) * 6 + int(c.b) < 600) c = SDL_Color{255, 255, 255, 255};
        c.a = 110;
        fill({f[0], f[1], f[2], f[3]}, c);
    }
    if (edge_glow_ && !md) draw_glow(buckets);
    for (int i = 0; i < kBuckets; ++i)
    {
        const SDL_Color c = mix(th.edge, th.bg, float(i) / kBuckets);
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        const auto& b = buckets[size_t(i)];
        for (size_t k = 0; k + 1 < b.size(); k += 2) SDL_RenderLine(r_, b[k].x, b[k].y, b[k + 1].x, b[k + 1].y);
    }
    draw_hud(w, h);
}

// ---- Real Graphics
const char* Hallway::media_name(Media m)
{
    switch (m)
    {
    case Media::Image: return "image";
    case Media::Audio: return "audio";
    case Media::Video: return "video";
    case Media::Books: return "books";
    case Media::Models: return "models";
    case Media::Binary: return "binary";
    default: return "pages";
    }
}

// The current line's models, loaded the first time they are needed.
const Hallway::Models& Hallway::models()
{
    Models& m = models_[li_];
    if (!m.loaded)
    {
        const std::string medium = media_name(media());
        m.hallway = load_model("hallway", medium);
        m.bookshelf = load_model("bookshelf", medium);
        m.book = load_model("book", medium);
        if (m.book) m.book_far = facing_x(*m.book);
        m.marker = load_model("marker", medium);
        if (li_ == kBinaryLine) m.edge = load_model("edge", "binary");
        if (li_ == kBinaryLine && m.hallway && !m.edge)
            for (int k = 0; k < 2; ++k)
            {
                const float sign = k == 0 ? 1.0f : -1.0f;
                m.half[k] = half_x(*m.hallway, sign, kHalfWidth - 0.3f);
                const float x = sign * kEdgeRail;
                m.rail[k] = box_mesh({std::min(x, x - sign * 0.12f), 0, 0},
                                     {std::max(x, x - sign * 0.12f), kEdgeRailTop, kTile}, kRailColour);
            }
        m.loaded = true;
        std::cerr << "real graphics for the " << medium << " line:";
        for (const auto& [name, mesh] : {std::pair{"hallway", li_ == kBinaryLine && m.edge ? m.edge : m.hallway}, {"bookshelf", m.bookshelf},
                                         {"book", m.book}, {"marker", m.marker}})
            std::cerr << " " << name << "=" << (mesh ? std::filesystem::path(mesh->source).filename().string() : std::string("wireframe"));
        std::cerr << "\n";
    }
    return m;
}

// Every visible tile's models, drawn with a depth buffer (see MeshBatch in mesh.hpp).
void Hallway::draw_models(const Models& md, const bool* visible, int back, int ahead, int w, int h)
{
    const Theme& th = theme();
    models_batch_.begin(cam_, th.bg, w, h);
    const bool varied = sizes_vary();
    const FilterMode fm = effective_mode();
    for (int t = -back; t <= ahead; ++t)
    {
        if (!visible[t + back]) continue;
        const float z0 = float(t) * kTile;
        // The binary line is the same tile of corridor with one side taken out, and a short
        // wall standing where it went. Everything else about it is an ordinary tile.
        const int bs = binary_shelf_;
        if (on_binary() && md.edge) models_batch_.add(*md.edge, {{0, 0, z0}, 1.0f, 1.0f, bs == 1});
        else if (on_binary() && md.half[bs]) models_batch_.add(*md.half[bs], {{0, 0, z0}});
        else if (md.hallway) models_batch_.add(*md.hallway, {{0, 0, z0}});
        if (on_binary() && md.rail[bs]) models_batch_.add(*md.rail[bs], {{0, 0, z0}});
        if (md.marker && offset_loop_tile(t).is_zero())
        {
            // Pulled a little nearer than the floor it lies on, so the floor never shows through.
            Placement at{{0, 0, z0}};
            at.depth_bias = 0.002f;
            models_batch_.add(*md.marker, at);
            at.offset.z += 1.0f; // every line starts here: a second strip
            if (all_start(t)) models_batch_.add(*md.marker, at);
        }
        // Tiles more than one away from the camera's are far enough for the simpler models.
        const bool near = t >= -1 && t <= 1;
        if (md.bookshelf)
        {
            // One wall, one bookcase, on the binary line; both walls everywhere else.
            if (!on_binary() || bs == 0) models_batch_.add(*md.bookshelf, {{0, 0, z0}});
            if (!on_binary() || bs == 1) models_batch_.add(*md.bookshelf, {{0, 0, z0}, 1.0f, 1.0f, true});
        }
        if (!md.book) continue;
        const Mesh& book_mesh = near ? *md.book : *md.book_far;
        const uint32_t books = books_in_tile(t);
        for (uint32_t k = 0; k < books; ++k)
        {
            float dim = 0;
            if (fm == FilterMode::Mark || fm == FilterMode::Hide)
                if (!book(t, k).passes)
                {
                    if (fm == FilterMode::Hide) continue;
                    dim = 0.8f;
                }
            if (fm == FilterMode::Excluded && book(t, k).passes) continue; // only what the stack excludes
            if (book(t, k).withheld) // the vault: blank, and never in the excluded view
            {
                if (fm == FilterMode::Excluded) continue;
                dim = 0.8f;
            }
            const BookSlot b = BookSlot::of(0, k);
            const float y0 = kRowTop - float(b.row + 1) * kRowHeight + 0.02f;
            const float zc = z0 + float(b.col) * book_pitch() + book_pitch() * 0.5f;
            const bool right = b.side == Side::Right;
            const float sx = right ? 1.0f : -1.0f;
            // Skip books that cannot be on screen (their slot's box).
            if (!cam_.box_visible({right ? kCaseFront - 0.4f : -kHalfWidth, y0, zc - book_pitch() * 0.5f},
                                  {right ? kHalfWidth : -kCaseFront + 0.4f, y0 + 0.55f, zc + book_pitch() * 0.5f}))
                continue;
            // Everything on a shelf is scaled to its slot, keeping its proportions, so more
            // to a tile means smaller items rather than squashed ones. On top of that, pages,
            // pictures and books take their slot's own height, where records and tapes do not
            // -- a record is one size whatever else changes (world.hpp: media_sizes_vary).
            const float tall = varied ? book_height(b.row, b.col, true) / (kUniformBookHeight * shelf_scale()) : 1.0f;
            models_batch_.add(book_mesh, {{sx * kCaseFront, y0, zc}, shelf_scale(), tall, right, dim});
        }
    }
    models_batch_.draw(r_);
    fps_tris_ = models_batch_.drawn();
}

// Geometry Edge Glow: under each edge, a soft band in the edge's colour that fades to nothing
// on both sides, in two layers (wide and faint, narrow and brighter). Ordinary alpha blending,
// so it brightens light edges on dark lines and darkens dark edges on light ones (books).
// Far edges, already faded towards the background, get a thinner and fainter glow. All of it
// is drawn in a few batched calls, one per distance bucket.
void Hallway::draw_glow(const std::vector<std::vector<SDL_FPoint>>& buckets)
{
    const Theme& th = theme();
    const int n = int(buckets.size());
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    for (int i = 0; i < n; ++i)
    {
        const auto& b = buckets[size_t(i)];
        if (b.empty()) continue;
        const float near = 1.0f - float(i) / float(n);
        const SDL_Color c = mix(th.edge, th.bg, float(i) / float(n));
        v.clear();
        idx.clear();
        v.reserve(b.size() * 6);
        idx.reserve(b.size() * 12);
        for (const auto& [width, alpha] : {std::pair{10.0f, 0.22f}, std::pair{4.0f, 0.45f}})
        {
            const float w = width * (0.35f + 0.65f * near);
            const SDL_FColor core{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, alpha * (0.4f + 0.6f * near)};
            const SDL_FColor clear{core.r, core.g, core.b, 0.0f};
            for (size_t k = 0; k + 1 < b.size(); k += 2)
            {
                const SDL_FPoint a = b[k], e = b[k + 1];
                float dx = e.x - a.x, dy = e.y - a.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len < 0.5f) continue;
                dx /= len;
                dy /= len;
                // The band runs a little past each end, so corners glow too.
                const SDL_FPoint a2{a.x - dx * w * 0.5f, a.y - dy * w * 0.5f}, e2{e.x + dx * w * 0.5f, e.y + dy * w * 0.5f};
                const float nx = -dy * w, ny = dx * w;
                const int base = int(v.size());
                v.push_back({{a2.x + nx, a2.y + ny}, clear, {0, 0}});
                v.push_back({{e2.x + nx, e2.y + ny}, clear, {0, 0}});
                v.push_back({{a2.x, a2.y}, core, {0, 0}});
                v.push_back({{e2.x, e2.y}, core, {0, 0}});
                v.push_back({{a2.x - nx, a2.y - ny}, clear, {0, 0}});
                v.push_back({{e2.x - nx, e2.y - ny}, clear, {0, 0}});
                for (int t : {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4}) idx.push_back(base + t);
            }
        }
        if (!v.empty()) SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), idx.data(), int(idx.size()));
    }
}

// A checkered strip across the floor at z0: the start (and end) of a loop of the current line.
// Where every line starts together there are two strips, a metre apart.
void Hallway::draw_start_line(float z0)
{
    const Theme& th = theme();
    constexpr int kSquares = 16;
    const float sq = 2 * kHalfWidth / kSquares;
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < kSquares; ++i)
        {
            const SDL_Color c = (i + row) % 2 ? th.bg : th.edge;
            const float x0 = -kHalfWidth + i * sq, x1 = x0 + sq;
            const float za = z0 - sq + row * sq, zb = za + sq;
            fill({{x0, 0.001f, za}, {x1, 0.001f, za}, {x1, 0.001f, zb}, {x0, 0.001f, zb}}, c);
        }
}

void Hallway::fill(const std::vector<Vec3>& quad, SDL_Color c)
{
    const auto pts = cam_.project_polygon(quad);
    if (pts.size() < 3) return;
    std::vector<SDL_Vertex> v;
    const SDL_FColor fc{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
    for (size_t i = 1; i + 1 < pts.size(); ++i)
        for (size_t k : {size_t(0), i, i + 1}) v.push_back({{pts[k].x, pts[k].y}, fc, {0, 0}});
    SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
}

// One crate's image on its front face. The quad is split into a grid and every grid point is
// projected, so the picture keeps its perspective instead of skewing across two triangles.
void Hallway::draw_face_image(SDL_Texture* tex, const Vec3 quad[4], std::vector<SDL_Vertex>& verts, float uu0,
                     float uu1)
{
    // SDL draws a textured triangle with the texture spread evenly across it on the screen
    // (affine), not with perspective, so a picture drawn as two triangles bends as you move round
    // it: close up, and looking along the face, the far half is squeezed and the near half
    // stretched, and the bend follows the camera. The cure is to cut the face into cells small
    // enough that each is nearly flat to the eye, placing every cell corner by the true
    // projection. How many cells that takes depends on how much the depth changes across the face
    // (the ratio of its farthest corner to its nearest) and on how large it is on screen, so a
    // picture across the corridor stays a handful of triangles and one at your nose gets many.
    Vec3 corner[4];
    float zmin = 1e30f, zmax = 0;
    for (int i = 0; i < 4; ++i)
    {
        corner[i] = cam_.to_camera(quad[i]);
        if (corner[i].z < cam_.near_z + 0.01f) return; // partly behind you: leave it be
        zmin = std::min(zmin, corner[i].z);
        zmax = std::max(zmax, corner[i].z);
    }
    Point2 c2[4];
    for (int i = 0; i < 4; ++i) c2[i] = cam_.project_camera(corner[i]);
    float extent = 0;
    for (int i = 0; i < 4; ++i)
        extent = std::max(extent, std::max(std::abs(c2[i].x - c2[(i + 2) % 4].x), std::abs(c2[i].y - c2[(i + 2) % 4].y)));
    // About 2% of depth change per cell hides the bend; so does a cell of 48 pixels or less.
    const int by_depth = int(std::ceil((zmax / zmin - 1.0f) / 0.02f));
    const int by_size = int(std::ceil(extent / 48.0f));
    const int n = std::clamp(std::max(by_depth, by_size), 2, 32);
    grid_.resize(size_t(n + 1) * size_t(n + 1));
    for (int i = 0; i <= n; ++i)
        for (int j = 0; j <= n; ++j)
        {
            const float u = float(j) / float(n), v = float(i) / float(n);
            // Bilinear in camera space, then projected: exact perspective at every grid point.
            const Vec3 top = corner[0] + (corner[1] - corner[0]) * u;
            const Vec3 bot = corner[3] + (corner[2] - corner[3]) * u;
            grid_[size_t(i) * size_t(n + 1) + size_t(j)] = cam_.project_camera(top + (bot - top) * v);
        }
    const SDL_FColor white{1, 1, 1, 1};
    verts.clear();
    auto at = [&](int i, int j) { return grid_[size_t(i) * size_t(n + 1) + size_t(j)]; };
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
        {
            const float u0 = uu0 + (uu1 - uu0) * float(j) / float(n);
            const float u1 = uu0 + (uu1 - uu0) * float(j + 1) / float(n);
            const float v0 = float(i) / float(n), v1 = float(i + 1) / float(n);
            const Point2 pa = at(i, j), pb = at(i, j + 1), pc = at(i + 1, j + 1), pd = at(i + 1, j);
            const SDL_Vertex a{{pa.x, pa.y}, white, {u0, v0}};
            const SDL_Vertex b{{pb.x, pb.y}, white, {u1, v0}};
            const SDL_Vertex c{{pc.x, pc.y}, white, {u1, v1}};
            const SDL_Vertex d{{pd.x, pd.y}, white, {u0, v1}};
            for (const SDL_Vertex& vx : {a, b, c, a, c, d}) verts.push_back(vx);
        }
    SDL_RenderGeometry(r_, tex, verts.data(), int(verts.size()), nullptr, 0);
}

Hallway::~Hallway()
{
    stop_locator();
    stop_graph();
    stop_face_workers(); // before the model space they read from goes
    release_textures();
}

// Frees textures that belong to the renderer; call before destroying it.
void Hallway::release_textures()
{
    models_batch_.release();
    clear_faces();
    release_signs();
    if (portal_.tex)
    {
        SDL_DestroyTexture(portal_.tex);
        portal_.tex = nullptr;
        portal_.w = portal_.h = 0;
    }
    for (PictureCache& c : picture_)
        if (c.tex)
        {
            SDL_DestroyTexture(c.tex);
            c.tex = nullptr;
        }
}

// From the main menu's settings: mouse look, and the graphics options (Geometry Edge Glow and
// Real Graphics are recorded here for the renderer; both are off by default).
void Hallway::set_controls(int sensitivity_percent, bool invert_y)
{
    look_ = 0.0025f * float(sensitivity_percent) / 100.0f;
    invert_y_ = invert_y;
}

// How big a crate's picture is drawn (the setup menu's MODELS section). Changing it throws
// the cache away, because every picture in it is the wrong size now.
void Hallway::set_face_px(uint32_t px)
{
    const int n = int(std::clamp<uint32_t>(px, 16, 1024));
    if (n == face_px_) return;
    face_px_ = n;
    clear_faces();
}

void Hallway::set_model_cache(int megabytes)
{
    face_budget_mb_ = uint32_t(std::clamp(megabytes, 8, 4096));
    while (faces_.size() > face_capacity())
    {
        auto oldest = faces_.begin();
        for (auto it = faces_.begin(); it != faces_.end(); ++it)
            if (it->second.used < oldest->second.used) oldest = it;
        if (oldest->second.tex) SDL_DestroyTexture(oldest->second.tex);
        faces_.erase(oldest);
    }
}

void Hallway::set_graphics(bool edge_glow, bool real_graphics, bool door_portals)
{
    edge_glow_ = edge_glow;
    real_graphics_ = real_graphics;
    door_portals_ = door_portals;
    for (Models& m : models_) m = Models{}; // reloaded on first use (picks up edited files)
}

// True if tile dt away is the start of this line's loop, which the double start flag marks: a
// door there leads to every other line's start too (a door keeps your angle, and 0 is 0 on
// every line), so every line starts together here, wherever the loop comes round.
bool Hallway::all_start(int64_t dt) const
{
    return offset_loop_tile(all_loops_[li_], all_loop_tiles_[li_], dt).is_zero();
}

BigUint Hallway::offset_loop_tile(const LineLoop& loop, const BigUint& loop_tile, int64_t dt)
{
    const BigUint& tiles = loop.tiles();
    if (tiles.bit_length() <= 62)
    {
        auto u64 = [](const BigUint& v) {
            BigUint hi = v;
            hi >>= 32;
            return (uint64_t(hi.low_bits(32)) << 32) | v.low_bits(32);
        };
        const int64_t n = int64_t(u64(tiles));
        const int64_t cur = int64_t(u64(loop_tile));
        return BigUint(uint64_t(((cur + dt % n) % n + n) % n));
    }
    BigUint r = loop_tile;
    if (dt >= 0)
    {
        r += BigUint(uint64_t(dt));
        if (r >= tiles) r -= tiles;
    }
    else
    {
        const BigUint d(0 - uint64_t(dt));
        if (r >= d) r -= d;
        else
        {
            r += tiles;
            r -= d;
        }
    }
    return r;
}

// How many books tile dt holds: all of them, except the last tile of a padded loop.
uint32_t Hallway::books_in_tile(int64_t dt) const
{
    // The binary line's files are in the left wall's slots, which come first.
    const uint32_t most = on_binary() ? uint32_t(sieve::books_per_tile() / 2) : uint32_t(sieve::books_per_tile());
    if (loop_.fills_whole_tiles()) return most;
    BigUint lt = offset_loop_tile(dt);
    lt.add_small(1);
    return std::min(most, lt == loop_.tiles() ? uint32_t(sieve::books_per_tile() - loop_.padding()) : uint32_t(sieve::books_per_tile()));
}

// A file's place on the binary line to its place in the loop and back: with h slots a wall,
// unit u stands in tile u / h, left-wall slot u mod h, so its loop position is
// (u / h) * 2h + u mod h. Both are shifts, since h is a power of two. The identity elsewhere.
BigUint Hallway::loop_pos(const BigUint& unit) const
{
    const size_t h = size_t(std::countr_zero(uint32_t(sieve::books_per_tile() / 2)));
    BigUint tiles = unit;
    tiles >>= h;
    BigUint pos = tiles;
    pos <<= h + 1;
    BigUint below = tiles;
    below <<= h;
    BigUint slot = unit;
    slot -= below;
    pos += slot;
    return pos;
}

BigUint Hallway::unit_of_pos(const BigUint& pos) const
{
    const size_t h = size_t(std::countr_zero(uint32_t(sieve::books_per_tile() / 2)));
    BigUint tiles = pos;
    tiles >>= h + 1;
    BigUint at = tiles;
    at <<= h + 1;
    BigUint slot = pos;
    slot -= at;
    BigUint unit = tiles;
    unit <<= h;
    unit += slot;
    return unit;
}

// What a file's first bytes say it is: file-kinds-v1 (sieve/filekind.hpp), which the binary
// line's filters read too, so the label and the filters always agree.
std::string Hallway::file_type(const std::vector<uint8_t>& h, uint64_t size) { return sieve::file_kind(h, size); }

Space::Digits Hallway::title_for_name(const std::string& name) const
{
    const TitledSpace& ts = *titled_[kBinaryLine];
    if (!ts.title_space() || !lines_[0].alphabet) return ts.blank_title();
    const Space& t = *ts.title_space();
    const CanonResult c = canonicalise_text(name, *lines_[0].alphabet, t.unit_length(), lines_[0].canon);
    return c.units.empty() ? ts.blank_title() : t.digits_of(c.units[0]);
}

std::string Hallway::hex_of(const Book& b)
{
    if (!b.is_file) return b.hex;
    if (b.survivor && binary_sieve_) return binary_sieve_->hex_of(b.index); // its compact address
    if (!(memo_hex_ok_ && memo_index_ == b.index))
    {
        if (!(memo_index_ == b.index)) memo_file_ok_ = false;
        memo_index_ = b.index;
        memo_hex_ = titled_[kBinaryLine]->hex_of(b.index);
        memo_hex_ok_ = true;
    }
    return memo_hex_;
}

const BinarySpace::Bytes& Hallway::file_of(const Book& b)
{
    static const BinarySpace::Bytes kNone;
    if (!b.is_file) return kNone;
    // By its place on binary-v1 itself, which neither the ordering nor the filters change.
    const BigUint content = b.content ? *b.content : titled_[kBinaryLine]->parts_at(b.index, mode_).content;
    if (!(memo_file_ok_ && memo_content_ == content))
    {
        memo_content_ = content;
        memo_file_ = binary_space_->bytes_at(content, AddressMode::Positional);
        memo_withheld_ = cli::vault::withheld_bytes(memo_file_);
        memo_file_ok_ = true;
    }
    return memo_file_;
}

bool Hallway::file_withheld(const Book& b)
{
    if (!b.is_file) return false;
    file_of(b);
    return memo_withheld_;
}

// The vault: a model's .obj, a melody's MIDI file, a file's bytes (file_withheld), every picture
// drawn by PDQ (an image, each frame of a video, every cover), and text as a file written out
// (vault_decode.hpp): a title, a book's each page, and its pages read as one, since a file too long
// for a page runs on over the next. Never by what the text says (docs/VAULT.md). Failed closed,
// everything is withheld.
bool Hallway::vault_withholds(const Book& b) const
{
    if (b.empty) return false;
    // A cover (a book's, or an audio or video unit's on a titled line) is a picture of its own.
    if (b.parts && cli::picture_withheld(lines_[1].image, b.parts->cover)) return true;
    if (!b.cover.empty() && cli::picture_withheld(lines_[1].image, b.cover)) return true;
    if (!b.parts && !b.title.empty() && cli::vault::withheld_written(title_text(b))) return true;
    if (b.parts)
    {
        const Line& l = line();
        if (cli::vault::withheld_written(utf8_encode(l.space.text_of(b.parts->title)))) return true;
        std::string all;
        for (const auto& page : b.parts->pages)
        {
            if (cli::unit_withheld(l, page)) return true;
            std::string t = utf8_encode(l.space.text_of(page));
            while (!t.empty() && t.back() == ' ') t.pop_back();
            all += t + "\n";
        }
        return b.parts->pages.size() > 1 && cli::vault::withheld_written(all);
    }
    if (b.model && model_space_)
    {
        const std::string obj = model_space_->to_obj(*b.model);
        return cli::vault::withheld_bytes(std::vector<uint8_t>(obj.begin(), obj.end()));
    }
    if (b.is_file) return false; // worked out with its bytes: file_withheld
    return !b.unit.empty() && cli::unit_withheld(line(), b.unit);
}

// Puts down what is in hand if the vault withholds it; true if it did.
bool Hallway::refuse_if_withheld()
{
    if (!in_hand_ || !withheld(*in_hand_)) return false;
    in_hand_.reset();
    message(tr("vault.withheld"));
    refused_ = true;
    return true;
}

void Hallway::set_thin(bool on)
{
    if (thin_ == on) return;
    thin_ = on;
    cache_.clear();
    clear_faces();
}

void Hallway::set_binary_length(uint64_t bytes)
{
    if (bytes == binary_space_->max_bytes()) return;
    const std::string key = lines_[0].space.key();
    const TitledSpace& old = *titled_[kBinaryLine];
    binary_space_ = std::make_unique<BinarySpace>(std::max<uint64_t>(1, bytes), key);
    titled_[kBinaryLine] = std::make_unique<TitledSpace>(old.title_space(), std::nullopt, binary_space_->size(), binary_space_->shape(), key);
    memo_hex_ok_ = memo_file_ok_ = false;
    rebuild_binary_sieve();
    rebase();
}

void Hallway::rebuild_binary_sieve()
{
    try
    {
        binary_sieve_ = std::make_unique<BinarySieve>(build_binary_sieve(*binary_space_, binary_filters_));
    }
    catch (const std::exception& e)
    {
        std::cerr << "filters for the binary line: " << e.what() << "\n";
        binary_sieve_.reset();
    }
}

BigUint Hallway::line_units() const
{
    if (!on_binary()) return loop_.units();
    return effective_mode() == FilterMode::Compact ? binary_sieve_->count() : titled_[kBinaryLine]->size();
}

// The books of one tile, slot by slot (4 edges each), drawn separately so padding can be bare.
// `varied`: heights vary from slot to slot; else every book is the same size (audio, video).
std::vector<std::array<Segment, 4>> Hallway::build_books(bool varied)
{
    std::vector<std::array<Segment, 4>> out(sieve::books_per_tile());
    for (uint32_t k = 0; k < uint32_t(sieve::books_per_tile()); ++k)
    {
        const BookSlot b = BookSlot::of(0, k);
        Vec3 f[4];
        book_face(0, b.side, b.row, b.col, f, varied);
        out[k] = {Segment{f[0], f[1]}, Segment{f[1], f[2]}, Segment{f[2], f[3]}, Segment{f[3], f[0]}};
    }
    return out;
}

} // namespace hallway::hall
