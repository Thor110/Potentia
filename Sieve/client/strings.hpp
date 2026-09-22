// Sieve hallway — the language system: every string the menus show comes from a text file.
//
// Each language is one UTF-8 file in the lang folder (data/lang while developing, lang/ next to
// the executable once built), named by its code: en.txt, fr.txt, ... A file is plain text:
//
//   ; comments start with ';' or '#'
//   language.name = English          ; shown in the language list
//   main.start = Start Sieve
//   controls.move = Move: W A S D
//
// A key missing from the chosen language falls back to English, and a key missing from English
// shows as the key itself, so a half-finished translation still works. "\n" in a value is a line
// break; "{0}", "{1}", ... are filled in by trf(). A value in double quotes keeps the spaces at
// its ends ("  F1 menu").
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace hallway {

struct LanguageInfo
{
    std::string code; // file name without .txt ("en")
    std::string name; // its language.name ("English")
    std::filesystem::path file;
};

// Every language file found, English first, then by name.
std::vector<LanguageInfo> available_languages();

// Loads a language (English is always loaded underneath as the fallback). Returns false, and
// keeps English, if there is no file for the code.
bool set_language(const std::string& code);
const std::string& language_code();

// The string for a key in the current language.
const std::string& tr(std::string_view key);
// The string for a key, or `fallback` when no language has it (for text that has an English
// original elsewhere, such as a filter's description in the core's registry).
std::string tr_or(std::string_view key, const std::string& fallback);
// The same, with {0}, {1}, ... replaced by the arguments.
std::string trf(std::string_view key, const std::vector<std::string>& args);

// The folder the language files are read from (first that exists).
std::filesystem::path language_folder();

} // namespace hallway
