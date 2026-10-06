#include "media_decode.hpp"

#include "dictionaries.hpp" // executable_dir
#include "image_io.hpp"
#include "sieve/filekind.hpp"
#include "sieve/sound.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <span>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <stdio.h> // _wpopen, _pclose
#endif

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

std::mutex g_mx;
std::string g_path; // set by set_ffmpeg_path

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// An environment variable, as UTF-8 (wide on Windows, so a path in any script comes through).
std::string env(const char* name)
{
#ifdef _WIN32
    const fs::path n(name);
    wchar_t* v = nullptr;
    size_t len = 0;
    std::string out;
    if (_wdupenv_s(&v, &len, n.c_str()) == 0 && v)
    {
        out = u8(fs::path(v));
        free(v);
    }
    return out;
#else
    const char* v = std::getenv(name);
    return v ? v : "";
#endif
}

bool is_file(const fs::path& p)
{
    std::error_code ec;
    return !p.empty() && fs::is_regular_file(p, ec);
}

#ifdef _WIN32
const char* const kNames[] = {"ffmpeg.exe"};
const char kPathSep = ';';
#else
const char* const kNames[] = {"ffmpeg"};
const char kPathSep = ':';
#endif

// One argument, quoted for the system's shell.
std::string quoted(const std::string& s)
{
#ifdef _WIN32
    std::string out = "\"";
    for (char c : s)
    {
        if (c == '"') throw std::invalid_argument("a path with a double quote cannot be given to ffmpeg");
        out += c;
    }
    return out + "\"";
#else
    std::string out = "'";
    for (char c : s) out += c == '\'' ? std::string("'\\''") : std::string(1, c);
    return out + "'";
#endif
}

// A command line run with its standard output read as it comes.
class Pipe
{
public:
    explicit Pipe(const std::string& command)
    {
#ifdef _WIN32
        // cmd /c takes the whole line in one more pair of quotes.
        const fs::path wide = from_u8("\"" + command + "\"");
        p_ = _wpopen(wide.c_str(), L"rb");
#else
        p_ = popen(command.c_str(), "r");
#endif
        if (!p_) throw std::runtime_error("cannot run ffmpeg");
    }
    ~Pipe() { close(); }
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    // Up to n bytes into out (fewer only at the end).
    size_t read(char* out, size_t n)
    {
        size_t got = 0;
        while (got < n)
        {
            const size_t k = fread(out + got, 1, n - got, p_);
            if (k == 0) break;
            got += k;
        }
        return got;
    }
    // One line, without its '\n'; false at the end.
    bool line(std::string& out)
    {
        out.clear();
        int c;
        while ((c = fgetc(p_)) != EOF && c != '\n') out += char(c);
        return c != EOF || !out.empty();
    }
    // Its exit status (the program is waited for).
    int close()
    {
        if (!p_) return status_;
        // Closing our end first: a program still writing gets a broken pipe and stops.
#ifdef _WIN32
        status_ = _pclose(p_);
#else
        status_ = pclose(p_);
#endif
        p_ = nullptr;
        return status_;
    }

private:
    FILE* p_ = nullptr;
    int status_ = 0;
};

// Runs a command line and reads all it writes to its standard output.
std::string run(const std::string& command)
{
    Pipe p(command);
    std::string out, l;
    while (p.line(l)) out += l + "\n";
    return out;
}

std::string read_text(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    std::string s(std::istreambuf_iterator<char>(in), {});
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

// Reads the next PAM frame ffmpeg wrote into `frame`; false at the end.
bool next_pam(Pipe& p, RgbaImage& frame)
{
    std::string l;
    if (!p.line(l)) return false;
    if (l != "P7") throw std::runtime_error("ffmpeg wrote something other than PAM frames");
    uint64_t w = 0, h = 0, depth = 0, maxval = 0;
    for (;;)
    {
        if (!p.line(l)) throw std::runtime_error("ffmpeg's last frame was cut short");
        if (l == "ENDHDR") break;
        const size_t sp = l.find(' ');
        const std::string key = l.substr(0, sp);
        const uint64_t v = sp == std::string::npos ? 0 : std::strtoull(l.c_str() + sp + 1, nullptr, 10);
        if (key == "WIDTH") w = v;
        else if (key == "HEIGHT") h = v;
        else if (key == "DEPTH") depth = v;
        else if (key == "MAXVAL") maxval = v;
    }
    if (w == 0 || h == 0 || depth != 4 || maxval != 255 || w > 0xFFFFFFFFull || h > 0xFFFFFFFFull || w * h > SIZE_MAX / 4)
        throw std::runtime_error("ffmpeg wrote a frame Sieve cannot read (not 8-bit RGBA)");
    frame.width = uint32_t(w);
    frame.height = uint32_t(h);
    frame.rgba.resize(size_t(w * h * 4));
    if (p.read(reinterpret_cast<char*>(frame.rgba.data()), frame.rgba.size()) != frame.rgba.size())
        throw std::runtime_error("ffmpeg's last frame was cut short");
    return true;
}

} // namespace

TempFile::TempFile(const std::string& suffix)
{
    static std::atomic<uint64_t> n{0};
    const auto t = std::chrono::steady_clock::now().time_since_epoch().count();
    path = fs::temp_directory_path() / ("sieve-" + std::to_string(t) + "-" + std::to_string(n++) + suffix);
}

TempFile::~TempFile()
{
    std::error_code ec;
    fs::remove(path, ec);
}

std::string TempFile::u8() const
{
    const std::u8string s = path.u8string();
    return std::string(s.begin(), s.end());
}

std::string ffmpeg_missing()
{
    const std::optional<std::string> exe = find_ffmpeg();
    if (!exe)
        return "ffmpeg was not found (give it with --ffmpeg PATH, the SIEVE_FFMPEG environment variable, or put it beside Sieve or on "
               "the PATH)";
    if (!is_file(from_u8(*exe))) return "there is no ffmpeg at '" + *exe + "'";
    return {};
}

namespace {

// The ffmpeg to run, or why there is none. `cannot` says what could not be done ("cannot decode
// 'a.mp4'"), `own` what Sieve does itself ("reads WAV").
std::string ffmpeg_or_throw(const std::string& cannot, const char* own)
{
    const std::optional<std::string> exe = find_ffmpeg();
    if (!exe)
        throw std::runtime_error(cannot + ": Sieve " + own + " itself, and every other format through ffmpeg, which was not found "
                                 "(give it with --ffmpeg PATH, the SIEVE_FFMPEG environment variable, or put it beside Sieve or on "
                                 "the PATH)");
    if (!is_file(from_u8(*exe))) throw std::runtime_error(cannot + ": there is no ffmpeg at '" + *exe + "'");
    return *exe;
}

MediaRead ffmpeg_frames(const fs::path& file, const std::string& what, uint32_t max_frames, const EachFrame& each)
{
    const std::optional<std::string> exe = ffmpeg_or_throw("cannot decode " + what, "reads PNG, JPEG, BMP, GIF and TGA");
    const TempFile err(".txt");
    // One frame more than wanted, to know whether there are more; each stored frame once, in
    // order; colours converted exactly the same way on every machine.
    std::string cmd = quoted(*exe) + " -hide_banner -nostdin -v error -i " + quoted(u8(file)) + " -map 0:v:0";
    if (max_frames < UINT32_MAX) cmd += " -frames:v " + std::to_string(uint64_t(max_frames) + 1);
    cmd += " -fps_mode passthrough -sws_flags +accurate_rnd+full_chroma_int+bitexact -flags +bitexact"
           " -pix_fmt rgba -c:v pam -f image2pipe - 2> " +
           quoted(u8(err.path));
    MediaRead r;
    uint64_t n = 0;
    {
        // A reader thread takes frames from the pipe while this one fits the last: ffmpeg decodes
        // the next frame meanwhile, rather than waiting on a full pipe. Two frames are held at most.
        Pipe p(cmd);
        std::mutex mx;
        std::condition_variable cv;
        std::deque<RgbaImage> ready;
        bool done = false, stop = false;
        std::exception_ptr failed;
        std::thread reader([&] {
            try
            {
                for (;;)
                {
                    RgbaImage frame;
                    const bool got = next_pam(p, frame);
                    std::unique_lock<std::mutex> lock(mx);
                    if (!got) break;
                    cv.wait(lock, [&] { return ready.size() < 2 || stop; });
                    if (stop) break;
                    ready.push_back(std::move(frame));
                    cv.notify_all();
                }
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(mx);
                failed = std::current_exception();
            }
            std::lock_guard<std::mutex> lock(mx);
            done = true;
            cv.notify_all();
        });
        std::exception_ptr thrown;
        try
        {
            for (;;)
            {
                RgbaImage frame;
                {
                    std::unique_lock<std::mutex> lock(mx);
                    cv.wait(lock, [&] { return !ready.empty() || done; });
                    if (ready.empty()) break;
                    frame = std::move(ready.front());
                    ready.pop_front();
                    cv.notify_all();
                }
                if (n == max_frames)
                {
                    r.more = true;
                    break;
                }
                each(frame);
                ++n;
            }
        }
        catch (...)
        {
            thrown = std::current_exception();
        }
        {
            std::lock_guard<std::mutex> lock(mx);
            stop = true;
            cv.notify_all();
        }
        reader.join();
        if (thrown) std::rethrow_exception(thrown);
        if (failed) std::rethrow_exception(failed);
    }
    if (n == 0)
    {
        const std::string e = read_text(err.path);
        throw std::runtime_error("cannot decode " + what + " with ffmpeg" + (e.empty() ? std::string() : ": " + e));
    }
    r.decoder = ffmpeg_version();
    if (r.decoder.empty()) r.decoder = "ffmpeg";
    r.complaints = read_text(err.path); // a damaged file: what ffmpeg said of the frames it could not read
    return r;
}

// Blocks of this many frames are handed on at a time.
constexpr size_t kAudioBlockFrames = 65536;

AudioRead ffmpeg_audio(const fs::path& file, const std::string& what, const AudioStart& start, const AudioBlock& block)
{
    const std::string exe = ffmpeg_or_throw("cannot decode " + what, "reads WAV");
    const TempFile err(".txt");
    // The first audio stream, as it is stored: 32-bit samples, at its own rate and channels, as a
    // WAV stream (its sizes unknown, so its data runs to the end).
    const std::string cmd = quoted(exe) + " -hide_banner -nostdin -v error -i " + quoted(u8(file)) +
                            " -vn -map 0:a:0? -flags +bitexact -fflags +bitexact -c:a pcm_s32le -f wav - 2> " + quoted(u8(err.path));
    AudioRead r;
    uint64_t frames = 0;
    {
        Pipe p(cmd);
        std::vector<uint8_t> head(12);
        if (p.read(reinterpret_cast<char*>(head.data()), 12) == 12 && is_wav(head))
        {
            std::optional<WavFormat> f;
            for (;;)
            {
                uint8_t ch[8];
                if (p.read(reinterpret_cast<char*>(ch), 8) != 8) break;
                const std::string id(reinterpret_cast<const char*>(ch), 4);
                const uint32_t size = uint32_t(ch[4]) | uint32_t(ch[5]) << 8 | uint32_t(ch[6]) << 16 | uint32_t(ch[7]) << 24;
                if (id == "data")
                {
                    if (!f) break;
                    r.rate = f->rate;
                    r.channels = f->channels;
                    start(r.rate, r.channels);
                    const size_t bytes = f->bits / 8;
                    std::vector<uint8_t> raw(kAudioBlockFrames * f->align);
                    std::vector<int32_t> samples;
                    for (;;)
                    {
                        const size_t got = p.read(reinterpret_cast<char*>(raw.data()), raw.size());
                        const size_t n = got / f->align;
                        if (n == 0) break;
                        samples.resize(n * f->channels);
                        for (size_t i = 0; i < samples.size(); ++i) samples[i] = wav_sample(*f, raw.data() + i * bytes);
                        block(samples);
                        frames += n;
                        if (got < raw.size()) break;
                    }
                    break;
                }
                std::vector<uint8_t> body(size + (size & 1));
                if (p.read(reinterpret_cast<char*>(body.data()), body.size()) != body.size()) break;
                if (id == "fmt ") f = wav_format(std::span<const uint8_t>(body.data(), size));
            }
        }
    }
    r.complaints = read_text(err.path);
    if (frames == 0)
        throw std::runtime_error("cannot decode " + what + ": it has no sound, or ffmpeg cannot read it" +
                                 (r.complaints.empty() ? std::string() : " (ffmpeg: " + r.complaints + ")"));
    r.decoder = ffmpeg_version();
    if (r.decoder.empty()) r.decoder = "ffmpeg";
    return r;
}

AudioRead own_wav(std::span<const uint8_t> bytes, const AudioStart& start, const AudioBlock& block)
{
    const PcmAudio a = read_wav(bytes);
    start(a.rate, a.channels);
    for (size_t at = 0; at < a.samples.size(); at += kAudioBlockFrames * a.channels)
        block(std::span<const int32_t>(a.samples.data() + at, std::min(a.samples.size() - at, kAudioBlockFrames * a.channels)));
    return {"sieve-wav", {}, a.rate, a.channels};
}

// Whether stb_image reads a file: the four signed kinds it reads by their signatures alone (a
// JPEG's header can lie past any head), the rest (TGA, PSD, PNM, ...) by its own test of the head.
bool stb_reads(const std::string& kind, const uint8_t* head, size_t size)
{
    if (kind == "PNG" || kind == "JPG" || kind == "GIF" || kind == "BMP") return true;
    uint32_t w = 0, h = 0;
    return image_info(head, size, w, h);
}

} // namespace

void set_ffmpeg_path(const std::string& path)
{
    std::lock_guard<std::mutex> lock(g_mx);
    g_path = path;
}

std::optional<std::string> find_ffmpeg()
{
    {
        std::lock_guard<std::mutex> lock(g_mx);
        if (!g_path.empty()) return g_path;
    }
    if (std::string e = env("SIEVE_FFMPEG"); !e.empty()) return e;
    for (const char* n : kNames)
        if (const fs::path p = executable_dir() / n; is_file(p)) return u8(p);
    if (const std::string all = env("PATH"); !all.empty())
    {
        size_t at = 0;
        while (at <= all.size())
        {
            size_t end = all.find(kPathSep, at);
            if (end == std::string::npos) end = all.size();
            if (end > at)
                for (const char* n : kNames)
                    if (const fs::path p = from_u8(all.substr(at, end - at)) / n; is_file(p)) return u8(p);
            at = end + 1;
        }
    }
    return std::nullopt;
}

std::string ffmpeg_version()
{
    const std::optional<std::string> exe = find_ffmpeg();
    if (!exe) return {};
    static std::mutex mx;
    static std::string for_exe, version;
    std::lock_guard<std::mutex> lock(mx);
    if (for_exe == *exe) return version;
    std::string out;
    try
    {
        out = run(quoted(*exe) + " -hide_banner -version");
    }
    catch (const std::exception&)
    {
        return {};
    }
    // "ffmpeg version 7.0.2-static https://... Copyright ..." -> "ffmpeg 7.0.2-static"
    const std::string head = "ffmpeg version ";
    version.clear();
    if (out.rfind(head, 0) == 0) version = "ffmpeg " + out.substr(head.size(), out.find_first_of(" \r\n", head.size()) - head.size());
    for_exe = *exe;
    return version;
}

MediaRead read_media_frames(const uint8_t* data, size_t size, const std::string& what, uint32_t max_frames, const EachFrame& each)
{
    if (max_frames == 0) throw std::invalid_argument("asked for no frames of " + what);
    if (stb_reads(file_kind(std::span<const uint8_t>(data, size), size), data, size))
    {
        // stb_image reads it: exactly as before. What it cannot decode after all, ffmpeg may.
        std::vector<RgbaImage> frames;
        try
        {
            frames = decode_image_frames(data, size, what);
        }
        catch (const std::exception&)
        {
            if (!find_ffmpeg()) throw;
        }
        if (!frames.empty())
        {
            for (const RgbaImage& f : frames) each(f);
            return {"stb_image", false, {}};
        }
    }
    const TempFile tmp(".media");
    {
        std::ofstream out(tmp.path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data), std::streamsize(size));
        if (!out) throw std::runtime_error("cannot write a temporary copy of " + what + " for ffmpeg");
    }
    return ffmpeg_frames(tmp.path, what, max_frames, each);
}

MediaRead read_media_frames(const std::string& path, uint32_t max_frames, const EachFrame& each)
{
    if (max_frames == 0) throw std::invalid_argument("asked for no frames of '" + path + "'");
    std::ifstream in(from_u8(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    // Only the head is needed to tell whether stb_image reads it; ffmpeg reads the file itself.
    std::error_code ec;
    const uint64_t size = std::filesystem::file_size(from_u8(path), ec);
    std::vector<uint8_t> head(1 << 16);
    in.read(reinterpret_cast<char*>(head.data()), std::streamsize(head.size()));
    head.resize(size_t(in.gcount()));
    if (stb_reads(file_kind(head, ec ? head.size() : size), head.data(), head.size()))
    {
        std::vector<RgbaImage> frames;
        try
        {
            frames = load_image_frames(path);
        }
        catch (const std::exception&)
        {
            if (!find_ffmpeg()) throw;
        }
        if (!frames.empty())
        {
            for (const RgbaImage& f : frames) each(f);
            return {"stb_image", false, {}};
        }
    }
    return ffmpeg_frames(from_u8(path), "'" + path + "'", max_frames, each);
}

AudioRead read_media_audio(const uint8_t* data, size_t size, const std::string& what, const AudioStart& start, const AudioBlock& block)
{
    const std::span<const uint8_t> bytes(data, size);
    if (is_wav(bytes))
    {
        // Sieve reads PCM and floating-point WAV itself; a WAV of another encoding (ADPCM, ...) is ffmpeg's.
        try
        {
            return own_wav(bytes, start, block);
        }
        catch (const std::invalid_argument&)
        {
            if (!find_ffmpeg()) throw;
        }
    }
    const TempFile tmp(".media");
    {
        std::ofstream out(tmp.path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data), std::streamsize(size));
        if (!out) throw std::runtime_error("cannot write a temporary copy of " + what + " for ffmpeg");
    }
    return ffmpeg_audio(tmp.path, what, start, block);
}

AudioRead read_media_audio(const std::string& path, const AudioStart& start, const AudioBlock& block)
{
    std::ifstream in(from_u8(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    std::vector<uint8_t> head(12);
    in.read(reinterpret_cast<char*>(head.data()), 12);
    head.resize(size_t(in.gcount()));
    if (is_wav(head))
    {
        in.seekg(0);
        const std::vector<uint8_t> all((std::istreambuf_iterator<char>(in)), {});
        try
        {
            return own_wav(all, start, block);
        }
        catch (const std::invalid_argument&)
        {
            if (!find_ffmpeg()) throw;
        }
    }
    return ffmpeg_audio(from_u8(path), "'" + path + "'", start, block);
}

bool ffmpeg_has_encoder(const std::string& name)
{
    const std::optional<std::string> exe = find_ffmpeg();
    if (!exe || !is_file(from_u8(*exe))) return false;
    static std::mutex mx;
    static std::string for_exe;
    static std::vector<std::string> encoders;
    std::lock_guard<std::mutex> lock(mx);
    if (for_exe != *exe)
    {
        encoders.clear();
        try
        {
            // Lines like " V....D libx264  H.264 ...": the flags, then the name.
            const std::string out = run(quoted(*exe) + " -hide_banner -encoders");
            size_t at = 0;
            while (at < out.size())
            {
                const size_t nl = std::min(out.find('\n', at), out.size());
                const std::string l = out.substr(at, nl - at);
                at = nl + 1;
                if (l.size() < 9 || l[0] != ' ' || l[7] != ' ') continue;
                const size_t end = l.find(' ', 8);
                encoders.push_back(l.substr(8, end == std::string::npos ? std::string::npos : end - 8));
            }
        }
        catch (const std::exception&)
        {
        }
        for_exe = *exe;
    }
    return std::find(encoders.begin(), encoders.end(), name) != encoders.end();
}

void ffmpeg_convert(const std::vector<std::string>& input_options, const std::string& input, const std::vector<std::string>& output_options,
                    const std::string& output)
{
    const std::string exe = ffmpeg_or_throw("cannot write '" + output + "'", "writes PNG, WAV and MIDI");
    const TempFile err(".txt");
    // A file already there (the one being saved over: ffmpeg's -y replaces it) goes first, so one
    // ffmpeg fails to write is not taken for its work below.
    {
        std::error_code ec;
        fs::remove(from_u8(output), ec);
    }
    // Every argument quoted on its own: a filter graph's ; and [ ] are the shell's otherwise.
    std::string cmd = quoted(exe) + " -hide_banner -nostdin -v error -y";
    for (const std::string& o : input_options) cmd += " " + quoted(o);
    cmd += " -i " + quoted(input);
    for (const std::string& o : output_options) cmd += " " + quoted(o);
    cmd += " " + quoted(output) + " 2> " + quoted(u8(err.path));
    run(cmd);
    std::error_code ec;
    if (!fs::exists(from_u8(output), ec) || fs::file_size(from_u8(output), ec) == 0)
    {
        const std::string e = read_text(err.path);
        throw std::runtime_error("ffmpeg could not write '" + output + "'" + (e.empty() ? std::string() : ": " + e));
    }
}

bool looks_like_mpeg_audio(std::span<const uint8_t> h)
{
    if (h.size() < 4 || h[0] != 0xFF) return false;
    if ((h[1] & 0xF6) == 0xF0) return ((h[2] >> 2) & 0x0F) < 13; // ADTS: a sampling index in use
    if ((h[1] & 0xE0) != 0xE0) return false;
    const uint8_t version = (h[1] >> 3) & 3, layer = (h[1] >> 1) & 3, bitrate = h[2] >> 4, rate = (h[2] >> 2) & 3;
    return version != 1 && layer != 0 && bitrate != 15 && rate != 3;
}

} // namespace sieve::cli
