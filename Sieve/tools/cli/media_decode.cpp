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
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h> // _open_osfhandle
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
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

#ifdef _WIN32
// One argument as the C runtime reads a command line back into arguments (CommandLineToArgvW's
// rules): in double quotes, a quote written \", and the backslashes before a quote, or before the
// closing one, doubled. No shell sees it, so % and the like are only characters.
std::wstring quoted(const std::wstring& s)
{
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (wchar_t c : s)
    {
        if (c == L'\\')
        {
            ++slashes;
            continue;
        }
        out.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        out += c;
    }
    out.append(slashes * 2, L'\\');
    return out + L"\"";
}
#endif

// A program run with its arguments, no shell between (so nothing in a path is ever read as the
// shell's), its standard output read as it comes and its standard error written to a file, or
// left where the caller's goes (on Windows, nowhere) when none is given.
class Pipe
{
public:
    Pipe(const std::vector<std::string>& args, const fs::path* err)
    {
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
        HANDLE rd = nullptr, wr = nullptr;
        if (!CreatePipe(&rd, &wr, &sa, 0)) throw std::runtime_error("cannot run ffmpeg");
        SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0); // ours alone
        const HANDLE in = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
        const HANDLE errh = CreateFileW(err ? err->c_str() : L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                                        err ? CREATE_ALWAYS : OPEN_EXISTING, 0, nullptr);
        std::wstring line;
        for (const std::string& a : args) line += (line.empty() ? L"" : L" ") + quoted(from_u8(a).wstring());
        // Only these three handles go to the child, so another thread's pipes are never held open by it.
        HANDLE pass[3] = {in, wr, errh};
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        std::vector<char> attrs(size);
        auto* list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs.data());
        bool ok = in != INVALID_HANDLE_VALUE && errh != INVALID_HANDLE_VALUE && InitializeProcThreadAttributeList(list, 1, 0, &size) &&
                  UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, pass, sizeof(pass), nullptr, nullptr);
        STARTUPINFOEXW si{};
        si.StartupInfo.cb = sizeof(si);
        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        si.StartupInfo.hStdInput = in;
        si.StartupInfo.hStdOutput = wr;
        si.StartupInfo.hStdError = errh;
        si.lpAttributeList = list;
        PROCESS_INFORMATION pi{};
        ok = ok && CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                                  nullptr, &si.StartupInfo, &pi);
        if (size) DeleteProcThreadAttributeList(list);
        CloseHandle(wr); // the child's now: the pipe ends when it does
        if (in != INVALID_HANDLE_VALUE) CloseHandle(in);
        if (errh != INVALID_HANDLE_VALUE) CloseHandle(errh);
        if (!ok)
        {
            CloseHandle(rd);
            throw std::runtime_error("cannot run ffmpeg");
        }
        CloseHandle(pi.hThread);
        process_ = pi.hProcess;
        const int fd = _open_osfhandle(intptr_t(rd), _O_RDONLY | _O_BINARY);
        p_ = fd >= 0 ? _fdopen(fd, "rb") : nullptr;
#else
        int fds[2];
        if (pipe(fds) != 0) throw std::runtime_error("cannot run ffmpeg");
        posix_spawn_file_actions_t fa;
        posix_spawn_file_actions_init(&fa);
        posix_spawn_file_actions_adddup2(&fa, fds[1], STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&fa, fds[0]);
        posix_spawn_file_actions_addclose(&fa, fds[1]);
        posix_spawn_file_actions_addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
        const std::string err_path = err ? u8(*err) : std::string();
        if (err) posix_spawn_file_actions_addopen(&fa, STDERR_FILENO, err_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        std::vector<char*> argv;
        for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        const int rc = posix_spawnp(&pid_, argv[0], &fa, nullptr, argv.data(), environ);
        posix_spawn_file_actions_destroy(&fa);
        ::close(fds[1]);
        if (rc != 0)
        {
            ::close(fds[0]);
            throw std::runtime_error("cannot run ffmpeg");
        }
        p_ = fdopen(fds[0], "rb");
#endif
        if (!p_)
        {
            close();
            throw std::runtime_error("cannot run ffmpeg");
        }
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
        // Closing our end first: a program still writing gets a broken pipe and stops.
        if (p_) fclose(p_);
        p_ = nullptr;
#ifdef _WIN32
        if (process_)
        {
            WaitForSingleObject(process_, INFINITE);
            DWORD code = 0;
            GetExitCodeProcess(process_, &code);
            status_ = int(code);
            CloseHandle(process_);
            process_ = nullptr;
        }
#else
        if (pid_ > 0)
        {
            int st = 0;
            while (waitpid(pid_, &st, 0) < 0 && errno == EINTR) {}
            status_ = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
            pid_ = -1;
        }
#endif
        return status_;
    }

private:
    FILE* p_ = nullptr;
    int status_ = 0;
#ifdef _WIN32
    HANDLE process_ = nullptr;
#else
    pid_t pid_ = -1;
#endif
};

// Runs a program and reads all it writes to its standard output.
std::string run(const std::vector<std::string>& args, const fs::path* err = nullptr)
{
    Pipe p(args, err);
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
    std::vector<std::string> cmd = {*exe, "-hide_banner", "-nostdin", "-v", "error", "-i", u8(file), "-map", "0:v:0"};
    if (max_frames < UINT32_MAX) cmd.insert(cmd.end(), {"-frames:v", std::to_string(uint64_t(max_frames) + 1)});
    cmd.insert(cmd.end(), {"-fps_mode", "passthrough", "-sws_flags", "+accurate_rnd+full_chroma_int+bitexact", "-flags", "+bitexact", "-pix_fmt",
                           "rgba", "-c:v", "pam", "-f", "image2pipe", "-"});
    MediaRead r;
    uint64_t n = 0;
    {
        // A reader thread takes frames from the pipe while this one fits the last: ffmpeg decodes
        // the next frame meanwhile, rather than waiting on a full pipe. Two frames are held at most.
        Pipe p(cmd, &err.path);
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
    const std::vector<std::string> cmd = {exe,         "-hide_banner", "-nostdin", "-v",        "error", "-i",   u8(file), "-vn",
                                          "-map",      "0:a:0?",      "-flags",   "+bitexact", "-fflags", "+bitexact", "-c:a",  "pcm_s32le",
                                          "-f",        "wav",         "-"};
    AudioRead r;
    uint64_t frames = 0;
    {
        Pipe p(cmd, &err.path);
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

// The same from a file, read a block at a time rather than whole: what read_wav() does, chunk by
// chunk (sieve/sound.hpp), and the same blocks handed on. Throws std::invalid_argument, as
// read_wav() does, on anything it does not read, before anything is handed on.
AudioRead own_wav_file(std::istream& in, uint64_t file_size, const AudioStart& start, const AudioBlock& block)
{
    auto le = [](const uint8_t* p, int n) {
        uint32_t v = 0;
        for (int i = n - 1; i >= 0; --i) v = v << 8 | p[i];
        return v;
    };
    uint8_t head[12];
    if (!in.read(reinterpret_cast<char*>(head), 12) || !is_wav(std::span<const uint8_t>(head, 12))) throw std::invalid_argument("not a WAV file");
    uint64_t at = 12;
    std::optional<WavFormat> f;
    while (at + 8 <= file_size)
    {
        uint8_t ch[8];
        in.seekg(std::streamoff(at));
        if (!in.read(reinterpret_cast<char*>(ch), 8)) break;
        const std::string id(reinterpret_cast<const char*>(ch), 4);
        const uint32_t size = le(ch + 4, 4);
        const uint64_t body = at + 8;
        if (id == "fmt ")
        {
            std::vector<uint8_t> fmt(size_t(std::min<uint64_t>(size, file_size - body)));
            in.read(reinterpret_cast<char*>(fmt.data()), std::streamsize(fmt.size()));
            f = wav_format(fmt);
        }
        else if (id == "data")
        {
            if (!f) throw std::invalid_argument("a WAV file's data comes before its fmt chunk");
            const uint64_t end = (size == 0 || size == 0xFFFFFFFFu) ? file_size : std::min(file_size, body + uint64_t(size));
            const uint64_t samples = (end - body) / f->align * f->channels;
            const size_t bytes = f->bits / 8;
            start(f->rate, f->channels);
            std::vector<uint8_t> raw;
            std::vector<int32_t> out;
            for (uint64_t done = 0; done < samples;)
            {
                const size_t n = size_t(std::min<uint64_t>(samples - done, uint64_t(kAudioBlockFrames) * f->channels));
                raw.resize(n * bytes);
                in.read(reinterpret_cast<char*>(raw.data()), std::streamsize(raw.size()));
                out.resize(n);
                for (size_t i = 0; i < n; ++i) out[i] = wav_sample(*f, raw.data() + i * bytes);
                block(out);
                done += n;
            }
            return {"sieve-wav", {}, f->rate, f->channels};
        }
        if (size == 0xFFFFFFFFu) break;
        at = body + size + (size & 1);
    }
    throw std::invalid_argument("a WAV file with no data chunk");
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
        out = run({*exe, "-hide_banner", "-version"});
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
        // Read a block at a time: a long recording is never held whole.
        in.clear();
        in.seekg(0, std::ios::end);
        const uint64_t size = uint64_t(in.tellg());
        in.seekg(0);
        try
        {
            return own_wav_file(in, size, start, block);
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
            const std::string out = run({*exe, "-hide_banner", "-encoders"});
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
    // Each argument as it is (no shell: a filter graph's ; and [ ] are only characters).
    std::vector<std::string> cmd = {exe, "-hide_banner", "-nostdin", "-v", "error", "-y"};
    cmd.insert(cmd.end(), input_options.begin(), input_options.end());
    cmd.insert(cmd.end(), {"-i", input});
    cmd.insert(cmd.end(), output_options.begin(), output_options.end());
    cmd.push_back(output);
    run(cmd, &err.path);
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
