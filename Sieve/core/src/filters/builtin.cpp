// The compiled-in filters, in the order they are listed. To add a filter: write it in its own
// file (see text_m1.cpp, statistics.cpp, media.cpp, picture.cpp, audio.cpp,
// written.cpp, crossline.cpp, binary.cpp), give it an id and version, and add its
// registration call here. To change a filter, register it again with a new version and keep
// the old one, so earlier results stay reproducible.

#include "sieve/filter.hpp"

namespace sieve {

void add_text_m1_filters(std::vector<FilterSpec>& out);
void add_statistics_filters(std::vector<FilterSpec>& out);
void add_media_filters(std::vector<FilterSpec>& out);
void add_audio_filters(std::vector<FilterSpec>& out);
void add_binary_filters(std::vector<FilterSpec>& out);
void add_written_filters(std::vector<FilterSpec>& out);
void add_crossline_filters(std::vector<FilterSpec>& out);
void add_model_filters(std::vector<FilterSpec>& out);
void add_pattern_filters(std::vector<FilterSpec>& out);
void add_other_line_filters(std::vector<FilterSpec>& out);
void add_picture_filters(std::vector<FilterSpec>& out);

std::vector<FilterSpec> builtin_filters()
{
    std::vector<FilterSpec> out;
    add_text_m1_filters(out);
    add_statistics_filters(out);
    add_media_filters(out);
    add_audio_filters(out);
    add_written_filters(out);
    add_crossline_filters(out);
    add_model_filters(out);
    add_pattern_filters(out);
    add_other_line_filters(out);
    add_binary_filters(out);
    add_picture_filters(out);
    // Hard and soft, in one place (FilterSpec::category).
    static const char* const kHard[] = {"not-written", "not-a-file", "not-other-line", "not-packed", "not-an-item", "binary-kind",
                                        "distinct-vertices", "distinct-indices", "max-run", "not-a-pattern", "canonical-mesh"};
    for (FilterSpec& f : out)
    {
        f.category = "soft";
        for (const char* h : kHard)
            if (f.id == h) f.category = "hard";
    }
    return out;
}

} // namespace sieve
