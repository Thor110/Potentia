// Sieve — worlds: models placed in a world (SPECIFICATIONS §12.0; IDEAS §13, §14, §16.7).
//
// A world is a cover (one unit of the image line), a title (one unit of the titled lines' title
// space, or none when titles are off) and N slots, each a model of the models line placed in the
// world: its cell, one of a grid of G cells along each axis, and its turn, one of the 24 rotations
// that take a cube onto itself. So a worlds dimension holds
//     |cover| * |title| * (|models| * G^3 * 24)^N
// worlds, and "worldspace-v1" numbers them as composition-v1 numbers tracks:
//   positional  one mixed-radix number, most significant first: the cover's digits (base |cover
//               symbols|), the title's (base |title symbols|), then each slot in turn: its model's
//               positional index on the models line (base |models|), its cell's x, y and z (base G
//               each) and its turn (base 24). Neighbours differ in the last slot's turn.
//   scrambled   the positional index passed through shuffle-sha256-v1 (sieve/compact.hpp) over
//               [0, N), keyed with the models line's key, domain = the world space's id.
// Addresses are that index in hex, zero-padded to the width N - 1 needs.
//
// The turns, "turns-24-v1": every 3x3 matrix with one entry of +1 or -1 in each row and column and
// determinant +1. Row i takes the old coordinate perm[i] times sign[i]. They are ordered by the
// permutation (perm read as a word, in lexicographic order: 012, 021, 102, 120, 201, 210), then by
// the signs (rows 0, 1, 2 as the bits 4, 2, 1 of a number counting up from 0, a set bit meaning
// minus), keeping those of determinant +1: four for each permutation. Turn 0 is the identity.
//
// Placing: a model's coordinates lie in (-1, 1), each a multiple of 1/C (modelspace-v1); turned,
// they still do. Cell (x, y, z) has its centre at (2x + 1 - G, 2y + 1 - G, 2z + 1 - G), so a world
// is G model-widths along each axis and each model fills its cell. A world's coordinates are all
// multiples of 1/C, so its .obj text is exact: every slot's vertices placed, then every slot's faces,
// their indices moved past the slots before it.
//
// Filters ("world-compact-v1"): a world passes when its cover passes the cover stack, its title the
// title stack, and every slot's model the models line's stack (ModelSieve). A slot's place is not
// filtered. When every part with filters can rank its survivors, so can the worlds: there are
//     Nc * Nt * (Nm * G^3 * 24)^N
// of them, survivor k having ranks (kc, kt, then for each slot its model's survivor number and its
// place) as one mixed-radix number, cover first, so survivors keep the positional order. Compact
// worlds: positional = k; scrambled = shuffle-sha256-v1 of k over [0, count), keyed with the models
// line's key, domain "world-compact-v1/<space id>/<cover>/<title>/<models>", each part its stack's
// id or "-" for a part with no filters.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/filter.hpp"
#include "sieve/modelsieve.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/space.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kWorldSpaceVersion = "worldspace-v1";
inline constexpr const char* kWorldCompactVersion = "world-compact-v1";
inline constexpr const char* kTurnsVersion = "turns-24-v1";
inline constexpr uint32_t kTurns = 24;

// A turn of turns-24-v1, as its row permutation and signs: new[i] = sign[i] * old[perm[i]].
struct Turn
{
    std::array<uint8_t, 3> perm;
    std::array<int8_t, 3> sign;
};
const std::array<Turn, kTurns>& turns();

class WorldSpace
{
public:
    using Digits = Space::Digits;
    struct Slot
    {
        BigUint model;          // its positional index on the models line
        uint32_t x = 0, y = 0, z = 0; // its cell, each below G
        uint32_t turn = 0;      // below 24
        bool operator==(const Slot&) const = default;
    };
    struct Parts
    {
        Digits cover, title; // title empty when the space has none
        std::vector<Slot> slots;
        bool operator==(const Parts&) const = default;
    };

    // `title` is nullopt for no title. slots >= 1; grid in [1, 1024].
    WorldSpace(const Space& cover, std::optional<Space> title, const ModelSpace& models, uint32_t slots, uint32_t grid);

    const Space& cover_space() const { return cover_; }
    const std::optional<Space>& title_space() const { return title_; }
    const ModelSpace& model_space() const { return models_; }
    uint32_t slots() const { return slots_; }
    uint32_t grid() const { return grid_; }
    uint64_t places() const { return places_; } // a slot's places: G^3 * 24
    const BigUint& slot_size() const { return slot_size_; } // |models| * places()
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    // "worlds/<cover symbols>/L<n>+<title symbols>/L<n>+V<v>/F<f>/C<c>x<N>/G<g>/key=<key>/worldspace-v1",
    // the title written "-" when there is none.
    std::string id() const;

    BigUint index_of(const Parts& p, AddressMode m) const; // throws if p is not of this shape
    Parts parts_at(const BigUint& index, AddressMode m) const; // index < size()
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked
    void check(const Parts& p) const;          // throws std::invalid_argument if p is not of this shape

    // A slot's place (its cell and turn) as one number below places(), and back.
    uint64_t place_of(const Slot& s) const;
    void set_place(Slot& s, uint64_t place) const;

    // The world as one mesh: every slot's model turned and moved to its cell. Coordinates in model
    // widths, exact multiples of 1/C.
    struct Mesh
    {
        std::vector<ModelSpace::Vertex> vertices;
        std::vector<ModelSpace::Face> faces;
    };
    Mesh mesh_of(const Parts& p) const;
    // Its .obj text: fixed width, line feeds only, as the models line's (every coordinate with its
    // sign, its whole part to the width the grid needs and log2(C) places).
    std::string to_obj(const Parts& p) const;

private:
    Space cover_;
    std::optional<Space> title_;
    ModelSpace models_;
    uint32_t slots_, grid_;
    uint64_t places_;
    BigUint slot_size_, size_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
};

class WorldSieve
{
public:
    // The stacks are not owned and must outlive this object; any may be empty. `title` is ignored
    // when the space has no title.
    WorldSieve(const WorldSpace& space, const FilterStack& cover, const FilterStack& title, const ModelSieve& models);

    bool empty() const { return cover_->empty() && title_->empty() && models_->empty(); }
    // "" if it passes, else which part fails and the filter: "cover: palette-size-v1",
    // "model 3: canonical-mesh-v1".
    std::string first_failure(const WorldSpace::Parts& p) const;

    bool can_rank() const { return ranks_; }
    const std::string& blocker() const { return blocker_; }
    const BigUint& count() const { return count_; }
    BigUint rank(const WorldSpace::Parts& p) const;   // throws if it does not pass
    WorldSpace::Parts unrank(const BigUint& k) const; // k < count()

    BigUint index_of(const WorldSpace::Parts& p, AddressMode m) const;
    BigUint index_of_rank(const BigUint& k, AddressMode m) const;
    WorldSpace::Parts parts_at(const BigUint& index, AddressMode m) const;
    size_t hex_width() const { return hex_width_; }
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const;
    std::string domain() const;

private:
    const WorldSpace* space_;
    const FilterStack *cover_, *title_;
    const ModelSieve* models_;
    BigUint cover_count_, title_count_, model_count_, slot_count_, slots_count_;
    bool ranks_ = false;
    std::string blocker_;
    BigUint count_;
    size_t hex_width_ = 1;
    std::unique_ptr<Shuffle> shuffle_;
    BigUint model_rank(const BigUint& positional) const;
    BigUint model_unrank(const BigUint& k) const;
};

} // namespace sieve
