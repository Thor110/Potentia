#include "lines.hpp"

#include "models.hpp"

#include "image_io.hpp"

#include "sieve/audio.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace {

std::string read_file(const std::string& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    return std::string(std::istreambuf_iterator<char>(in), {});
}

std::string joined_positional(const Args& a)
{
    std::string s;
    for (size_t i = 0; i < a.positional.size(); ++i) s += (i ? " " : "") + a.positional[i];
    return s;
}

std::string codepoint_list(const std::u32string& s)
{
    std::ostringstream o;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (i) o << ", ";
        char buf[16];
        std::snprintf(buf, sizeof buf, "U+%04X", static_cast<unsigned>(s[i]));
        o << buf;
        if (s[i] >= 0x21 && s[i] != 0x7F) o << " '" << utf8_encode(s[i]) << "'";
    }
    if (s.size() == 16) o << ", ...";
    return o.str();
}

std::string count_line(const char* label, size_t n, const std::string& detail = "")
{
    std::string s = "  ";
    s += label;
    s.resize(24, ' ');
    s += std::to_string(n);
    if (!detail.empty()) s += " (" + detail + ")";
    return s;
}

// Two characters per pixel, darkest to brightest.
constexpr const char* kRamp = " .:-=+*#%@";

char shade(Rgb c)
{
    const uint32_t lum = (299u * c.r + 587u * c.g + 114u * c.b) / 1000u; // 0..255
    return kRamp[lum * 10 / 256];
}

} // namespace

const char* to_string(LineKind k)
{
    switch (k)
    {
    case LineKind::Text: return "text";
    case LineKind::Image: return "image";
    case LineKind::Audio: return "audio";
    case LineKind::Video: return "video";
    }
    return "?";
}

LineKind line_from_string(std::string_view s)
{
    if (s == "text" || s == "pages") return LineKind::Text; // its units are pages
    if (s == "image") return LineKind::Image;
    if (s == "audio") return LineKind::Audio;
    if (s == "video") return LineKind::Video;
    throw std::invalid_argument("unknown line '" + std::string(s) + "' (text or pages|image|audio|video)");
}

Line make_line(const Args& a)
{
    const LineKind kind = line_from_string(a.get("line", "text"));
    const std::string key = a.get("key", "sieve");
    switch (kind)
    {
    case LineKind::Text:
    {
        // A built-in id, or Unicode blocks and ranges stacked with '+' (see sieve alphabets).
        const Alphabet& alpha = alphabet_of(a.get("alphabet", "lower27"));
        Line line{kind, &alpha, canon_version_from_string(a.get("canon", "v2")), {},
                  Space(alpha, a.require_positive("length"), key), nullptr, {}};
        if (a.get("model") != "none")
        {
            const LoadedModel m = resolve_model(a.get("model"), alpha.id());
            if (m.model)
            {
                line.guided = std::make_shared<const GuidedLine>(m.model, line.space.unit_length());
                line.model_id = m.id;
            }
        }
        return line;
    }
    case LineKind::Image:
    case LineKind::Video:
    {
        ImageFormat f;
        const bool video = kind == LineKind::Video;
        f.width = a.get_positive("width", video ? 5 : 10);
        f.height = a.get_positive("height", video ? 5 : 10);
        f.frames = video ? a.get_positive("frames", 8) : 1;
        if (uint64_t(f.width) * f.height * f.frames > 0xFFFFFFFFull)
            throw std::invalid_argument("a picture of " + std::to_string(uint64_t(f.width) * f.height * f.frames) +
                                        " pixels is more than one unit can hold (2^32 - 1 positions)");
        f.palette = &palette_by_id(a.get("palette", "mono"));
        return Line{kind, nullptr, kDefaultCanon, f, Space(f.symbols_id(), f.palette->size(), f.unit_length(), key), nullptr, {}};
    }
    case LineKind::Audio:
        return Line{kind, nullptr, kDefaultCanon, {}, Space(kNotesSymbolsId, kNoteSymbols, a.get_positive("length", 16), key), nullptr, {}};
    }
    throw std::logic_error("unhandled line");
}

std::string Line::describe_symbols() const
{
    switch (kind)
    {
    case LineKind::Text:
        return "alphabet " + alphabet->id() + " (" + std::to_string(alphabet->size()) + " symbols: " +
               alphabet->description() + ")";
    case LineKind::Image:
        return std::to_string(image.width) + "x" + std::to_string(image.height) + " pixels, palette " +
               image.palette->id() + " (" + image.palette->description() + ")";
    case LineKind::Video:
        return std::to_string(image.frames) + " frames of " + std::to_string(image.width) + "x" +
               std::to_string(image.height) + " pixels, palette " + image.palette->id() + " (" +
               image.palette->description() + ")";
    case LineKind::Audio:
        return std::to_string(space.unit_length()) + " note events, each one of 104 (rest or C4-C6, x 4 durations)";
    }
    return "";
}

WarpInput read_warp_input(const Line& line, const Args& a)
{
    WarpInput w;
    switch (line.kind)
    {
    case LineKind::Text:
    {
        const std::string input = a.has("file") ? read_file(a.get("file")) : joined_positional(a);
        if (input.empty()) throw std::invalid_argument("nothing to warp: give TEXT or --file PATH");
        // On a line that holds every byte, the input is bytes rather than text: reading it as
        // UTF-8 would refuse most files, and folding anything would stop it coming back.
        const bool bytes = holds_all_bytes(*line.alphabet);
        const CanonResult c = bytes ? canonicalise_bytes(input, *line.alphabet, line.space.unit_length())
                                    : canonicalise_text(input, *line.alphabet, line.space.unit_length(), line.canon);
        w.report.push_back(std::string(to_string(c.version)) + ": " + std::to_string(c.input_codepoints) +
                           " codepoints in, " + std::to_string(c.canonical_length) + " symbols out, " +
                           std::to_string(c.units.size()) + " unit(s)");
        if (c.whitespace_mapped) w.report.push_back(count_line("whitespace mapped", c.whitespace_mapped));
        if (c.transliterated) w.report.push_back(count_line("punctuation to ASCII", c.transliterated));
        if (c.accents_folded) w.report.push_back(count_line("accents folded", c.accents_folded));
        if (c.case_folded) w.report.push_back(count_line("case folded", c.case_folded));
        if (c.separated) w.report.push_back(count_line("became spaces", c.separated, codepoint_list(c.separated_examples)));
        if (c.dropped) w.report.push_back(count_line("removed", c.dropped, codepoint_list(c.dropped_examples)));
        if (c.spaces_collapsed) w.report.push_back(count_line("spaces collapsed", c.spaces_collapsed));
        if (c.padding)
            w.report.push_back(count_line("padding", c.padding,
                                          std::string(bytes ? "NUL bytes"
                                                      : line.alphabet->contains(U'\n') && line.alphabet->symbol(0) == U'\n'
                                                          ? "line feeds"
                                                          : "spaces") +
                                              " on the last unit"));
        for (const auto& u : c.units) w.units.push_back(line.space.digits_of(u));
        break;
    }
    case LineKind::Audio:
    {
        const std::string input = a.has("file") ? read_file(a.get("file")) : joined_positional(a);
        if (input.empty()) throw std::invalid_argument("nothing to warp: give notes like \"C4q E4q G4h\" or --file PATH");
        const NotesCanonResult c = canonicalise_notes(input, line.space.unit_length());
        w.report.push_back(std::string(kNotesCanonVersion) + ": " + std::to_string(c.events) + " event(s), " +
                           std::to_string(c.units.size()) + " unit(s)");
        if (c.flats_rewritten) w.report.push_back(count_line("flats as sharps", c.flats_rewritten));
        if (c.octave_shifted) w.report.push_back(count_line("octave shifted", c.octave_shifted, "moved into C4-C6"));
        if (c.default_durations) w.report.push_back(count_line("no duration, used q", c.default_durations));
        if (c.padding) w.report.push_back(count_line("padding", c.padding, "eighth rests on the last unit"));
        w.units = c.units;
        break;
    }
    case LineKind::Image:
    case LineKind::Video:
    {
        if (!a.has("file")) throw std::invalid_argument("give the picture with --file PATH (PNG, JPEG, BMP, GIF, TGA)");
        if (!a.positional.empty()) throw std::invalid_argument("the image line takes --file PATH, not text");
        std::vector<RgbaImage> frames = load_image_frames(a.get("file"));
        const size_t source_frames = frames.size();
        if (line.kind == LineKind::Image) frames.resize(1); // first frame of an animation
        ImageCanonReport r;
        w.units.push_back(canonicalise_image(frames, line.image, &r));
        w.report.push_back(std::string(kImageCanonVersion) + ": " + std::to_string(r.source_width) + "x" +
                           std::to_string(r.source_height) + " source, " + std::to_string(source_frames) +
                           " frame(s) -> " + line.image.symbols_id());
        if (line.kind == LineKind::Image && source_frames > 1)
            w.report.push_back(count_line("frames ignored", source_frames - 1, "the image line uses the first frame"));
        if (r.transparent_pixels) w.report.push_back(count_line("transparent pixels", r.transparent_pixels, "composited onto black"));
        if (r.frames_dropped) w.report.push_back(count_line("frames dropped", r.frames_dropped));
        if (r.frames_padded) w.report.push_back(count_line("frames padded", r.frames_padded, "black frames added"));
        break;
    }
    }
    return w;
}

std::string preview(const Line& line, const std::vector<uint32_t>& digits)
{
    switch (line.kind)
    {
    case LineKind::Text: return "\"" + utf8_encode(line.space.text_of(digits)) + "\"";
    case LineKind::Audio: return notes_to_notation(digits);
    case LineKind::Image:
    case LineKind::Video:
    {
        const auto px = render_image(digits, line.image);
        const uint32_t W = line.image.width, H = line.image.height, F = line.image.frames;
        const uint32_t per_row = std::max<uint32_t>(1, 96 / (W * 2 + 2)); // frames side by side
        std::string out;
        for (uint32_t f0 = 0; f0 < F; f0 += per_row)
        {
            if (f0) out += "\n";
            for (uint32_t y = 0; y < H; ++y)
            {
                for (uint32_t f = f0; f < std::min(F, f0 + per_row); ++f)
                {
                    if (f != f0) out += "  ";
                    for (uint32_t x = 0; x < W; ++x)
                    {
                        const char c = shade(px[size_t(f) * W * H + size_t(y) * W + x]);
                        out += c;
                        out += c;
                    }
                }
                if (y + 1 < H) out += "\n";
            }
        }
        return out;
    }
    }
    return "";
}

void save_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& path, uint32_t scale)
{
    switch (line.kind)
    {
    case LineKind::Text:
    {
        std::ofstream out(std::filesystem::path(path), std::ios::binary);
        if (!out) throw std::runtime_error("cannot write '" + path + "'");
        // On a line that holds every byte, a digit is a byte: the file is the unit exactly, and
        // nothing may be encoded or appended.
        if (line.alphabet && holds_all_bytes(*line.alphabet))
        {
            for (uint32_t d : digits) out.put(char(static_cast<unsigned char>(d)));
            return;
        }
        // Exactly the unit. A trailing newline is added only where the alphabet cannot hold one
        // itself, as a courtesy so the file does not end mid-line; on an alphabet that can (see
        // ascii96), the file IS the unit, byte for byte, and adding anything would spoil that.
        out << utf8_encode(line.space.text_of(digits));
        if (!line.alphabet || !line.alphabet->contains(U'\n')) out << "\n";
        return;
    }
    case LineKind::Audio:
    {
        const std::string midi = notes_to_midi(digits);
        std::ofstream out(std::filesystem::path(path), std::ios::binary);
        if (!out) throw std::runtime_error("cannot write '" + path + "'");
        out.write(midi.data(), static_cast<std::streamsize>(midi.size()));
        return;
    }
    case LineKind::Image:
    case LineKind::Video:
    {
        // Frames side by side, separated by a one-pixel gap.
        const auto px = render_image(digits, line.image);
        const uint32_t W = line.image.width, H = line.image.height, F = line.image.frames;
        const uint32_t gap = F > 1 ? 1 : 0;
        const uint32_t sheet_w = F * W + (F - 1) * gap;
        std::vector<Rgb> sheet(size_t(sheet_w) * H, Rgb{64, 64, 64});
        for (uint32_t f = 0; f < F; ++f)
            for (uint32_t y = 0; y < H; ++y)
                for (uint32_t x = 0; x < W; ++x)
                    sheet[size_t(y) * sheet_w + f * (W + gap) + x] = px[size_t(f) * W * H + size_t(y) * W + x];
        write_png(path, sheet_w, H, sheet, scale);
        return;
    }
    }
}

} // namespace sieve::cli
