#include "sieve/sound.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace sieve {

namespace {

// a / b rounded towards minus infinity, b > 0.
int64_t floor_div(int64_t a, int64_t b)
{
    const int64_t q = a / b;
    return (a % b != 0 && a < 0) ? q - 1 : q;
}

// a / b rounded half up, b > 0: floor((a + floor(b / 2)) / b), which rounds an exact half up for
// even b and is exact for odd b (which has no exact half).
int64_t round_half_up(int64_t a, int64_t b) { return floor_div(a + b / 2, b); }

uint32_t parse_u32(std::string_view s, const char* what)
{
    if (s.empty() || s.size() > 10) throw std::invalid_argument(std::string("bad ") + what + " in a pcm id");
    uint64_t v = 0;
    for (char c : s)
    {
        if (c < '0' || c > '9') throw std::invalid_argument(std::string("bad ") + what + " in a pcm id");
        v = v * 10 + uint64_t(c - '0');
    }
    if (v > 0xFFFFFFFFull) throw std::invalid_argument(std::string("bad ") + what + " in a pcm id");
    return uint32_t(v);
}

uint32_t le(std::span<const uint8_t> b, size_t at, int n)
{
    uint32_t v = 0;
    for (int i = 0; i < n; ++i) v |= uint32_t(b[at + size_t(i)]) << (8 * i);
    return v;
}

void put(std::string& out, uint64_t v, int n)
{
    for (int i = 0; i < n; ++i) out += char((v >> (8 * i)) & 0xFF);
}

// A floating-point sample as signed 32 bits: scaled by 2^31, rounded half up, clamped.
int32_t from_float(double x)
{
    if (!(x == x)) return 0; // NaN
    const double v = std::floor(x * 2147483648.0 + 0.5);
    if (v >= 2147483647.0) return 2147483647;
    if (v <= -2147483648.0) return INT32_MIN;
    return int32_t(v);
}

} // namespace

std::string PcmFormat::id() const
{
    return "pcm/" + std::to_string(rate) + "/" + std::to_string(bits) + "/C" + std::to_string(channels);
}

PcmFormat make_pcm_format(uint32_t rate, uint32_t bits, uint32_t channels)
{
    if (rate == 0 || rate > 0x7FFFFFFFu) throw std::invalid_argument("a sample rate must be from 1 to 2147483647 a second");
    if (bits == 0 || bits > kPcmMaxBits)
        throw std::invalid_argument("a sample must be from 1 to " + std::to_string(kPcmMaxBits) + " bits (a digit is a 32-bit number)");
    if (channels == 0 || channels > kPcmMaxChannels)
        throw std::invalid_argument("channels must be from 1 to " + std::to_string(kPcmMaxChannels) + " (what a WAV file can carry)");
    return PcmFormat{rate, bits, channels};
}

bool is_pcm_symbols(std::string_view id)
{
    if (id.substr(0, 4) != "pcm/") return false; // without parsing: this is asked while drawing
    try
    {
        pcm_format_of(id);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

PcmFormat pcm_format_of(std::string_view id)
{
    // pcm/RATE/BITS/Cn
    if (id.rfind("pcm/", 0) != 0) throw std::invalid_argument("'" + std::string(id) + "' is not a pcm id");
    std::string_view rest = id.substr(4);
    const size_t a = rest.find('/');
    if (a == std::string_view::npos) throw std::invalid_argument("bad pcm id '" + std::string(id) + "'");
    const size_t b = rest.find('/', a + 1);
    if (b == std::string_view::npos || rest.size() < b + 3 || rest[b + 1] != 'C') throw std::invalid_argument("bad pcm id '" + std::string(id) + "'");
    const PcmFormat f = make_pcm_format(parse_u32(rest.substr(0, a), "rate"), parse_u32(rest.substr(a + 1, b - a - 1), "bits"),
                                        parse_u32(rest.substr(b + 2), "channels"));
    if (f.id() != id) throw std::invalid_argument("bad pcm id '" + std::string(id) + "' (written " + f.id() + ")");
    return f;
}

int32_t pcm_sample(const PcmFormat& f, uint32_t digit)
{
    const uint32_t mask = f.base() - 1;
    digit &= mask;
    // Two's complement in f.bits: the top bit is the sign.
    return (digit >> (f.bits - 1)) ? int32_t(int64_t(digit) - int64_t(f.base())) : int32_t(digit);
}

uint32_t pcm_digit(const PcmFormat& f, int32_t sample) { return uint32_t(sample) & (f.base() - 1); }

// ---------------------------------------------------------------- canon-pcm-v1

PcmCanoniser::PcmCanoniser(const PcmFormat& target, uint32_t length, uint32_t source_rate, uint32_t source_channels)
    : f_(target), length_(length), src_rate_(source_rate), src_ch_(source_channels)
{
    if (length == 0) throw std::invalid_argument("a unit must hold at least one sample");
    if (source_rate == 0 || source_channels == 0) throw std::invalid_argument("the sound has no rate or no channels");
    // A target sample spans R_src units of 1/(R_src x R) seconds, a source sample R of them; both
    // divided by their greatest common divisor.
    const uint64_t g = std::gcd(uint64_t(source_rate), uint64_t(f_.rate));
    num_ = source_rate / g; // target span
    den_ = f_.rate / g;     // source span
    out_.resize(f_.channels);
    acc_.assign(f_.channels, 0);
    r_.source_rate = source_rate;
    r_.source_channels = source_channels;
}

// 3. depth: a 32-bit value rounded half up to the set's bits, and clamped.
int32_t PcmCanoniser::depth(int64_t v)
{
    const uint32_t shift = 32 - f_.bits;
    int64_t s = floor_div(v + (int64_t(1) << (shift - 1)), int64_t(1) << shift);
    const int64_t hi = (int64_t(1) << (f_.bits - 1)) - 1, lo = -(int64_t(1) << (f_.bits - 1));
    if (s > hi || s < lo)
    {
        s = std::clamp(s, lo, hi);
        ++r_.clipped;
    }
    return int32_t(s);
}

void PcmCanoniser::add(std::span<const int32_t> in)
{
    if (in.size() % src_ch_ != 0) throw std::invalid_argument("a block of samples must hold whole frames");
    const uint32_t C = f_.channels, S = src_ch_;
    std::vector<int32_t> frame(C);
    for (size_t at = 0; at < in.size(); at += S)
    {
        ++r_.source_frames;
        // 1. channels
        if (C == 1 && S > 1)
        {
            int64_t sum = 0;
            for (uint32_t c = 0; c < S; ++c) sum += in[at + c];
            frame[0] = int32_t(round_half_up(sum, S));
        }
        else
            for (uint32_t c = 0; c < C; ++c) frame[c] = in[at + std::min(c, S - 1)];
        // 2. rate: this source sample covers den_ units, laid over target spans of num_ units.
        uint64_t left = den_;
        while (left > 0)
        {
            const uint64_t w = std::min(left, num_ - filled_);
            for (uint32_t c = 0; c < C; ++c) acc_[c] += int64_t(frame[c]) * int64_t(w);
            filled_ += w;
            left -= w;
            if (filled_ == num_)
            {
                for (uint32_t c = 0; c < C; ++c)
                {
                    out_[c].push_back(depth(round_half_up(acc_[c], int64_t(num_))));
                    acc_[c] = 0;
                }
                filled_ = 0;
            }
        }
    }
}

PcmCanonResult PcmCanoniser::finish()
{
    if (r_.source_frames == 0) throw std::invalid_argument("there is no sound in it");
    const uint32_t C = f_.channels;
    // A last target sample the sound ends inside: the mean of what it covers.
    if (filled_ > 0)
    {
        for (uint32_t c = 0; c < C; ++c) out_[c].push_back(depth(round_half_up(acc_[c], int64_t(filled_))));
        filled_ = 0;
    }
    // 4. units
    const uint64_t n = out_[0].size();
    r_.samples = n;
    const uint64_t runs = std::max<uint64_t>(1, (n + length_ - 1) / length_);
    r_.padding = (runs * length_ - n) * C;
    for (uint64_t k = 0; k < runs; ++k)
    {
        std::vector<uint32_t> unit;
        unit.reserve(size_t(length_) * C);
        for (uint32_t c = 0; c < C; ++c)
            for (uint64_t i = k * length_; i < (k + 1) * length_; ++i) unit.push_back(i < n ? pcm_digit(f_, out_[c][size_t(i)]) : 0u);
        r_.units.push_back(std::move(unit));
    }
    out_.clear();
    return std::move(r_);
}

// ---------------------------------------------------------------- WAV

bool is_wav(std::span<const uint8_t> h)
{
    return h.size() >= 12 && std::memcmp(h.data(), "RIFF", 4) == 0 && std::memcmp(h.data() + 8, "WAVE", 4) == 0;
}

WavFormat wav_format(std::span<const uint8_t> b)
{
    if (b.size() < 16) throw std::invalid_argument("a WAV file's fmt chunk is too short");
    WavFormat f;
    uint32_t tag = le(b, 0, 2);
    f.channels = le(b, 2, 2);
    f.rate = le(b, 4, 4);
    f.align = le(b, 12, 2);
    f.bits = le(b, 14, 2);
    if (tag == 0xFFFE)
    {
        if (b.size() < 40) throw std::invalid_argument("a WAV file's extensible fmt chunk is too short");
        tag = le(b, 24, 2); // the subformat GUID's first two bytes: 1 PCM, 3 floating point
    }
    if (tag == 3) f.is_float = true;
    else if (tag != 1) throw std::invalid_argument("a WAV file of format " + std::to_string(tag) + " (only PCM and floating point are read)");
    if (f.channels == 0 || f.rate == 0) throw std::invalid_argument("a WAV file with no channels or no rate");
    if (f.is_float ? (f.bits != 32 && f.bits != 64) : (f.bits != 8 && f.bits != 16 && f.bits != 24 && f.bits != 32))
        throw std::invalid_argument("a WAV file of " + std::to_string(f.bits) + "-bit samples");
    if (f.align != f.channels * (f.bits / 8)) throw std::invalid_argument("a WAV file whose frames are not whole samples");
    return f;
}

int32_t wav_sample(const WavFormat& f, const uint8_t* p)
{
    const std::span<const uint8_t> b(p, f.bits / 8);
    if (f.is_float)
    {
        if (f.bits == 32)
        {
            const uint32_t u = le(b, 0, 4);
            float x;
            std::memcpy(&x, &u, 4);
            return from_float(x);
        }
        const uint64_t u = uint64_t(le(b, 0, 4)) | (uint64_t(le(b, 4, 4)) << 32);
        double x;
        std::memcpy(&x, &u, 8);
        return from_float(x);
    }
    if (f.bits == 8) return int32_t((int32_t(p[0]) - 128) * 16777216);
    return int32_t(le(b, 0, int(f.bits / 8)) << (32 - f.bits));
}

PcmAudio read_wav(std::span<const uint8_t> b)
{
    if (!is_wav(b)) throw std::invalid_argument("not a WAV file");
    size_t at = 12;
    std::optional<WavFormat> f;
    while (at + 8 <= b.size())
    {
        const std::string id(reinterpret_cast<const char*>(b.data() + at), 4);
        const uint32_t size = le(b, at + 4, 4);
        const size_t body = at + 8;
        if (id == "fmt ") f = wav_format(b.subspan(body, std::min<size_t>(size, b.size() - body)));
        else if (id == "data")
        {
            if (!f) throw std::invalid_argument("a WAV file's data comes before its fmt chunk");
            const size_t end = (size == 0 || size == 0xFFFFFFFFu) ? b.size() : std::min(b.size(), body + size_t(size));
            const size_t frames = (end - body) / f->align;
            PcmAudio a;
            a.rate = f->rate;
            a.channels = f->channels;
            a.samples.resize(frames * f->channels);
            const size_t bytes = f->bits / 8;
            for (size_t i = 0; i < a.samples.size(); ++i) a.samples[i] = wav_sample(*f, b.data() + body + i * bytes);
            return a;
        }
        if (size == 0xFFFFFFFFu) break;
        at = body + size + (size & 1);
    }
    throw std::invalid_argument("a WAV file with no data chunk");
}

std::string pcm_to_wav(const PcmFormat& f, const std::vector<uint32_t>& digits)
{
    if (digits.size() % f.channels != 0) throw std::invalid_argument("a unit's samples are not whole channels");
    const uint32_t bytes = (f.bits + 7) / 8, container = bytes * 8, shift = container - f.bits;
    const uint64_t frames = digits.size() / f.channels;
    const uint64_t data = frames * f.channels * bytes;
    const bool plain = (f.bits == 8 || f.bits == 16 || f.bits == 24) && f.channels <= 2;
    const uint32_t fmt = plain ? 16 : 40;
    if (data + 20 + fmt + 8 > 0xFFFFFFFFull) throw std::invalid_argument("the sound is too long for a WAV file (4 GB)");
    std::string out;
    out.reserve(size_t(data) + 68);
    out += "RIFF";
    put(out, 4 + 8 + fmt + 8 + data, 4);
    out += "WAVEfmt ";
    put(out, fmt, 4);
    put(out, plain ? 1 : 0xFFFE, 2);
    put(out, f.channels, 2);
    put(out, f.rate, 4);
    put(out, uint64_t(f.rate) * f.channels * bytes, 4);
    put(out, f.channels * bytes, 2);
    put(out, container, 2);
    if (!plain)
    {
        put(out, 22, 2);
        put(out, f.bits, 2); // valid bits
        put(out, f.channels == 1 ? 0x4 : f.channels == 2 ? 0x3 : 0, 4);
        static const uint8_t pcm_guid[16] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
        out.append(reinterpret_cast<const char*>(pcm_guid), 16);
    }
    out += "data";
    put(out, data, 4);
    const uint64_t L = frames;
    for (uint64_t i = 0; i < L; ++i)
        for (uint32_t c = 0; c < f.channels; ++c)
        {
            const int64_t s = int64_t(pcm_sample(f, digits[size_t(c * L + i)])) * (int64_t(1) << shift);
            if (bytes == 1) put(out, uint64_t(s + 128), 1);
            else put(out, uint64_t(s), int(bytes));
        }
    return out;
}

std::vector<PcmSpan> pcm_envelope(const PcmFormat& f, const std::vector<uint32_t>& digits, uint32_t channel, uint32_t columns)
{
    const uint64_t L = digits.size() / f.channels;
    std::vector<PcmSpan> out(columns);
    if (L == 0 || columns == 0 || channel >= f.channels) return out;
    for (uint32_t x = 0; x < columns; ++x)
    {
        const uint64_t lo = L * x / columns, hi = std::max(lo + 1, L * (uint64_t(x) + 1) / columns);
        PcmSpan s{INT32_MAX, INT32_MIN};
        for (uint64_t i = lo; i < hi && i < L; ++i)
        {
            const int32_t v = pcm_sample(f, digits[size_t(channel * L + i)]);
            s.lo = std::min(s.lo, v);
            s.hi = std::max(s.hi, v);
        }
        out[x] = s;
    }
    return out;
}

std::vector<int16_t> pcm_to_s16(const PcmFormat& f, const std::vector<uint32_t>& digits)
{
    const uint64_t L = digits.size() / f.channels;
    std::vector<int16_t> out(size_t(L) * f.channels);
    for (uint64_t i = 0; i < L; ++i)
        for (uint32_t c = 0; c < f.channels; ++c)
        {
            const int64_t s = pcm_sample(f, digits[size_t(c * L + i)]);
            out[size_t(i * f.channels + c)] = int16_t(f.bits >= 16 ? s >> (f.bits - 16) : s * (int64_t(1) << (16 - f.bits)));
        }
    return out;
}

} // namespace sieve
