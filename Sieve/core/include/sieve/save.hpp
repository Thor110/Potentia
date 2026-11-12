// Sieve — a character's save (IDEAS 19.150, 19.183): everything chosen in character creation, named
// together, so it is kept, versioned and shared as one (the infinite RAID, sieve/raid.hpp).
//
// "sieve-save-v1". The character itself (sieve-character-v1 to v3) is named by its SHA-256, as is
// its animation (sieve-animation-v1), and every other system that joins character creation adds a
// line of its own; the first is the spoken languages, entries of the language catalogue
// (data/languages, sieve/catalogue.hpp):
//     sieve-save-v1
//     character SHA-256
//     animation SHA-256         (where it has one)
//     speaks ID                 (one a language, in the order of their ids, each once)
// Each line ends in a line feed; nothing else. Its identity is the SHA-256 of that text, so a
// change to anything in it (another language, another seed) is a new version of the save.
#pragma once

#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kSaveVersion = "sieve-save-v1";

struct CharacterSave
{
    std::string character;           // the character's SHA-256
    std::string animation;           // its animation's SHA-256, or empty
    std::vector<std::string> speaks; // language entries' ids, kept in order and each once
    void speak(const std::string& id, bool yes); // adds or takes away one, keeping the order
    std::string text() const;
    std::string id() const;          // SHA-256 of the text
};

// A save read back, checked (its version, a character's SHA-256, an animation's where given, the
// languages in order and each once, as ids). Throws std::invalid_argument saying what is wrong.
CharacterSave read_save(const std::string& text);

} // namespace sieve
