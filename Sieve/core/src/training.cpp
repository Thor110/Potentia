// Sieve — training a model of the AI line in pinned integer arithmetic (see training.hpp).
#include "sieve/training.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64)
#define SIEVE_TRAIN_X86 1
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace sieve::training {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("training-v1: " + what); }

} // namespace

// ---------------------------------------------------------------- fixed point

namespace fx {

namespace {

// 2^f = e^(f ln 2) = sum of (f ln 2)^k / k!: the coefficients (ln 2)^k / k! in Q30, k = 0 .. 7.
constexpr int64_t kPow2[8] = {1073741824, 744261118, 257941248, 59597083, 10327387, 1431680, 165394, 16377};
constexpr int64_t kLog2E = 1549082005;          // log2(e) in Q30
constexpr int64_t kTwoPi = 26986075409;         // 2 pi in Q32
constexpr int64_t kHalfPi = 6746518852;         // pi / 2 in Q32
constexpr int64_t kSin[6] = {268435456, -44739243, 2236962, -53261, 740, -7}; // (-1)^k / (2k+1)!, Q28
constexpr int64_t kCos[7] = {268435456, -134217728, 11184811, -372827, 6658, -74, 1}; // (-1)^k / (2k)!, Q28

} // namespace

int64_t fdiv(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if ((a % b != 0) && (a < 0)) --q;
    return q;
}

uint64_t isqrt(uint64_t n)
{
    uint64_t r = uint64_t(std::sqrt(double(n)));
    // The estimate is within a step or two; settle it exactly.
    while (r > 0 && (r > 0xFFFFFFFFull || r * r > n)) --r;
    while (r + 1 <= 0xFFFFFFFFull && (r + 1) * (r + 1) <= n) ++r;
    return r;
}

int64_t pow2_frac(int64_t f)
{
    int64_t acc = kPow2[7];
    for (int k = 6; k >= 0; --k) acc = ((acc * f) >> 30) + kPow2[k];
    return acc;
}

int64_t exp_q16(int64_t z)
{
    const int64_t y = (z * kLog2E) >> 16; // z log2(e), Q30
    const int64_t ip = y >> 30, frac = y - (ip << 30);
    const int64_t shift = 14 - ip;
    return shift >= 62 ? 0 : pow2_frac(frac) >> shift;
}

int64_t sigmoid_q16(int64_t x)
{
    if (x >= 0) return (int64_t(1) << 32) / (kOne + exp_q16(-x));
    const int64_t e = exp_q16(x);
    return (e << 16) / (kOne + e);
}

int64_t pow2_q32(int64_t y)
{
    const int64_t ip = y >> 32, frac = y - (ip << 32);
    const int64_t shift = -ip;
    return shift >= 62 ? 0 : (pow2_frac(frac >> 2) << 2) >> shift;
}

void sincos_q16(int64_t angle, int64_t& s, int64_t& c)
{
    const int64_t a = angle % kTwoPi;
    const int64_t k = a / kHalfPi;
    const int64_t t = (a - k * kHalfPi) >> 4; // Q28, in [0, pi / 2]
    const int64_t u = (t * t) >> 28;
    int64_t sa = kSin[5];
    for (int j = 4; j >= 0; --j) sa = ((sa * u) >> 28) + kSin[j];
    const int64_t s0 = (t * sa) >> 28;
    int64_t c0 = kCos[6];
    for (int j = 5; j >= 0; --j) c0 = ((c0 * u) >> 28) + kCos[j];
    int64_t ss, cc;
    switch (k & 3)
    {
    case 0: ss = s0, cc = c0; break;
    case 1: ss = c0, cc = -s0; break;
    case 2: ss = -s0, cc = -c0; break;
    default: ss = -c0, cc = s0; break;
    }
    s = ss >> 12;
    c = cc >> 12;
}

int64_t mul_shift(int64_t a, int64_t b, unsigned s)
{
    // The product's 128 bits from four 32-bit halves, shifted down, rounded towards minus
    // infinity as every shift here is.
    const bool neg = (a < 0) != (b < 0);
    const uint64_t x = a < 0 ? 0 - uint64_t(a) : uint64_t(a), y = b < 0 ? 0 - uint64_t(b) : uint64_t(b);
    const uint64_t x0 = x & 0xFFFFFFFFull, x1 = x >> 32, y0 = y & 0xFFFFFFFFull, y1 = y >> 32;
    const uint64_t p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, p11 = x1 * y1;
    const uint64_t mid = (p00 >> 32) + (p01 & 0xFFFFFFFFull) + (p10 & 0xFFFFFFFFull);
    const uint64_t lo = (p00 & 0xFFFFFFFFull) | (mid << 32);
    const uint64_t hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    uint64_t q;
    bool rest;
    if (s == 0) q = lo, rest = false;
    else if (s < 64) q = (lo >> s) | (hi << (64 - s)), rest = (lo << (64 - s)) != 0;
    else q = hi >> (s - 64), rest = lo != 0 || (s > 64 && (hi << (128 - s)) != 0);
    return neg ? -int64_t(q) - (rest ? 1 : 0) : int64_t(q);
}

int64_t scale_q32(uint32_t cols, bool embedding)
{
    if (embedding) return int64_t(1) << 32;
    return int64_t(isqrt((uint64_t(3) << 60) / cols)) << 2; // sqrt(3 / cols)
}

} // namespace fx

using namespace fx;

namespace {

constexpr int64_t kEps = 42950;                 // 1e-5 in Q32 (rms_norm_eps)
constexpr int64_t kLog2Theta = 57070290109;     // log2(10000) in Q32 (rope_theta)
constexpr int64_t kGradLimit = int64_t(1) << 43;
constexpr int64_t kVelocityLimit = int64_t(1) << 46;

inline int64_t sat(int64_t v) { return std::clamp(v, -kLimit, kLimit); }

// Decimal text (digits, an optional point and digits) to round(x * 2^bits), halves up.
int64_t decimal_q(const std::string& s, const char* what, unsigned bits)
{
    uint64_t num = 0, den = 1;
    bool point = false, any = false;
    for (const char ch : s)
    {
        if (ch == '.' && !point) { point = true; continue; }
        if (ch < '0' || ch > '9') bad(std::string(what) + " is a decimal, not \"" + s + "\"");
        if (num > 100000000000ull) bad(std::string(what) + " has too many digits");
        num = num * 10 + uint64_t(ch - '0');
        if (point) den *= 10;
        any = true;
    }
    if (!any) bad(std::string(what) + " is a decimal, not \"" + s + "\"");
    return int64_t((num * (uint64_t(2) << bits) + den) / (2 * den));
}

} // namespace

// ---------------------------------------------------------------- recipe, order, result

int64_t Recipe::learning_rate_q16() const { return decimal_q(learning_rate, "learning-rate", 16); }
int64_t Recipe::learning_rate_q() const { return decimal_q(learning_rate, "learning-rate", 16 + loss_scale_bits); }
int64_t Recipe::momentum_q16() const { return decimal_q(momentum, "momentum", 16); }

void Recipe::check() const
{
    shape.check();
    if (shape.width > 4096) bad("a width up to 4096");
    if (version != 1 && version != 2) bad("a run of training-v1 or training-v2");
    if (loss_scale_bits > 7 || (version == 1 && loss_scale_bits != 0)) bad("a loss scale of 0 to 7 bits (training-v2 only)");
    const int64_t lr = learning_rate_q(), mo = momentum_q16();
    if (lr < 1 || learning_rate_q16() > kOne) bad("a learning rate from 2^-" + std::to_string(16 + loss_scale_bits) + " to 1");
    if (mo < 0 || mo >= kOne) bad("a momentum below 1");
    if (batch < 1 || batch > 32) bad("from 1 to 32 windows a step");
    if (checkpoint_every < 1) bad("a checkpoint every step or more");
    if (start.empty() || start.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) bad("a start address in hex");
}

std::string Recipe::text() const
{
    std::string t = "[training]\n";
    t += "version = " + std::string(version == 1 ? kTrainingVersion : kTrainingVersion2) + "\n";
    t += "layers = " + std::to_string(shape.layers) + "\n";
    t += "width = " + std::to_string(shape.width) + "\n";
    t += "heads = " + std::to_string(shape.heads) + "\n";
    t += "bits = " + std::to_string(shape.bits) + "\n";
    t += "key = " + key + "\n";
    t += "start = " + std::string(to_string(start_mode)) + " " + start + "\n";
    t += "learning-rate = " + learning_rate + "\n";
    t += "momentum = " + momentum + "\n";
    if (version >= 2) t += "loss-scale-bits = " + std::to_string(loss_scale_bits) + "\n";
    t += "batch = " + std::to_string(batch) + "\n";
    t += "checkpoint-every = " + std::to_string(checkpoint_every) + "\n";
    if (order.empty()) return t; // the first runs' format
    t += "checkpoint-addresses = " + std::string(checkpoint_addresses ? "yes" : "no") + "\n";
    t += "order = " + order + "\n";
    if (order == "order-v1")
    {
        t += "reading = " + std::string(training::to_string(reading)) + "\n";
        t += "window = " + std::to_string(window) + "\n";
        t += "epochs = " + std::to_string(epochs) + "\n";
        t += "seed = " + std::to_string(seed) + "\n";
        for (const std::string& f : files) t += "file = " + f + "\n";
    }
    return t;
}

const char* to_string(Reading r)
{
    switch (r)
    {
    case Reading::InOrder: return "in-order";
    case Reading::ShuffledWithinFiles: return "shuffled-within-files";
    default: return "shuffled";
    }
}

Reading reading_from_string(std::string_view s)
{
    if (s == "in-order") return Reading::InOrder;
    if (s == "shuffled-within-files") return Reading::ShuffledWithinFiles;
    if (s == "shuffled") return Reading::Shuffled;
    bad("a reading is in-order, shuffled-within-files or shuffled, not \"" + std::string(s) + "\"");
}

namespace {

std::string trim(std::string_view s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return std::string(s.substr(a, b - a));
}

uint64_t number(const std::string& s, const char* what)
{
    if (s.empty() || s.size() > 18 || s.find_first_not_of("0123456789") != std::string::npos)
        bad(std::string(what) + " is a whole number, not \"" + s + "\"");
    return std::stoull(s);
}

std::vector<std::string_view> lines_of(std::string_view text)
{
    std::vector<std::string_view> out;
    size_t at = 0;
    while (at < text.size())
    {
        size_t nl = text.find('\n', at);
        if (nl == std::string_view::npos) nl = text.size();
        out.push_back(text.substr(at, nl - at));
        at = nl + 1;
    }
    return out;
}

std::vector<std::string> split(std::string_view line, char sep)
{
    std::vector<std::string> out;
    size_t at = 0;
    for (;;)
    {
        const size_t p = line.find(sep, at);
        out.emplace_back(line.substr(at, p == std::string_view::npos ? std::string_view::npos : p - at));
        if (p == std::string_view::npos) return out;
        at = p + 1;
    }
}

} // namespace

Recipe Recipe::parse(std::string_view text)
{
    Recipe r;
    r.order.clear(); // no order line: the first runs' format
    bool version = false;
    for (std::string_view raw : lines_of(text))
    {
        const std::string line = trim(raw);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line == "[training]") continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) bad("a line of training.ini is \"key = value\", not \"" + line + "\"");
        const std::string k = trim(std::string_view(line).substr(0, eq)), v = trim(std::string_view(line).substr(eq + 1));
        if (k == "version")
        {
            if (v == kTrainingVersion) r.version = 1;
            else if (v == kTrainingVersion2) r.version = 2;
            else bad("this is training-v1 and training-v2, and the run asks for " + v);
            version = true;
        }
        else if (k == "layers") r.shape.layers = uint32_t(number(v, "layers"));
        else if (k == "width") r.shape.width = uint32_t(number(v, "width"));
        else if (k == "heads") r.shape.heads = uint32_t(number(v, "heads"));
        else if (k == "bits") r.shape.bits = uint32_t(number(v, "bits"));
        else if (k == "key") r.key = v;
        else if (k == "start")
        {
            const size_t sp = v.find(' ');
            if (sp == std::string::npos) bad("start is \"positional ADDRESS\" or \"scrambled ADDRESS\"");
            r.start_mode = address_mode_from_string(v.substr(0, sp));
            r.start = trim(std::string_view(v).substr(sp + 1));
        }
        else if (k == "learning-rate") r.learning_rate = v;
        else if (k == "momentum") r.momentum = v;
        else if (k == "loss-scale-bits") r.loss_scale_bits = uint32_t(number(v, "loss-scale-bits"));
        else if (k == "batch") r.batch = uint32_t(number(v, "batch"));
        else if (k == "checkpoint-every") r.checkpoint_every = uint32_t(number(v, "checkpoint-every"));
        else if (k == "checkpoint-addresses")
        {
            if (v != "yes" && v != "no") bad("checkpoint-addresses is yes or no");
            r.checkpoint_addresses = v == "yes";
        }
        else if (k == "order")
        {
            if (v != "order-v1" && v != "order.txt") bad("an order is order-v1 or order.txt, not \"" + v + "\"");
            r.order = v;
        }
        else if (k == "reading") r.reading = reading_from_string(v);
        else if (k == "window") r.window = uint32_t(number(v, "window"));
        else if (k == "epochs") r.epochs = uint32_t(number(v, "epochs"));
        else if (k == "seed") r.seed = number(v, "seed");
        else if (k == "file") r.files.push_back(v);
        else bad("training.ini has no key \"" + k + "\"");
    }
    if (!version) bad("training.ini names no version");
    if (r.order == "order-v1")
    {
        if (r.window < 2 || r.window > 513) bad("a window of 2 to 513 bytes");
        if (r.epochs < 1) bad("an epoch or more");
        if (r.files.empty()) bad("order-v1 names the files it reads (file = ...)");
    }
    r.check();
    return r;
}

std::string order_text(const std::vector<Window>& order)
{
    std::string t = "sieve-order-v1\n";
    for (const Window& w : order) t += w.path + "\t" + std::to_string(w.offset) + "\t" + std::to_string(w.length) + "\n";
    return t;
}

std::vector<Window> parse_order(std::string_view text)
{
    const std::vector<std::string_view> lines = lines_of(text);
    if (lines.empty() || trim(lines[0]) != "sieve-order-v1") bad("an order begins \"sieve-order-v1\"");
    std::vector<Window> out;
    for (size_t i = 1; i < lines.size(); ++i)
    {
        const std::string line = trim(lines[i]);
        if (line.empty()) continue;
        const std::vector<std::string> f = split(line, '\t');
        if (f.size() != 3) bad("an order line is path, offset and length, separated by tabs");
        Window w{f[0], number(f[1], "an offset"), uint32_t(number(f[2], "a length"))};
        if (w.length < 2 || w.length > 513) bad("a window of 2 to 513 bytes");
        out.push_back(std::move(w));
    }
    return out;
}

std::vector<Window> make_order(const std::vector<std::pair<std::string, uint64_t>>& files, uint32_t length, uint32_t epochs,
                               uint64_t seed, Reading reading)
{
    if (length < 2 || length > 513) bad("a window of 2 to 513 bytes");
    std::vector<std::vector<Window>> per_file;
    for (const auto& [path, size] : files)
    {
        per_file.emplace_back();
        for (uint64_t off = 0; off + 1 < size; off += length - 1)
            per_file.back().push_back({path, off, uint32_t(std::min<uint64_t>(length, size - off))});
    }
    std::vector<Window> out;
    for (uint32_t e = 0; e < epochs; ++e)
    {
        uint64_t state = seed + e;
        auto next = [&]() {
            uint64_t z = (state += 0x9E3779B97F4A7C15ull);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return z ^ (z >> 31);
        };
        auto shuffle = [&](std::vector<Window>& w) {
            for (size_t i = w.size(); i > 1; --i) std::swap(w[i - 1], w[size_t(next() % i)]);
        };
        if (reading == Reading::Shuffled)
        {
            std::vector<Window> w;
            for (const auto& f : per_file) w.insert(w.end(), f.begin(), f.end());
            shuffle(w);
            out.insert(out.end(), w.begin(), w.end());
            continue;
        }
        for (const auto& f : per_file)
        {
            std::vector<Window> w = f;
            if (reading == Reading::ShuffledWithinFiles) shuffle(w);
            out.insert(out.end(), w.begin(), w.end());
        }
    }
    return out;
}

double Score::bits_per_byte() const { return predictions ? bits / double(predictions) : 0.0; }

std::string Result::text() const
{
    std::string t = "sieve-training-result-v" + std::to_string(version) + "\nversion " + trainer + "\nsteps " +
                    std::to_string(steps) + "\n";
    for (const Checkpoint& c : checkpoints)
    {
        t += "checkpoint " + std::to_string(c.step) + " " + std::to_string(c.score.prob_sum) + " " + std::to_string(c.score.predictions);
        if (version >= 2) t += " " + c.digest;
        if (version == 1 || !c.address.empty()) t += " " + c.address;
        t += "\n";
    }
    t += "final " + final_address + "\nmodel-sha256 " + model_sha256 + "\n";
    return t;
}

Result Result::parse(std::string_view text)
{
    const std::vector<std::string_view> lines = lines_of(text);
    Result r;
    if (!lines.empty() && trim(lines[0]) == "sieve-training-result-v1") r.version = 1;
    else if (!lines.empty() && trim(lines[0]) == "sieve-training-result-v2") r.version = 2;
    else bad("a result begins \"sieve-training-result-v2\" (or -v1)");
    for (size_t i = 1; i < lines.size(); ++i)
    {
        const std::string line = trim(lines[i]);
        if (line.empty()) continue;
        const std::vector<std::string> f = split(line, ' ');
        if (f[0] == "version" && f.size() == 2)
        {
            if (f[1] != kTrainingVersion && f[1] != kTrainingVersion2) bad("this result is of " + f[1]);
            r.trainer = f[1];
        }
        else if (f[0] == "steps" && f.size() == 2) r.steps = number(f[1], "steps");
        else if (f[0] == "checkpoint" && (r.version == 1 ? f.size() == 5 : (f.size() == 5 || f.size() == 6)))
        {
            Checkpoint c;
            c.step = number(f[1], "a step");
            c.score.prob_sum = number(f[2], "a score");
            c.score.predictions = number(f[3], "a count");
            if (r.version == 1) c.address = f[4];
            else
            {
                c.digest = f[4];
                if (f.size() == 6) c.address = f[5];
            }
            r.checkpoints.push_back(std::move(c));
        }
        else if (f[0] == "final" && f.size() <= 2) r.final_address = f.size() == 2 ? f[1] : ""; // empty in a run's saved state
        else if (f[0] == "model-sha256" && f.size() <= 2) r.model_sha256 = f.size() == 2 ? f[1] : "";
        else bad("a result has no line \"" + line.substr(0, 40) + "\"");
    }
    return r;
}

// ---------------------------------------------------------------- the network

struct Trainer::Net
{
    struct Mat
    {
        size_t first = 0;
        uint32_t rows = 0, cols = 0;
    };
    struct Layer
    {
        Mat q, k, v, o, gate, up, down;
    };
    Mat embed;
    std::vector<Layer> layers;
    uint32_t H = 0, A = 0, D = 0, F = 0;
    int64_t isd = 0;    // 1 / sqrt(D), Q16
    unsigned shift = 0; // training-v2: the loss scaled by 2^shift

    explicit Net(const AiSpace& space)
    {
        const AiShape& s = space.shape();
        H = s.width, A = s.heads, D = s.width / s.heads, F = s.ffn();
        isd = (int64_t(1) << 32) / int64_t(isqrt(uint64_t(D) << 32));
        auto find = [&](const std::string& name) {
            for (const AiSpace::Tensor& t : space.tensors())
                if (t.name == name) return Mat{size_t(t.first), t.rows, t.cols};
            bad("the shape has no tensor " + name);
        };
        embed = find("model.embed_tokens.weight");
        for (uint32_t l = 0; l < s.layers; ++l)
        {
            const std::string p = "model.layers." + std::to_string(l) + ".";
            layers.push_back({find(p + "self_attn.q_proj.weight"), find(p + "self_attn.k_proj.weight"), find(p + "self_attn.v_proj.weight"),
                              find(p + "self_attn.o_proj.weight"), find(p + "mlp.gate_proj.weight"), find(p + "mlp.up_proj.weight"),
                              find(p + "mlp.down_proj.weight")});
        }
    }
};

// ---------------------------------------------------------------- the inner loops

// The two loops nearly all the time goes to: a dot product of 32-bit values summed in 64 bits, and
// a 64-bit row adding a 32-bit row times a number. Plain C++ everywhere; on processors with AVX2
// (chosen when the program starts, as SHA-256's SHA-NI path is), the same integer sums four and
// eight at a time. Integer sums: both give the same result, which the tests check.
namespace {

int64_t dot_plain(const int32_t* a, const int32_t* b, size_t n)
{
    int64_t s = 0;
    for (size_t i = 0; i < n; ++i) s += int64_t(a[i]) * b[i];
    return s;
}

void axpy_plain(int64_t* acc, const int32_t* x, int32_t d, size_t n)
{
    for (size_t i = 0; i < n; ++i) acc[i] += int64_t(x[i]) * d;
}

#if SIEVE_TRAIN_X86

bool cpu_has_avx2()
{
#if defined(_MSC_VER)
    int r[4];
    __cpuid(r, 0);
    if (r[0] < 7) return false;
    __cpuid(r, 1);
    const bool osxsave = ((r[2] >> 27) & 1) != 0, avx = ((r[2] >> 28) & 1) != 0;
    if (!osxsave || !avx || (_xgetbv(0) & 6) != 6) return false; // the system saves the wide registers
    __cpuidex(r, 7, 0);
    return ((r[1] >> 5) & 1) != 0;
#else
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid_max(0, nullptr) < 7) return false;
    __get_cpuid(1, &a, &b, &c, &d);
    const bool osxsave = ((c >> 27) & 1) != 0, avx = ((c >> 28) & 1) != 0;
    if (!osxsave || !avx) return false;
    unsigned lo = 0, hi = 0;
    __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0)); // the system saves the wide registers
    if ((lo & 6) != 6) return false;
    __get_cpuid_count(7, 0, &a, &b, &c, &d);
    return ((b >> 5) & 1) != 0;
#endif
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
int64_t dot_avx2(const int32_t* a, const int32_t* b, size_t n)
{
    // mul_epi32 multiplies the low 32 bits of each 64-bit lane, signed: the even elements, and,
    // shifted down, the odd ones.
    __m256i even = _mm256_setzero_si256(), odd = _mm256_setzero_si256();
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        const __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        const __m256i y = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        even = _mm256_add_epi64(even, _mm256_mul_epi32(x, y));
        odd = _mm256_add_epi64(odd, _mm256_mul_epi32(_mm256_srli_epi64(x, 32), _mm256_srli_epi64(y, 32)));
    }
    alignas(32) int64_t t[4];
    _mm256_store_si256(reinterpret_cast<__m256i*>(t), _mm256_add_epi64(even, odd));
    int64_t s = t[0] + t[1] + t[2] + t[3];
    for (; i < n; ++i) s += int64_t(a[i]) * b[i];
    return s;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
void axpy_avx2(int64_t* acc, const int32_t* x, int32_t d, size_t n)
{
    const __m256i dv = _mm256_set1_epi64x(d);
    size_t i = 0;
    for (; i + 4 <= n; i += 4)
    {
        const __m256i xv = _mm256_cvtepi32_epi64(_mm_loadu_si128(reinterpret_cast<const __m128i*>(x + i)));
        const __m256i av = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(acc + i));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(acc + i), _mm256_add_epi64(av, _mm256_mul_epi32(xv, dv)));
    }
    for (; i < n; ++i) acc[i] += int64_t(x[i]) * d;
}

const bool kHasAvx2 = cpu_has_avx2();
std::atomic<bool> g_vector{kHasAvx2};

#else

const bool kHasAvx2 = false;
std::atomic<bool> g_vector{false};

#endif

inline int64_t dot(const int32_t* a, const int32_t* b, size_t n)
{
#if SIEVE_TRAIN_X86
    if (g_vector.load(std::memory_order_relaxed)) return dot_avx2(a, b, n);
#endif
    return dot_plain(a, b, n);
}

inline void axpy(int64_t* acc, const int32_t* x, int32_t d, size_t n)
{
#if SIEVE_TRAIN_X86
    if (g_vector.load(std::memory_order_relaxed)) return axpy_avx2(acc, x, d, n);
#endif
    axpy_plain(acc, x, d, n);
}

} // namespace

bool vector_kernels_available() { return kHasAvx2; }
bool use_vector_kernels(bool enable)
{
    g_vector = enable && kHasAvx2;
    return g_vector;
}
bool using_vector_kernels() { return g_vector; }

// ---------------------------------------------------------------- the work, shared out

// A pool of threads kept for the trainer's life. each(n, f) calls f(begin, end) over [0, n) in
// as many parts as there are threads (or items), one a thread, and waits. Every sum the training
// takes is of integers, so how the work is cut changes nothing in what it gives.
class Pool
{
public:
    explicit Pool(unsigned n) : n_(std::max(1u, n))
    {
        for (unsigned i = 1; i < n_; ++i) workers_.emplace_back([this, i] { run(i); });
    }
    ~Pool()
    {
        {
            std::lock_guard<std::mutex> l(m_);
            quit_ = true;
        }
        cv_.notify_all();
        for (std::thread& t : workers_) t.join();
    }
    unsigned size() const { return n_; }
    void each(size_t n, const std::function<void(size_t, size_t)>& f)
    {
        const unsigned parts = unsigned(std::min<size_t>(n_, n));
        if (parts <= 1)
        {
            if (n) f(0, n);
            return;
        }
        {
            std::lock_guard<std::mutex> l(m_);
            job_ = &f;
            total_ = n;
            parts_ = parts;
            pending_ = parts - 1;
            ++generation_;
        }
        cv_.notify_all();
        f(0, n / parts);
        std::unique_lock<std::mutex> l(m_);
        done_.wait(l, [this] { return pending_ == 0; });
    }

private:
    void run(unsigned i)
    {
        uint64_t seen = 0;
        for (;;)
        {
            const std::function<void(size_t, size_t)>* job;
            size_t total;
            unsigned parts;
            {
                std::unique_lock<std::mutex> l(m_);
                cv_.wait(l, [&] { return quit_ || generation_ != seen; });
                if (quit_) return;
                seen = generation_;
                job = job_, total = total_, parts = parts_;
            }
            if (i < parts) (*job)(total * i / parts, total * (i + 1) / parts);
            if (i < parts)
            {
                std::lock_guard<std::mutex> l(m_);
                if (--pending_ == 0) done_.notify_one();
            }
        }
    }
    unsigned n_;
    std::vector<std::thread> workers_;
    std::mutex m_;
    std::condition_variable cv_, done_;
    const std::function<void(size_t, size_t)>* job_ = nullptr;
    size_t total_ = 0;
    unsigned parts_ = 0, pending_ = 0;
    uint64_t generation_ = 0;
    bool quit_ = false;
};

namespace {

using Mat = Trainer::Net::Mat;
using I32 = int32_t; // every stored value is held to ±2^23, so it fits; products are summed in 64 bits

constexpr uint64_t kShareWork = uint64_t(1) << 15; // below this many products, one thread does it

inline I32 hold(int64_t v) { return I32(std::clamp(v, -kLimit, kLimit)); }

// f over [0, n), shared out when the work (about `work` products) is worth it.
void share(Pool* pool, size_t n, uint64_t work, const std::function<void(size_t, size_t)>& f)
{
    if (pool && pool->size() > 1 && work >= kShareWork) pool->each(n, f);
    else if (n) f(0, n);
}

// Y = X W^T: Y[p][r] = hold((sum over c of W[r][c] X[p][c]) >> 16). Each row of the matrix is
// read once and taken against every position while it is in the cache; rows are shared out.
void forward(Pool* pool, const I32* w, const Mat& m, const I32* X, size_t T, I32* Y)
{
    share(pool, m.rows, uint64_t(T) * m.rows * m.cols, [&](size_t a, size_t b) {
        for (size_t r = a; r < b; ++r)
        {
            const I32* row = w + m.first + r * m.cols;
            for (size_t p = 0; p < T; ++p) Y[p * m.rows + r] = hold(dot(row, X + p * m.cols, m.cols) >> 16);
        }
    });
}

// ACC[p][c] += sum over r of W[r][c] dY[p][r], unshifted: the caller shifts once, after every
// matrix that read the same input has added its part.
void backward(Pool* pool, const I32* w, const Mat& m, const I32* dY, size_t T, int64_t* ACC)
{
    // Positions shared out (each thread adds only to its own); within them, each row is read once
    // and added into every position it has a part in.
    share(pool, T, uint64_t(T) * m.rows * m.cols, [&](size_t a, size_t b) {
        const I32* row = w + m.first;
        for (uint32_t r = 0; r < m.rows; ++r, row += m.cols)
            for (size_t p = a; p < b; ++p)
                if (const I32 d = dY[p * m.rows + r]; d != 0) axpy(ACC + p * m.cols, row, d, m.cols);
    });
}

// grad(W)[r][c] += sum over p of dY[p][r] X[p][c], in Q32; rows shared out, so no two threads
// add to one weight.
void gradient(Pool* pool, int64_t* g, const Mat& m, const I32* dY, const I32* X, size_t T)
{
    share(pool, m.rows, uint64_t(T) * m.rows * m.cols, [&](size_t a, size_t b) {
        for (size_t r = a; r < b; ++r)
        {
            int64_t* row = g + m.first + r * m.cols;
            for (size_t p = 0; p < T; ++p)
                if (const I32 d = dY[p * m.rows + r]; d != 0) axpy(row, X + p * m.cols, d, m.cols);
        }
    });
}

// y = x / sqrt(mean(x^2) + eps); r is that factor, Q16.
int64_t rmsnorm(const I32* x, uint32_t n, I32* y)
{
    int64_t s = 0;
    for (uint32_t i = 0; i < n; ++i) s += int64_t(x[i]) * x[i];
    const int64_t ms = s / int64_t(n); // s >= 0
    const int64_t root = int64_t(isqrt(uint64_t(ms + kEps) << 16));
    const int64_t r = (int64_t(1) << 40) / root;
    for (uint32_t i = 0; i < n; ++i) y[i] = hold((x[i] * r) >> 16);
    return r;
}

// dx += r (dy - y sum(dy y) / n), held.
void rmsnorm_back(const I32* dy, const I32* y, int64_t r, uint32_t n, I32* dx)
{
    int64_t s = 0;
    for (uint32_t i = 0; i < n; ++i) s += int64_t(dy[i]) * y[i];
    const int64_t c = fdiv(s, int64_t(n) << 16);
    for (uint32_t i = 0; i < n; ++i) dx[i] = hold(dx[i] + hold((r * hold(dy[i] - ((y[i] * c) >> 16))) >> 16));
}

void softmax(const int64_t* z, size_t n, int64_t* p)
{
    int64_t m = z[0];
    for (size_t i = 1; i < n; ++i) m = std::max(m, z[i]);
    int64_t sum = 0;
    for (size_t i = 0; i < n; ++i) sum += (p[i] = exp_q16(z[i] - m));
    for (size_t i = 0; i < n; ++i) p[i] = (p[i] << 16) / sum;
}

// The rotary embedding's sines and cosines, Q16, [position][pair].
struct Rope
{
    uint32_t half = 0;
    std::vector<int64_t> sin, cos;
    Rope(uint32_t D, uint32_t positions) : half(D / 2), sin(size_t(positions) * (D / 2)), cos(sin.size())
    {
        for (uint32_t i = 0; i < half; ++i)
        {
            const int64_t f = pow2_q32(-fdiv(int64_t(2 * i) * kLog2Theta, int64_t(D)));
            for (uint32_t p = 0; p < positions; ++p) sincos_q16(int64_t(p) * f, sin[size_t(p) * half + i], cos[size_t(p) * half + i]);
        }
    }
    // Each head's pairs (i, i + D/2) turned by the position's angle (sign -1: turned back).
    void turn(I32* v, uint32_t heads, uint32_t D, uint32_t p, int sign) const
    {
        for (uint32_t a = 0; a < heads; ++a)
            for (uint32_t i = 0; i < half; ++i)
            {
                I32& x = v[a * D + i];
                I32& y = v[a * D + i + half];
                const int64_t c = cos[size_t(p) * half + i], s = sign * sin[size_t(p) * half + i];
                const I32 nx = hold(((x * c) >> 16) - ((y * s) >> 16));
                const I32 ny = hold(((y * c) >> 16) + ((x * s) >> 16));
                x = nx;
                y = ny;
            }
    }
};

const Rope& rope_for(uint32_t D)
{
    static std::mutex m;
    static std::deque<std::pair<uint32_t, Rope>> cache; // a deque: what it hands out stays put
    std::lock_guard<std::mutex> lock(m);
    for (const auto& [d, r] : cache)
        if (d == D) return r;
    cache.emplace_back(D, Rope(D, 512));
    return cache.back().second;
}

// One example forward, and (with `grad`) back, adding its gradient (Q32) to grad. With a pool, its
// matrices' work is shared out among the pool's threads. A window of text is the example whose
// inputs are its bytes but the last and whose targets are the bytes after them, attending
// backwards. With `logits`, the head's logits (T x 256, Q16) are written there.
void run_window(const Trainer::Net& net, const I32* w, const Example& ex, int64_t* grad, Score& score, Pool* pool,
                std::vector<I32>* logits_out = nullptr)
{
    const uint32_t H = net.H, A = net.A, D = net.D, F = net.F;
    const std::span<const uint8_t> tok = ex.input;
    const size_t T = tok.size();
    const bool causal = ex.causal;
    // How far each position looks: back to the start, or (not causal) at every position.
    auto reach = [&](size_t p) { return causal ? p + 1 : T; };
    const Rope& rope = rope_for(D);
    struct LayerCache
    {
        std::vector<I32> x, h1, q, k, v, att, o, x1, h2, g, u, sg, sl, a;
        std::vector<int64_t> r1, r2;
    };
    std::vector<LayerCache> lc(net.layers.size());
    std::vector<I32> x(T * H);
    for (size_t p = 0; p < T; ++p)
        for (uint32_t i = 0; i < H; ++i) x[p * H + i] = w[net.embed.first + size_t(tok[p]) * H + i];

    for (size_t l = 0; l < net.layers.size(); ++l)
    {
        const auto& L = net.layers[l];
        LayerCache& c = lc[l];
        c.x = x;
        c.h1.resize(T * H), c.q.resize(T * H), c.k.resize(T * H), c.v.resize(T * H), c.r1.resize(T);
        for (size_t p = 0; p < T; ++p) c.r1[p] = rmsnorm(&x[p * H], H, &c.h1[p * H]);
        forward(pool, w, L.q, c.h1.data(), T, c.q.data());
        forward(pool, w, L.k, c.h1.data(), T, c.k.data());
        forward(pool, w, L.v, c.h1.data(), T, c.v.data());
        for (size_t p = 0; p < T; ++p)
        {
            rope.turn(&c.q[p * H], A, D, uint32_t(p), 1);
            rope.turn(&c.k[p * H], A, D, uint32_t(p), 1);
        }
        c.att.assign(size_t(A) * T * T, 0); // [head][p][u], u < reach(p)
        c.o.assign(T * H, 0);
        share(pool, size_t(A) * T, uint64_t(A) * T * T * D, [&](size_t from, size_t to) {
            std::vector<int64_t> sc(T), P(T);
            for (size_t i = from; i < to; ++i)
            {
                const size_t a = i / T, p = i % T;
                const size_t n = reach(p);
                for (size_t u = 0; u < n; ++u)
                {
                    int64_t s = 0;
                    for (uint32_t d = 0; d < D; ++d) s += int64_t(c.q[p * H + a * D + d]) * c.k[u * H + a * D + d];
                    sc[u] = hold((hold(s >> 16) * net.isd) >> 16);
                }
                softmax(sc.data(), n, P.data());
                I32* Pi = &c.att[(a * T + p) * T];
                for (size_t u = 0; u < n; ++u) Pi[u] = I32(P[u]);
                for (uint32_t d = 0; d < D; ++d)
                {
                    int64_t s = 0;
                    for (size_t u = 0; u < n; ++u) s += int64_t(Pi[u]) * c.v[u * H + a * D + d];
                    c.o[p * H + a * D + d] = hold(s >> 16);
                }
            }
        });
        c.x1.resize(T * H), c.h2.resize(T * H), c.r2.resize(T);
        c.g.resize(T * F), c.u.resize(T * F), c.sg.resize(T * F), c.sl.resize(T * F), c.a.resize(T * F);
        std::vector<I32> y(std::max<size_t>(T * H, 1));
        forward(pool, w, L.o, c.o.data(), T, y.data());
        for (size_t i = 0; i < T * H; ++i) c.x1[i] = hold(int64_t(x[i]) + y[i]);
        for (size_t p = 0; p < T; ++p) c.r2[p] = rmsnorm(&c.x1[p * H], H, &c.h2[p * H]);
        forward(pool, w, L.gate, c.h2.data(), T, c.g.data());
        forward(pool, w, L.up, c.h2.data(), T, c.u.data());
        for (size_t at = 0; at < T * F; ++at)
        {
            c.sg[at] = I32(sigmoid_q16(c.g[at]));
            c.sl[at] = hold((int64_t(c.g[at]) * c.sg[at]) >> 16);
            c.a[at] = hold((int64_t(c.sl[at]) * c.u[at]) >> 16);
        }
        forward(pool, w, L.down, c.a.data(), T, y.data());
        for (size_t i = 0; i < T * H; ++i) x[i] = hold(int64_t(c.x1[i]) + y[i]);
    }

    // The head: the final norm, then the embedding again (tied), then the next byte's chances.
    std::vector<I32> hf(T * H), logits(T * 256), dlog(grad ? T * 256 : 0);
    std::vector<int64_t> rf(T);
    for (size_t p = 0; p < T; ++p) rf[p] = rmsnorm(&x[p * H], H, &hf[p * H]);
    forward(pool, w, net.embed, hf.data(), T, logits.data());
    if (logits_out) *logits_out = logits;
    std::vector<int64_t> z(256), probs(256);
    for (size_t p = 0; p < T; ++p)
    {
        if (ex.target[p] < 0)
        {
            if (grad)
                for (uint32_t t = 0; t < 256; ++t) dlog[p * 256 + t] = 0; // no prediction here
            continue;
        }
        for (uint32_t t = 0; t < 256; ++t) z[t] = logits[p * 256 + t];
        softmax(z.data(), 256, probs.data());
        const uint8_t target = uint8_t(ex.target[p]);
        score.prob_sum += uint64_t(probs[target]);
        score.predictions += 1;
        score.bits += -std::log2(double(std::max<int64_t>(probs[target], 1)) / 65536.0);
        if (grad)
            for (uint32_t t = 0; t < 256; ++t) dlog[p * 256 + t] = hold((probs[t] - (t == target ? kOne : 0)) * (int64_t(1) << net.shift));
    }
    if (!grad) return;
    std::vector<I32> dx(T * H, 0);
    {
        std::vector<int64_t> acc(T * H, 0);
        backward(pool, w, net.embed, dlog.data(), T, acc.data());
        gradient(pool, grad, net.embed, dlog.data(), hf.data(), T);
        std::vector<I32> dhf(H);
        for (size_t p = 0; p < T; ++p)
        {
            for (uint32_t i = 0; i < H; ++i) dhf[i] = hold(acc[p * H + i] >> 16);
            rmsnorm_back(dhf.data(), &hf[p * H], rf[p], H, &dx[p * H]);
        }
    }

    for (size_t l = net.layers.size(); l-- > 0;)
    {
        const auto& L = net.layers[l];
        LayerCache& c = lc[l];
        // The feed-forward: x2 = x1 + down(silu(gate h2) * up h2).
        std::vector<I32> dx1 = dx;
        std::vector<int64_t> accF(T * F, 0);
        backward(pool, w, L.down, dx.data(), T, accF.data());
        gradient(pool, grad, L.down, dx.data(), c.a.data(), T);
        std::vector<I32> dg(T * F), du(T * F);
        for (size_t at = 0; at < T * F; ++at)
        {
            const int64_t da = hold(accF[at] >> 16);
            const int64_t dsl = hold((da * c.u[at]) >> 16);
            du[at] = hold((da * c.sl[at]) >> 16);
            const int64_t sg = c.sg[at];
            const int64_t dsilu = hold(sg + ((((int64_t(c.g[at]) * sg) >> 16) * (kOne - sg)) >> 16));
            dg[at] = hold((dsl * dsilu) >> 16);
        }
        gradient(pool, grad, L.gate, dg.data(), c.h2.data(), T);
        gradient(pool, grad, L.up, du.data(), c.h2.data(), T);
        std::vector<int64_t> accH(T * H, 0);
        backward(pool, w, L.gate, dg.data(), T, accH.data());
        backward(pool, w, L.up, du.data(), T, accH.data());
        std::vector<I32> dh(H);
        for (size_t p = 0; p < T; ++p)
        {
            for (uint32_t i = 0; i < H; ++i) dh[i] = hold(accH[p * H + i] >> 16);
            rmsnorm_back(dh.data(), &c.h2[p * H], c.r2[p], H, &dx1[p * H]);
        }
        // Attention: x1 = x + o_proj(attention).
        std::vector<I32> dxo = dx1; // what flows on to x
        std::fill(accH.begin(), accH.end(), 0);
        backward(pool, w, L.o, dx1.data(), T, accH.data());
        gradient(pool, grad, L.o, dx1.data(), c.o.data(), T);
        std::vector<I32> dO(T * H);
        for (size_t i = 0; i < T * H; ++i) dO[i] = hold(accH[i] >> 16);
        // First each query's part (dq, and the scores' gradient, kept), then each key's and value's,
        // so that no two threads add to one place.
        std::vector<I32> dq(T * H), dk(T * H), dv(T * H), dd(size_t(A) * T * T, 0);
        share(pool, size_t(A) * T, uint64_t(A) * T * T * D, [&](size_t from, size_t to) {
            std::vector<int64_t> dP(T);
            for (size_t i = from; i < to; ++i)
            {
                const size_t a = i / T, p = i % T;
                const I32* P = &c.att[(a * T + p) * T];
                I32* ddp = &dd[(a * T + p) * T];
                const size_t n = reach(p);
                int64_t cp = 0;
                for (size_t u = 0; u < n; ++u)
                {
                    int64_t s = 0;
                    for (uint32_t d = 0; d < D; ++d) s += int64_t(dO[p * H + a * D + d]) * c.v[u * H + a * D + d];
                    dP[u] = hold(s >> 16);
                    cp += P[u] * dP[u];
                }
                cp >>= 16;
                for (size_t u = 0; u < n; ++u)
                {
                    const int64_t ds = hold((int64_t(P[u]) * hold(dP[u] - cp)) >> 16);
                    ddp[u] = hold((ds * net.isd) >> 16);
                }
                for (uint32_t d = 0; d < D; ++d)
                {
                    int64_t s = 0;
                    for (size_t u = 0; u < n; ++u) s += int64_t(ddp[u]) * c.k[u * H + a * D + d];
                    dq[p * H + a * D + d] = hold(s >> 16);
                }
            }
        });
        share(pool, size_t(A) * T, uint64_t(A) * T * T * D, [&](size_t from, size_t to) {
            for (size_t i = from; i < to; ++i)
            {
                const size_t a = i / T, u = i % T;
                for (uint32_t d = 0; d < D; ++d)
                {
                    int64_t sk = 0, sv = 0;
                    for (size_t p = causal ? u : 0; p < T; ++p)
                    {
                        sk += int64_t(dd[(a * T + p) * T + u]) * c.q[p * H + a * D + d];
                        sv += int64_t(c.att[(a * T + p) * T + u]) * dO[p * H + a * D + d];
                    }
                    dk[u * H + a * D + d] = hold(sk >> 16);
                    dv[u * H + a * D + d] = hold(sv >> 16);
                }
            }
        });
        for (size_t p = 0; p < T; ++p)
        {
            rope.turn(&dq[p * H], A, D, uint32_t(p), -1);
            rope.turn(&dk[p * H], A, D, uint32_t(p), -1);
        }
        gradient(pool, grad, L.q, dq.data(), c.h1.data(), T);
        gradient(pool, grad, L.k, dk.data(), c.h1.data(), T);
        gradient(pool, grad, L.v, dv.data(), c.h1.data(), T);
        std::fill(accH.begin(), accH.end(), 0);
        backward(pool, w, L.q, dq.data(), T, accH.data());
        backward(pool, w, L.k, dk.data(), T, accH.data());
        backward(pool, w, L.v, dv.data(), T, accH.data());
        for (size_t p = 0; p < T; ++p)
        {
            for (uint32_t i = 0; i < H; ++i) dh[i] = hold(accH[p * H + i] >> 16);
            rmsnorm_back(dh.data(), &c.h1[p * H], c.r1[p], H, &dxo[p * H]);
        }
        dx = std::move(dxo);
    }
    for (size_t p = 0; p < T; ++p)
        for (uint32_t i = 0; i < H; ++i) grad[net.embed.first + size_t(tok[p]) * H + i] += int64_t(dx[p * H + i]) << 16;
}

} // namespace

// ---------------------------------------------------------------- the trainer

Trainer::Trainer(const AiSpace& space, const AiSpace::Digits& start, const Recipe& recipe, unsigned threads)
    : space_(space), digits_(start), lr_(recipe.learning_rate_q()), momentum_(recipe.momentum_q16()), shift_(recipe.loss_scale_bits)
{
    recipe.check();
    if (start.size() != space.weights()) bad("a start of " + std::to_string(space.weights()) + " digits");
    const int64_t m = (int64_t(1) << space.shape().bits) - 1;
    master_.resize(start.size());
    velocity_.assign(start.size(), 0);
    values_.resize(start.size());
    for (const AiSpace::Tensor& t : space.tensors())
    {
        const int64_t S = scale_q32(t.cols, t.name == "model.embed_tokens.weight");
        scale_.push_back(S);
        for (uint64_t i = 0; i < uint64_t(t.rows) * t.cols; ++i)
        {
            const size_t at = size_t(t.first + i);
            master_[at] = fdiv((2 * int64_t(start[at]) - m) * S, m);
        }
    }
    threads_ = threads ? threads : std::max(1u, std::thread::hardware_concurrency());
    pool_ = std::make_unique<Pool>(threads_);
    refresh_values();
}

Trainer::~Trainer() = default;

void Trainer::refresh_values()
{
    const int64_t m = (int64_t(1) << space_.shape().bits) - 1;
    const std::vector<AiSpace::Tensor>& ts = space_.tensors();
    for (size_t ti = 0; ti < ts.size(); ++ti)
    {
        const int64_t S = scale_[ti];
        for (uint64_t i = 0; i < uint64_t(ts[ti].rows) * ts[ti].cols; ++i)
        {
            const size_t at = size_t(ts[ti].first + i);
            values_[at] = int32_t(fdiv((2 * int64_t(digits_[at]) - m) * S, m << 16));
        }
    }
}

namespace {

// A step's windows, forward and (with `grad`) back. A small model takes its windows a thread each,
// each with a gradient of its own, summed after; a large one takes them one at a time, each with
// every thread on its matrices, into the one gradient. Integer sums: either way, the same result.
constexpr uint64_t kWindowAtATime = uint64_t(1) << 20; // weights at which a window gets every thread (and no gradient of its own)

void check_examples(const std::vector<Example>& batch)
{
    for (const Example& ex : batch)
    {
        if (ex.input.empty() || ex.input.size() > 512) bad("an example of 1 to 512 positions");
        if (ex.target.size() != ex.input.size()) bad("an example with a target for every position (-1: none)");
        for (const int16_t t : ex.target)
            if (t < -1 || t > 255) bad("a target that is a byte, or -1 for none");
    }
}

Score run_examples(const Trainer::Net& net, const int32_t* w, uint64_t weights, const std::vector<Example>& batch,
                   std::vector<int64_t>* grad, Pool& pool)
{
    const size_t n = batch.size();
    Score total;
    if (pool.size() <= 1 || n < pool.size() || weights >= kWindowAtATime)
    {
        for (const auto& ex : batch) run_window(net, w, ex, grad ? grad->data() : nullptr, total, &pool);
        return total;
    }
    std::vector<std::vector<int64_t>> grads(grad ? n : 0);
    std::vector<Score> scores(n);
    pool.each(n, [&](size_t a, size_t b) {
        for (size_t i = a; i < b; ++i)
        {
            if (grad) grads[i].assign(grad->size(), 0);
            run_window(net, w, batch[i], grad ? grads[i].data() : nullptr, scores[i], nullptr);
        }
    });
    for (size_t i = 0; i < n; ++i)
    {
        total.prob_sum += scores[i].prob_sum, total.predictions += scores[i].predictions, total.bits += scores[i].bits;
        if (grad)
            for (size_t k = 0; k < grad->size(); ++k) (*grad)[k] += grads[i][k];
    }
    return total;
}

// Windows of text as examples: each byte but the last predicts the next, attending backwards.
Score run_batch(const Trainer::Net& net, const int32_t* w, uint64_t weights, const std::vector<std::span<const uint8_t>>& batch,
                std::vector<int64_t>* grad, Pool& pool)
{
    for (const auto& win : batch)
        if (win.size() < 2) bad("a window of at least two bytes");
    std::vector<std::vector<int16_t>> targets(batch.size());
    std::vector<Example> ex(batch.size());
    for (size_t i = 0; i < batch.size(); ++i)
    {
        targets[i].assign(batch[i].begin() + 1, batch[i].end());
        ex[i] = Example{batch[i].first(batch[i].size() - 1), targets[i], true};
    }
    return run_examples(net, w, weights, ex, grad, pool);
}

} // namespace

Score Trainer::step(const std::vector<std::span<const uint8_t>>& batch)
{
    Net net(space_);
    net.shift = shift_;
    std::vector<int64_t> grad(values_.size(), 0);
    const Score s = run_batch(net, values_.data(), values_.size(), batch, &grad, *pool_);
    apply(grad);
    return s;
}

Score Trainer::step(const std::vector<Example>& batch)
{
    check_examples(batch);
    Net net(space_);
    net.shift = shift_;
    std::vector<int64_t> grad(values_.size(), 0);
    const Score s = run_examples(net, values_.data(), values_.size(), batch, &grad, *pool_);
    apply(grad);
    return s;
}

void Trainer::apply(const std::vector<int64_t>& grad)
{
    const int64_t m = (int64_t(1) << space_.shape().bits) - 1;
    const std::vector<AiSpace::Tensor>& ts = space_.tensors();
    for (size_t ti = 0; ti < ts.size(); ++ti)
    {
        const int64_t S = scale_[ti];
        for (uint64_t i = 0; i < uint64_t(ts[ti].rows) * ts[ti].cols; ++i)
        {
            const size_t at = size_t(ts[ti].first + i);
            // (v1 is v2 with no shift: the limits and the shifts below come to v1's then.)
            const int64_t gl = kGradLimit << shift_, vl = kVelocityLimit << shift_;
            const int64_t g = std::clamp(grad[at], -gl, gl);
            velocity_[at] = std::clamp(mul_shift(velocity_[at], momentum_, 16) + g, -vl, vl);
            // The rate is held to 16 + k bits and the gradient carries 2^k: both come off here.
            master_[at] = std::clamp(master_[at] - mul_shift(velocity_[at], lr_, 16 + 2 * shift_), -S, S);
            digits_[at] = uint8_t(std::clamp<int64_t>(fdiv((master_[at] + S) * m + S, 2 * S), 0, m));
        }
    }
    refresh_values();
}

void Trainer::restore(const std::vector<int64_t>& masters, const std::vector<int64_t>& velocities)
{
    if (masters.size() != master_.size() || velocities.size() != velocity_.size()) bad("a saved state of another shape");
    const int64_t m = (int64_t(1) << space_.shape().bits) - 1;
    const std::vector<AiSpace::Tensor>& ts = space_.tensors();
    for (size_t ti = 0; ti < ts.size(); ++ti)
    {
        const int64_t S = scale_[ti];
        for (uint64_t i = 0; i < uint64_t(ts[ti].rows) * ts[ti].cols; ++i)
        {
            const size_t at = size_t(ts[ti].first + i);
            if (masters[at] < -S || masters[at] > S) bad("a saved state whose weights are off the line");
            master_[at] = masters[at];
            velocity_[at] = velocities[at];
            digits_[at] = uint8_t(std::clamp<int64_t>(fdiv((master_[at] + S) * m + S, 2 * S), 0, m));
        }
    }
    refresh_values();
}

Score evaluate_model(const AiSpace& space, const AiSpace::Digits& digits, const std::vector<std::span<const uint8_t>>& windows)
{
    if (digits.size() != space.weights()) bad("a model of " + std::to_string(space.weights()) + " digits");
    const int64_t m = (int64_t(1) << space.shape().bits) - 1;
    std::vector<int32_t> values(digits.size());
    for (const AiSpace::Tensor& t : space.tensors())
    {
        const int64_t S = scale_q32(t.cols, t.name == "model.embed_tokens.weight");
        for (uint64_t i = 0; i < uint64_t(t.rows) * t.cols; ++i)
        {
            const size_t at = size_t(t.first + i);
            values[at] = int32_t(fdiv((2 * int64_t(digits[at]) - m) * S, m << 16));
        }
    }
    Trainer::Net net(space);
    Pool pool(1);
    return run_batch(net, values.data(), values.size(), windows, nullptr, pool);
}

Score Trainer::evaluate(const std::vector<std::span<const uint8_t>>& windows) const
{
    Net net(space_);
    net.shift = shift_;
    return run_batch(net, values_.data(), values_.size(), windows, nullptr, *pool_);
}

Score Trainer::evaluate(const std::vector<Example>& batch) const
{
    check_examples(batch);
    Net net(space_);
    net.shift = shift_;
    return run_examples(net, values_.data(), values_.size(), batch, nullptr, *pool_);
}

std::vector<int32_t> model_logits(const AiSpace& space, const AiSpace::Digits& digits, const Example& ex)
{
    check_examples({ex});
    if (digits.size() != space.weights()) bad("a model of " + std::to_string(space.weights()) + " digits");
    const int64_t m = (int64_t(1) << space.shape().bits) - 1;
    std::vector<int32_t> values(digits.size());
    for (const AiSpace::Tensor& t : space.tensors())
    {
        const int64_t S = scale_q32(t.cols, t.name == "model.embed_tokens.weight");
        for (uint64_t i = 0; i < uint64_t(t.rows) * t.cols; ++i)
        {
            const size_t at = size_t(t.first + i);
            values[at] = int32_t(fdiv((2 * int64_t(digits[at]) - m) * S, m << 16));
        }
    }
    Trainer::Net net(space);
    Score score;
    std::vector<int32_t> logits;
    run_window(net, values.data(), ex, nullptr, score, nullptr, &logits);
    return logits;
}

} // namespace sieve::training
