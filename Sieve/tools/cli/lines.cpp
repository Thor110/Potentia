#include "lines.hpp"
#include "vault.hpp"
#include "vault_decode.hpp"

#include "models.hpp"

#include "image_io.hpp"
#include "media_decode.hpp"

#include "sieve/audio.hpp"
#include "sieve/sound.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <atomic>
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
    {
        // notes104 (the default), or a notes2 set: --note-set notes2 with --low, --high,
        // --durations and --voices; --length is then events per voice, and a unit is voices x that.
        const std::string family = a.get("note-set", "notes104");
        if (family == "pcm")
        {
            // Sound itself (sieve/sound.hpp): --rate, --bits, --channels; --length is samples per
            // channel, one second at the rate unless given.
            for (const char* k : {"low", "high", "durations", "voices"})
                if (a.has(k)) throw std::invalid_argument(std::string("--") + k + " is for --note-set notes2 (pcm takes --rate, --bits and --channels)");
            const PcmFormat f = make_pcm_format(a.get_positive("rate", 8000), a.get_positive("bits", 8), a.get_positive("channels", 1));
            const uint32_t samples = a.get_positive("length", f.rate);
            if (uint64_t(samples) * f.channels > 0xFFFFFFFFull) throw std::invalid_argument("too many samples in one unit (2^32 - 1 at most)");
            return Line{kind, nullptr, kDefaultCanon, {}, Space(f.id(), f.base(), samples * f.channels, key), nullptr, {}};
        }
        for (const char* k : {"rate", "bits", "channels"})
            if (a.has(k)) throw std::invalid_argument(std::string("--") + k + " is for --note-set pcm");
        const uint32_t length = a.get_positive("length", 16);
        if (family == "notes104")
        {
            for (const char* k : {"low", "high", "durations", "voices"})
                if (a.has(k)) throw std::invalid_argument(std::string("--") + k + " is for --note-set notes2 (notes104 is fixed: C4-C6, e q h w, one voice)");
            return Line{kind, nullptr, kDefaultCanon, {}, Space(kNotesSymbolsId, kNoteSymbols, length, key), nullptr, {}};
        }
        if (family != "notes2") throw std::invalid_argument("unknown note set '" + family + "' (notes104, notes2 or pcm)");
        const NoteSet set = make_note_set(note_midi_of(a.get("low", "C3")), note_midi_of(a.get("high", "C6")), a.get("durations", kNoteDurationCodes),
                                          a.get_positive("voices", 1));
        if (uint64_t(length) * set.voices > 0xFFFFFFFFull) throw std::invalid_argument("too many events in one unit");
        return Line{kind, nullptr, kDefaultCanon, {}, Space(set.id(), set.base(), length * set.voices, key), nullptr, {}};
    }
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
    {
        if (is_pcm_symbols(space.symbols_id()))
        {
            const PcmFormat f = pcm_format_of(space.symbols_id());
            return std::to_string(f.channels) + " channel(s) x " + std::to_string(space.unit_length() / f.channels) + " samples at " +
                   std::to_string(f.rate) + " a second, each one of " + std::to_string(f.base()) + " (" + std::to_string(f.bits) +
                   "-bit, two's complement)";
        }
        const NoteSet set = note_set_of(space.symbols_id());
        if (set.legacy) return std::to_string(space.unit_length()) + " note events, each one of 104 (rest or C4-C6, x 4 durations)";
        std::string names;
        for (char c : set.durations) names += std::string(names.empty() ? "" : " ") + (c == 'E' ? "e." : c == 'Q' ? "q." : c == 'H' ? "h." : std::string(1, c));
        return std::to_string(set.voices) + " voice(s) x " + std::to_string(space.unit_length() / set.voices) + " note events, each one of " +
               std::to_string(set.base()) + " (rest or " + note_name(set.low) + "-" + note_name(set.high) + ", " + std::to_string(set.pitches()) +
               " pitches, x " + std::to_string(set.duration_count()) + " durations: " + names + ")";
    }
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
        if (is_pcm_symbols(line.space.symbols_id()))
        {
            if (!a.has("file")) throw std::invalid_argument("give the sound with --file PATH (WAV; any other sound format through ffmpeg)");
            if (!a.positional.empty()) throw std::invalid_argument("the pcm set takes --file PATH, not notes");
            const PcmFormat f = pcm_format_of(line.space.symbols_id());
            std::optional<PcmCanoniser> c;
            const AudioRead m = read_media_audio(
                a.get("file"), [&](uint32_t rate, uint32_t channels) { c.emplace(f, line.space.unit_length() / f.channels, rate, channels); },
                [&](std::span<const int32_t> b) { c->add(b); });
            if (m.decoder != "sieve-wav") w.report.push_back("decoded by " + m.decoder);
            if (!m.complaints.empty()) w.report.push_back("ffmpeg reported: " + m.complaints);
            const PcmCanonResult r = c->finish();
            w.report.push_back(std::string(kPcmCanonVersion) + ": " + std::to_string(r.source_frames) + " sample(s) per channel at " +
                               std::to_string(r.source_rate) + " a second, " + std::to_string(r.source_channels) + " channel(s) -> " + f.id() +
                               ", " + std::to_string(r.units.size()) + " unit(s)");
            if (r.source_channels != f.channels)
                w.report.push_back(count_line("channels", r.source_channels,
                                              f.channels == 1 ? "mixed to one" : r.source_channels > f.channels ? "the first kept" : "the last repeated"));
            if (r.source_rate != f.rate) w.report.push_back(count_line("samples made", r.samples, "resampled by area"));
            if (r.clipped) w.report.push_back(count_line("clipped", r.clipped));
            if (r.padding) w.report.push_back(count_line("padding", r.padding, "silence filling the channels"));
            w.units = r.units;
            break;
        }
        const std::string input = a.has("file") ? read_file(a.get("file")) : joined_positional(a);
        if (input.empty()) throw std::invalid_argument("nothing to warp: give notes like \"C4q E4q G4h\" or --file PATH");
        const NoteSet set = note_set_of(line.space.symbols_id());
        const NotesCanonResult c = set.legacy ? canonicalise_notes(input, line.space.unit_length())
                                              : canonicalise_notes2(input, set, line.space.unit_length() / set.voices);
        w.report.push_back(std::string(set.legacy ? kNotesCanonVersion : kNotes2CanonVersion) + ": " + std::to_string(c.events) + " event(s), " +
                           std::to_string(c.units.size()) + " unit(s)");
        if (c.flats_rewritten) w.report.push_back(count_line("flats as sharps", c.flats_rewritten));
        if (c.octave_shifted)
            w.report.push_back(count_line("octave shifted", c.octave_shifted, set.legacy ? "moved into C4-C6" : "moved into the line's range"));
        if (c.default_durations) w.report.push_back(count_line("no duration, used q", c.default_durations));
        if (c.durations_changed) w.report.push_back(count_line("durations the set lacks", c.durations_changed, "the nearest used"));
        if (c.padding) w.report.push_back(count_line("padding", c.padding, set.legacy ? "eighth rests on the last unit" : "rests filling the voices"));
        w.units = c.units;
        break;
    }
    case LineKind::Image:
    case LineKind::Video:
    {
        if (!a.has("file"))
            throw std::invalid_argument("give the picture with --file PATH (PNG, JPEG, BMP, GIF, TGA; any other picture or "
                                        "video format through ffmpeg)");
        if (!a.positional.empty()) throw std::invalid_argument("the image line takes --file PATH, not text");
        const uint32_t wanted = line.kind == LineKind::Image ? 1 : line.image.frames;
        ImageCanoniser c(line.image); // frame by frame: a long video is never held whole
        const MediaRead m = read_media_frames(a.get("file"), wanted, [&](const RgbaImage& f) { c.add(f); });
        if (m.decoder != "stb_image")
            w.report.push_back("decoded by " + m.decoder +
                               (m.more ? ", which stopped after " + std::to_string(wanted) + " frame(s): the file has more" : ""));
        if (!m.complaints.empty()) w.report.push_back("ffmpeg reported: " + m.complaints);
        ImageCanonReport r;
        w.units.push_back(c.finish(&r));
        const size_t source_frames = r.source_frames;
        w.report.push_back(std::string(kImageCanonVersion) + ": " + std::to_string(r.source_width) + "x" +
                           std::to_string(r.source_height) + " source, " + std::to_string(source_frames) +
                           " frame(s) -> " + line.image.symbols_id());
        if (line.kind == LineKind::Image && source_frames > 1)
            w.report.push_back(count_line("frames ignored", source_frames - 1, "the image line uses the first frame"));
        if (r.transparent_pixels) w.report.push_back(count_line("transparent pixels", r.transparent_pixels, "composited onto black"));
        if (r.frames_dropped && line.kind == LineKind::Video) w.report.push_back(count_line("frames dropped", r.frames_dropped));
        if (r.frames_padded) w.report.push_back(count_line("frames padded", r.frames_padded, "black frames added"));
        break;
    }
    }
    // The vault: an address is its content in another form, so withheld content is refused here,
    // before any address of it is made.
    for (const auto& u : w.units)
        if (unit_withheld(line, u)) throw VaultWithheld("withheld by the vault: the content given");
    return w;
}

std::string preview(const Line& line, const std::vector<uint32_t>& digits)
{
    if (unit_withheld(line, digits)) return "(withheld)"; // the vault: never shown
    switch (line.kind)
    {
    case LineKind::Text: return "\"" + utf8_encode(line.space.text_of(digits)) + "\"";
    case LineKind::Audio:
        if (is_pcm_symbols(line.space.symbols_id())) return pcm_preview(pcm_format_of(line.space.symbols_id()), digits, 64);
        return notes_to_notation(note_set_of(line.space.symbols_id()), digits);
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

std::string audio_file(const Line& line, const std::vector<uint32_t>& digits)
{
    const std::string& id = line.space.symbols_id();
    if (is_pcm_symbols(id)) return pcm_to_wav(pcm_format_of(id), digits);
    return notes_to_midi(note_set_of(id), digits);
}

std::string pcm_preview(const PcmFormat& f, const std::vector<uint32_t>& digits, uint32_t columns)
{
    // Each channel a row: each column the loudest sample over its share of the unit, as a share of
    // full scale.
    static const char kShades[] = " .:-=+*#%@";
    const uint64_t L = digits.size() / f.channels;
    const uint32_t cols = uint32_t(std::min<uint64_t>(columns, std::max<uint64_t>(1, L)));
    const double full = double(uint64_t(1) << (f.bits - 1));
    std::string out;
    for (uint32_t c = 0; c < f.channels; ++c)
    {
        if (c) out += "\n";
        if (f.channels > 1) out += "C" + std::to_string(c + 1) + " ";
        out += "|";
        for (uint32_t x = 0; x < cols; ++x)
        {
            const uint64_t lo = L * x / cols, hi = std::max(lo + 1, L * (x + 1) / cols);
            int64_t peak = 0;
            for (uint64_t i = lo; i < hi && i < L; ++i) peak = std::max<int64_t>(peak, std::abs(int64_t(pcm_sample(f, digits[size_t(c * L + i)]))));
            const double share = double(peak) / full;
            out += kShades[peak == 0 ? 0 : std::min<size_t>(9, 1 + size_t(share * 9))];
        }
        out += "|";
    }
    return out;
}

bool unit_withheld(const Line& line, const std::vector<uint32_t>& digits)
{
    switch (line.kind)
    {
    case LineKind::Text:
    {
        // A line that holds every byte is a line of files: a unit is a file, by its bytes. Any other
        // text is checked as a file written out (vault_decode.hpp): its own bytes, and each
        // well-known encoding decoded. Never by what it says (docs/VAULT.md).
        if (line.alphabet && holds_all_bytes(*line.alphabet))
        {
            std::vector<uint8_t> b;
            b.reserve(digits.size());
            for (uint32_t d : digits) b.push_back(uint8_t(d));
            return vault::withheld_bytes(b);
        }
        return vault::withheld_written(utf8_encode(line.space.text_of(digits)));
    }
    case LineKind::Audio:
    {
        const std::string file = audio_file(line, digits);
        return vault::withheld_bytes(std::vector<uint8_t>(file.begin(), file.end()));
    }
    case LineKind::Image:
    case LineKind::Video: return picture_withheld(line.image, digits);
    }
    return vault::status().failed_closed;
}

bool picture_withheld(const ImageFormat& format, const std::vector<uint32_t>& digits)
{
    // Each frame as it is drawn, by PDQ (vault.hpp): a video is withheld if any frame is.
    const auto px = render_image(digits, format);
    const uint32_t W = format.width, H = format.height;
    std::vector<uint8_t> rgba(size_t(W) * H * 4);
    for (uint32_t f = 0; f < format.frames; ++f)
    {
        for (size_t i = 0; i < size_t(W) * H; ++i)
        {
            const Rgb& c = px[size_t(f) * W * H + i];
            rgba[i * 4] = c.r;
            rgba[i * 4 + 1] = c.g;
            rgba[i * 4 + 2] = c.b;
            rgba[i * 4 + 3] = 255;
        }
        if (vault::withheld_picture(rgba.data(), W, H)) return true;
    }
    return false;
}

std::vector<uint8_t> unit_file(const Line& line, const std::vector<uint32_t>& digits, uint32_t scale)
{
    if (unit_withheld(line, digits)) throw VaultWithheld("withheld by the vault: the unit");
    switch (line.kind)
    {
    case LineKind::Text:
    {
        // On a line that holds every byte, a digit is a byte: the file is the unit exactly, and
        // nothing may be encoded or appended.
        std::vector<uint8_t> out;
        if (line.alphabet && holds_all_bytes(*line.alphabet))
        {
            for (uint32_t d : digits) out.push_back(uint8_t(d));
            return out;
        }
        // Exactly the unit's text, on every alphabet: nothing is appended, so a page and its file
        // are one to one, and J lands on the file of exactly that text. (A trailing line feed was
        // once added where the alphabet has none; files saved that way keep their own addresses
        // and still open, but F no longer writes them.)
        const std::string t = utf8_encode(line.space.text_of(digits));
        out.assign(t.begin(), t.end());
        return out;
    }
    case LineKind::Audio:
    {
        const std::string file = audio_file(line, digits);
        return std::vector<uint8_t>(file.begin(), file.end());
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
        const std::string png = encode_png(sheet_w, H, sheet, scale);
        return std::vector<uint8_t>(png.begin(), png.end());
    }
    }
    return {};
}

namespace {

struct Candidate
{
    const char* ext;
    const char* name;
    std::vector<const char*> encoders; // the first the ffmpeg has is used
};

// What ffmpeg can be asked to write, for each kind of unit.
const std::vector<Candidate>& candidates(const Line& line)
{
    static const std::vector<Candidate> picture = {{".jpg", "JPEG", {"mjpeg"}}, {".webp", "WebP", {"libwebp"}}, {".bmp", "BMP", {"bmp"}},
                                                   {".tiff", "TIFF", {"tiff"}}};
    static const std::vector<Candidate> video = {{".gif", "Animated GIF", {"gif"}}, {".mp4", "MP4 video", {"libx264", "mpeg4"}},
                                                 {".webm", "WebM video", {"libvpx-vp9", "libvpx"}}};
    static const std::vector<Candidate> sound = {{".flac", "FLAC", {"flac"}}, {".mp3", "MP3", {"libmp3lame"}}, {".ogg", "Ogg Vorbis", {"libvorbis"}},
                                                 {".opus", "Opus", {"libopus"}}, {".m4a", "AAC", {"aac"}}};
    static const std::vector<Candidate> none;
    if (line.kind == LineKind::Image) return picture;
    if (line.kind == LineKind::Video) return video;
    if (line.kind == LineKind::Audio && is_pcm_symbols(line.space.symbols_id())) return sound;
    return none;
}

std::string lower(std::string s)
{
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// A file of our own in the temporary folder, removed when this goes.
struct Temp
{
    std::filesystem::path path;
    explicit Temp(const std::string& ext)
    {
        static std::atomic<uint64_t> n{0};
        path = std::filesystem::temp_directory_path() /
               ("sieve-export-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(n++) + ext);
    }
    ~Temp()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    std::string u8() const
    {
        const std::u8string s = path.u8string();
        return std::string(s.begin(), s.end());
    }
};

void write_bytes(const std::filesystem::path& p, const void* data, size_t n)
{
    std::ofstream out(p, std::ios::binary);
    out.write(static_cast<const char*>(data), std::streamsize(n));
    if (!out) throw std::runtime_error("cannot write a temporary file for ffmpeg");
}

std::vector<uint8_t> read_bytes(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), {});
}

} // namespace

std::vector<ExportFormat> export_formats(const Line& line)
{
    std::vector<ExportFormat> out;
    switch (line.kind)
    {
    case LineKind::Text: out.push_back({".txt", "Text", ""}); break;
    case LineKind::Image: out.push_back({".png", "PNG", ""}); break;
    case LineKind::Video: out.push_back({".png", "PNG (frames side by side)", ""}); break;
    case LineKind::Audio:
        out.push_back(is_pcm_symbols(line.space.symbols_id()) ? ExportFormat{".wav", "WAV", ""} : ExportFormat{".mid", "MIDI", ""});
        break;
    }
    for (const Candidate& c : candidates(line))
        for (const char* e : c.encoders)
            if (ffmpeg_has_encoder(e))
            {
                out.push_back({c.ext, c.name, e});
                break;
            }
    return out;
}

std::vector<uint8_t> export_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& ext_in, uint32_t scale, uint32_t fps)
{
    const std::string ext = lower(ext_in);
    const std::vector<ExportFormat> formats = export_formats(line);
    if (formats.empty() || ext.empty() || ext == formats.front().ext) return unit_file(line, digits, scale);
    const auto f = std::find_if(formats.begin(), formats.end(), [&](const ExportFormat& x) { return x.ext == ext; });
    if (f == formats.end())
    {
        // An extension none of ffmpeg's formats has: the line's own file, as it always was.
        const auto& cs = candidates(line);
        if (std::none_of(cs.begin(), cs.end(), [&](const Candidate& c) { return ext == c.ext; })) return unit_file(line, digits, scale);
        std::string have;
        for (const auto& x : formats) have += (have.empty() ? "" : ", ") + x.ext;
        if (const std::string why = ffmpeg_missing(); !why.empty())
            throw std::invalid_argument("saving as " + ext + " needs ffmpeg, and " + why + "; without it this line saves as " + have);
        throw std::invalid_argument("this ffmpeg has no encoder for " + ext + "; it saves this line as " + have);
    }
    if (unit_withheld(line, digits)) throw VaultWithheld("withheld by the vault: the unit");
    const Temp out(ext);
    if (line.kind == LineKind::Audio)
    {
        const std::string wav = audio_file(line, digits);
        const Temp in(".wav");
        write_bytes(in.path, wav.data(), wav.size());
        ffmpeg_convert({}, in.u8(), {"-map_metadata", "-1", "-c:a", f->encoder}, out.u8());
    }
    else
    {
        // The frames as PAM images, one after another, for ffmpeg's pam_pipe; each pixel scaled up
        // to a block (nearest neighbour), to even sizes where the video codecs need them.
        const auto px = render_image(digits, line.image);
        const uint32_t W = line.image.width, H = line.image.height, F = line.image.frames;
        std::string pam;
        for (uint32_t k = 0; k < F; ++k)
        {
            pam += "P7\nWIDTH " + std::to_string(W) + "\nHEIGHT " + std::to_string(H) + "\nDEPTH 3\nMAXVAL 255\nTUPLTYPE RGB\nENDHDR\n";
            for (size_t i = 0; i < size_t(W) * H; ++i)
            {
                const Rgb& c = px[size_t(k) * W * H + i];
                pam += char(c.r);
                pam += char(c.g);
                pam += char(c.b);
            }
        }
        const Temp in(".pam");
        write_bytes(in.path, pam.data(), pam.size());
        const std::string S = std::to_string(std::max<uint32_t>(1, scale));
        const bool codec = ext == ".mp4" || ext == ".webm";
        std::string vf = codec ? "scale=ceil(iw*" + S + "/2)*2:ceil(ih*" + S + "/2)*2:flags=neighbor" : "scale=iw*" + S + ":ih*" + S + ":flags=neighbor";
        // A GIF keeps the line's own colours: its palette is made from the frames, without dithering.
        if (ext == ".gif") vf += ",split[a][b];[a]palettegen=reserve_transparent=0[p];[b][p]paletteuse=dither=none";
        std::vector<std::string> opts = {"-map_metadata", "-1", "-vf", vf, "-c:v", f->encoder};
        if (codec) opts.insert(opts.end(), {"-pix_fmt", "yuv420p"});
        if (line.kind == LineKind::Image) opts.insert(opts.end(), {"-frames:v", "1", "-update", "1"});
        ffmpeg_convert({"-f", "pam_pipe", "-framerate", std::to_string(std::max<uint32_t>(1, fps))}, in.u8(), opts, out.u8());
    }
    return read_bytes(out.path);
}

void save_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& path, uint32_t scale, uint32_t fps)
{
    // By the path's extension: a format of the line's own, or one ffmpeg writes from it.
    const std::string ext = std::filesystem::path(std::u8string(path.begin(), path.end())).extension().string();
    const std::vector<uint8_t> bytes = export_unit(line, digits, ext, scale, fps);
    std::ofstream out(std::filesystem::path(path), std::ios::binary);
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
}

} // namespace sieve::cli
