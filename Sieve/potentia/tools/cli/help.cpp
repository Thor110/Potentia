#include "help.hpp"

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace potentia::cli {

namespace {

struct Option
{
    std::string flag;
    std::string text;
};

struct Page
{
    std::string name;
    std::string summary;
    std::string usage;
    std::string about;
    std::vector<Option> options;
    std::vector<std::pair<std::string, std::string>> examples; // command line, what it does
};

const std::string kModeText =
    "How the line is ordered.\n"
    "  positional  The address is the content itself, read as a number. Units that share a\n"
    "              beginning sit next to each other (\"this is a\" beside \"this is b\").\n"
    "  scrambled   The content is shuffled by a fixed, reversible permutation first, so\n"
    "              neighbouring addresses hold unrelated units, as in Borges' library.";

const Option kLine = {"--line text|image|audio|video",
                      "Which line (kind of content) to work on. Default: text. Each line has its own\n"
                      "options below; `potentia help lines` explains them in full."};

const Option kLineOptions = {
    "line options",
    "text   --length L (required)  --alphabet lower27|babel29|ascii95  --canon v2|v1\n"
    "image  --width W (10)  --height H (10)  --palette mono|ega16|rgb332|rgb24 (mono)\n"
    "audio  --length N note events (16)\n"
    "video  --width W (5)  --height H (5)  --frames F (8)  --palette ... (mono)\n"
    "These must match between warp and read, or you will read a different unit."};

const Option kKey = {"--key K",
                     "A label (not a password) that seeds the scrambled ordering. Default: potentia.\n"
                     "Different keys give completely different scrambled addresses for the same\n"
                     "content. Positional addresses ignore the key."};

const Option kShort = {"--short",
                       "Abbreviate long addresses on screen as start...end (N digits). Use the full\n"
                       "address (without --short) when you want to read it back later."};

const std::vector<Page>& pages()
{
    static const std::vector<Page> list = {
        {"info", "Show how big a space is and how long its addresses are.",
         "potentia info [--line LINE] [line options] [--key K]",
         "A 'space' is every possible unit of one line with one set of options (for text: every\n"
         "string of --length characters). This prints how many units exist and how many bits and\n"
         "hex digits it takes to write down any address.",
         {kLine, kLineOptions, kKey},
         {{"potentia info --length 32", "a 32-character line of text: ~10^45.8 units"},
          {"potentia info --length 1000", "paragraph scale: ~10^1431 units, 1189-digit addresses"},
          {"potentia info --length 3200 --alphabet babel29", "one page of the libraryofbabel.info library"},
          {"potentia info --line image", "every 10x10 black-and-white image: ~10^30 units"},
          {"potentia info --line image --palette rgb24", "every 10x10 full-colour image: ~10^722 units"},
          {"potentia info --line audio", "every 16-note melody"},
          {"potentia info --line video", "every 8-frame 5x5 black-and-white animation"}}},

        {"warp", "Give some content and get the address where it lives.",
         "potentia warp [--line LINE] [line options] [--key K] [--mode MODE] [--short] (TEXT... | --file PATH)",
         "Fits the input to the line with fixed, versioned rules, reports every change it made,\n"
         "then prints the address of each unit plus a preview of what is on that shelf.\n"
         "  text   TEXT or --file. Lower-cased, accents folded, punctuation turned into spaces,\n"
         "         then split into units of --length characters (a long text becomes a trail).\n"
         "  image  --file PATH (PNG, JPEG, BMP, GIF, TGA). Stretched to WxH by area averaging,\n"
         "         then each pixel set to the nearest palette colour.\n"
         "  audio  Notes such as \"C4q E4q G4h Rq\", or --file with notes. See `potentia help lines`.\n"
         "  video  --file PATH, normally an animated GIF. Frames beyond --frames are dropped;\n"
         "         missing frames are filled with black.\n"
         "Put multi-word text in quotes.",
         {kLine, kLineOptions, kKey,
          {"--mode MODE", "positional, scrambled or both (default both). " + kModeText},
          kShort,
          {"--file PATH", "Read the input from a file instead of the command line."}},
         {{"potentia warp --length 32 \"It was the best of times\"", "both addresses of one line of text"},
          {"potentia warp --length 32 --mode scrambled --short \"It was the best of times\"", "just the scrambled one, abbreviated"},
          {"potentia warp --length 1000 --file chapter1.txt", "a whole text file, 1000 characters per unit"},
          {"potentia warp --line image --file sprite.png", "a picture as a 10x10 black-and-white image"},
          {"potentia warp --line image --palette ega16 --width 16 --height 16 --file sprite.png", "16x16 in EGA colours"},
          {"potentia warp --line audio \"E4q D4q C4q D4q E4q E4q E4h\"", "a melody"},
          {"potentia warp --line video --file walk.gif", "an animation as 8 frames of 5x5"}}},

        {"read", "Give an address and get back what is stored there.",
         "potentia read [--line LINE] [line options] [--key K] --mode MODE [--out PATH] [--scale S] ADDRESS",
         "The reverse of warp. ADDRESS is the hex string warp or browse printed. Every option that\n"
         "shaped the address (--line, the line options, --key, --mode) must match, or you will read\n"
         "a different unit: every address in range holds something. Text is printed; images and\n"
         "video are drawn in ASCII; audio is printed as notes. --out saves the unit as a file.",
         {kLine, kLineOptions, kKey,
          {"--mode MODE", "positional or scrambled (required). " + kModeText},
          {"--out PATH", "Also save the unit: .png for image and video (frames side by side), .mid for\n"
                         "audio (a playable MIDI file), a .txt file for text."},
          {"--scale S", "Enlarge each pixel to SxS in the saved PNG. Default 16."},
          {"--around N", "Also show the N units on either side, in the chosen ordering: what the hallway\n"
                         "shows around this shelf. In positional order the neighbours differ only at\n"
                         "the end; in scrambled order they are unrelated. The line loops, so the last\n"
                         "address is followed by the first."},
          kShort},
         {{"potentia read --length 32 --mode scrambled 0ec971d8ace67261044340a270bfadb62f567ec",
           "prints \"it was the best of times\""},
          {"potentia read --length 3 --mode positional 0312", "prints \"abc\""},
          {"potentia read --length 32 --mode positional --around 3 <address>", "the shelf and 3 neighbours either side"},
          {"potentia read --line image --mode scrambled <address> --out found.png", "draws it and saves a PNG"},
          {"potentia read --line audio --mode scrambled <address> --out tune.mid", "prints the notes and saves a MIDI file"}}},

        {"browse", "Pull random units off the shelves.",
         "potentia browse [--line LINE] [line options] [--key K] [--count N] [--short]",
         "Picks uniformly random addresses and shows what is there. This is what wandering the raw\n"
         "Library of Babel is like: almost everything is noise. Each result shows its scrambled\n"
         "address, so you can read or save it later.",
         {kLine, kLineOptions, kKey,
          {"--count N", "How many units to show. Default 5."},
          kShort},
         {{"potentia browse --length 32", "five random 32-character lines of text"},
          {"potentia browse --length 80 --count 20 --alphabet babel29", "twenty lines of Borges' library"},
          {"potentia browse --line image --count 3", "three random 10x10 images, drawn in ASCII"},
          {"potentia browse --line audio", "five random melodies"}}},

        {"sieve", "Count how much of the text space survives each noise filter (milestone M1).",
         "potentia sieve [--dict ID|PATH] [--lengths SPEC] [--brute-max N] [--pruned-max N] [--threads T] [--csv PATH]",
         "For each unit length, counts exactly how many lower27 text units pass three filters,\n"
         "each stricter than the last:\n"
         "  clean   no double spaces, and at least one letter\n"
         "  window  could be a snippet cut from English text (whole words inside, a word ending at\n"
         "          the left edge, a word beginning at the right edge)\n"
         "  words   every token is a complete dictionary word\n"
         "Counts are exact at any length. At short lengths they are double-checked by visiting\n"
         "every unit (brute) and by walking the prefix tree while skipping dead branches\n"
         "(pruned). Output is CSV: one row per length, with counts and log10 surviving fractions.",
         {{"--dict ID|PATH",
           "Word list to use. An ID picks a registered dictionary (list them with `potentia dicts`):\n"
           "  scowl-en-35  common English words (40,201)\n"
           "  scowl-en-60  SCOWL's recommended spell-check size (79,645) - the default\n"
           "  scowl-en-80  large, includes rare words (251,174)\n"
           "35, 60 and 80 are accepted as short forms. A registered dictionary is checked against\n"
           "its SHA-256 before use. A value containing / or \\ or ending in .txt is read as a file\n"
           "path instead (one word per line, not checked). The CSV header records the id and hash."},
          {"--lengths SPEC", "Unit lengths to count: numbers and ranges, comma-separated. Default: 1-12.\n"
                             "Example: 1-32,64,100,1000"},
          {"--brute-max N", "Also check lengths up to N by visiting every unit. Default 5. Cost grows 27x per\n"
                            "length: on 2 cores N=6 takes about half a minute, N=7 about 10-15 minutes.\n"
                            "Use a Release build for N=6 and above."},
          {"--pruned-max N", "Also check lengths up to N by pruned tree walk. Default 6. On 2 cores N=7 takes\n"
                             "~15 s, N=8 ~2 minutes."},
          {"--threads T", "Worker threads for the checks. Default: all cores."},
          {"--csv PATH", "Write the CSV to a file instead of the screen."}},
         {{"potentia sieve", "lengths 1-12 with the default dictionary"},
          {"potentia sieve --lengths 1-64,100,1000 --csv sieve.csv", "out to paragraph scale, saved to a file"},
          {"potentia sieve --dict scowl-en-35 --lengths 1-20", "the same with the smaller, common-words dictionary"},
          {"potentia sieve --dict my_words.txt", "your own word list, without registering it"},
          {"potentia sieve --lengths 1-7 --brute-max 6 --pruned-max 7", "maximum cross-checking (slow)"}}},

        {"dicts", "List the registered dictionaries, or hash a new one for registration.",
         "potentia dicts [--hash FILE]",
         "Dictionaries are listed in data/dictionaries/dictionaries.tsv (the build copies the folder\n"
         "next to the executable). Each line gives an id, a file, a language, whether it is the\n"
         "default, the file's SHA-256 and a description. Commands that take --dict ID use this\n"
         "list, and refuse a file whose hash no longer matches, so a result recorded with an id\n"
         "can always be reproduced with exactly the same words.\n"
         "\n"
         "To add a dictionary:\n"
         "  1. Put the word list (one word per line) in data/dictionaries/.\n"
         "  2. Run `potentia dicts --hash data/dictionaries/FILE`. It prints the hash and a\n"
         "     ready-made registry line.\n"
         "  3. Add that line to dictionaries.tsv with your own id and description. To make it the\n"
         "     default, set its default column to yes and the old default's to no.\n"
         "  4. Rebuild (or copy the folder) so the executable's copy is updated.\n"
         "Never change an existing entry's file or hash; register a changed list under a new id.",
         {{"--hash FILE", "Print a word list's word count and SHA-256, plus a registry line to paste."}},
         {{"potentia dicts", "list dictionaries, check their files and hashes, and show the default"},
          {"potentia dicts --hash data/dictionaries/my-words.txt", "prepare a new dictionary for the registry"},
          {"potentia sieve --dict my-dictionary", "use it once registered"}}},

        {"version", "Show the tool version and every pinned rule version.",
         "potentia version   (or: potentia --version)",
         "Prints the version of the tool, the scramble construction, the canonicalisation rules,\n"
         "the alphabets and palettes, and the default dictionary with its SHA-256. Record this\n"
         "alongside any result you publish so it can be reproduced exactly.",
         {},
         {{"potentia version", "print everything that determines an address"}}},

        {"lines", "The four lines (kinds of content) and their options.",
         "potentia <command> --line text|image|audio|video [line options]",
         "Every line is a shelf of units of one fixed shape. The same address machinery serves all\n"
         "four; only the shape of a unit and the rules for fitting input to it differ.\n"
         "\n"
         "TEXT (default)\n"
         "  A unit is --length characters.\n"
         "  --length L         Required. 32 is a line, 1000 a paragraph, 3200 a Babel page.\n"
         "  --alphabet ID      lower27  space + a-z (default)\n"
         "                     babel29  space + a-z + comma + period (libraryofbabel.info)\n"
         "                     ascii95  every printable ASCII character, case kept\n"
         "  --canon v2|v1      Rules for fitting pasted text. v2 (default): accents folded\n"
         "                     (cafe), apostrophes removed (didnt), other punctuation becomes a\n"
         "                     space (well known). v1: all punctuation removed (wellknown).\n"
         "\n"
         "IMAGE\n"
         "  A unit is a WxH picture; each pixel is one palette colour.\n"
         "  --width W          Default 10.\n"
         "  --height H         Default 10.\n"
         "  --palette ID       mono    black and white (default)\n"
         "                     ega16   the 16-colour EGA palette\n"
         "                     rgb332  256 colours\n"
         "                     rgb24   every 24-bit colour\n"
         "  Input: --file PATH (PNG, JPEG, BMP, GIF, TGA). Stretched to WxH; transparency\n"
         "  becomes black.\n"
         "\n"
         "AUDIO\n"
         "  A unit is a melody of --length note events (default 16). Each event is a rest or a\n"
         "  note from C4 to C6, lasting an eighth (e), quarter (q), half (h) or whole (w).\n"
         "  Notation: C4q D#4e Bb4h Rq  (letter, optional # or b, octave, duration; R = rest).\n"
         "  Missing durations mean q; notes outside C4-C6 move by octaves into range; | and\n"
         "  commas are ignored. Short melodies are padded with eighth rests.\n"
         "\n"
         "VIDEO\n"
         "  A unit is --frames pictures of WxH. Defaults are small because the space grows fast.\n"
         "  --width W (5)  --height H (5)  --frames F (8)  --palette ID (mono)\n"
         "  Input: --file PATH, usually an animated GIF.\n"
         "\n"
         "All lines also take --key K (default potentia), which seeds the scrambled ordering.",
         {},
         {{"potentia info --line image --width 16 --height 16 --palette ega16", "size of the 16x16 EGA image space"},
          {"potentia warp --line audio --length 8 \"C4q E4q G4q C5h\"", "an 8-event melody"},
          {"potentia browse --line video --count 1", "one random animation, frames side by side"}}},
    };
    return list;
}

void print_indented(const std::string& text, const std::string& indent)
{
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) std::cout << indent << line << "\n";
}

} // namespace

void print_usage()
{
    std::cout << "potentia - Gallery of Babel core (spec v2.0)\n\n"
                 "Every possible unit of content of a fixed shape (a line of text, a small picture, a\n"
                 "short melody) has exactly one address, and every address holds exactly one unit.\n"
                 "These commands move between the two.\n\n"
                 "Commands:\n";
    for (const auto& p : pages())
    {
        if (p.name == "lines") continue;
        std::string name = p.name;
        name.resize(8, ' ');
        std::cout << "  " << name << p.summary << "\n";
    }
    std::cout << "\nLines (choose with --line): text (default), image, audio, video.\n"
                 "\nDetailed help and examples:\n"
                 "  potentia help warp        (or: potentia warp --help)\n"
                 "  potentia help lines       the four lines and their options\n\n"
                 "Quick start:\n"
                 "  potentia warp --length 32 \"It was the best of times\"\n"
                 "  potentia read --length 32 --mode scrambled <address printed above>\n"
                 "  potentia browse --line image --count 3\n"
                 "  potentia warp --line audio \"E4q D4q C4q D4q E4h\"\n"
                 "  potentia sieve\n";
}

bool print_help(const std::string& name)
{
    for (const auto& p : pages())
    {
        if (p.name != name) continue;
        std::cout << "potentia " << p.name << " - " << p.summary << "\n\nUsage:\n  " << p.usage << "\n\n";
        print_indented(p.about, "");
        if (!p.options.empty())
        {
            std::cout << "\nOptions:\n";
            for (const auto& o : p.options)
            {
                std::cout << "  " << o.flag << "\n";
                print_indented(o.text, "      ");
            }
        }
        std::cout << "\nExamples:\n";
        for (const auto& [line, what] : p.examples) std::cout << "  " << line << "\n      " << what << "\n";
        return true;
    }
    return false;
}

} // namespace potentia::cli
