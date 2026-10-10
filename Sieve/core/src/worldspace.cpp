#include "sieve/worldspace.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

BigUint unit_count(const Space& s) { return BigUint::pow(s.base(), s.unit_length()); }

std::string part_id(const Space& s) { return s.symbols_id() + "/L" + std::to_string(s.unit_length()); }

std::string shape_id(const Space& cover, const std::optional<Space>& title, const ModelSpace& models, uint32_t slots, uint32_t grid)
{
    return "worlds/" + part_id(cover) + "+" + (title ? part_id(*title) : std::string("-")) + "+V" + std::to_string(models.vertices()) + "/F" +
           std::to_string(models.face_count()) + "/C" + std::to_string(models.coords()) + "x" + std::to_string(slots) + "/G" +
           std::to_string(grid) + "/key=" + models.key() + "/" + kWorldSpaceVersion;
}

size_t width_of(const BigUint& size)
{
    BigUint top = size;
    top -= BigUint(1);
    return std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::array<Turn, kTurns> make_turns()
{
    std::array<Turn, kTurns> out{};
    std::array<uint8_t, 3> perm{0, 1, 2};
    size_t n = 0;
    do
    {
        // The permutation's parity, then the signs that make the determinant +1.
        int inversions = 0;
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j) inversions += perm[i] > perm[j] ? 1 : 0;
        for (int bits = 0; bits < 8; ++bits)
        {
            const std::array<int8_t, 3> sign{int8_t(bits & 4 ? -1 : 1), int8_t(bits & 2 ? -1 : 1), int8_t(bits & 1 ? -1 : 1)};
            const int det = (inversions % 2 ? -1 : 1) * sign[0] * sign[1] * sign[2];
            if (det == 1) out[n++] = Turn{perm, sign};
        }
    } while (std::next_permutation(perm.begin(), perm.end()));
    return out;
}

// A world coordinate, exactly: `num` / C, written with its sign, its whole part to `whole` digits
// and `decimals` places (10^decimals / C is a whole number, C being 2^decimals).
std::string world_coord(int64_t num, uint32_t c, uint32_t decimals, size_t whole)
{
    int64_t scaled = 1;
    for (uint32_t i = 0; i < decimals; ++i) scaled *= 10;
    const int64_t v = num * (scaled / int64_t(c));
    const int64_t mag = v < 0 ? -v : v;
    std::string w = std::to_string(mag / scaled);
    w.insert(0, whole - std::min(whole, w.size()), '0');
    std::string frac = std::to_string(mag % scaled);
    frac.insert(0, size_t(decimals) - frac.size(), '0');
    return (v < 0 ? "-" : "+") + w + (decimals ? "." + frac : std::string());
}

// A number known to be below 2^64.
uint64_t low64(const BigUint& v) { return v.limbs().empty() ? 0 : v.limbs()[0]; }

uint64_t places_of(uint32_t grid)
{
    if (grid == 0 || grid > 1024) throw std::invalid_argument("a world's grid is 1 to 1024 cells along each axis");
    return uint64_t(grid) * grid * grid * kTurns;
}

BigUint size_of(const Space& cover, const std::optional<Space>& title, const BigUint& slot_size, uint32_t slots)
{
    if (slots == 0) throw std::invalid_argument("a world holds at least one model");
    BigUint n = unit_count(cover);
    if (title) n = BigUint::mul(n, unit_count(*title));
    return BigUint::mul(n, BigUint::pow(slot_size, slots));
}

} // namespace

const std::array<Turn, kTurns>& turns()
{
    static const std::array<Turn, kTurns> t = make_turns();
    return t;
}

WorldSpace::WorldSpace(const Space& cover, std::optional<Space> title, const ModelSpace& models, uint32_t slots, uint32_t grid)
    : cover_(cover), title_(std::move(title)), models_(models), slots_(slots), grid_(grid), places_(places_of(grid)),
      slot_size_(BigUint::mul(models_.size(), BigUint(places_))), size_(size_of(cover_, title_, slot_size_, slots_)),
      hex_width_(width_of(size_)), shuffle_(size_, models_.key(), shape_id(cover_, title_, models_, slots_, grid_))
{
}

std::string WorldSpace::id() const { return shape_id(cover_, title_, models_, slots_, grid_); }

void WorldSpace::check(const Parts& p) const
{
    auto fits = [](const Digits& d, const Space& s) {
        if (d.size() != s.unit_length()) return false;
        return std::all_of(d.begin(), d.end(), [&](uint32_t x) { return x < s.base(); });
    };
    bool ok = fits(p.cover, cover_) && (title_ ? fits(p.title, *title_) : p.title.empty()) && p.slots.size() == slots_;
    for (const Slot& s : p.slots)
        ok = ok && s.model < models_.size() && s.x < grid_ && s.y < grid_ && s.z < grid_ && s.turn < kTurns;
    if (!ok) throw std::invalid_argument("the world does not have this dimension's shape");
}

uint64_t WorldSpace::place_of(const Slot& s) const { return ((uint64_t(s.x) * grid_ + s.y) * grid_ + s.z) * kTurns + s.turn; }

void WorldSpace::set_place(Slot& s, uint64_t place) const
{
    s.turn = uint32_t(place % kTurns);
    place /= kTurns;
    s.z = uint32_t(place % grid_);
    place /= grid_;
    s.y = uint32_t(place % grid_);
    s.x = uint32_t(place / grid_);
}

BigUint WorldSpace::index_of(const Parts& p, AddressMode m) const
{
    check(p);
    BigUint v = BigUint::from_digits(p.cover, cover_.base());
    if (title_)
    {
        v = BigUint::mul(v, unit_count(*title_));
        v += BigUint::from_digits(p.title, title_->base());
    }
    for (const Slot& s : p.slots)
    {
        v = BigUint::mul(v, slot_size_);
        BigUint slot = BigUint::mul(s.model, BigUint(places_));
        slot += BigUint(place_of(s));
        v += slot;
    }
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

WorldSpace::Parts WorldSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("address beyond the dimension");
    BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    Parts p;
    p.slots.resize(slots_);
    for (size_t k = slots_; k-- > 0;)
    {
        BigUint q, slot;
        BigUint::divmod(v, slot_size_, q, slot);
        BigUint model, place;
        BigUint::divmod(slot, BigUint(places_), model, place);
        p.slots[k].model = std::move(model);
        set_place(p.slots[k], low64(place));
        v = std::move(q);
    }
    if (title_)
    {
        BigUint cover, title;
        BigUint::divmod(v, unit_count(*title_), cover, title);
        p.cover = cover.to_digits(cover_.base(), cover_.unit_length());
        p.title = title.to_digits(title_->base(), title_->unit_length());
    }
    else p.cover = v.to_digits(cover_.base(), cover_.unit_length());
    return p;
}

BigUint WorldSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("address beyond the dimension");
    return v;
}

WorldSpace::Mesh WorldSpace::mesh_of(const Parts& p) const
{
    check(p);
    Mesh m;
    const float c = float(models_.coords());
    uint32_t base = 0;
    for (const Slot& s : p.slots)
    {
        const ModelSpace::Parts mp = models_.parts_at(s.model, AddressMode::Positional);
        const Turn& t = turns()[s.turn];
        const int64_t centre[3] = {int64_t(2) * s.x + 1 - grid_, int64_t(2) * s.y + 1 - grid_, int64_t(2) * s.z + 1 - grid_};
        for (uint32_t i = 0; i < models_.vertices(); ++i)
        {
            float out[3];
            for (int k = 0; k < 3; ++k)
            {
                const int64_t d = mp.verts[size_t(i) * 3 + t.perm[size_t(k)]];
                const int64_t num = t.sign[size_t(k)] * (2 * d + 1 - int64_t(models_.coords())) + centre[k] * int64_t(models_.coords());
                out[k] = float(num) / c;
            }
            m.vertices.push_back({out[0], out[1], out[2]});
        }
        for (const ModelSpace::Face& f : models_.faces_of(mp)) m.faces.push_back({f.a + base, f.b + base, f.c + base});
        base += models_.vertices();
    }
    return m;
}

std::string WorldSpace::to_obj(const Parts& p) const
{
    check(p);
    const int64_t c = models_.coords();
    const size_t whole = std::to_string(grid_).size(); // |coordinate| < G
    const size_t wi = std::to_string(uint64_t(models_.vertices()) * slots_).size();
    std::vector<ModelSpace::Parts> mps;
    for (const Slot& s : p.slots) mps.push_back(models_.parts_at(s.model, AddressMode::Positional));
    std::string out;
    for (size_t n = 0; n < p.slots.size(); ++n)
    {
        const Slot& s = p.slots[n];
        const Turn& t = turns()[s.turn];
        const int64_t centre[3] = {int64_t(2) * s.x + 1 - grid_, int64_t(2) * s.y + 1 - grid_, int64_t(2) * s.z + 1 - grid_};
        for (uint32_t i = 0; i < models_.vertices(); ++i)
        {
            out += "v";
            for (int k = 0; k < 3; ++k)
            {
                const int64_t d = mps[n].verts[size_t(i) * 3 + t.perm[size_t(k)]];
                const int64_t num = t.sign[size_t(k)] * (2 * d + 1 - c) + centre[k] * c;
                out += " " + world_coord(num, uint32_t(c), models_.decimals(), whole);
            }
            out += '\n';
        }
    }
    for (size_t n = 0; n < p.slots.size(); ++n)
    {
        const uint64_t base = uint64_t(n) * models_.vertices();
        for (uint32_t i = 0; i < models_.face_count(); ++i)
        {
            out += "f";
            for (int k = 0; k < 3; ++k)
            {
                std::string idx = std::to_string(base + mps[n].faces[size_t(i) * 3 + size_t(k)] + 1);
                idx.insert(0, wi - idx.size(), '0');
                out += " " + idx;
            }
            out += '\n';
        }
    }
    return out;
}

// ---------------------------------------------------------------- the filters

WorldSieve::WorldSieve(const WorldSpace& space, const FilterStack& cover, const FilterStack& title, const ModelSieve& models)
    : space_(&space), cover_(&cover), title_(&title), models_(&models)
{
    const Space& c = space.cover_space();
    const std::optional<Space>& t = space.title_space();
    ranks_ = true;
    auto part = [&](const FilterStack& st, const Space& s, const char* name, BigUint& count) {
        if (st.empty()) count = BigUint::pow(s.base(), s.unit_length());
        else if (st.ranker()) count = st.ranker()->count();
        else if (ranks_)
        {
            ranks_ = false;
            blocker_ = std::string(name) + ": " + st.compact_blocker();
        }
    };
    part(cover, c, "cover", cover_count_);
    if (t) part(title, *t, "title", title_count_);
    else title_count_ = BigUint(1); // no title: one way to have none
    if (models.empty()) model_count_ = space.model_space().size();
    else if (models.can_rank()) model_count_ = models.count();
    else if (ranks_)
    {
        ranks_ = false;
        blocker_ = "models: " + models.blocker();
    }
    if (!ranks_) return;
    slot_count_ = BigUint::mul(model_count_, BigUint(space.places()));
    slots_count_ = BigUint::pow(slot_count_, space.slots());
    count_ = BigUint::mul(BigUint::mul(cover_count_, title_count_), slots_count_);
    if (!count_.is_zero())
    {
        hex_width_ = width_of(count_);
        shuffle_ = std::make_unique<Shuffle>(count_, space.model_space().key(), domain());
    }
}

std::string WorldSieve::domain() const
{
    auto part = [](const FilterStack* st) { return st->empty() ? std::string("-") : st->id(); };
    const std::string title = space_->title_space() ? part(title_) : std::string("-");
    const std::string models = models_->empty() ? std::string("-") : models_->id();
    return std::string(kWorldCompactVersion) + "/" + space_->id() + "/" + part(cover_) + "/" + title + "/" + models;
}

std::string WorldSieve::first_failure(const WorldSpace::Parts& p) const
{
    space_->check(p);
    if (int f = cover_->first_failure(p.cover); f >= 0) return "cover: " + cover_->filter_name(size_t(f));
    if (space_->title_space())
        if (int f = title_->first_failure(p.title); f >= 0) return "title: " + title_->filter_name(size_t(f));
    for (size_t k = 0; k < p.slots.size(); ++k)
        if (const std::string f = models_->first_failure(p.slots[k].model); !f.empty()) return "model " + std::to_string(k + 1) + ": " + f;
    return "";
}

BigUint WorldSieve::model_rank(const BigUint& positional) const
{
    return models_->empty() ? positional : models_->index_of(positional, AddressMode::Positional);
}

BigUint WorldSieve::model_unrank(const BigUint& k) const
{
    return models_->empty() ? k : models_->model_at(k, AddressMode::Positional);
}

BigUint WorldSieve::rank(const WorldSpace::Parts& p) const
{
    if (!ranks_) throw std::logic_error("these filters cannot rank: " + blocker_);
    if (const std::string f = first_failure(p); !f.empty()) throw std::invalid_argument("it does not pass (" + f + ")");
    auto part_rank = [](const FilterStack* st, const Space& s, const Space::Digits& d) {
        return st->empty() ? BigUint::from_digits(d, s.base()) : st->ranker()->rank(d);
    };
    BigUint k = part_rank(cover_, space_->cover_space(), p.cover);
    if (space_->title_space())
    {
        k = BigUint::mul(k, title_count_);
        k += part_rank(title_, *space_->title_space(), p.title);
    }
    for (const WorldSpace::Slot& s : p.slots)
    {
        k = BigUint::mul(k, slot_count_);
        BigUint slot = BigUint::mul(model_rank(s.model), BigUint(space_->places()));
        slot += BigUint(space_->place_of(s));
        k += slot;
    }
    return k;
}

WorldSpace::Parts WorldSieve::unrank(const BigUint& k) const
{
    if (!ranks_) throw std::logic_error("these filters cannot rank: " + blocker_);
    if (k >= count_) throw std::out_of_range("beyond the surviving worlds");
    auto part_unrank = [](const FilterStack* st, const Space& s, const BigUint& r) {
        return st->empty() ? r.to_digits(s.base(), s.unit_length()) : st->ranker()->unrank(r);
    };
    WorldSpace::Parts p;
    p.slots.resize(space_->slots());
    BigUint rest = k;
    for (size_t n = space_->slots(); n-- > 0;)
    {
        BigUint q, slot;
        BigUint::divmod(rest, slot_count_, q, slot);
        BigUint model, place;
        BigUint::divmod(slot, BigUint(space_->places()), model, place);
        p.slots[n].model = model_unrank(model);
        space_->set_place(p.slots[n], low64(place));
        rest = std::move(q);
    }
    if (space_->title_space())
    {
        BigUint kc, kt;
        BigUint::divmod(rest, title_count_, kc, kt);
        p.title = part_unrank(title_, *space_->title_space(), kt);
        rest = std::move(kc);
    }
    p.cover = part_unrank(cover_, space_->cover_space(), rest);
    return p;
}

BigUint WorldSieve::index_of(const WorldSpace::Parts& p, AddressMode m) const { return index_of_rank(rank(p), m); }

BigUint WorldSieve::index_of_rank(const BigUint& k, AddressMode m) const
{
    if (!shuffle_) throw std::logic_error("no compact worlds here");
    if (k >= count_) throw std::out_of_range("beyond the surviving worlds");
    return m == AddressMode::Scrambled ? shuffle_->forward(k) : k;
}

WorldSpace::Parts WorldSieve::parts_at(const BigUint& index, AddressMode m) const
{
    if (!shuffle_) throw std::logic_error("no compact worlds here");
    if (index >= count_) throw std::out_of_range("beyond the surviving worlds");
    return unrank(m == AddressMode::Scrambled ? shuffle_->inverse(index) : index);
}

BigUint WorldSieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= count_) throw std::out_of_range("beyond the surviving worlds");
    return v;
}

} // namespace sieve
