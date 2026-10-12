// Sieve CLI -- files carried as items of Sieve's lines (sieve-manifest-v5, SPECIFICATIONS §12.2).
//
// A file that is exactly an item of a line (item_of: a page as its text, a picture or film as its
// PNG at one pixel a pixel, a melody as its MIDI file, byte for byte) need not be carried: its line
// and its positional address there make it again (unit_file). A v5 listing names each line once,
// as "<its space id> <the options that make it>", the options as `sieve` takes them; making the
// line again from those options must give the same space id, or the file is refused, so a change
// to a line's defaults can never make another file quietly.
#pragma once

#include "cli/args.hpp"
#include "cli/lines.hpp"
#include "cli/pack.hpp"

#include <string>
#include <vector>

namespace sieve::cli {

struct ItemLine
{
    std::string spec; // "<space id> <options>", as the listing writes it
    Line line;
};

// The lines files are carried as items of: the pages (of --alphabet and --page-length, as the
// File Locator's weighing takes them), the image, video and audio lines at their defaults.
std::vector<ItemLine> item_lines(const Args& a);
// A line from its options ("line=text alphabet=lower27 length=32 model=none").
Line line_of_options(const std::string& options);
// Finds, for a file's bytes, the first of `lines` it is an item of, and its positional address.
ItemFinder item_finder(const std::vector<ItemLine>& lines);
// Lets manifests with items be read (set_item_maker): each line made from its options once, and
// checked against its space id.
void use_item_maker();

} // namespace sieve::cli
