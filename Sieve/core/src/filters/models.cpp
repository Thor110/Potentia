// Sieve — the models line's own filters (SPECIFICATIONS §12, the first two tiers). A model is V
// vertices on a grid of C steps per axis and F triangles naming them (sieve/modelspace.hpp):
//   distinct-vertices-v1  no two vertices stand at the same point of the grid
//   distinct-indices-v1   no face names the same vertex twice (no degenerate triangles)
//   every-vertex-used-v1  every vertex is named by at least one face
// A model is a mixed-radix number, not a run of symbols of one base, so these are judged, counted
// and ranked by the models line's own stack (ModelSieve, sieve/modelsieve.hpp), in closed form:
//   vertices  distinct: C^3 (C^3 - 1) ... (C^3 - V + 1) of the C^(3V) coordinate strings
//   faces     with n vertices to choose from, one face is n(n-1)(n-2) ways when its indices must
//             be distinct and n^3 otherwise; "every vertex used" by inclusion and exclusion over
//             the vertices left out: sum over j of (-1)^j C(V, j) (the face count with V - j)^F
// The specs here give the filters their names and places in every list.

#include "sieve/filter.hpp"

#include <stdexcept>

namespace sieve {

void add_model_filters(std::vector<FilterSpec>& out)
{
    auto spec = [&](const char* id, const char* description) {
        FilterSpec s;
        s.id = id;
        s.title = id;
        s.description = description;
        s.applies = [](const FilterLine& l) { return l.kind == "models"; };
        s.counts_as = "model-rule";
        s.make = [](const FilterLine&, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
            throw std::invalid_argument("the models line's filters judge models (a ModelSieve), not units of digits");
        };
        out.push_back(s);
    };
    spec("distinct-vertices", "No two vertices at the same point of the grid. Exact: C^3 (C^3 - 1) ... (C^3 - V + 1) "
                              "of the coordinate strings pass, counted and ranked at any shape.");
    spec("distinct-indices", "No face names the same vertex twice: no degenerate triangles. Exact: each face is "
                             "V(V-1)(V-2) ways instead of V^3.");
    spec("every-vertex-used", "Every vertex is named by at least one face: no stray points. Exact, by inclusion and "
                              "exclusion over the vertices left out; ranks while the shape is small enough.");
}

} // namespace sieve
