// Sieve — editing a character by its joints (IDEAS 19.203): limbs dragged, grown and taken away, joints
// added and taken away, on every side of a mirrored character at once.
//
// An edited character is no longer made from its seed alone, so the first edit turns it into a character
// of given spheres that keeps its symmetry (sieve-character-v4, character.hpp; v3 where it has none). A
// seed-made character's images are found from its unfolding: under bilateral symmetry each sphere off the
// centre plane and its mirror (x negated, its parent the mirror of its parent's), under radial symmetry
// each limb's spheres and the first limb's at the same place in the limb, turned as the limb is.
//
// Every edit is made to the chosen sphere and to each of its images, carried across by the symmetry: what
// is done to one side is mirrored or turned onto the others. A sphere alone in its set under bilateral
// symmetry is on the centre plane and stays there; one alone on the upright under radial symmetry (the
// body) grows a limb at each of the character's turns. The body itself is never moved or taken away.
//
// After each edit the spheres are put back in their order (each parent before its children, a sphere's
// children in the order they were made), the held models kept with their spheres (dropped with a sphere
// taken away), and the result checked as a read file is. Lengths are the character's units, rounded to
// whole units once an edit is worked out, so the text, and with it the SHA-256, is the same each time the
// same edits are made to the same character.
#pragma once

#include "sieve/character.hpp"

#include <cstdint>
#include <vector>

namespace sieve {

// The character as one that can be edited: a seed-made one as its spheres, with its symmetry and the
// images found as above; one of given spheres as it is.
Character editable_character(const Character& c);

// The chosen sphere and its images (the chosen first), in the character as it is now.
std::vector<uint32_t> sphere_images(const Character& c, uint32_t sphere);

// Each edit throws std::invalid_argument, the character unchanged, where it cannot be made (the body
// moved or taken away, a sphere that is not there, more than 4,096 spheres). Each returns the sphere to
// choose after it, as numbered after it.
//
// Moves the sphere to `to` (x, y, z in the character's units) and each image to its image of the move;
// the limb beyond each comes with it. Its bone keeps its length (the point taken onto the sphere round its
// parent at that length) unless `stretch`.
uint32_t edit_move(Character& c, uint32_t sphere, const double to[3], bool stretch);
// Moves the sphere by (dx, dy, dz) as edit_move does.
uint32_t edit_nudge(Character& c, uint32_t sphere, const double by[3], bool stretch);
// A new limb of one joint from the sphere, in the direction of the 25 not straight up farthest from its
// bones (the first such in character.hpp's order), its radius three quarters of the sphere's (at least
// 8), touching it. Returns the new joint.
uint32_t edit_grow(Character& c, uint32_t sphere);
// Takes the sphere away and everything beyond it. Returns its parent.
uint32_t edit_remove_limb(Character& c, uint32_t sphere);
// A joint added after the sphere, one bone (the sphere's own, from its parent) further along, of its
// radius: what was beyond the sphere moves out by that bone. Returns the new joint.
uint32_t edit_add_joint(Character& c, uint32_t sphere);
// The sphere taken out of its limb: what was beyond it joins its parent, moved in by its bone. Returns its
// parent.
uint32_t edit_remove_joint(Character& c, uint32_t sphere);

} // namespace sieve
