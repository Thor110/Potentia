// Sieve — the models line's filter stack: not-a-file-v1 on a model's positional index, and the
// line's own rules (distinct vertices, distinct indices, every vertex used) in closed form, counted
// and ranked exactly, and the compact orderings over its survivors. See sieve/modelsieve.hpp.

#include "sieve/modelsieve.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

// ---------------------------------------------------------------- NumberFiles

NumberFiles::NumberFiles(const BigUint& size) : size_(size)
{
    // The numbers run to size - 1, whose file is the longest asked about: a file of L bytes sits at
    // binary places below (256^(L+1) - 1) / 255, so size * 255 + 1 bounds the bytes needed.
    BigUint t = size;
    t.mul_small(255);
    t.add_small(1);
    signed_ = std::make_unique<KindCounter>(uint64_t((t.bit_length() - 1) / 8) + 1, kind_set_of("signed"));
    count_ = kept_below(size);
}

bool NumberFiles::passes(const BigUint& x) const { return !is_signed_kind(file_kind_at(x)); }

BigUint NumberFiles::kept_below(const BigUint& x) const
{
    BigUint r = x;
    r -= signed_->count_before(x);
    return r;
}

BigUint NumberFiles::unrank(const BigUint& k) const
{
    if (!(k < count_)) throw std::out_of_range("survivor number beyond the survivors");
    // The smallest v with more than k survivors in 0..v: between k and the last number.
    BigUint lo = k, hi = size_;
    hi -= BigUint(1);
    while (lo < hi)
    {
        BigUint mid = lo;
        mid += hi;
        mid >>= 1;
        BigUint through = mid;
        through.add_small(1);
        if (kept_below(through) > k) hi = mid;
        else
        {
            lo = mid;
            lo.add_small(1);
        }
    }
    return lo;
}

// ---------------------------------------------------------------- MeshRules

// The line's own rules on a model's two parts, counted and ranked. Vertices: with distinct points,
// the i-th vertex has P - i points left (P = C^3), so the strings kept are the falling factorial,
// and a vertex's place among them is the points left below it. Faces: the walk's state is how
// many vertices are still unused, which vertices the current face has named, and how many faces
// are left; completions are a closed form in those (see models.cpp).
class MeshRules
{
public:
    MeshRules(uint32_t v, uint32_t f, uint32_t c, bool distinct_vertices, bool distinct_indices, bool all_used)
        : v_(v), f_(f), c_(c), dv_(distinct_vertices), di_(distinct_indices), au_(all_used)
    {
        const uint64_t points = uint64_t(c) * c * c;
        // Vertices kept from vertex i on: fall_[i] = (P - i)(P - i - 1) ... (P - V + 1).
        fall_.assign(v + 1, BigUint(1));
        for (uint32_t i = v; i-- > 0;)
        {
            fall_[i] = fall_[i + 1];
            if (dv_) fall_[i] = points > i ? BigUint::mul(fall_[i], BigUint::from_decimal(std::to_string(points - i))) : BigUint();
        }
        vcount_ = dv_ ? fall_[0] : BigUint::pow(c, uint64_t(3) * v);
        // Faces: (one face's ways with n vertices)^e for every n and e, and the binomials.
        const bool small = uint64_t(v + 1) * (f + 1) <= (1u << 20) && v <= 512;
        can_rank_ = small && uint64_t(3) * f * v * (au_ ? v + 1 : 1) <= 4000000;
        if (small)
        {
            pow_.assign(v + 1, {});
            for (uint32_t n = 0; n <= v; ++n)
            {
                const BigUint face = face_ways(n);
                pow_[n].assign(f + 1, BigUint(1));
                for (uint32_t e = 1; e <= f; ++e) pow_[n][e] = BigUint::mul(pow_[n][e - 1], face);
            }
            if (au_)
            {
                binom_.assign(v + 1, {});
                for (uint32_t m = 0; m <= v; ++m)
                {
                    binom_[m].assign(m + 1, BigUint(1));
                    for (uint32_t j = 1; j < m; ++j) binom_[m][j] = BigUint(binom_[m - 1][j - 1]) += binom_[m - 1][j];
                }
            }
            fcount_ = completions(v, 0, f);
        }
        else fcount_ = completions_direct(v);
        count_ = BigUint::mul(vcount_, fcount_);
    }

    bool can_rank() const { return can_rank_; }
    const BigUint& count() const { return count_; }

    // Which rule the model fails: 0 distinct vertices, 1 distinct indices, 2 every vertex used, -1 none.
    int first_failure(const ModelSpace::Parts& p) const
    {
        if (dv_)
        {
            std::vector<uint64_t> seen;
            for (uint32_t i = 0; i < v_; ++i) seen.push_back(point_of(p.verts, i));
            std::sort(seen.begin(), seen.end());
            if (std::adjacent_find(seen.begin(), seen.end()) != seen.end()) return 0;
        }
        if (di_)
            for (uint32_t k = 0; k < f_; ++k)
            {
                const uint32_t a = p.faces[3 * k], b = p.faces[3 * k + 1], c = p.faces[3 * k + 2];
                if (a == b || b == c || a == c) return 1;
            }
        if (au_)
        {
            std::vector<bool> used(v_, false);
            for (uint32_t x : p.faces) used[x] = true;
            if (std::find(used.begin(), used.end(), false) != used.end()) return 2;
        }
        return -1;
    }

    BigUint rank(const ModelSpace::Parts& p) const
    {
        BigUint r = BigUint::mul(vertex_rank(p.verts), fcount_);
        r += face_rank(p.faces);
        return r;
    }

    ModelSpace::Parts unrank(const BigUint& k) const
    {
        BigUint q, r;
        BigUint::divmod(k, fcount_, q, r);
        return {vertex_unrank(q), face_unrank(r)};
    }

private:
    uint32_t v_, f_, c_;
    bool dv_, di_, au_, can_rank_ = true;
    std::vector<BigUint> fall_;
    std::vector<std::vector<BigUint>> pow_, binom_;
    BigUint vcount_, fcount_, count_;

    uint64_t point_of(const Space::Digits& verts, uint32_t i) const
    {
        return (uint64_t(verts[3 * i]) * c_ + verts[3 * i + 1]) * c_ + verts[3 * i + 2];
    }
    BigUint face_ways(uint32_t n) const
    {
        if (di_) return n >= 3 ? BigUint::from_decimal(std::to_string(uint64_t(n) * (n - 1) * (n - 2))) : BigUint();
        BigUint w(n);
        w.mul_small(n);
        w.mul_small(n);
        return w;
    }
    // The rest of a face that has named k of its vertices (all distinct), with n to choose from.
    BigUint tail(uint32_t n, uint32_t k) const
    {
        BigUint t(1);
        for (uint32_t i = k; k > 0 && i < 3; ++i)
        {
            if (di_ && n <= i) return {};
            t.mul_small(di_ ? n - i : n);
        }
        return t;
    }
    // Completions with m vertices still unused, k of the current face named, and `left` whole
    // faces after it: every string over the V - j vertices that leaves out j chosen unused ones,
    // added and taken away in turn (inclusion and exclusion), or just V's when every vertex need
    // not be used.
    BigUint completions(uint32_t m, uint32_t k, uint32_t left) const
    {
        auto ways = [&](uint32_t n) { return BigUint::mul(tail(n, k), pow_[n][left]); };
        if (!au_) return ways(v_);
        BigUint plus, minus;
        for (uint32_t j = 0; j <= m; ++j)
        {
            const BigUint t = BigUint::mul(binom_[m][j], ways(v_ - j));
            (j % 2 ? minus : plus) += t;
        }
        return plus -= minus;
    }
    // The count alone at a shape too large for the tables.
    BigUint completions_direct(uint32_t m) const
    {
        auto ways = [&](uint32_t n) {
            BigUint w(1);
            const BigUint face = face_ways(n);
            for (uint32_t e = 0; e < f_; ++e) w = BigUint::mul(w, face);
            return w;
        };
        if (!au_) return ways(v_);
        BigUint plus, minus, b(1);
        for (uint32_t j = 0; j <= m; ++j)
        {
            (j % 2 ? minus : plus) += BigUint::mul(b, ways(v_ - j));
            b.mul_small(m - j);
            b.divmod_small(j + 1);
        }
        return plus -= minus;
    }

    BigUint vertex_rank(const Space::Digits& verts) const
    {
        if (!dv_) return BigUint::from_digits(verts, c_);
        BigUint r;
        std::vector<uint64_t> taken;
        for (uint32_t i = 0; i < v_; ++i)
        {
            const uint64_t pt = point_of(verts, i);
            uint64_t below = pt;
            for (uint64_t t : taken) below -= t < pt ? 1 : 0;
            r += BigUint::mul(BigUint::from_decimal(std::to_string(below)), fall_[i + 1]);
            taken.push_back(pt);
        }
        return r;
    }
    Space::Digits vertex_unrank(const BigUint& k) const
    {
        if (!dv_) return k.to_digits(c_, 3 * v_);
        Space::Digits out;
        std::vector<uint64_t> taken;
        BigUint rest = k;
        for (uint32_t i = 0; i < v_; ++i)
        {
            BigUint q, r;
            BigUint::divmod(rest, fall_[i + 1], q, r);
            rest = r;
            // The q-th point not yet taken.
            uint64_t pt = std::stoull(q.to_decimal());
            std::sort(taken.begin(), taken.end());
            for (uint64_t t : taken) pt += t <= pt ? 1 : 0;
            taken.push_back(pt);
            out.push_back(uint32_t(pt / (uint64_t(c_) * c_)));
            out.push_back(uint32_t(pt / c_ % c_));
            out.push_back(uint32_t(pt % c_));
        }
        return out;
    }

    // The faces walked digit by digit; `pick` either adds up the completions below each digit
    // (rank) or chooses the digit whose block holds k (unrank).
    BigUint face_rank(const Space::Digits& faces) const
    {
        if (!dv_ && !di_ && !au_) return BigUint::from_digits(faces, v_);
        BigUint r;
        std::vector<bool> used(v_, false);
        uint32_t unused = v_;
        for (uint32_t p = 0; p < 3 * f_; ++p)
        {
            const uint32_t k = p % 3, left = f_ - p / 3 - 1, d = faces[p];
            for (uint32_t s = 0; s < d; ++s)
            {
                if (di_ && named(faces, p, k, s)) continue;
                r += completions(unused - (used[s] ? 0 : 1), (k + 1) % 3, left);
            }
            if (!used[d]) --unused;
            used[d] = true;
        }
        return r;
    }
    Space::Digits face_unrank(const BigUint& k) const
    {
        if (!dv_ && !di_ && !au_) return k.to_digits(v_, 3 * f_);
        Space::Digits faces(3 * f_, 0);
        std::vector<bool> used(v_, false);
        uint32_t unused = v_;
        BigUint rest = k;
        for (uint32_t p = 0; p < 3 * f_; ++p)
        {
            const uint32_t at = p % 3, left = f_ - p / 3 - 1;
            uint32_t s = 0;
            for (;; ++s)
            {
                if (s >= v_) throw std::logic_error("face unrank ran past the vertices");
                if (di_ && named(faces, p, at, s)) continue;
                const BigUint block = completions(unused - (used[s] ? 0 : 1), (at + 1) % 3, left);
                if (rest < block) break;
                rest -= block;
            }
            faces[p] = s;
            if (!used[s]) --unused;
            used[s] = true;
        }
        return faces;
    }
    // Whether s is one of the k vertices the face at position p has already named.
    static bool named(const Space::Digits& faces, uint32_t p, uint32_t k, uint32_t s)
    {
        for (uint32_t i = 1; i <= k; ++i)
            if (faces[p - i] == s) return true;
        return false;
    }
};

// ---------------------------------------------------------------- ModelSieve

ModelSieve::ModelSieve() = default;
ModelSieve::~ModelSieve() = default;
ModelSieve::ModelSieve(ModelSieve&&) noexcept = default;
ModelSieve& ModelSieve::operator=(ModelSieve&&) noexcept = default;

ModelSieve::ModelSieve(const ModelSpace& space, const std::vector<FilterStack::Entry>& entries) : space_(space), size_(space.size())
{
    provenance_ = space.id();
    bool dv = false, di = false, au = false;
    for (const auto& e : entries)
    {
        const std::string& id = e.spec->id;
        if (std::find(ids_.begin(), ids_.end(), id) != ids_.end()) continue; // ticked twice: once is enough
        if (id == "not-a-file") files_ = std::make_unique<NumberFiles>(size_);
        else if (id == "distinct-vertices") dv = true;
        else if (id == "distinct-indices") di = true;
        else if (id == "every-vertex-used") au = true;
        else throw std::invalid_argument(e.spec->name() + " does not apply to the models line");
        names_.push_back(e.spec->name());
        ids_.push_back(id);
        provenance_ += "; " + e.spec->name() + (id == "not-a-file" ? "{" + std::string(kFileKindsVersion) + " line=binary-v1}" : "{}");
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));
    if (dv || di || au) rules_ = std::make_unique<MeshRules>(space.vertices(), space.face_count(), space.coords(), dv, di, au);
    if (files_ && rules_)
    {
        can_rank_ = false;
        blocker_ = "not-a-file-v1 can rank, but not together with the models line's own filters";
    }
    else if (rules_ && !rules_->can_rank())
    {
        can_rank_ = false;
        blocker_ = "the models line's filters count this shape but are too large to rank it: they judge only";
    }
    count_ = files_ && !rules_ ? files_->count() : rules_ && !files_ ? rules_->count() : files_ ? BigUint() : size_;
    shuffle_ = std::make_unique<Shuffle>(count_.is_zero() ? BigUint(1) : count_, space.key(), id_);
    BigUint top = count_;
    if (!top.is_zero()) top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::string ModelSieve::first_failure(const BigUint& index) const
{
    if (names_.empty()) return {};
    const int rule = rules_ ? rules_->first_failure(space_->parts_at(index, AddressMode::Positional)) : -1;
    static const char* const kRuleIds[] = {"distinct-vertices", "distinct-indices", "every-vertex-used"};
    for (size_t i = 0; i < ids_.size(); ++i)
    {
        if (ids_[i] == "not-a-file" && !files_->passes(index)) return names_[i];
        if (rule >= 0 && ids_[i] == kRuleIds[rule]) return names_[i];
    }
    return {};
}

BigUint ModelSieve::rank(const BigUint& positional) const
{
    if (files_) return files_->kept_below(positional);
    if (rules_) return rules_->rank(space_->parts_at(positional, AddressMode::Positional));
    return positional;
}

BigUint ModelSieve::unrank(const BigUint& k) const
{
    if (files_) return files_->unrank(k);
    if (rules_) return space_->index_of(rules_->unrank(k), AddressMode::Positional);
    return k;
}

BigUint ModelSieve::index_of(const BigUint& positional, AddressMode m) const
{
    if (!can_rank_) throw std::logic_error("this models sieve cannot rank: " + blocker_);
    if (!(positional < size_) || !first_failure(positional).empty()) throw std::invalid_argument("the model is not a survivor");
    const BigUint r = rank(positional);
    return m == AddressMode::Scrambled ? shuffle_->forward(r) : r;
}

BigUint ModelSieve::rank_of_index(const BigUint& compact, AddressMode m) const
{
    return m == AddressMode::Scrambled ? shuffle_->inverse(compact) : compact;
}

BigUint ModelSieve::model_at(const BigUint& compact, AddressMode m) const
{
    if (!can_rank_) throw std::logic_error("this models sieve cannot rank: " + blocker_);
    if (!(compact < count_)) throw std::out_of_range("compact address beyond the survivors");
    return unrank(rank_of_index(compact, m));
}

std::string ModelSieve::hex_of(const BigUint& compact) const { return compact.to_hex(hex_width_); }

BigUint ModelSieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (!(v < count_)) throw std::out_of_range("compact address beyond the survivors");
    return v;
}

} // namespace sieve
