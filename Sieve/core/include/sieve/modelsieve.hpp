// Sieve — the models line's filter stack (SPECIFICATIONS §12, FILTER-PLUGINS §16): what the models
// line sets aside, counted and ranked exactly, and its compact orderings.
//
// A model is not a run of symbols of one base, as the other lines' units are: it is a mixed-radix
// number (coordinates in base C, then face indices in base V; sieve/modelspace.hpp), and its
// positional index is that number. So its stack is its own, as the binary line's is (BinarySieve),
// and judges a model by its positional index. It holds:
//   not-a-file-v1         the model's own number, read as a place on the binary line (binary-v1),
//                         does not hold a file whose first bytes carry a signature (file-kinds-v1).
//                         As on every other line: with E(x) the signed files at binary places below
//                         x (KindCounter::count_before, from the file's head alone), the survivors
//                         below x are S(x) = x - E(x); the k-th is found by halving.
//   canonical-mesh-v1     one encoding of each mesh: vertices in increasing order, faces rotated to
//                         their smallest index and in increasing order; C(P, V) x C(T, F), with
//                         every-vertex-used by inclusion and exclusion (modelsieve.cpp)
//   distinct-vertices-v1, distinct-indices-v1, every-vertex-used-v1: the line's own rules
//                         (core/src/filters/models.cpp), counted in closed form. A model is its
//                         vertices (base C) then its faces (base V), and the rules on each part
//                         are separate, so the survivors are (vertex strings kept) x (face strings
//                         kept), and a survivor's number is (its vertices' rank) x (face strings
//                         kept) + (its faces' rank). Each part is ranked digit by digit: below each
//                         digit, the completions of every smaller one, which depend only on how
//                         many points are taken (vertices) or on how many vertices are still
//                         unused and where in its face the walk is (faces).
// not-a-file-v1 is arithmetic on the whole number, and the rules walk its digits, so the two
// together are judged but not counted (hide, not compact), as on the other lines.
// The other lines' content a model could be is already taken care of from their side: a page
// holding a model's .obj text (not-other-line-v1) and a file that is a model's .obj
// (not-an-item-v1).
//
// Compact: survivor k stands at compact address k (positional), or at shuffle-sha256-v1 of k over
// the survivors, keyed with the line's key and the stack's id as its domain (scrambled), as the
// other lines' compact orderings are (sieve/compact.hpp).
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/filekind.hpp"
#include "sieve/filter.hpp"
#include "sieve/modelspace.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

// The survivors of not-a-file-v1 among the numbers [0, size): counted, ranked and unranked
// exactly. The same rule as not-a-file-v1 on the other lines, worked on the number itself.
class NumberFiles
{
public:
    explicit NumberFiles(const BigUint& size);
    const BigUint& count() const { return count_; }
    bool passes(const BigUint& x) const;          // x's file on the binary line has no signature
    BigUint kept_below(const BigUint& x) const;  // survivors among [0, x)
    BigUint unrank(const BigUint& k) const;       // the k-th survivor, k < count()

private:
    BigUint size_, count_;
    std::unique_ptr<KindCounter> signed_;
};

class MeshRules;     // the line's own rules, counted and ranked (modelsieve.cpp)
class CanonicalMesh; // canonical-mesh-v1: one encoding of each mesh (modelsieve.cpp)

// The canonical encoding of a mesh (canonical-mesh-v1): its vertices in increasing order as grid
// points, each face rotated to start at its smallest index (keeping its winding), its faces in
// increasing order. None when the mesh has none: two vertices at one point, a face naming a vertex
// twice, or a face twice.
std::optional<ModelSpace::Parts> canonical_mesh(const ModelSpace& space, const ModelSpace::Parts& parts);

class ModelSieve
{
public:
    ModelSieve();
    ~ModelSieve();
    ModelSieve(ModelSieve&&) noexcept;
    ModelSieve& operator=(ModelSieve&&) noexcept;
    // The models line's ticked filters. Throws on a filter that does not apply.
    ModelSieve(const ModelSpace& space, const std::vector<FilterStack::Entry>& entries);

    bool empty() const { return names_.empty(); }
    size_t size() const { return names_.size(); }
    // The first filter the model (by its positional index) fails, or "".
    std::string first_failure(const BigUint& index) const;
    // Whether the survivors can be counted and ranked (not with not-a-file-v1 and the line's own
    // rules together, nor past the ranking budget), and why not.
    bool can_rank() const { return can_rank_; }
    const std::string& blocker() const { return blocker_; }
    const BigUint& count() const { return count_; } // can_rank() only
    const std::string& provenance() const { return provenance_; }
    const std::string& id() const { return id_; }

    // Compact addresses (can_rank() only): a survivor's positional index to its compact address,
    // and back. index_of throws std::invalid_argument when the model is not a survivor.
    BigUint index_of(const BigUint& positional, AddressMode m) const;
    BigUint model_at(const BigUint& compact, AddressMode m) const;
    BigUint rank_of_index(const BigUint& compact, AddressMode m) const; // its survivor number
    std::string hex_of(const BigUint& compact) const;
    BigUint parse(std::string_view hex) const;

private:
    std::optional<ModelSpace> space_;
    std::vector<std::string> names_, ids_;
    std::unique_ptr<NumberFiles> files_; // not-a-file-v1, when ticked
    std::unique_ptr<MeshRules> rules_;   // the line's own rules, when any is ticked
    std::unique_ptr<CanonicalMesh> canon_; // canonical-mesh-v1, when ticked: it counts the line then
    BigUint size_, count_;
    bool can_rank_ = true;
    std::string blocker_;
    std::unique_ptr<Shuffle> shuffle_;
    size_t hex_width_ = 1;
    std::string provenance_, id_;
    BigUint rank(const BigUint& positional) const;
    BigUint unrank(const BigUint& k) const;
};

} // namespace sieve
