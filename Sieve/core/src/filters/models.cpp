// Sieve — the models line's own filters (SPECIFICATIONS §12, the first two tiers). A model is V
// vertices on a grid of C steps per axis and F triangles naming them (sieve/modelspace.hpp):
//   distinct-vertices-v1  no two vertices stand at the same point of the grid
//   distinct-indices-v1   no face names the same vertex twice (no degenerate triangles)
//   every-vertex-used-v1  every vertex is named by at least one face
//   canonical-mesh-v1     one encoding of each mesh, so re-orderings of the same mesh are set aside
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
    auto spec = [&](const char* id, const char* description, std::vector<std::string> implies = {}) {
        FilterSpec s;
        s.implies = std::move(implies);
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
    spec("canonical-mesh", "One encoding of each mesh: vertices in increasing order as grid points, each face rotated to "
                           "start at its smallest index (winding kept), faces in increasing order. Removes only "
                           "re-orderings of the same mesh: about 10^-19 of the default line. Exact: C(C^3, V) x "
                           "C(V(V-1)(V-2)/3, F); with every-vertex-used it ranks while its table fits in the filter "
                           "memory (16 vertices in 512 MB).",
         {"distinct-vertices-v1", "distinct-indices-v1"});

    // The fractal dimension on the models line (list Z2): the charted fractals' models, found or culled.
    spec("fractal-modelled", "The fractal dimension in the models line: a model that is a charted fractal's lines at a "
                             "depth (an L-system's turtle, in three dimensions for v3 and in the plane for v1 and v2, or a "
                             "flow's path), each line a face naming its two ends (a, b, b), its ends the vertices in the "
                             "order it reaches them, where they fit the line's V and its lines its F; fitted as a mesh "
                             "warped in is (the faces it lacks filled with (0, 1, 2)), and again with its own lines in "
                             "their place, and with turns = every in each of the 24 turns of a cube. keep = include "
                             "passes those models alone (find them); exclude every other (cull them). Set from the "
                             "FRACTALS rows of the setup page, with fractal-drawn-v2. Exact: counts and ranks where the "
                             "line's other filters do, and with include always (a list).");
    FilterSpec& fm = out.back();
    fm.set_elsewhere = true;
    fm.params = {{"keep", "include: only the fractals' models; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1,
                  {"include", "exclude"}},
                 {"turns", "drawn: each model as fitted; every: also in each of the 24 turns of a cube", FilterParam::Kind::Text, "every", 0,
                  0, 1, {"drawn", "every"}}};
}

} // namespace sieve
