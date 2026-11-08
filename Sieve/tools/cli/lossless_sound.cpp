#include "cli/lossless_sound.hpp"

#include "cli/flac_wav.hpp"
#include "cli/media_decode.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

namespace sieve::cli {

namespace {

uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

// Where the sound starts: after an ID3v2 tag (TTA and Monkey's Audio files sometimes begin with one).
size_t after_id3(const std::vector<uint8_t>& b)
{
    if (b.size() < 10 || std::memcmp(b.data(), "ID3", 3) != 0) return 0;
    if ((b[6] | b[7] | b[8] | b[9]) & 0x80) return 0;
    const size_t n = size_t(b[6]) << 21 | size_t(b[7]) << 14 | size_t(b[8]) << 7 | b[9];
    return 10 + n + ((b[5] & 0x10) ? 10 : 0);
}

// An MP4's boxes walked for Apple Lossless: one track, a sound track ('soun'), whose sample entry is
// 'alac'. (The containers on the way: moov, trak, mdia, minf, stbl; iTunes' tags in udta are not
// walked into, so their own 'hdlr' is not counted.)
struct Mp4Seen
{
    int sound = 0, other = 0;
    bool alac = false, broken = false;
};

void walk_mp4(const uint8_t* p, size_t n, int depth, Mp4Seen& s)
{
    size_t at = 0;
    while (at + 8 <= n && !s.broken)
    {
        uint64_t size = be32(p + at), head = 8;
        if (size == 1)
        {
            if (at + 16 > n) return void(s.broken = true);
            size = uint64_t(be32(p + at + 8)) << 32 | be32(p + at + 12);
            head = 16;
        }
        else if (size == 0)
            size = n - at;
        if (size < head || size > n - at) return void(s.broken = true);
        const uint8_t* type = p + at + 4;
        const uint8_t* body = p + at + head;
        const size_t len = size_t(size - head);
        auto is = [&](const char* t) { return std::memcmp(type, t, 4) == 0; };
        if (depth < 6 && (is("moov") || is("trak") || is("mdia") || is("minf") || is("stbl"))) walk_mp4(body, len, depth + 1, s);
        else if (depth > 0 && is("hdlr") && len >= 12)
            (std::memcmp(body + 8, "soun", 4) == 0 ? s.sound : s.other) += 1;
        else if (depth > 0 && is("stsd") && len >= 16 && std::memcmp(body + 12, "alac", 4) == 0)
            s.alac = true;
        at += size_t(size);
    }
}

std::vector<uint8_t> read_file(const std::filesystem::path& p)
{
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

bool write_file(const std::filesystem::path& p, const std::vector<uint8_t>& b)
{
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
    return bool(f);
}

// The bits a sample a FLAC's STREAMINFO gives (0 where it is not a FLAC).
int flac_bits(const std::vector<uint8_t>& f)
{
    if (f.size() < 22 || std::memcmp(f.data(), "fLaC", 4) != 0 || (f[4] & 0x7F) != 0) return 0;
    return ((f[20] & 1) << 4 | f[21] >> 4) + 1;
}

// The one sound stream's line of ffmpeg's report ("Stream #0:0: Audio: wavpack, 44100 Hz, stereo,
// s16p"): its codec, and the bits a sample it decodes to (16; N for "s32p (N bit)"; 32 for s32 with no
// note, which may hold fewer), 0 for any other sample format (8-bit, floating point). Nothing where there is not exactly one.
struct Probe
{
    std::string codec;
    int bits = 0;
};

std::optional<Probe> probe(const std::string& report)
{
    std::optional<Probe> r;
    size_t at = 0;
    while ((at = report.find("Audio: ", at)) != std::string::npos)
    {
        if (r) return std::nullopt; // more than one sound stream
        at += 7;
        const std::string line = report.substr(at, report.find('\n', at) - at);
        Probe p;
        p.codec = line.substr(0, line.find_first_of(" ,"));
        size_t f = 0;
        while (f < line.size())
        {
            const size_t e = std::min(line.find(", ", f), line.size());
            const std::string field = line.substr(f, e - f);
            if (field == "s16" || field == "s16p") p.bits = 16;
            else if (field == "s32" || field == "s32p") p.bits = 32;
            else if (field.rfind("s32 (", 0) == 0 || field.rfind("s32p (", 0) == 0)
                p.bits = std::atoi(field.c_str() + field.find('(') + 1);
            f = e + 2;
        }
        r = p;
    }
    return r;
}

} // namespace

const char* lossless_sound_name(const std::vector<uint8_t>& b)
{
    const size_t o = after_id3(b);
    if (o + 32 <= b.size())
    {
        const uint8_t* p = b.data() + o;
        const unsigned version = p[8] | p[9] << 8;
        if (std::memcmp(p, "wvpk", 4) == 0 && version >= 0x402 && version <= 0x410) return "WavPack";
        if (std::memcmp(p, "MAC ", 4) == 0) return "Monkey's Audio";
        if (std::memcmp(p, "TTA1", 4) == 0) return "TTA";
    }
    if (b.size() >= 16 && std::memcmp(b.data() + 4, "ftyp", 4) == 0)
    {
        Mp4Seen s;
        walk_mp4(b.data(), b.size(), 0, s);
        if (!s.broken && s.alac && s.sound == 1 && s.other == 0) return "ALAC";
    }
    return "";
}

std::optional<std::vector<uint8_t>> lossless_as_flac(const std::vector<uint8_t>& b, int effort, std::string* why_out)
{
    auto no = [&](const std::string& w) {
        if (why_out) *why_out = w;
        return std::nullopt;
    };
    const std::string name = lossless_sound_name(b);
    if (name.empty()) return no("not WavPack, Monkey's Audio, TTA or ALAC");
    if (!ffmpeg_missing().empty()) return no("needs ffmpeg");
    const char* codec = name == "WavPack" ? "wavpack" : name == "TTA" ? "tta" : name == "ALAC" ? "alac" : "ape";
    if (name == "WavPack")
    {
        const uint32_t flags = le32(b.data() + after_id3(b) + 24);
        if (flags & 0x8) return no("WavPack's hybrid (lossy) mode");
        if (flags & 0x80000080u) return no("WavPack of floating-point or DSD samples");
    }
    try
    {
        const TempFile in(name == "ALAC" ? ".m4a" : name == "WavPack" ? ".wv" : name == "TTA" ? ".tta" : ".ape");
        if (!write_file(in.path, b)) return no("cannot write a temporary file");
        const FfmpegRun look = ffmpeg_run({"-i", in.u8()});
        const std::optional<Probe> pr = probe(look.err);
        if (!pr || pr->codec != codec) return no("ffmpeg does not read it as one " + name + " stream");
        // (ffmpeg names a 24-bit WavPack "s32p" alone, as it does a 32-bit one: its FLAC says which, below)
        const int bits = pr->bits;
        if (bits < 16 || bits > 32) return no("not 16 to 24-bit integer samples");
        // Decoded once: the FLAC (tags, cover pictures), and the MD5 of its samples as ffmpeg decodes them.
        const TempFile mid(".flac");
        auto convert = [&](bool pictures) {
            std::vector<std::string> a = {"-v", "error", "-y", "-err_detect", "crccheck+explode", "-i", in.u8(), "-map", "0:a:0"};
            if (pictures) a.insert(a.end(), {"-map", "0:v?", "-c:v", "copy"});
            a.insert(a.end(), {"-map_metadata", "0", "-c:a", "flac", "-compression_level", "0", mid.u8(), "-map", "0:a:0", "-c:a", "pcm_s32le", "-f", "md5", "-"});
            return ffmpeg_run(a);
        };
        FfmpegRun made = convert(true);
        if (made.status != 0 || !made.err.empty())
        {
            if (made.err.find("CRC") != std::string::npos) return no("ffmpeg: " + made.err);
            made = convert(false); // (a cover picture FLAC cannot hold: the sound and tags alone)
        }
        if (made.status != 0 || !made.err.empty() || made.out.rfind("MD5=", 0) != 0) return no("ffmpeg: " + (made.err.empty() ? std::string("no FLAC made") : made.err));
        const std::vector<uint8_t> first = read_file(mid.path);
        const int fb = flac_bits(first);
        if (bits == 32 ? (fb < 16 || fb > 24) : fb != bits) return no("not 16 to 24-bit integer samples");
        std::optional<std::vector<uint8_t>> f = flac_recoded(first, effort);
        if (!f) return no("libFLAC did not code it again");
        // The FLAC installed, decoded by ffmpeg's own FLAC decoder: the source's samples, every one.
        const TempFile out(".flac");
        if (!write_file(out.path, *f)) return no("cannot write a temporary file");
        const FfmpegRun check = ffmpeg_run({"-v", "error", "-i", out.u8(), "-map", "0:a:0", "-c:a", "pcm_s32le", "-f", "md5", "-"});
        if (!check.err.empty() || check.out != made.out) return no("the FLAC's samples are not the source's");
        return f;
    }
    catch (const std::exception& e)
    {
        return no(e.what());
    }
}

} // namespace sieve::cli
