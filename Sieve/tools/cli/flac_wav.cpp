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

// What a sound file's chunks say, up to its samples.
struct Layout
{
    uint32_t channels = 0, bits = 0, rate = 0;
    size_t samples_at = 0;  // where its samples start
    uint64_t data = 0;      // their size, in bytes
    uint64_t chunk_end = 0; // where the chunk holding them ends (the file's own, not the skeleton's)
    bool big = false;       // big-endian samples (AIFF's)
    bool unsigned8 = false; // 8-bit samples unsigned (WAV's)
};

// A WAV's chunks walked from byte 12 to the data chunk: one "fmt " before it, plain integer PCM;
// every chunk before it whole within the n bytes. Nothing if the file is not so.
std::optional<Layout> walk_wav(const uint8_t* p, size_t n)
{
    Layout l;
    l.unsigned8 = true;
    bool fmt = false;
    for (size_t at = 12; at + 8 <= n;)
    {
        const uint8_t* c = p + at;
        const uint64_t size = le32(c + 4);
        if (std::memcmp(c, "data", 4) == 0)
        {
            if (!fmt) return std::nullopt;
            l.samples_at = at + 8;
            l.data = size;
            l.chunk_end = at + 8 + size;
            return l;
        }
        if (std::memcmp(c, "fmt ", 4) == 0)
        {
            if (fmt || size < 16 || at + 8 + size > n) return std::nullopt;
            fmt = true;
            const uint32_t tag = le16(c + 8);
            l.channels = le16(c + 10);
            l.rate = le32(c + 12);
            const uint32_t align = le16(c + 20);
            l.bits = le16(c + 22);
            if (tag == 0xFFFE)
            {
                // WAVE_FORMAT_EXTENSIBLE: its sub-format must be PCM's GUID.
                static const uint8_t pcm[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
                if (size < 40 || le16(c + 24) < 22 || std::memcmp(c + 32, pcm, 16) != 0) return std::nullopt;
            }
            else if (tag != 1)
                return std::nullopt;
            if (l.channels < 1 || l.channels > 8 || (l.bits != 8 && l.bits != 16 && l.bits != 24 && l.bits != 32) ||
                align != l.channels * (l.bits / 8) || !FLAC__format_sample_rate_is_valid(l.rate))
                return std::nullopt;
        }
        const uint64_t next = uint64_t(at) + 8 + size + (size & 1);
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

// An AIFF's (or AIFF-C's) chunks walked from byte 12 to its SSND chunk: one COMM before it, 1 to 8
// channels, 8, 16, 24 or 32 bits a sample, at least one sample frame, AIFF-C's samples not
// compressed ("NONE" or "twos", big-endian; "sowt", little-endian); every chunk before it whole within
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
            if (!FLAC__format_sample_rate_is_valid(l.rate)) l.rate = 44100;
            if (aifc)
            {
                if (std::memcmp(c + 26, "sowt", 4) == 0) l.big = false;
                else if (std::memcmp(c + 26, "NONE", 4) != 0 && std::memcmp(c + 26, "twos", 4) != 0) return std::nullopt;
            }
            if (l.channels < 1 || l.channels > 8 || (l.bits != 8 && l.bits != 16 && l.bits != 24 && l.bits != 32) || frames == 0) return std::nullopt;
        }
        const uint64_t next = uint64_t(at) + 8 + size + (size & 1);
        if (next > n) return std::nullopt;
        at = size_t(next);
    }
    return std::nullopt;
}

bool is_aiff_head(const uint8_t* p, size_t n)
{
    return n >= 12 && std::memcmp(p, "FORM", 4) == 0 && (std::memcmp(p + 8, "AIFF", 4) == 0 || std::memcmp(p + 8, "AIFC", 4) == 0);
}

std::optional<Layout> walk(const uint8_t* p, size_t n)
{
    if (n >= 12 && std::memcmp(p, "RIFF", 4) == 0 && std::memcmp(p + 8, "WAVE", 4) == 0) return walk_wav(p, n);
    if (is_aiff_head(p, n)) return walk_aiff(p, n);
    return std::nullopt;
}

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
    const uint32_t u = bytes == 1 && l.unsigned8 ? uint32_t(v + 128) : uint32_t(v);
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
    if (s->info || i.channels != s->l.channels || i.bits_per_sample != s->l.bits || i.total_samples * block != s->l.data) s->bad = true;
    s->info = true;
}
FLAC__StreamDecoderWriteStatus source_write(const FLAC__StreamDecoder*, const FLAC__Frame* f, const FLAC__int32* const ch[], void* c)
{
    auto* s = static_cast<Source*>(c);
    const uint32_t bytes = s->l.bits / 8;
    const uint64_t frame = uint64_t(f->header.blocksize) * s->l.channels * bytes;
    if (!s->info || s->bad || f->header.channels != s->l.channels || f->header.bits_per_sample != s->l.bits || s->written + frame > s->l.data ||
        (*s->too_big)(s->out->size() + size_t(frame)))
    {
        s->bad = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    // (each sample within its bits, as the specification and the oracle hold it)
    const int64_t lo = -(int64_t(1) << (s->l.bits - 1)), hi = (int64_t(1) << (s->l.bits - 1)) - 1;
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

std::vector<uint8_t> wav_remake(const std::vector<uint8_t>& inner, size_t skeleton, const std::function<bool(size_t)>& too_big)
{
    if (skeleton > inner.size()) throw std::runtime_error("a sound file's skeleton is larger than what is carried");
    const size_t flac = inner.size() - skeleton;
    const uint8_t* sk = inner.data() + flac;
    const auto l = walk(sk, skeleton);
    if (!l || l->data == 0 || l->data % (uint64_t(l->channels) * (l->bits / 8)) != 0)
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
    if (!whole || s.bad || !s.info || s.written != l->data || s.at != flac) throw std::runtime_error("a sound file's FLAC stream that does not give back its samples");
    out.insert(out.end(), sk + l->samples_at, sk + skeleton);
    return out;
}

std::optional<WavOpened> wav_open(const std::vector<uint8_t>& b, int effort)
{
    const auto l = walk(b.data(), b.size());
    if (!l) return std::nullopt;
    const uint32_t bytes = l->bits / 8, block = l->channels * bytes;
    if (l->data == 0 || l->data % block != 0 || l->samples_at + l->data > std::min<uint64_t>(b.size(), l->chunk_end)) return std::nullopt;
    const uint64_t frames = l->data / block;

    FLAC__StreamEncoder* e = FLAC__stream_encoder_new();
    if (!e) return std::nullopt;
    bool ok = FLAC__stream_encoder_set_channels(e, l->channels) && FLAC__stream_encoder_set_bits_per_sample(e, l->bits) &&
              FLAC__stream_encoder_set_sample_rate(e, l->rate) && FLAC__stream_encoder_set_streamable_subset(e, false) &&
              FLAC__stream_encoder_set_compression_level(e, effort == 0 ? 5 : 8) && FLAC__stream_encoder_set_total_samples_estimate(e, frames);
    if (effort >= 2) ok = ok && FLAC__stream_encoder_set_max_lpc_order(e, 32) && FLAC__stream_encoder_set_do_qlp_coeff_prec_search(e, true);
    Sink sink;
    ok = ok && FLAC__stream_encoder_init_stream(e, sink_write, sink_seek, sink_tell, nullptr, &sink) == FLAC__STREAM_ENCODER_INIT_STATUS_OK;
    std::vector<FLAC__int32> buf;
    const uint8_t* p = b.data() + l->samples_at;
    for (uint64_t at = 0; ok && at < frames;)
    {
        const uint64_t n = std::min<uint64_t>(frames - at, 1 << 16);
        buf.resize(size_t(n) * l->channels);
        for (size_t k = 0; k < buf.size(); ++k, p += bytes)
            buf[k] = sample_at(p, *l);
        ok = FLAC__stream_encoder_process_interleaved(e, buf.data(), uint32_t(n));
        at += n;
    }
    ok = FLAC__stream_encoder_finish(e) && ok;
    FLAC__stream_encoder_delete(e);
    if (!ok) return std::nullopt;

    WavOpened o;
    o.samples = frames;
    o.inner = std::move(sink.b);
    const size_t skeleton = b.size() - size_t(l->data);
    o.inner.insert(o.inner.end(), b.begin(), b.begin() + std::ptrdiff_t(l->samples_at));
    o.inner.insert(o.inner.end(), b.begin() + std::ptrdiff_t(l->samples_at + l->data), b.end());
    o.recipe = "flac " + std::to_string(skeleton) + " " + std::to_string(o.inner.size());
    // Made again and compared, before it is ever chosen.
    try
    {
        if (wav_remake(o.inner, skeleton, [](size_t) { return false; }) != b) return std::nullopt;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
    return o;
}

} // namespace sieve::cli
