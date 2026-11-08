#include "cli/flac_wav.hpp"

#include <FLAC/format.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace sieve::cli {

namespace {

uint32_t le16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
uint32_t be16(const uint8_t* p) { return uint32_t(p[0]) << 8 | uint32_t(p[1]); }
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]); }

uint64_t le64(const uint8_t* p) { return uint64_t(le32(p)) | uint64_t(le32(p + 4)) << 32; }
uint64_t be64(const uint8_t* p) { return uint64_t(be32(p)) << 32 | be32(p + 4); }

// What a sound file's chunks say, up to its samples.
struct Layout
{
    uint32_t channels = 0, bits = 0, rate = 0;
    size_t samples_at = 0;  // where its samples start
    uint64_t data = 0;      // their size, in bytes
    uint64_t chunk_end = 0; // where the chunk holding them ends (the file's own, not the skeleton's)
    bool big = false;       // big-endian samples (AIFF's, .au's)
    bool unsigned8 = false; // 8-bit samples unsigned (WAV's)
    bool rate_true = true;  // the rate read is the file's own (else a label given FLAC)
    bool to_end = false;    // the samples run to the end of the file, their size not written (.au, CAF):
                            // in a skeleton, nothing after them, and their size the FLAC stream's
    bool flt = false;       // IEEE floating point of 32 or 64 bits (8 November 2026): carried as FLAC of 24
                            // bits, where every value is a whole number over 2^23 (float_sample)
    uint32_t mask = 0;      // a WAVE_FORMAT_EXTENSIBLE file's channel mask (0: none given)
};

bool bits_ok(uint32_t bits) { return bits == 8 || bits == 16 || bits == 24 || bits == 32; }
bool float_bits_ok(uint32_t bits) { return bits == 32 || bits == 64; }
bool bits_ok(const Layout& l) { return l.flt ? float_bits_ok(l.bits) : bits_ok(l.bits); }
// The bits a sample has in the FLAC stream: a float's 24, else the file's own.
uint32_t coded_bits(const Layout& l) { return l.flt ? 24 : l.bits; }

// A floating-point sample (`u` its bits, 32 or 64 of them) as the whole number n it is n / 2^23 of,
// -2^23 <= n < 2^23, read by integer arithmetic alone; nothing where it is not one (a fraction of a
// step, out of range, -0, a subnormal, an infinity or not a number). Every such n is written back as
// one float alone (int_float), so the file is given back bit for bit.
std::optional<int32_t> float_sample(uint64_t u, uint32_t bits)
{
    if (u == 0) return 0;
    const uint32_t man_bits = bits == 32 ? 23 : 52, exp_bits = bits == 32 ? 8 : 11;
    const int bias = bits == 32 ? 127 : 1023;
    const bool negative = (u >> (bits - 1)) & 1;
    const int exp = int((u >> man_bits) & ((uint64_t(1) << exp_bits) - 1));
    const uint64_t m = (u & ((uint64_t(1) << man_bits) - 1)) | uint64_t(1) << man_bits;
    if (exp == 0 || exp == (1 << exp_bits) - 1) return std::nullopt;
    // n = m * 2^(exp - bias - man_bits + 23): a whole number where the shift right leaves no bits
    const int shift = bias + int(man_bits) - 23 - exp;
    if (shift < 0 || shift > int(man_bits) || (m & ((uint64_t(1) << shift) - 1)) != 0) return std::nullopt;
    const uint64_t a = m >> shift;
    if (negative ? a > (uint64_t(1) << 23) : a >= (uint64_t(1) << 23)) return std::nullopt;
    return negative ? -int32_t(a) : int32_t(a);
}

// n / 2^23 as a float of 32 or 64 bits, by integer arithmetic alone.
uint64_t int_float(int32_t n, uint32_t bits)
{
    if (n == 0) return 0;
    const uint32_t man_bits = bits == 32 ? 23 : 52;
    const int bias = bits == 32 ? 127 : 1023;
    const uint64_t a = n < 0 ? uint64_t(-int64_t(n)) : uint64_t(n);
    int e = 63;
    while (!((a >> e) & 1)) --e; // (a's top bit: 0 to 23)
    const uint64_t man = (a << (man_bits - uint32_t(e))) & ((uint64_t(1) << man_bits) - 1);
    return uint64_t(n < 0) << (bits - 1) | uint64_t(e - 23 + bias) << man_bits | man;
}

// A WAV's "fmt " chunk's body (WAV, RF64, Wave64 alike): plain integer PCM, format 1, or 0xFFFE whose
// sub-format is PCM's GUID; 1 to 8 channels; 8, 16, 24 or 32 bits; a block of the channels times the
// bytes a sample; a rate FLAC can hold. Or IEEE floating point: format 3, or 0xFFFE whose sub-format
// is its GUID; 32 or 64 bits.
bool wav_fmt(const uint8_t* c, uint64_t size, Layout& l)
{
    if (size < 16) return false;
    const uint32_t tag = le16(c);
    l.channels = le16(c + 2);
    l.rate = le32(c + 4);
    const uint32_t align = le16(c + 12);
    l.bits = le16(c + 14);
    if (tag == 0xFFFE)
    {
        static const uint8_t pcm[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
        static const uint8_t flt[16] = {3, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
        if (size < 40 || le16(c + 16) < 22) return false;
        l.mask = le32(c + 20);
        if (std::memcmp(c + 24, flt, 16) == 0) l.flt = true;
        else if (std::memcmp(c + 24, pcm, 16) != 0) return false;
    }
    else if (tag == 3)
        l.flt = true;
    else if (tag != 1)
        return false;
    return l.channels >= 1 && l.channels <= 8 && bits_ok(l) && align == l.channels * (l.bits / 8) && FLAC__format_sample_rate_is_valid(l.rate);
}

// A WAV's chunks (or RF64's, BW64's) walked from byte 12 to the data chunk: one "fmt " before it;
// every chunk before it whole within the n bytes. RF64: a "ds64" chunk first, whose data size (eight
// bytes after the RIFF size's eight) is the data chunk's where its own size is 0xFFFFFFFF. Nothing if
// the file is not so.
std::optional<Layout> walk_wav(const uint8_t* p, size_t n)
{
    Layout l;
    l.unsigned8 = true;
    const bool rf64 = std::memcmp(p, "RIFF", 4) != 0;
    bool fmt = false;
    std::optional<uint64_t> ds64;
    for (size_t at = 12; at + 8 <= n;)
    {
        const uint8_t* c = p + at;
        uint64_t size = le32(c + 4);
        if (rf64 && at == 12)
        {
            if (std::memcmp(c, "ds64", 4) != 0 || size < 28 || at + 8 + size > n) return std::nullopt;
            ds64 = le64(c + 16);
        }
        if (std::memcmp(c, "data", 4) == 0)
        {
            if (!fmt) return std::nullopt;
            if (rf64 && size == 0xFFFFFFFFu) size = *ds64;
            l.samples_at = at + 8;
            l.data = size;
            l.chunk_end = at + 8 + size;
            return l;
        }
        if (std::memcmp(c, "fmt ", 4) == 0)
        {
            if (fmt || at + 8 + size > n || !wav_fmt(c + 8, size, l)) return std::nullopt;
            fmt = true;
        }
        const uint64_t next = uint64_t(at) + 8 + size + (size & 1);
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

// Sony's Wave64: chunks named by 16-byte GUIDs, sizes of eight bytes (little-endian, the 24 bytes of
// the chunk's own head counted), each chunk padded to eight bytes; from byte 40 (the riff GUID, its
// size, the wave GUID) to the data chunk, one fmt before it, as a WAV's.
const uint8_t kW64Riff[16] = {0x72, 0x69, 0x66, 0x66, 0x2E, 0x91, 0xCF, 0x11, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00};
const uint8_t kW64Wave[16] = {0x77, 0x61, 0x76, 0x65, 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t kW64Fmt[16] = {0x66, 0x6D, 0x74, 0x20, 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t kW64Data[16] = {0x64, 0x61, 0x74, 0x61, 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};

std::optional<Layout> walk_w64(const uint8_t* p, size_t n)
{
    Layout l;
    l.unsigned8 = true;
    bool fmt = false;
    for (size_t at = 40; at + 24 <= n;)
    {
        const uint8_t* c = p + at;
        const uint64_t size = le64(c + 16);
        if (size < 24) return std::nullopt;
        if (std::memcmp(c, kW64Data, 16) == 0)
        {
            if (!fmt) return std::nullopt;
            l.samples_at = at + 24;
            l.data = size - 24;
            l.chunk_end = at + size;
            return l;
        }
        if (std::memcmp(c, kW64Fmt, 16) == 0)
        {
            if (fmt || at + size > n || !wav_fmt(c + 24, size - 24, l)) return std::nullopt;
            fmt = true;
        }
        const uint64_t next = uint64_t(at) + ((size + 7) & ~uint64_t(7));
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

// An AIFF's (or AIFF-C's) chunks walked from byte 12 to its SSND chunk: one COMM before it, 1 to 8
// channels, 8, 16, 24 or 32 bits a sample, at least one sample frame, AIFF-C's samples not
// compressed ("NONE" or "twos", big-endian; "sowt", little-endian) or floating point ("fl32" or "FL32",
// 32 bits; "fl64" or "FL64", 64; big-endian); every chunk before it whole within
// the n bytes; SSND's offset within them. Its samples are its frames times its block, from SSND's
// eight bytes of offset and block size and the offset's bytes after them. Nothing if not so.
std::optional<Layout> walk_aiff(const uint8_t* p, size_t n)
{
    Layout l;
    l.big = true;
    const bool aifc = std::memcmp(p + 8, "AIFC", 4) == 0;
    uint64_t frames = 0;
    bool comm = false;
    for (size_t at = 12; at + 8 <= n;)
    {
        const uint8_t* c = p + at;
        const uint64_t size = be32(c + 4);
        if (std::memcmp(c, "SSND", 4) == 0)
        {
            if (!comm || size < 8 || at + 16 > n) return std::nullopt;
            const uint64_t offset = be32(c + 8);
            if (at + 16 + offset > n) return std::nullopt;
            l.samples_at = size_t(at + 16 + offset);
            l.data = frames * l.channels * (l.bits / 8);
            l.chunk_end = at + 8 + size;
            return l;
        }
        if (std::memcmp(c, "COMM", 4) == 0)
        {
            if (comm || size < (aifc ? 22u : 18u) || at + 8 + size > n) return std::nullopt;
            comm = true;
            l.channels = be16(c + 8);
            frames = be32(c + 10);
            l.bits = be16(c + 14);
            // The rate, an 80-bit extended number: kept in the skeleton, and given to FLAC only as a
            // label (44,100 where it is not a whole rate FLAC can hold).
            const int exponent = int(be16(c + 16) & 0x7FFF) - 16383;
            const uint64_t mantissa = uint64_t(be32(c + 18)) << 32 | be32(c + 22);
            l.rate = exponent >= 0 && exponent < 20 ? uint32_t(mantissa >> (63 - exponent)) : 0;
            if (!FLAC__format_sample_rate_is_valid(l.rate)) l.rate = 44100, l.rate_true = false;
            if (aifc)
            {
                const uint8_t* k = c + 26;
                if (std::memcmp(k, "sowt", 4) == 0) l.big = false;
                else if (std::memcmp(k, "fl32", 4) == 0 || std::memcmp(k, "FL32", 4) == 0) l.flt = true, l.bits = 32;
                else if (std::memcmp(k, "fl64", 4) == 0 || std::memcmp(k, "FL64", 4) == 0) l.flt = true, l.bits = 64;
                else if (std::memcmp(k, "NONE", 4) != 0 && std::memcmp(k, "twos", 4) != 0) return std::nullopt;
                if (l.flt && be16(c + 14) != l.bits) return std::nullopt;
            }
            if (l.channels < 1 || l.channels > 8 || !bits_ok(l) || frames == 0) return std::nullopt;
        }
        const uint64_t next = uint64_t(at) + 8 + size + (size & 1);
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

// Sun's .au: ".snd", then four big-endian numbers of four bytes: where the samples start (24 or more),
// their size (0xFFFFFFFF: to the end of the file), the encoding (2, 3, 4, 5: linear PCM of 8, 16, 24,
// 32 bits, signed, big-endian; 6, 7: floating point of 32, 64 bits), the rate; then the channels (1
// to 8). The bytes between are kept.
std::optional<Layout> walk_au(const uint8_t* p, size_t n)
{
    if (n < 24) return std::nullopt;
    Layout l;
    l.big = true;
    const uint64_t offset = be32(p + 4), size = be32(p + 8), encoding = be32(p + 12);
    l.rate = be32(p + 16);
    l.channels = be32(p + 20);
    if (offset < 24 || offset > n || encoding < 2 || encoding > 7 || l.channels < 1 || l.channels > 8) return std::nullopt;
    if (!FLAC__format_sample_rate_is_valid(l.rate)) l.rate = 44100, l.rate_true = false;
    l.flt = encoding >= 6;
    l.bits = l.flt ? (encoding == 6 ? 32 : 64) : uint32_t(encoding - 1) * 8;
    l.samples_at = size_t(offset);
    l.to_end = size == 0xFFFFFFFFu;
    l.data = l.to_end ? n - offset : size;
    l.chunk_end = l.to_end ? n : offset + size;
    return l;
}

// Apple's CAF: "caff", version 1, flags 0; then chunks of a type (four bytes) and a size (eight,
// big-endian, signed), from byte 8, to the data chunk: one "desc" before it, 32 bytes or more: the
// rate (a double), "lpcm", its flags (bit 1 floating point, of 32 or 64 bits; bit 2 little-endian),
// bytes a packet (the channels times the bytes a sample), one frame a packet, 1 to 8 channels, 8, 16,
// 24 or 32 bits. The data chunk's samples follow its four bytes of edit count; its size counts them, or is -1
// where they run to the end of the file.
std::optional<Layout> walk_caf(const uint8_t* p, size_t n)
{
    if (n < 8 || be16(p + 4) != 1 || be16(p + 6) != 0) return std::nullopt;
    Layout l;
    l.big = true;
    bool desc = false;
    for (size_t at = 8; at + 12 <= n;)
    {
        const uint8_t* c = p + at;
        const uint64_t size = be64(c + 4);
        if (std::memcmp(c, "data", 4) == 0)
        {
            if (!desc || at + 16 > n) return std::nullopt;
            l.samples_at = at + 16;
            l.to_end = size == ~uint64_t(0);
            if (!l.to_end && size < 4) return std::nullopt;
            l.data = l.to_end ? n - l.samples_at : size - 4;
            l.chunk_end = l.to_end ? n : at + 12 + size;
            return l;
        }
        if (size >= (uint64_t(1) << 62)) return std::nullopt;
        if (std::memcmp(c, "desc", 4) == 0)
        {
            if (desc || size < 32 || at + 12 + size > n) return std::nullopt;
            desc = true;
            const uint8_t* d = c + 12;
            double rate = 0;
            const uint64_t r = be64(d);
            std::memcpy(&rate, &r, 8);
            const uint32_t flags = be32(d + 12), packet = be32(d + 16), frames = be32(d + 20);
            l.channels = be32(d + 24);
            l.bits = be32(d + 28);
            l.big = !(flags & 2);
            l.flt = flags & 1;
            l.rate_true = rate >= 1 && rate <= 1048575 && rate == double(uint32_t(rate));
            l.rate = l.rate_true ? uint32_t(rate) : 44100;
            if (std::memcmp(d + 8, "lpcm", 4) != 0 || frames != 1 || l.channels < 1 || l.channels > 8 || !bits_ok(l) ||
                packet != l.channels * (l.bits / 8))
                return std::nullopt;
        }
        const uint64_t next = uint64_t(at) + 12 + size;
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

// Which kind of sound file b starts as, by its first bytes; 0 for none of them.
int sound_kind(const uint8_t* p, size_t n)
{
    if (n >= 12 && (std::memcmp(p, "RIFF", 4) == 0 || std::memcmp(p, "RF64", 4) == 0 || std::memcmp(p, "BW64", 4) == 0) && std::memcmp(p + 8, "WAVE", 4) == 0)
        return 1;
    if (n >= 12 && std::memcmp(p, "FORM", 4) == 0 && (std::memcmp(p + 8, "AIFF", 4) == 0 || std::memcmp(p + 8, "AIFC", 4) == 0)) return 2;
    if (n >= 40 && std::memcmp(p, kW64Riff, 16) == 0 && std::memcmp(p + 24, kW64Wave, 16) == 0) return 3;
    if (n >= 4 && std::memcmp(p, ".snd", 4) == 0) return 4;
    if (n >= 4 && std::memcmp(p, "caff", 4) == 0) return 5;
    return 0;
}

bool is_aiff_head(const uint8_t* p, size_t n) { return sound_kind(p, n) == 2; }

std::optional<Layout> walk(const uint8_t* p, size_t n)
{
    switch (sound_kind(p, n))
    {
    case 1: return walk_wav(p, n);
    case 2: return walk_aiff(p, n);
    case 3: return walk_w64(p, n);
    case 4: return walk_au(p, n);
    case 5: return walk_caf(p, n);
    default: return std::nullopt;
    }
}

uint64_t raw_at(const uint8_t* p, const Layout& l)
{
    const uint32_t bytes = l.bits / 8;
    uint64_t u = 0;
    for (uint32_t k = 0; k < bytes; ++k) u |= uint64_t(p[l.big ? bytes - 1 - k : k]) << (8 * k);
    return u;
}

// (an integer sample; a float's is float_sample's)
int32_t sample_at(const uint8_t* p, const Layout& l)
{
    const uint32_t bytes = l.bits / 8;
    uint32_t u = 0;
    for (uint32_t k = 0; k < bytes; ++k) u |= uint32_t(p[l.big ? bytes - 1 - k : k]) << (8 * k);
    if (bytes == 1 && l.unsigned8) return int32_t(u) - 128;
    return int32_t(u << (32 - l.bits)) >> (32 - l.bits);
}

void put_sample(std::vector<uint8_t>& o, int32_t v, const Layout& l)
{
    const uint32_t bytes = l.bits / 8;
    const uint64_t u = l.flt ? int_float(v, l.bits) : bytes == 1 && l.unsigned8 ? uint32_t(v + 128) : uint32_t(v);
    for (uint32_t k = 0; k < bytes; ++k) o.push_back(uint8_t(u >> (8 * (l.big ? bytes - 1 - k : k))));
}

// libFLAC's encoder writing into memory; seeking lets it write STREAMINFO again at the end, with the
// samples' MD5 the decoder checks.
struct Sink
{
    std::vector<uint8_t> b;
    size_t at = 0;
};

FLAC__StreamEncoderWriteStatus sink_write(const FLAC__StreamEncoder*, const FLAC__byte buf[], size_t n, uint32_t, uint32_t, void* c)
{
    auto* s = static_cast<Sink*>(c);
    if (s->at + n > s->b.size()) s->b.resize(s->at + n);
    std::memcpy(s->b.data() + s->at, buf, n);
    s->at += n;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}
FLAC__StreamEncoderSeekStatus sink_seek(const FLAC__StreamEncoder*, FLAC__uint64 to, void* c)
{
    auto* s = static_cast<Sink*>(c);
    if (to > s->b.size()) return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    s->at = size_t(to);
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}
FLAC__StreamEncoderTellStatus sink_tell(const FLAC__StreamEncoder*, FLAC__uint64* at, void* c)
{
    *at = static_cast<Sink*>(c)->at;
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

struct Source
{
    const uint8_t* p = nullptr;
    size_t n = 0, at = 0;
    Layout l;
    std::vector<uint8_t>* out = nullptr;
    uint64_t written = 0; // samples' bytes
    const std::function<bool(size_t)>* too_big = nullptr;
    bool bad = false, info = false;
};

FLAC__StreamDecoderReadStatus source_read(const FLAC__StreamDecoder*, FLAC__byte buf[], size_t* n, void* c)
{
    auto* s = static_cast<Source*>(c);
    const size_t k = std::min(*n, s->n - s->at);
    std::memcpy(buf, s->p + s->at, k);
    s->at += k;
    *n = k;
    return k ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}
void source_metadata(const FLAC__StreamDecoder*, const FLAC__StreamMetadata* m, void* c)
{
    auto* s = static_cast<Source*>(c);
    if (m->type != FLAC__METADATA_TYPE_STREAMINFO) return;
    const auto& i = m->data.stream_info;
    const uint64_t block = uint64_t(s->l.channels) * (s->l.bits / 8);
    // The stream must say what the skeleton's chunks say: the same channels, bits and samples.
    // (where the skeleton does not say how many, the samples running to its end, the stream does)
    if (s->l.to_end && !s->info) s->l.data = i.total_samples * block;
    if (s->info || i.channels != s->l.channels || i.bits_per_sample != coded_bits(s->l) || i.total_samples == 0 || i.total_samples * block != s->l.data)
        s->bad = true;
    s->info = true;
}
FLAC__StreamDecoderWriteStatus source_write(const FLAC__StreamDecoder*, const FLAC__Frame* f, const FLAC__int32* const ch[], void* c)
{
    auto* s = static_cast<Source*>(c);
    const uint32_t bytes = s->l.bits / 8;
    const uint64_t frame = uint64_t(f->header.blocksize) * s->l.channels * bytes;
    if (!s->info || s->bad || f->header.channels != s->l.channels || f->header.bits_per_sample != coded_bits(s->l) || s->written + frame > s->l.data ||
        (*s->too_big)(s->out->size() + size_t(frame)))
    {
        s->bad = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    // (each sample within its bits, as the specification and the oracle hold it)
    const int64_t lo = -(int64_t(1) << (coded_bits(s->l) - 1)), hi = (int64_t(1) << (coded_bits(s->l) - 1)) - 1;
    for (uint32_t k = 0; k < f->header.blocksize; ++k)
        for (uint32_t j = 0; j < s->l.channels; ++j)
        {
            if (ch[j][k] < lo || ch[j][k] > hi)
            {
                s->bad = true;
                return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
            }
            put_sample(*s->out, ch[j][k], s->l);
        }
    s->written += frame;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}
void source_error(const FLAC__StreamDecoder*, FLAC__StreamDecoderErrorStatus, void* c) { static_cast<Source*>(c)->bad = true; }

} // namespace

bool is_wav(const std::vector<uint8_t>& b)
{
    return b.size() >= 12 && std::memcmp(b.data(), "RIFF", 4) == 0 && std::memcmp(b.data() + 8, "WAVE", 4) == 0;
}

bool is_aiff(const std::vector<uint8_t>& b) { return is_aiff_head(b.data(), b.size()); }

bool is_sound(const std::vector<uint8_t>& b) { return sound_kind(b.data(), b.size()) != 0; }

const char* sound_name(const std::vector<uint8_t>& b)
{
    switch (sound_kind(b.data(), b.size()))
    {
    case 1: return std::memcmp(b.data(), "RIFF", 4) == 0 ? "WAV" : "RF64";
    case 2: return "AIFF";
    case 3: return "Wave64";
    case 4: return ".au";
    case 5: return "CAF";
    default: return "";
    }
}

std::vector<uint8_t> wav_remake(const std::vector<uint8_t>& inner, size_t skeleton, const std::function<bool(size_t)>& too_big)
{
    if (skeleton > inner.size()) throw std::runtime_error("a sound file's skeleton is larger than what is carried");
    const size_t flac = inner.size() - skeleton;
    const uint8_t* sk = inner.data() + flac;
    const auto l = walk(sk, skeleton);
    if (!l || (l->to_end ? l->samples_at != skeleton : l->data == 0 || l->data % (uint64_t(l->channels) * (l->bits / 8)) != 0))
        throw std::runtime_error("a sound file's skeleton this Sieve does not write");
    std::vector<uint8_t> out(sk, sk + l->samples_at);
    Source s;
    s.p = inner.data();
    s.n = flac;
    s.l = *l;
    s.out = &out;
    s.too_big = &too_big;
    FLAC__StreamDecoder* d = FLAC__stream_decoder_new();
    if (!d) throw std::runtime_error("libFLAC could not start");
    FLAC__stream_decoder_set_md5_checking(d, true);
    bool whole = FLAC__stream_decoder_init_stream(d, source_read, nullptr, nullptr, nullptr, nullptr, source_write, source_metadata, source_error, &s) ==
                 FLAC__STREAM_DECODER_INIT_STATUS_OK;
    whole = whole && FLAC__stream_decoder_process_until_end_of_stream(d) && FLAC__stream_decoder_get_state(d) == FLAC__STREAM_DECODER_END_OF_STREAM;
    // (finish checks the samples against the MD5 STREAMINFO carries)
    whole = FLAC__stream_decoder_finish(d) && whole;
    FLAC__stream_decoder_delete(d);
    if (!whole || s.bad || !s.info || s.written != s.l.data || s.at != flac) throw std::runtime_error("a sound file's FLAC stream that does not give back its samples");
    out.insert(out.end(), sk + l->samples_at, sk + skeleton);
    return out;
}

namespace {

// Samples coded by libFLAC, `fill` giving each run of frames interleaved: out of the streamable subset
// for what Sieve carries (any rate, prediction up to 32 back at Size), in it for a .flac file to install.
std::optional<std::vector<uint8_t>> encode_frames(uint32_t channels, uint32_t bits, uint32_t rate, uint64_t frames, int effort, bool subset,
                                                  const std::function<void(FLAC__int32*, size_t)>& fill)
{
    FLAC__StreamEncoder* e = FLAC__stream_encoder_new();
    if (!e) return std::nullopt;
    bool ok = FLAC__stream_encoder_set_channels(e, channels) && FLAC__stream_encoder_set_bits_per_sample(e, bits) &&
              FLAC__stream_encoder_set_sample_rate(e, rate) && FLAC__stream_encoder_set_streamable_subset(e, subset) &&
              FLAC__stream_encoder_set_compression_level(e, effort == 0 ? 5 : 8) && FLAC__stream_encoder_set_total_samples_estimate(e, frames);
    if (effort >= 2) ok = ok && (subset || FLAC__stream_encoder_set_max_lpc_order(e, 32)) && FLAC__stream_encoder_set_do_qlp_coeff_prec_search(e, true);
    Sink sink;
    ok = ok && FLAC__stream_encoder_init_stream(e, sink_write, sink_seek, sink_tell, nullptr, &sink) == FLAC__STREAM_ENCODER_INIT_STATUS_OK;
    std::vector<FLAC__int32> buf;
    for (uint64_t at = 0; ok && at < frames;)
    {
        const uint64_t n = std::min<uint64_t>(frames - at, 1 << 16);
        buf.resize(size_t(n) * channels);
        fill(buf.data(), buf.size());
        ok = FLAC__stream_encoder_process_interleaved(e, buf.data(), uint32_t(n));
        at += n;
    }
    ok = FLAC__stream_encoder_finish(e) && ok;
    FLAC__stream_encoder_delete(e);
    if (!ok) return std::nullopt;
    return std::move(sink.b);
}

// The samples of b (laid out as l says) coded by libFLAC.
// Floats: only where every one is a whole number over 2^23 (looked at first, in one pass).
std::optional<std::vector<uint8_t>> encode(const std::vector<uint8_t>& b, const Layout& l, int effort, bool subset)
{
    const uint32_t bytes = l.bits / 8;
    if (l.flt)
        for (const uint8_t *q = b.data() + l.samples_at, *end = q + l.data; q < end; q += bytes)
            if (!float_sample(raw_at(q, l), l.bits)) return std::nullopt;
    const uint8_t* p = b.data() + l.samples_at;
    return encode_frames(l.channels, coded_bits(l), l.rate, l.data / (l.channels * bytes), effort, subset, [&](FLAC__int32* out, size_t n) {
        for (size_t k = 0; k < n; ++k, p += bytes) out[k] = l.flt ? *float_sample(raw_at(p, l), l.bits) : sample_at(p, l);
    });
}

// A whole FLAC file decoded: its channels, bits, rate and samples (interleaved), where libFLAC reads
// every byte of it as one stream, its MD5 right where it has one.
struct Decoded
{
    uint32_t channels = 0, bits = 0, rate = 0;
    uint64_t total = 0;
    std::vector<FLAC__int32> samples;
    bool bad = false, info = false;
    const uint8_t* p = nullptr;
    size_t n = 0, at = 0;
};

FLAC__StreamDecoderReadStatus whole_read(const FLAC__StreamDecoder*, FLAC__byte buf[], size_t* n, void* c)
{
    auto* d = static_cast<Decoded*>(c);
    const size_t k = std::min(*n, d->n - d->at);
    std::memcpy(buf, d->p + d->at, k);
    d->at += k;
    *n = k;
    return k ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}
void whole_metadata(const FLAC__StreamDecoder*, const FLAC__StreamMetadata* m, void* c)
{
    auto* d = static_cast<Decoded*>(c);
    if (m->type != FLAC__METADATA_TYPE_STREAMINFO) return;
    d->channels = m->data.stream_info.channels;
    d->bits = m->data.stream_info.bits_per_sample;
    d->rate = m->data.stream_info.sample_rate;
    d->total = m->data.stream_info.total_samples;
    d->info = true;
}
FLAC__StreamDecoderWriteStatus whole_write(const FLAC__StreamDecoder*, const FLAC__Frame* f, const FLAC__int32* const ch[], void* c)
{
    auto* d = static_cast<Decoded*>(c);
    if (!d->info || f->header.channels != d->channels || f->header.bits_per_sample != d->bits || d->samples.size() / d->channels + f->header.blocksize > d->total)
    {
        d->bad = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    for (uint32_t k = 0; k < f->header.blocksize; ++k)
        for (uint32_t j = 0; j < d->channels; ++j) d->samples.push_back(ch[j][k]);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}
void whole_error(const FLAC__StreamDecoder*, FLAC__StreamDecoderErrorStatus, void* c) { static_cast<Decoded*>(c)->bad = true; }

std::optional<Decoded> decode_whole(const std::vector<uint8_t>& f)
{
    auto d = std::make_optional<Decoded>();
    d->p = f.data();
    d->n = f.size();
    FLAC__StreamDecoder* dec = FLAC__stream_decoder_new();
    if (!dec) return std::nullopt;
    FLAC__stream_decoder_set_md5_checking(dec, true);
    bool whole = FLAC__stream_decoder_init_stream(dec, whole_read, nullptr, nullptr, nullptr, nullptr, whole_write, whole_metadata, whole_error, &*d) ==
                 FLAC__STREAM_DECODER_INIT_STATUS_OK;
    whole = whole && FLAC__stream_decoder_process_until_end_of_stream(dec) && FLAC__stream_decoder_get_state(dec) == FLAC__STREAM_DECODER_END_OF_STREAM;
    whole = FLAC__stream_decoder_finish(dec) && whole;
    FLAC__stream_decoder_delete(dec);
    if (!whole || d->bad || !d->info || d->total == 0 || d->samples.size() != d->total * d->channels || d->at != d->n) return std::nullopt;
    return d;
}

// A FLAC file's metadata blocks (each with its four-byte head) and where its frames start; nothing
// where it is not "fLaC" and blocks to a last one.
std::optional<std::pair<std::vector<std::vector<uint8_t>>, size_t>> flac_blocks(const std::vector<uint8_t>& f)
{
    if (f.size() < 8 || std::memcmp(f.data(), "fLaC", 4) != 0) return std::nullopt;
    std::vector<std::vector<uint8_t>> blocks;
    size_t at = 4;
    for (bool last = false; !last;)
    {
        if (at + 4 > f.size()) return std::nullopt;
        last = f[at] & 0x80;
        const size_t len = size_t(f[at + 1]) << 16 | size_t(f[at + 2]) << 8 | f[at + 3];
        if ((f[at] & 0x7F) == 127 || at + 4 + len > f.size()) return std::nullopt;
        blocks.emplace_back(f.begin() + std::ptrdiff_t(at), f.begin() + std::ptrdiff_t(at + 4 + len));
        at += 4 + len;
    }
    return std::make_pair(std::move(blocks), at);
}

// b's layout, where its samples are whole blocks within their chunk and the file.
std::optional<Layout> layout_of(const std::vector<uint8_t>& b)
{
    const auto l = walk(b.data(), b.size());
    if (!l) return std::nullopt;
    const uint32_t block = l->channels * (l->bits / 8);
    if (l->data == 0 || l->data % block != 0 || l->samples_at + l->data > std::min<uint64_t>(b.size(), l->chunk_end)) return std::nullopt;
    return l;
}

// The skeleton: b without its samples.
std::vector<uint8_t> skeleton_of(const std::vector<uint8_t>& b, const Layout& l)
{
    std::vector<uint8_t> s(b.begin(), b.begin() + std::ptrdiff_t(l.samples_at));
    s.insert(s.end(), b.begin() + std::ptrdiff_t(l.samples_at + l.data), b.end());
    return s;
}

// Whether the FLAC stream and the skeleton give back b exactly.
bool gives_back(std::vector<uint8_t> inner, const std::vector<uint8_t>& skeleton, const std::vector<uint8_t>& b)
{
    inner.insert(inner.end(), skeleton.begin(), skeleton.end());
    try
    {
        return wav_remake(inner, skeleton.size(), [](size_t) { return false; }) == b;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

} // namespace

std::optional<WavOpened> wav_open(const std::vector<uint8_t>& b, int effort)
{
    const auto l = layout_of(b);
    if (!l) return std::nullopt;
    auto flac = encode(b, *l, effort, false);
    if (!flac) return std::nullopt;
    const std::vector<uint8_t> skeleton = skeleton_of(b, *l);
    // Made again and compared, before it is ever chosen.
    if (!gives_back(*flac, skeleton, b)) return std::nullopt;
    WavOpened o;
    o.samples = l->data / (l->channels * (l->bits / 8));
    o.inner = std::move(*flac);
    o.inner.insert(o.inner.end(), skeleton.begin(), skeleton.end());
    o.recipe = "flac " + std::to_string(skeleton.size()) + " " + std::to_string(o.inner.size());
    return o;
}

bool is_flac(const std::vector<uint8_t>& b) { return b.size() >= 8 && std::memcmp(b.data(), "fLaC", 4) == 0; }

std::optional<std::vector<uint8_t>> flac_recoded(const std::vector<uint8_t>& b, int effort)
{
    const auto old_blocks = flac_blocks(b);
    if (!old_blocks || old_blocks->first.empty() || (old_blocks->first[0][0] & 0x7F) != 0) return std::nullopt;
    const auto d = decode_whole(b);
    // (in FLAC's streamable subset, as players expect: at most 24 bits, a rate the subset holds)
    if (!d || d->bits > 24 || d->bits < 4 || !FLAC__format_sample_rate_is_subset(d->rate)) return std::nullopt;
    size_t next = 0;
    auto coded = encode_frames(d->channels, d->bits, d->rate, d->total, effort, true, [&](FLAC__int32* out, size_t n) {
        std::copy(d->samples.begin() + std::ptrdiff_t(next), d->samples.begin() + std::ptrdiff_t(next + n), out);
        next += n;
    });
    if (!coded) return std::nullopt;
    const auto new_blocks = flac_blocks(*coded);
    if (!new_blocks || new_blocks->first.empty()) return std::nullopt;
    // The new STREAMINFO, then the file's own blocks but its padding and seek table (its seek points
    // would point into the old frames), then the new frames.
    std::vector<std::vector<uint8_t>> keep{new_blocks->first[0]};
    for (size_t k = 1; k < old_blocks->first.size(); ++k)
    {
        const int type = old_blocks->first[k][0] & 0x7F;
        if (type != 1 && type != 3) keep.push_back(old_blocks->first[k]);
    }
    std::vector<uint8_t> out{'f', 'L', 'a', 'C'};
    for (size_t k = 0; k < keep.size(); ++k)
    {
        keep[k][0] = uint8_t((keep[k][0] & 0x7F) | (k + 1 == keep.size() ? 0x80 : 0));
        out.insert(out.end(), keep[k].begin(), keep[k].end());
    }
    out.insert(out.end(), coded->begin() + std::ptrdiff_t(new_blocks->second), coded->end());
    // Checked: the same samples, from the file as it will be installed.
    const auto back = decode_whole(out);
    if (!back || back->samples != d->samples || back->rate != d->rate) return std::nullopt;
    return out;
}

namespace {

// FLAC's own order of 3 to 8 channels as a WAV's channel mask (RFC 9639, 9.1.3).
constexpr uint32_t kFlacOrder[9] = {0, 0, 0, 0x7, 0x33, 0x37, 0x3F, 0x70F, 0x63F};

// Text as UTF-8: as it is where it is UTF-8 already, else read as Latin-1; its NULs at the end and its
// spaces at either end dropped.
std::string utf8_of(const uint8_t* p, size_t n)
{
    while (n && (p[n - 1] == 0 || p[n - 1] == ' ')) --n;
    while (n && *p == ' ') ++p, --n;
    std::string t(reinterpret_cast<const char*>(p), n);
    bool utf8 = true;
    for (size_t i = 0; i < n && utf8;)
    {
        const uint8_t c = p[i];
        const size_t k = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (!k || c == 0 || i + k > n) utf8 = false;
        for (size_t j = 1; j < k && utf8; ++j) utf8 = (p[i + j] >> 6) == 2;
        i += k ? k : 1;
    }
    if (utf8) return t;
    std::string o;
    for (size_t i = 0; i < n; ++i)
        if (p[i] < 0x80) o += char(p[i]);
        else o += char(0xC0 | (p[i] >> 6)), o += char(0x80 | (p[i] & 0x3F));
    return o;
}

// A WAV's LIST INFO tags, or an AIFF's NAME, AUTH, ANNO and "(c) " chunks, as Vorbis comments
// ("TITLE=...", as FLAC players read them); none where it has none.
std::vector<std::string> sound_tags(const std::vector<uint8_t>& b)
{
    std::vector<std::string> tags;
    const size_t n = b.size();
    auto add = [&](const char* key, const uint8_t* p, size_t k) {
        const std::string v = utf8_of(p, k);
        if (!v.empty()) tags.push_back(std::string(key) + "=" + v);
    };
    const int kind = sound_kind(b.data(), n);
    if (kind == 1 && std::memcmp(b.data(), "RIFF", 4) == 0)
        for (size_t at = 12; at + 8 <= n;)
        {
            const uint8_t* c = b.data() + at;
            const uint64_t size = le32(c + 4);
            if (at + 8 + size > n) break;
            if (std::memcmp(c, "LIST", 4) == 0 && size >= 4 && std::memcmp(c + 8, "INFO", 4) == 0)
                for (size_t s = at + 12; s + 8 <= at + 8 + size;)
                {
                    const uint8_t* t = b.data() + s;
                    const uint64_t k = le32(t + 4);
                    if (s + 8 + k > at + 8 + size) break;
                    static const char* const map[][2] = {{"INAM", "TITLE"},   {"IART", "ARTIST"},   {"IPRD", "ALBUM"}, {"ICMT", "COMMENT"},
                                                         {"ICRD", "DATE"},    {"IGNR", "GENRE"},    {"ITRK", "TRACKNUMBER"},
                                                         {"IPRT", "TRACKNUMBER"}, {"ICOP", "COPYRIGHT"}};
                    for (const auto& m : map)
                        if (std::memcmp(t, m[0], 4) == 0) add(m[1], t + 8, size_t(k));
                    s += 8 + k + (k & 1);
                }
            at += 8 + size + (size & 1);
        }
    else if (kind == 2)
        for (size_t at = 12; at + 8 <= n;)
        {
            const uint8_t* c = b.data() + at;
            const uint64_t size = be32(c + 4);
            if (at + 8 + size > n) break;
            static const char* const map[][2] = {{"NAME", "TITLE"}, {"AUTH", "ARTIST"}, {"ANNO", "COMMENT"}, {"(c) ", "COPYRIGHT"}};
            for (const auto& m : map)
                if (std::memcmp(c, m[0], 4) == 0) add(m[1], c + 8, size_t(size));
            at += 8 + size + (size & 1);
        }
    return tags;
}

// A FLAC file with a Vorbis comment block of these tags after its STREAMINFO.
std::optional<std::vector<uint8_t>> with_tags(const std::vector<uint8_t>& f, const std::vector<std::string>& tags)
{
    const auto blocks = flac_blocks(f);
    if (!blocks || blocks->first.empty()) return std::nullopt;
    auto le = [](std::vector<uint8_t>& o, uint32_t v) {
        for (int k = 0; k < 4; ++k) o.push_back(uint8_t(v >> (8 * k)));
    };
    std::vector<uint8_t> body;
    const std::string vendor = "Sieve";
    le(body, uint32_t(vendor.size()));
    body.insert(body.end(), vendor.begin(), vendor.end());
    le(body, uint32_t(tags.size()));
    for (const std::string& t : tags)
    {
        le(body, uint32_t(t.size()));
        body.insert(body.end(), t.begin(), t.end());
    }
    if (body.size() >= (1u << 24)) return std::nullopt;
    std::vector<std::vector<uint8_t>> keep{blocks->first[0]};
    std::vector<uint8_t> vc{4, uint8_t(body.size() >> 16), uint8_t(body.size() >> 8), uint8_t(body.size())};
    vc.insert(vc.end(), body.begin(), body.end());
    keep.push_back(std::move(vc));
    for (size_t k = 1; k < blocks->first.size(); ++k)
        if ((blocks->first[k][0] & 0x7F) != 4) keep.push_back(blocks->first[k]);
    std::vector<uint8_t> out{'f', 'L', 'a', 'C'};
    for (size_t k = 0; k < keep.size(); ++k)
    {
        keep[k][0] = uint8_t((keep[k][0] & 0x7F) | (k + 1 == keep.size() ? 0x80 : 0));
        out.insert(out.end(), keep[k].begin(), keep[k].end());
    }
    out.insert(out.end(), f.begin() + std::ptrdiff_t(blocks->second), f.end());
    return out;
}

} // namespace

std::optional<std::vector<uint8_t>> sound_as_flac(const std::vector<uint8_t>& b, int effort)
{
    const auto l = layout_of(b);
    // (a .flac file any player opens: FLAC's streamable subset, its own rate, at most 24 bits; one or
    // two channels, or 3 to 8 where the file's channel mask is FLAC's own order for so many)
    if (!l || !l->rate_true || !FLAC__format_sample_rate_is_subset(l->rate) || coded_bits(*l) > 24) return std::nullopt;
    if (l->channels > 2 && l->mask != kFlacOrder[l->channels]) return std::nullopt;
    auto flac = encode(b, *l, effort, true);
    if (!flac || !gives_back(*flac, skeleton_of(b, *l), b)) return std::nullopt;
    // (its tags kept, as Vorbis comments: 8 November 2026)
    if (const std::vector<std::string> tags = sound_tags(b); !tags.empty())
    {
        auto tagged = with_tags(*flac, tags);
        if (!tagged || decode_whole(*tagged).has_value() == false) return std::nullopt;
        flac = std::move(tagged);
    }
    return flac;
}

} // namespace sieve::cli
