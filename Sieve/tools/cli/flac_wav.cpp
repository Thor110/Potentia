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

// What a WAV's chunks say, up to its data chunk.
struct Layout
{
    uint32_t channels = 0, bits = 0, rate = 0;
    size_t samples_at = 0; // just after the data chunk's header
    uint64_t data = 0;     // the data chunk's size, in bytes
};

// The chunks walked from byte 12 to the data chunk, as wav_open and wav_remake both walk them: one
// "fmt " before it, plain integer PCM; every chunk before it whole within the n bytes. Nothing if
// the file is not so.
std::optional<Layout> walk(const uint8_t* p, size_t n)
{
    if (n < 12 || std::memcmp(p, "RIFF", 4) != 0 || std::memcmp(p + 8, "WAVE", 4) != 0) return std::nullopt;
    Layout l;
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

int32_t sample_at(const uint8_t* p, uint32_t bytes)
{
    switch (bytes)
    {
    case 1: return int32_t(p[0]) - 128; // (8-bit WAV samples are unsigned)
    case 2: return int32_t(int16_t(le16(p)));
    case 3: return int32_t((uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16) << 8) >> 8;
    default: return int32_t(le32(p));
    }
}

void put_sample(std::vector<uint8_t>& o, int32_t v, uint32_t bytes)
{
    const uint32_t u = bytes == 1 ? uint32_t(v + 128) : uint32_t(v);
    for (uint32_t k = 0; k < bytes; ++k) o.push_back(uint8_t(u >> (8 * k)));
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
            put_sample(*s->out, ch[j][k], bytes);
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

std::vector<uint8_t> wav_remake(const std::vector<uint8_t>& inner, size_t skeleton, const std::function<bool(size_t)>& too_big)
{
    if (skeleton > inner.size()) throw std::runtime_error("a WAV's skeleton is larger than what is carried");
    const size_t flac = inner.size() - skeleton;
    const uint8_t* sk = inner.data() + flac;
    const auto l = walk(sk, skeleton);
    if (!l || l->data == 0 || l->data % (uint64_t(l->channels) * (l->bits / 8)) != 0)
        throw std::runtime_error("a WAV's skeleton this Sieve does not write");
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
    if (!whole || s.bad || !s.info || s.written != l->data || s.at != flac) throw std::runtime_error("a WAV's FLAC stream that does not give back its samples");
    out.insert(out.end(), sk + l->samples_at, sk + skeleton);
    return out;
}

std::optional<WavOpened> wav_open(const std::vector<uint8_t>& b, int effort)
{
    const auto l = walk(b.data(), b.size());
    if (!l) return std::nullopt;
    const uint32_t bytes = l->bits / 8, block = l->channels * bytes;
    if (l->data == 0 || l->data % block != 0 || l->samples_at + l->data > b.size()) return std::nullopt;
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
            buf[k] = sample_at(p, bytes);
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
