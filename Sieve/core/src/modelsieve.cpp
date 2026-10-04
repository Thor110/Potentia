// Sieve — the models line's filter stack: not-a-file-v1 on a model's positional index, and the
// line's own rules (distinct vertices, distinct indices, every vertex used) in closed form, counted
// and ranked exactly, and the compact orderings over its survivors. See sieve/modelsieve.hpp.

#include "sieve/modelsieve.hpp"

#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <optional>
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

// ---------------------------------------------------------------- CanonicalMesh

// canonical-mesh-v1 (core/src/filters/models.cpp): one encoding of each mesh. The same geometry
// can be written with its vertices in any order, its faces in any order, and each face started
// at any of its three corners; the canonical one has its vertices in increasing order as grid
// points, each face rotated so that its smallest index comes first (rotation keeps the winding, so
// a face and its mirror stay apart), and its faces in increasing order as triples.
//
// So the vertex strings kept are the increasing sequences of V of the P = C^3 points, C(P, V) of
// them, and the face strings the increasing sequences of F of the T = V(V-1)(V-2)/3 rotated
// triples, C(T, F). Both are ranked as combinations in lexicographic order, which is the models
// line's positional order of their digits, with the hockey-stick identity
//     sum over q in [a, b) of C(n - 1 - q, k) = C(n - a, k + 1) - C(n - b, k + 1)
// for the block below each element. With every-vertex-used, the faces still to come must name
// every vertex not yet used: by inclusion and exclusion over the subsets S of those vertices,
//     completions = sum over S of (-1)^|S| C(triples from here on that avoid S, faces left),
// with the triples avoiding each S counted once into a table. That table has 2^V rows of T + 1
// counts, so with every-vertex-used the faces rank as far as the table fits in the filter memory
// (sieve/plugin.hpp: 16 vertices in 512 MB, where it takes 294 MB; ranking stays quick, a few
// milliseconds) and are counted, in closed form, beyond: sum over j of (-1)^j C(V, j) C(T(V - j), F).
class CanonicalMesh
{
public:
    CanonicalMesh(uint32_t v, uint32_t f, uint32_t c, bool all_used) : v_(v), f_(f), c_(c), au_(all_used)
    {
        points_ = uint64_t(c) * c * c;
        triples_ = triples_on(v);
        vcount_ = binom(points_, v);
        if (au_)
        {
            BigUint plus, minus;
            for (uint32_t j = 0; j <= v; ++j)
                (j % 2 ? minus : plus) += BigUint::mul(binom(v, j), binom(triples_on(v - j), f));
            fcount_ = plus -= minus;
            table_bytes_ = avoid_table_bytes(v, f);
            can_rank_ = v < 32 && table_bytes_ <= filter_memory(); // (a set of vertices is a 32-bit mask)
            if (can_rank_) build_avoid_table();
        }
        else fcount_ = binom(triples_, f);
        count_ = BigUint::mul(vcount_, fcount_);
    }

    bool can_rank() const { return can_rank_; }
    const BigUint& count() const { return count_; }
    double table_bytes() const { return table_bytes_; } // every-vertex-used's table (0 without it)

    bool passes(const ModelSpace::Parts& p) const
    {
        for (uint32_t i = 1; i < v_; ++i)
            if (point_of(p.verts, i - 1) >= point_of(p.verts, i)) return false;
        for (uint32_t k = 0; k < f_; ++k)
        {
            const uint32_t a = p.faces[3 * k], b = p.faces[3 * k + 1], c = p.faces[3 * k + 2];
            if (!(a < b && a < c && b != c)) return false;
            if (k > 0 && !(face_key(p.faces, k - 1) < face_key(p.faces, k))) return false;
        }
        if (au_)
        {
            std::vector<bool> used(v_, false);
            for (uint32_t x : p.faces) used[x] = true;
            if (std::find(used.begin(), used.end(), false) != used.end()) return false;
        }
        return true;
    }

    BigUint rank(const ModelSpace::Parts& p) const
    {
        std::vector<uint64_t> pts(v_), tri(f_);
        for (uint32_t i = 0; i < v_; ++i) pts[i] = point_of(p.verts, i);
        for (uint32_t k = 0; k < f_; ++k) tri[k] = triple_index(p.faces[3 * k], p.faces[3 * k + 1], p.faces[3 * k + 2]);
        BigUint r = BigUint::mul(comb_rank(pts, points_), fcount_);
        r += au_ ? used_rank(tri) : comb_rank(tri, triples_);
        return r;
    }

    ModelSpace::Parts unrank(const BigUint& k) const
    {
        BigUint q, r;
        BigUint::divmod(k, fcount_, q, r);
        ModelSpace::Parts p;
        for (uint64_t pt : comb_unrank(q, points_, v_))
        {
            p.verts.push_back(uint32_t(pt / (uint64_t(c_) * c_)));
            p.verts.push_back(uint32_t(pt / c_ % c_));
            p.verts.push_back(uint32_t(pt % c_));
        }
        for (uint64_t t : au_ ? used_unrank(r) : comb_unrank(r, triples_, f_))
        {
            const auto [a, b, c] = triple_at(t);
            p.faces.push_back(a);
            p.faces.push_back(b);
            p.faces.push_back(c);
        }
        return p;
    }

    // The canonical encoding of a mesh, if it has one: no two vertices at one point, no face
    // naming a vertex twice, no face twice (as rotated triples), and with every-vertex-used, no
    // vertex unnamed. Faces keep their winding.
    std::optional<ModelSpace::Parts> canonical_form(const ModelSpace::Parts& p) const
    {
        std::vector<std::pair<uint64_t, uint32_t>> order;
        for (uint32_t i = 0; i < v_; ++i) order.emplace_back(point_of(p.verts, i), i);
        std::sort(order.begin(), order.end());
        std::vector<uint32_t> new_index(v_);
        ModelSpace::Parts out;
        for (uint32_t i = 0; i < v_; ++i)
        {
            if (i > 0 && order[i].first == order[i - 1].first) return std::nullopt;
            new_index[order[i].second] = i;
            for (uint32_t d = 0; d < 3; ++d) out.verts.push_back(p.verts[3 * order[i].second + d]);
        }
        std::vector<std::array<uint32_t, 3>> faces;
        for (uint32_t k = 0; k < f_; ++k)
        {
            std::array<uint32_t, 3> t{new_index[p.faces[3 * k]], new_index[p.faces[3 * k + 1]], new_index[p.faces[3 * k + 2]]};
            if (t[0] == t[1] || t[1] == t[2] || t[0] == t[2]) return std::nullopt;
            std::rotate(t.begin(), std::min_element(t.begin(), t.end()), t.end());
            faces.push_back(t);
        }
        std::sort(faces.begin(), faces.end());
        if (std::adjacent_find(faces.begin(), faces.end()) != faces.end()) return std::nullopt;
        for (const auto& t : faces) out.faces.insert(out.faces.end(), t.begin(), t.end());
        if (!passes(out)) return std::nullopt;
        return out;
    }

private:
    uint32_t v_, f_, c_;
    bool au_, can_rank_ = true;
    double table_bytes_ = 0;
    uint64_t points_ = 0, triples_ = 0;
    BigUint vcount_, fcount_, count_;
    // every-vertex-used: avoid_[S][i] is the number of triples from index i on naming no vertex of
    // the set S, and bin_[n][r] = C(n, r) for n up to T and r up to F.
    std::vector<std::vector<uint32_t>> avoid_;
    std::vector<std::vector<BigUint>> bin_;

    static uint64_t triples_on(uint64_t n) { return n < 3 ? 0 : n * (n - 1) * (n - 2) / 3; }
    // The memory build_avoid_table takes: avoid_'s 2^V rows of T + 1 counts (and each row's own
    // header), and bin_'s (T + 1)(F + 1) binomials, none longer than C(T, F).
    static double avoid_table_bytes(uint32_t v, uint32_t f)
    {
        const double t = double(triples_on(v)) + 1;
        const double row = t * sizeof(uint32_t) + sizeof(std::vector<uint32_t>);
        const double binomial = sizeof(BigUint) + 8.0 * std::ceil((double(f) * std::log2(std::max(t, 2.0)) + 64) / 64);
        return std::ldexp(row, int(std::min<uint32_t>(v, 1000))) + t * (double(f) + 1) * binomial;
    }

    // C(n, k), exactly: each partial product of i + 1 consecutive integers is divisible by (i + 1)!.
    static BigUint binom(uint64_t n, uint64_t k)
    {
        if (k > n) return {};
        k = std::min(k, n - k);
        BigUint b(1);
        for (uint64_t i = 0; i < k; ++i)
        {
            b = BigUint::mul(b, BigUint(n - i));
            b.divmod_small(uint32_t(i + 1));
        }
        return b;
    }

    uint64_t point_of(const Space::Digits& verts, uint32_t i) const
    {
        return (uint64_t(verts[3 * i]) * c_ + verts[3 * i + 1]) * c_ + verts[3 * i + 2];
    }
    std::array<uint32_t, 3> face_key(const Space::Digits& faces, uint32_t k) const
    {
        return {faces[3 * k], faces[3 * k + 1], faces[3 * k + 2]};
    }

    // Rotated triples (a < b, a < c, b != c) in lexicographic order: with a first, there are
    // n(n - 1) of them, n = V - 1 - a, and each b > a has n - 1 partners c.
    uint64_t triple_index(uint32_t a, uint32_t b, uint32_t c) const
    {
        uint64_t i = 0;
        for (uint32_t x = 0; x < a; ++x)
        {
            const uint64_t n = v_ - 1 - x;
            i += n * (n - 1);
        }
        const uint64_t n = v_ - 1 - a;
        i += uint64_t(b - a - 1) * (n - 1);
        return i + (c - a - 1) - (c > b ? 1 : 0);
    }
    std::array<uint32_t, 3> triple_at(uint64_t i) const
    {
        uint32_t a = 0;
        for (;; ++a)
        {
            const uint64_t n = v_ - 1 - a, block = n * (n - 1);
            if (i < block) break;
            i -= block;
        }
        const uint64_t n = v_ - 1 - a;
        const uint32_t b = uint32_t(a + 1 + i / (n - 1));
        uint32_t c = uint32_t(a + 1 + i % (n - 1));
        if (c >= b) ++c;
        return {a, b, c};
    }

    // The place of an increasing sequence of k of n items among all of them, lexicographically.
    static BigUint comb_rank(const std::vector<uint64_t>& seq, uint64_t n)
    {
        const uint64_t k = seq.size();
        BigUint r;
        uint64_t from = 0;
        for (uint64_t i = 0; i < k; ++i)
        {
            // The sequences whose i-th item is below seq[i]: C(n - from, rest + 1) - C(n - seq[i], rest + 1).
            const uint64_t rest = k - 1 - i;
            BigUint below = binom(n - from, rest + 1);
            below -= binom(n - seq[i], rest + 1);
            r += below;
            from = seq[i] + 1;
        }
        return r;
    }
    static std::vector<uint64_t> comb_unrank(BigUint r, uint64_t n, uint64_t k)
    {
        std::vector<uint64_t> out;
        uint64_t from = 0;
        for (uint64_t i = 0; i < k; ++i)
        {
            const uint64_t rest = k - 1 - i;
            const BigUint all = binom(n - from, rest + 1);
            // The largest x in [from, n - 1 - rest] with all - C(n - x, rest + 1) <= r.
            uint64_t lo = from, hi = n - 1 - rest;
            while (lo < hi)
            {
                const uint64_t mid = lo + (hi - lo + 1) / 2;
                BigUint below = all;
                below -= binom(n - mid, rest + 1);
                if (below <= r) lo = mid;
                else hi = mid - 1;
            }
            BigUint below = all;
            below -= binom(n - lo, rest + 1);
            r -= below;
            out.push_back(lo);
            from = lo + 1;
        }
        return out;
    }

    // every-vertex-used.
    uint32_t mask_of(uint64_t t) const
    {
        const auto [a, b, c] = triple_at(t);
        return (1u << a) | (1u << b) | (1u << c);
    }
    void build_avoid_table()
    {
        const uint32_t sets = 1u << v_;
        std::vector<uint32_t> masks(triples_);
        for (uint64_t t = 0; t < triples_; ++t) masks[t] = mask_of(t);
        avoid_.assign(sets, std::vector<uint32_t>(triples_ + 1, 0));
        for (uint32_t s = 0; s < sets; ++s)
            for (uint64_t t = triples_; t-- > 0;) avoid_[s][t] = avoid_[s][t + 1] + ((masks[t] & s) == 0 ? 1 : 0);
        bin_.assign(triples_ + 1, std::vector<BigUint>(f_ + 1));
        for (uint64_t n = 0; n <= triples_; ++n)
        {
            bin_[n][0] = BigUint(1);
            for (uint32_t r = 1; r <= f_ && n > 0; ++r) bin_[n][r] = BigUint(bin_[n - 1][r - 1]) += bin_[n - 1][r];
        }
    }
    // The ways to choose `left` more triples from index `from` on that between them name every
    // vertex of `unused`.
    BigUint completions(uint32_t unused, uint64_t from, uint32_t left) const
    {
        BigUint plus, minus;
        for (uint32_t s = unused;; s = (s - 1) & unused)
        {
            const BigUint& term = bin_[avoid_[s][from]][left];
            (std::popcount(s) % 2 ? minus : plus) += term;
            if (s == 0) break;
        }
        return plus -= minus;
    }
    BigUint used_rank(const std::vector<uint64_t>& tri) const
    {
        BigUint r;
        uint32_t unused = (1u << v_) - 1;
        uint64_t from = 0;
        for (uint32_t j = 0; j < f_; ++j)
        {
            for (uint64_t q = from; q < tri[j]; ++q) r += completions(unused & ~mask_of(q), q + 1, f_ - 1 - j);
            unused &= ~mask_of(tri[j]);
            from = tri[j] + 1;
        }
        return r;
    }
    std::vector<uint64_t> used_unrank(BigUint r) const
    {
        std::vector<uint64_t> out;
        uint32_t unused = (1u << v_) - 1;
        uint64_t q = 0;
        for (uint32_t j = 0; j < f_; ++j)
        {
            for (;; ++q)
            {
                if (q >= triples_) throw std::logic_error("canonical face unrank ran past the triples");
                const BigUint block = completions(unused & ~mask_of(q), q + 1, f_ - 1 - j);
                if (r < block) break;
                r -= block;
            }
            out.push_back(q);
            unused &= ~mask_of(q);
            ++q;
        }
        return out;
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
    bool dv = false, di = false, au = false, cm = false;
    for (const auto& e : entries)
    {
        const std::string& id = e.spec->id;
        if (std::find(ids_.begin(), ids_.end(), id) != ids_.end()) continue; // ticked twice: once is enough
        if (id == "not-a-file") files_ = std::make_unique<NumberFiles>(size_);
        else if (id == "distinct-vertices") dv = true;
        else if (id == "distinct-indices") di = true;
        else if (id == "every-vertex-used") au = true;
        else if (id == "canonical-mesh") cm = true;
        else throw std::invalid_argument(e.spec->name() + " does not apply to the models line");
        names_.push_back(e.spec->name());
        ids_.push_back(id);
        provenance_ += "; " + e.spec->name() + (id == "not-a-file" ? "{" + std::string(kFileKindsVersion) + " line=binary-v1}" : "{}");
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));
    // canonical-mesh-v1 implies distinct vertices and distinct indices, and takes every vertex used
    // into its own count, so with it ticked the line is counted by it alone; the other rules still
    // judge, so a model is reported against the first ticked filter it fails.
    if (cm) canon_ = std::make_unique<CanonicalMesh>(space.vertices(), space.face_count(), space.coords(), au);
    if (dv || di || au) rules_ = std::make_unique<MeshRules>(space.vertices(), space.face_count(), space.coords(), dv, di, au);
    if (files_ && (rules_ || canon_))
    {
        can_rank_ = false;
        blocker_ = "not-a-file-v1 can rank, but not together with the models line's own filters";
    }
    else if (rules_ && !canon_ && !rules_->can_rank())
    {
        can_rank_ = false;
        blocker_ = "the models line's filters count this shape but are too large to rank it: they judge only";
    }
    else if (canon_ && !canon_->can_rank())
    {
        can_rank_ = false;
        blocker_ = over_table_limit("canonical-mesh-v1 with every-vertex-used-v1's table", canon_->table_bytes()) + ": it judges only";
    }
    count_ = files_ && !rules_ && !canon_ ? files_->count()
           : canon_ && !files_            ? canon_->count()
           : rules_ && !files_            ? rules_->count()
           : files_                       ? BigUint()
                                          : size_;
    shuffle_ = std::make_unique<Shuffle>(count_.is_zero() ? BigUint(1) : count_, space.key(), id_);
    BigUint top = count_;
    if (!top.is_zero()) top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

double ModelSieve::table_bytes() const { return canon_ ? canon_->table_bytes() : 0.0; }

std::string ModelSieve::first_failure(const BigUint& index) const
{
    if (names_.empty()) return {};
    const ModelSpace::Parts parts = (rules_ || canon_) ? space_->parts_at(index, AddressMode::Positional) : ModelSpace::Parts{};
    const int rule = rules_ ? rules_->first_failure(parts) : -1;
    const bool canonical = !canon_ || canon_->passes(parts);
    static const char* const kRuleIds[] = {"distinct-vertices", "distinct-indices", "every-vertex-used"};
    for (size_t i = 0; i < ids_.size(); ++i)
    {
        if (ids_[i] == "not-a-file" && !files_->passes(index)) return names_[i];
        if (rule >= 0 && ids_[i] == kRuleIds[rule]) return names_[i];
        if (!canonical && ids_[i] == "canonical-mesh") return names_[i];
    }
    return {};
}

BigUint ModelSieve::rank(const BigUint& positional) const
{
    if (files_) return files_->kept_below(positional);
    if (canon_) return canon_->rank(space_->parts_at(positional, AddressMode::Positional));
    if (rules_) return rules_->rank(space_->parts_at(positional, AddressMode::Positional));
    return positional;
}

BigUint ModelSieve::unrank(const BigUint& k) const
{
    if (files_) return files_->unrank(k);
    if (canon_) return space_->index_of(canon_->unrank(k), AddressMode::Positional);
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

std::optional<ModelSpace::Parts> canonical_mesh(const ModelSpace& space, const ModelSpace::Parts& parts)
{
    return CanonicalMesh(space.vertices(), space.face_count(), space.coords(), false).canonical_form(parts);
}

std::string ModelSieve::hex_of(const BigUint& compact) const { return compact.to_hex(hex_width_); }

BigUint ModelSieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (!(v < count_)) throw std::out_of_range("compact address beyond the survivors");
    return v;
}

} // namespace sieve
