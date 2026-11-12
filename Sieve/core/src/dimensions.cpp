// Sieve — the catalogue of dimensions (see dimensions.hpp).
#include "sieve/dimensions.hpp"

namespace sieve {

const std::vector<Dimension>& dimensions()
{
    using S = DimensionState;
    static const std::vector<Dimension> d = {
        // The hallway's eleven lines (SPECIFICATIONS §3).
        {"pages", "Pages", S::Built, "a unit of text: a pinned alphabet's symbols at a fixed length",
         "any alphabet of up to 2^31 symbols, any length the setup menu sets", "books binary dna research", "SPECIFICATIONS §3"},
        {"image", "Image", S::Built, "a small picture: its pixels at a fixed size and depth", "the size and depth the setup menu sets",
         "movies binary", "SPECIFICATIONS §3"},
        {"audio", "Audio", S::Built, "a melody of note events, or sound itself (pcm)", "the events, voices and lengths set",
         "tracks binary", "SPECIFICATIONS §3.2-3.4"},
        {"video", "Video", S::Built, "frames of small pictures", "the frames and size set", "movies binary", "SPECIFICATIONS §3"},
        {"books", "Books", S::Built, "a book: pages of the text line bound in order", "pages per book, as set", "pages", "SPECIFICATIONS §11"},
        {"tracks", "Tracks", S::Built, "a track: units of the audio line in order", "units per track, as set", "audio", "SPECIFICATIONS §11"},
        {"movies", "Movies", S::Built, "a movie: video units in order", "units per movie, as set", "video image", "SPECIFICATIONS §11"},
        {"models", "Models", S::Built, "a mesh of V vertices and F triangles on a grid of C steps",
         "the shape set (held models: 8 vertices, 12 faces, 16 steps)", "worlds characters importers", "SPECIFICATIONS §12"},
        {"worlds", "Worlds", S::Built, "a world: models placed in a room", "the models and places set", "models games simulations",
         "SPECIFICATIONS §12.0"},
        {"ai", "AI", S::Built, "a model's weights, replayed bit for bit", "the model shape set", "pages languages research",
         "SPECIFICATIONS §12.0d"},
        {"binary", "Binary", S::Built, "any file of a fixed size: its bytes as one number", "the byte count set",
         "pages image audio video", "SPECIFICATIONS §12.1"},

        // Built since the lines, from 12 November 2026.
        {"characters", "Characters", S::Built, "a seed and settings unfolding a tree of spheres, each holding a model",
         "16 limbs, 12 joints, 3 branches a joint; a model of the held line in each sphere", "species models personal emotion animation",
         "IDEAS 19.147-19.159; SPECIFICATIONS §12.0j"},
        {"species", "Species", S::Built, "a character's settings with the seed left free", "every setting the characters line has",
         "characters dna languages", "IDEAS 19.152"},
        {"personal", "Personal dimensions", S::Partly, "one person's store of characters (later: a list of their ids, its own id)",
         "64 characters read back, as built", "characters others groups", "IDEAS 19.154, 19.157, 19.161"},
        {"importers", "Importers", S::Partly, "an existing file (.obj, .gltf, .glb) brought onto a line",
         "a held model: 8 vertices, 12 faces; a skeleton of 64 joints; animations of 300 frames, turns only", "models characters animation games formats",
         "IDEAS 19.159, 19.162, 19.170"},
        {"dna", "DNA", S::Partly, "a sequence of the four bases: a unit of the pages line over dna4 (a, c, g, t)",
         "the length the setup menu sets; not yet a reader of FASTA files", "pages species research", "IDEAS 19.139 c, 19.152, 19.162"},

        // Planned: each named where it was first written about.
        {"research", "Research", S::Planned, "a finding: a claim, what it rests on, where it was measured",
         "grows while research is open; once everything known is catalogued it pops into a hallway of links",
         "dna pages ai languages formats law", "IDEAS 19.162"},
        {"languages", "Languages", S::Planned, "a language: its alphabet, words and grammar, and the models that speak it",
         "every language written down (19.59)", "pages ai species characters formats", "IDEAS 19.22, 19.59, 19.152"},
        {"emotion", "Emotion", S::Planned, "a feeling, as a ring of doors", "not set", "characters", "IDEAS 19.144"},
        {"animation", "Animation", S::Built, "turns per bone per frame (sieve-animation-v1): each bone's turn from rest, 256 steps a number",
         "64 bones, 300 frames at 30 a second; turns only", "characters importers", "IDEAS 19.159, 19.170"},
        {"games", "Games and their generators", S::Planned,
         "a game's seed and settings: each generator (procedural worlds, random dungeons) a sub state space",
         "each generator's own seed width and settings", "worlds characters personal simulations boardgames engines", "IDEAS 19.104, 19.161, 19.163"},
        {"formats", "Formats", S::Planned,
         "a format in the widest sense: a file format, script, language, notation or emoji set, with its documentation, "
         "how it is recognised, Sieve's support, measured statistics and links",
         "every format recorded; each entry reaches the root by its links (the wiki principle)", "importers languages binary pages research",
         "IDEAS 19.165; docs/CATALOGUE-ROADMAP.md"},
        {"simulations", "Simulations", S::Planned, "a world to visit: a simulation, catalogued and reachable",
         "every world the worlds line and the games' generators hold", "worlds games", "IDEAS 19.166"},
        {"engines", "Game engines", S::Planned, "a game engine: what it reads, what it runs, the games and formats it handles",
         "not set", "games formats importers", "IDEAS 19.104, 19.163, 19.173"},
        {"boardgames", "Board games", S::Planned, "a board game: its boards, pieces, cards, rules and artwork, across genres and franchises",
         "not set", "games formats law", "IDEAS 19.171"},
        {"law", "Theoretical law", S::Planned, "a situation, with each rule a filter that keeps or sets it aside",
         "not set", "research languages", "IDEAS 19.164"},
        {"others", "Other people's personal dimensions", S::Planned, "another person's dimension, ranked and listed by category",
         "waits on sharing between machines", "personal", "IDEAS 19.157"},
        {"groups", "Group dimensions", S::Planned, "a dimension made together; in the long run, games",
         "waits on sharing between machines", "personal games", "IDEAS 19.157"},
    };
    return d;
}

const Dimension* dimension_by_id(std::string_view id)
{
    for (const auto& x : dimensions())
        if (x.id == id) return &x;
    return nullptr;
}

const char* dimension_state_name(DimensionState s)
{
    switch (s)
    {
    case DimensionState::Built: return "built";
    case DimensionState::Partly: return "partly built";
    case DimensionState::Planned: return "planned";
    }
    return "?";
}

} // namespace sieve
