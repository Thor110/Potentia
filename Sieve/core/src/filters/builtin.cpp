// The compiled-in filters, in the order they are listed. To add a filter: write it in its own
// file (see text_m1.cpp, statistics.cpp, media.cpp, audio.cpp), give it an id and version, and add its
// registration call here. To change a filter, register it again with a new version and keep
// the old one, so earlier results stay reproducible.

#include "sieve/filter.hpp"

namespace sieve {

void add_text_m1_filters(std::vector<FilterSpec>& out);
void add_statistics_filters(std::vector<FilterSpec>& out);
void add_media_filters(std::vector<FilterSpec>& out);
void add_audio_filters(std::vector<FilterSpec>& out);

std::vector<FilterSpec> builtin_filters()
{
    std::vector<FilterSpec> out;
    add_text_m1_filters(out);
    add_statistics_filters(out);
    add_media_filters(out);
    add_audio_filters(out);
    return out;
}

} // namespace sieve
