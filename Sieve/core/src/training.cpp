// Sieve — training a model of the AI line in pinned integer arithmetic (see training.hpp).
#include "sieve/training.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <stdexcept>
#include <thread>

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

// Decimal text (digits, an optional point and digits) to round(x * 2^16), halves up.
int64_t decimal_q16(const std::string& s, const char* what)
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
    return int64_t((num * 2 * 65536 + den) / (2 * den));
}

} // namespace

// ---------------------------------------------------------------- recipe, order, result

int64_t Recipe::learning_rate_q16() const { return decimal_q16(learning_rate, "learning-rate"); }
int64_t Recipe::momentum_q16() const { return decimal_q16(momentum, "momentum"); }

void Recipe::check() const
{
    shape.check();
    if (shape.width > 4096) bad("a width up to 4096");
    const int64_t lr = learning_rate_q16(), mo = momentum_q16();
    if (lr < 1 || lr > kOne) bad("a learning rate from 2^-16 to 1");
    if (mo < 0 || mo >= kOne) bad("a momentum below 1");
    if (batch < 1 || batch > 32) bad("from 1 to 32 windows a step");
    if (checkpoint_every < 1) bad("a checkpoint every step or more");
    if (start.empty() || start.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) bad("a start address in hex");
}

std::string Recipe::text() const
{
    std::string t = "[training]\n";
    t += "version = " + std::string(kTrainingVersion) + "\n";
    t += "layers = " + std::to_string(shape.layers) + "\n";
    t += "width = " + std::to_string(shape.width) + "\n";
    t += "heads = " + std::to_string(shape.heads) + "\n";
    t += "bits = " + std::to_string(shape.bits) + "\n";
    t += "key = " + key + "\n";
    t += "start = " + std::string(to_string(start_mode)) + " " + start + "\n";
    t += "learning-rate = " + learning_rate + "\n";
    t += "momentum = " + momentum + "\n";
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
            if (v != kTrainingVersion) bad("this is " + std::string(kTrainingVersion) + ", and the run asks for " + v);
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
    std::string t = "sieve-training-result-v" + std::to_string(version) + "\nversion " + std::string(kTrainingVersion) + "\nsteps " +
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
            if (f[1] != kTrainingVersion) bad("this result is of " + f[1]);
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
        else if (f[0] == "final" && f.size() == 2) r.final_address = f[1];
        else if (f[0] == "model-sha256" && f.size() == 2) r.model_sha256 = f[1];
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
    int64_t isd = 0; // 1 / sqrt(D), Q16

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

namespace {

using Mat = Trainer::Net::Mat;

// y = W x: each row's products summed, then shifted down once.
void matvec(const int64_t* w, const Mat& m, const int64_t* x, int64_t* y)
{
    const int64_t* row = w + m.first;
    for (uint32_t r = 0; r < m.rows; ++r, row += m.cols)
    {
        int64_t s = 0;
        for (uint32_t c = 0; c < m.cols; ++c) s += row[c] * x[c];
        y[r] = sat(s >> 16);
    }
}

// acc += W^T dy, unshifted (the caller shifts once, after every term it sums).
void matvec_t(const int64_t* w, const Mat& m, const int64_t* dy, int64_t* acc)
{
    const int64_t* row = w + m.first;
    for (uint32_t r = 0; r < m.rows; ++r, row += m.cols)
        for (uint32_t c = 0; c < m.cols; ++c) acc[c] += row[c] * dy[r];
}

// grad(W) += dy x^T, in Q32.
void outer(int64_t* g, const Mat& m, const int64_t* dy, const int64_t* x)
{
    int64_t* row = g + m.first;
    for (uint32_t r = 0; r < m.rows; ++r, row += m.cols)
        for (uint32_t c = 0; c < m.cols; ++c) row[c] += dy[r] * x[c];
}

// y = x / sqrt(mean(x^2) + eps); r is that factor, Q16.
int64_t rmsnorm(const int64_t* x, uint32_t n, int64_t* y)
{
    int64_t s = 0;
    for (uint32_t i = 0; i < n; ++i) s += x[i] * x[i];
    const int64_t ms = s / int64_t(n); // s >= 0
    const int64_t root = int64_t(isqrt(uint64_t(ms + kEps) << 16));
    const int64_t r = (int64_t(1) << 40) / root;
    for (uint32_t i = 0; i < n; ++i) y[i] = sat((x[i] * r) >> 16);
    return r;
}

// dx = r (dy - y sum(dy y) / n), added to dx.
void rmsnorm_back(const int64_t* dy, const int64_t* y, int64_t r, uint32_t n, int64_t* dx)
{
    int64_t s = 0;
    for (uint32_t i = 0; i < n; ++i) s += dy[i] * y[i];
    const int64_t c = fdiv(s, int64_t(n) << 16);
    for (uint32_t i = 0; i < n; ++i) dx[i] = sat(dx[i] + sat((r * sat(dy[i] - ((y[i] * c) >> 16))) >> 16));
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
    void turn(int64_t* v, uint32_t heads, uint32_t D, uint32_t p, int sign) const
    {
        for (uint32_t a = 0; a < heads; ++a)
            for (uint32_t i = 0; i < half; ++i)
            {
                int64_t& x = v[a * D + i];
                int64_t& y = v[a * D + i + half];
                const int64_t c = cos[size_t(p) * half + i], s = sign * sin[size_t(p) * half + i];
                const int64_t nx = sat(((x * c) >> 16) - ((y * s) >> 16));
                const int64_t ny = sat(((y * c) >> 16) + ((x * s) >> 16));
                x = nx;
                y = ny;
            }
    }
};

const Rope& rope_for(uint32_t D)
{
    static thread_local std::deque<std::pair<uint32_t, Rope>> cache; // a deque: what it hands out stays put
    for (const auto& [d, r] : cache)
        if (d == D) return r;
    cache.emplace_back(D, Rope(D, 512));
    return cache.back().second;
}

// One window forward, and (with `grad`) back, adding its gradient (Q32) to grad.
void run_window(const Trainer::Net& net, const int64_t* w, std::span<const uint8_t> tok, int64_t* grad, Score& score)
{
    const uint32_t H = net.H, A = net.A, D = net.D, F = net.F;
    const size_t T = tok.size() - 1;
    const Rope& rope = rope_for(D);
    struct LayerCache
    {
        std::vector<int64_t> x, h1, q, k, v, att, o, x1, h2, g, u, sg, sl, a;
        std::vector<int64_t> r1, r2;
    };
    std::vector<LayerCache> lc(net.layers.size());
    std::vector<int64_t> x(T * H);
    for (size_t p = 0; p < T; ++p)
        for (uint32_t i = 0; i < H; ++i) x[p * H + i] = w[net.embed.first + size_t(tok[p]) * H + i];

    for (size_t l = 0; l < net.layers.size(); ++l)
    {
        const auto& L = net.layers[l];
        LayerCache& c = lc[l];
        c.x = x;
        c.h1.resize(T * H), c.q.resize(T * H), c.k.resize(T * H), c.v.resize(T * H), c.r1.resize(T);
        for (size_t p = 0; p < T; ++p)
        {
            c.r1[p] = rmsnorm(&x[p * H], H, &c.h1[p * H]);
            matvec(w, L.q, &c.h1[p * H], &c.q[p * H]);
            matvec(w, L.k, &c.h1[p * H], &c.k[p * H]);
            matvec(w, L.v, &c.h1[p * H], &c.v[p * H]);
            rope.turn(&c.q[p * H], A, D, uint32_t(p), 1);
            rope.turn(&c.k[p * H], A, D, uint32_t(p), 1);
        }
        c.att.assign(size_t(A) * T * T, 0); // [head][p][u], u <= p
        c.o.assign(T * H, 0);
        std::vector<int64_t> sc(T);
        for (uint32_t a = 0; a < A; ++a)
            for (size_t p = 0; p < T; ++p)
            {
                for (size_t u = 0; u <= p; ++u)
                {
                    int64_t s = 0;
                    for (uint32_t d = 0; d < D; ++d) s += c.q[p * H + a * D + d] * c.k[u * H + a * D + d];
                    sc[u] = sat((sat(s >> 16) * net.isd) >> 16);
                }
                int64_t* P = &c.att[(size_t(a) * T + p) * T];
                softmax(sc.data(), p + 1, P);
                for (uint32_t d = 0; d < D; ++d)
                {
                    int64_t s = 0;
                    for (size_t u = 0; u <= p; ++u) s += P[u] * c.v[u * H + a * D + d];
                    c.o[p * H + a * D + d] = sat(s >> 16);
                }
            }
        c.x1.resize(T * H), c.h2.resize(T * H), c.r2.resize(T);
        c.g.resize(T * F), c.u.resize(T * F), c.sg.resize(T * F), c.sl.resize(T * F), c.a.resize(T * F);
        std::vector<int64_t> y(std::max(H, F));
        for (size_t p = 0; p < T; ++p)
        {
            matvec(w, L.o, &c.o[p * H], y.data());
            for (uint32_t i = 0; i < H; ++i) c.x1[p * H + i] = sat(x[p * H + i] + y[i]);
            c.r2[p] = rmsnorm(&c.x1[p * H], H, &c.h2[p * H]);
            matvec(w, L.gate, &c.h2[p * H], &c.g[p * F]);
            matvec(w, L.up, &c.h2[p * H], &c.u[p * F]);
            for (uint32_t j = 0; j < F; ++j)
            {
                const size_t at = p * F + j;
                c.sg[at] = sigmoid_q16(c.g[at]);
                c.sl[at] = sat((c.g[at] * c.sg[at]) >> 16);
                c.a[at] = sat((c.sl[at] * c.u[at]) >> 16);
            }
            matvec(w, L.down, &c.a[p * F], y.data());
            for (uint32_t i = 0; i < H; ++i) x[p * H + i] = sat(c.x1[p * H + i] + y[i]);
        }
    }

    // The head: the final norm, then the embedding again (tied), then the next byte's chances.
    std::vector<int64_t> hf(T * H), rf(T), probs(256), logits(256), dlog(256);
    std::vector<int64_t> dx(T * H, 0);
    const int64_t* E = w + net.embed.first;
    for (size_t p = 0; p < T; ++p)
    {
        rf[p] = rmsnorm(&x[p * H], H, &hf[p * H]);
        for (uint32_t t = 0; t < 256; ++t)
        {
            int64_t s = 0;
            for (uint32_t i = 0; i < H; ++i) s += E[size_t(t) * H + i] * hf[p * H + i];
            logits[t] = sat(s >> 16);
        }
        softmax(logits.data(), 256, probs.data());
        const uint8_t target = tok[p + 1];
        score.prob_sum += uint64_t(probs[target]);
        score.predictions += 1;
        score.bits += -std::log2(double(std::max<int64_t>(probs[target], 1)) / 65536.0);
        if (!grad) continue;
        for (uint32_t t = 0; t < 256; ++t) dlog[t] = probs[t] - (t == target ? kOne : 0);
        std::vector<int64_t> acc(H, 0);
        int64_t* gE = grad + net.embed.first;
        for (uint32_t t = 0; t < 256; ++t)
            for (uint32_t i = 0; i < H; ++i)
            {
                acc[i] += E[size_t(t) * H + i] * dlog[t];
                gE[size_t(t) * H + i] += dlog[t] * hf[p * H + i];
            }
        std::vector<int64_t> dhf(H);
        for (uint32_t i = 0; i < H; ++i) dhf[i] = sat(acc[i] >> 16);
        rmsnorm_back(dhf.data(), &hf[p * H], rf[p], H, &dx[p * H]);
    }
    if (!grad) return;

    for (size_t l = net.layers.size(); l-- > 0;)
    {
        const auto& L = net.layers[l];
        LayerCache& c = lc[l];
        // The feed-forward: x2 = x1 + down(silu(gate h2) * up h2).
        std::vector<int64_t> dx1 = dx;
        std::vector<int64_t> da(F), dg(F), du(F), acc(std::max(H, F));
        for (size_t p = 0; p < T; ++p)
        {
            std::fill(acc.begin(), acc.end(), 0);
            matvec_t(w, L.down, &dx[p * H], acc.data());
            for (uint32_t j = 0; j < F; ++j) da[j] = sat(acc[j] >> 16);
            outer(grad, L.down, &dx[p * H], &c.a[p * F]);
            for (uint32_t j = 0; j < F; ++j)
            {
                const size_t at = p * F + j;
                const int64_t dsl = sat((da[j] * c.u[at]) >> 16);
                du[j] = sat((da[j] * c.sl[at]) >> 16);
                const int64_t sg = c.sg[at];
                const int64_t dsilu = sat(sg + ((((c.g[at] * sg) >> 16) * (kOne - sg)) >> 16));
                dg[j] = sat((dsl * dsilu) >> 16);
            }
            outer(grad, L.gate, dg.data(), &c.h2[p * H]);
            outer(grad, L.up, du.data(), &c.h2[p * H]);
            std::fill(acc.begin(), acc.end(), 0);
            matvec_t(w, L.gate, dg.data(), acc.data());
            matvec_t(w, L.up, du.data(), acc.data());
            std::vector<int64_t> dh2(H);
            for (uint32_t i = 0; i < H; ++i) dh2[i] = sat(acc[i] >> 16);
            rmsnorm_back(dh2.data(), &c.h2[p * H], c.r2[p], H, &dx1[p * H]);
        }
        // Attention: x1 = x + o_proj(attention).
        std::vector<int64_t> dxo = dx1; // what flows on to x
        std::vector<int64_t> dO(T * H);
        for (size_t p = 0; p < T; ++p)
        {
            std::fill(acc.begin(), acc.end(), 0);
            matvec_t(w, L.o, &dx1[p * H], acc.data());
            for (uint32_t i = 0; i < H; ++i) dO[p * H + i] = sat(acc[i] >> 16);
            outer(grad, L.o, &dx1[p * H], &c.o[p * H]);
        }
        std::vector<int64_t> dq(T * H), dk(T * H), dv(T * H);
        std::vector<int64_t> sk(T * H, 0), sv(T * H, 0); // unshifted sums over queries
        std::vector<int64_t> dP(T), dd(T);
        for (uint32_t a = 0; a < A; ++a)
            for (size_t p = 0; p < T; ++p)
            {
                const int64_t* P = &c.att[(size_t(a) * T + p) * T];
                int64_t cp = 0;
                for (size_t u = 0; u <= p; ++u)
                {
                    int64_t s = 0;
                    for (uint32_t d = 0; d < D; ++d) s += dO[p * H + a * D + d] * c.v[u * H + a * D + d];
                    dP[u] = sat(s >> 16);
                    cp += P[u] * dP[u];
                }
                cp >>= 16;
                for (size_t u = 0; u <= p; ++u)
                {
                    const int64_t ds = sat((P[u] * sat(dP[u] - cp)) >> 16);
                    dd[u] = sat((ds * net.isd) >> 16);
                }
                for (uint32_t d = 0; d < D; ++d)
                {
                    int64_t s = 0;
                    for (size_t u = 0; u <= p; ++u)
                    {
                        s += dd[u] * c.k[u * H + a * D + d];
                        sk[u * H + a * D + d] += dd[u] * c.q[p * H + a * D + d];
                        sv[u * H + a * D + d] += P[u] * dO[p * H + a * D + d];
                    }
                    dq[p * H + a * D + d] = sat(s >> 16);
                }
            }
        for (size_t i = 0; i < T * H; ++i) dk[i] = sat(sk[i] >> 16), dv[i] = sat(sv[i] >> 16);
        for (size_t p = 0; p < T; ++p)
        {
            rope.turn(&dq[p * H], A, D, uint32_t(p), -1);
            rope.turn(&dk[p * H], A, D, uint32_t(p), -1);
            outer(grad, L.q, &dq[p * H], &c.h1[p * H]);
            outer(grad, L.k, &dk[p * H], &c.h1[p * H]);
            outer(grad, L.v, &dv[p * H], &c.h1[p * H]);
            std::fill(acc.begin(), acc.end(), 0);
            matvec_t(w, L.q, &dq[p * H], acc.data());
            matvec_t(w, L.k, &dk[p * H], acc.data());
            matvec_t(w, L.v, &dv[p * H], acc.data());
            std::vector<int64_t> dh1(H);
            for (uint32_t i = 0; i < H; ++i) dh1[i] = sat(acc[i] >> 16);
            rmsnorm_back(dh1.data(), &c.h1[p * H], c.r1[p], H, &dxo[p * H]);
        }
        dx = std::move(dxo);
    }
    for (size_t p = 0; p < T; ++p)
        for (uint32_t i = 0; i < H; ++i) grad[net.embed.first + size_t(tok[p]) * H + i] += dx[p * H + i] << 16;
}

} // namespace

// ---------------------------------------------------------------- the trainer

Trainer::Trainer(const AiSpace& space, const AiSpace::Digits& start, const Recipe& recipe, unsigned threads)
    : space_(space), digits_(start), lr_(recipe.learning_rate_q16()), momentum_(recipe.momentum_q16())
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
    refresh_values();
}

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
            values_[at] = fdiv((2 * int64_t(digits_[at]) - m) * S, m << 16);
        }
    }
}

namespace {

Score run_batch(const Trainer::Net& net, const int64_t* w, const std::vector<std::span<const uint8_t>>& batch, std::vector<int64_t>* grad,
                unsigned threads)
{
    const size_t n = batch.size(), workers = std::max<size_t>(1, std::min<size_t>(threads, n));
    std::vector<std::vector<int64_t>> grads(workers);
    std::vector<Score> scores(workers);
    auto work = [&](size_t k) {
        if (grad) grads[k].assign(grad->size(), 0);
        for (size_t i = k; i < n; i += workers)
        {
            if (batch[i].size() < 2) bad("a window of at least two bytes");
            run_window(net, w, batch[i], grad ? grads[k].data() : nullptr, scores[k]);
        }
    };
    if (workers == 1) work(0);
    else
    {
        std::vector<std::thread> pool;
        for (size_t k = 0; k < workers; ++k) pool.emplace_back(work, k);
        for (std::thread& t : pool) t.join();
    }
    Score total;
    for (size_t k = 0; k < workers; ++k)
    {
        total.prob_sum += scores[k].prob_sum, total.predictions += scores[k].predictions, total.bits += scores[k].bits;
        if (grad)
            for (size_t i = 0; i < grad->size(); ++i) (*grad)[i] += grads[k][i];
    }
    return total;
}

} // namespace

Score Trainer::step(const std::vector<std::span<const uint8_t>>& batch)
{
    const Net net(space_);
    std::vector<int64_t> grad(values_.size(), 0);
    const Score s = run_batch(net, values_.data(), batch, &grad, threads_);
    const int64_t m = (int64_t(1) << space_.shape().bits) - 1;
    const std::vector<AiSpace::Tensor>& ts = space_.tensors();
    for (size_t ti = 0; ti < ts.size(); ++ti)
    {
        const int64_t S = scale_[ti];
        for (uint64_t i = 0; i < uint64_t(ts[ti].rows) * ts[ti].cols; ++i)
        {
            const size_t at = size_t(ts[ti].first + i);
            const int64_t g = std::clamp(grad[at], -kGradLimit, kGradLimit);
            velocity_[at] = std::clamp(((velocity_[at] * momentum_) >> 16) + g, -kVelocityLimit, kVelocityLimit);
            master_[at] = std::clamp(master_[at] - ((velocity_[at] * lr_) >> 16), -S, S);
            digits_[at] = uint8_t(std::clamp<int64_t>(fdiv((master_[at] + S) * m + S, 2 * S), 0, m));
        }
    }
    refresh_values();
    return s;
}

Score Trainer::evaluate(const std::vector<std::span<const uint8_t>>& windows) const
{
    const Net net(space_);
    return run_batch(net, values_.data(), windows, nullptr, threads_);
}

} // namespace sieve::training
