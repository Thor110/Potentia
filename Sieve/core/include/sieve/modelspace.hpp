// Sieve — the models line: every possible mesh of one shape (SPECIFICATIONS §12).
//
// A model of this line is V vertices and F triangular faces:
//   vertices  3V coordinates, each a step on a grid of C points per axis
//   faces     3F vertex indices, each in [0, V)
// So there are
//     N = C^(3V) * V^(3F)
// models, and "modelspace-v1" numbers them:
//   positional  the model's index is its digits read as one mixed-radix number, most significant
//               first: every coordinate (base C), then every face index (base V). Neighbouring
//               models differ in the last face.
//   scrambled   the positional index passed through shuffle-sha256-v1 over [0, N), keyed with the
//               space's key, domain = the model space's id. Neighbours are unrelated models.
// Addresses are that index in hex, zero-padded to the width N - 1 needs.
//
// C is a power of two, so coordinate d is exactly (2d + 1 - C) / C, which is a terminating decimal
// with log2(C) places: the canonical .obj text of a model is exact, fixed-width, and reads back
// to the same address. That is what lets a models unit also be a text unit on an ascii96 line
// (SPECIFICATIONS §3): the same object, addressed on two lines.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/space.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kModelSpaceVersion = "modelspace-v1";

class ModelSpace
{
public:
    using Digits = Space::Digits;
    struct Parts
    {
        Digits verts; // 3V coordinates, vertex by vertex, x then y then z
        Digits faces; // 3F vertex indices, face by face
        bool operator==(const Parts&) const = default;
    };
    // One vertex's coordinates and one face's indices, for working with a mesh rather than digits.
    struct Vertex
    {
        float x = 0, y = 0, z = 0;
    };
    struct Face
    {
        uint32_t a = 0, b = 0, c = 0;
    };

    // vertices >= 3, faces >= 1, coords a power of two in [2, 4096].
    ModelSpace(uint32_t vertices, uint32_t faces, uint32_t coords, std::string key = "sieve");

    uint32_t vertices() const { return v_; }
    uint32_t face_count() const { return f_; }
    uint32_t coords() const { return c_; }
    uint32_t decimals() const { return decimals_; } // places a coordinate needs, = log2(C)
    const std::string& key() const { return key_; }
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    // "models/V8/F12/C16/key=sieve/modelspace-v1"
    std::string id() const;
    // Characters the canonical .obj text of one model takes, so it can be matched to a text line.
    size_t obj_length() const;

    BigUint index_of(const Parts& p, AddressMode m) const;
    Parts parts_at(const BigUint& index, AddressMode m) const; // index < size()
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked

    // Digits <-> a mesh. The grid is C cells across [-1, 1] and a digit holds the middle of its
    // cell, so coordinate d is exactly (2d + 1 - C) / C: symmetric about the origin, from
    // -1 + 1/C to 1 - 1/C. The origin itself is between two cells, as a pixel grid has no
    // centre pixel when it is an even number of pixels wide.
    std::vector<Vertex> mesh_of(const Parts& p) const;
    std::vector<Face> faces_of(const Parts& p) const;
    // The canonical .obj text: fixed width, line feeds only, no comments and nothing optional.
    std::string to_obj(const Parts& p) const;
    // The reverse, which accepts only that exact form (see obj_length). Throws on anything else.
    Parts from_obj(std::string_view obj) const;

    // Fits any mesh to this line: coordinates rounded to the nearest grid point after being
    // scaled to fit the grid, vertices past V dropped, faces past F dropped, missing faces
    // filled with face (0, 1, 2), missing vertices with the cell nearest the origin.
    struct Fitted
    {
        Parts parts;
        uint32_t vertices_dropped = 0, faces_dropped = 0, faces_added = 0;
        uint32_t indices_clamped = 0; // faces that named a vertex this line does not have
        float scale = 1;              // what the mesh was multiplied by to fit the grid
    };
    Fitted fit(const std::vector<Vertex>& verts, const std::vector<Face>& faces) const;

private:
    uint32_t v_, f_, c_, decimals_;
    std::string key_;
    Space vert_space_, face_space_;
    BigUint size_;
    size_t hex_width_;
    Shuffle shuffle_;
};

} // namespace sieve
