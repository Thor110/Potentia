// Reading the menus aloud (speech.hpp).
#include "speech.hpp"
#include "strings.hpp"

#include "cli/voice.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <sapi.h>
#endif

namespace hallway {

namespace {

std::atomic<bool> g_on{false};
std::atomic<bool> g_voice_on{false}; // Text to Voice
std::string g_voiced;                // (under g_mx)
std::atomic<bool> g_started{false}; // (the voice made, so turning off may cut it short)
std::mutex g_mx;
std::string g_last;

#ifdef _WIN32
// One thread of its own speaks, with COM started on it, so the menus' thread is never kept waiting
// and the program's own COM use (the folder picker) is left alone. It takes the newest text only.
struct Voice
{
    std::mutex mx;
    std::condition_variable cv;
    std::string next;
    bool pending = false;
    std::thread worker;
    std::atomic<bool> ok{true};

    Voice()
    {
        worker = std::thread([this] {
            if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
            {
                ok = false;
                return;
            }
            ISpVoice* voice = nullptr;
            if (FAILED(CoCreateInstance(__uuidof(SpVoice), nullptr, CLSCTX_ALL, __uuidof(ISpVoice), reinterpret_cast<void**>(&voice))) || !voice)
            {
                ok = false;
                CoUninitialize();
                return;
            }
            for (;;)
            {
                std::string text;
                {
                    std::unique_lock<std::mutex> lock(mx);
                    cv.wait(lock, [this] { return pending; });
                    text = std::move(next);
                    pending = false;
                }
                if (text == std::string("\0quit", 5)) break;
                const int n = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
                std::wstring w(size_t(n > 0 ? n : 0), L'\0');
                if (n > 0) MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), w.data(), n);
                // Asynchronous, cutting short what is being said; plain text, not SAPI's XML.
                voice->Speak(w.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, nullptr);
            }
            voice->Release();
            CoUninitialize();
        });
    }
    ~Voice()
    {
        say(std::string("\0quit", 5));
        if (worker.joinable()) worker.join();
    }
    void say(const std::string& text)
    {
        {
            std::lock_guard<std::mutex> lock(mx);
            next = text;
            pending = true;
        }
        cv.notify_one();
    }
};

Voice& voice()
{
    static Voice v; // (started the first time something is said, and ended with the program)
    g_started = true;
    return v;
}
#endif

// Sieve's own voice (cli/voice.hpp): loaded the first time it speaks, on a thread of its own, which
// makes each reply's samples there; the frame takes them and plays them (pump_speech), so SDL's
// audio is only ever touched from the frame's thread. The newest reply only: one cuts short the last.
struct SieveVoice
{
    std::mutex mx;
    std::condition_variable cv;
    std::string next;
    bool pending = false, quit = false;
    std::vector<int16_t> ready;
    bool has_ready = false;
    std::unique_ptr<sieve::cli::Voice> voice;
    bool tried = false;
    std::thread worker;

    ~SieveVoice()
    {
        {
            std::lock_guard<std::mutex> lock(mx);
            quit = true;
        }
        cv.notify_one();
        if (worker.joinable()) worker.join();
    }
    void say(const std::string& text)
    {
        {
            std::lock_guard<std::mutex> lock(mx);
            next = text;
            pending = true;
            if (!worker.joinable()) worker = std::thread([this] { run(); });
        }
        cv.notify_one();
    }
    void run()
    {
        for (;;)
        {
            std::string text;
            {
                std::unique_lock<std::mutex> lock(mx);
                cv.wait(lock, [this] { return pending || quit; });
                if (quit) return;
                text = std::move(next);
                pending = false;
            }
            if (!tried)
            {
                tried = true;
                try
                {
                    voice = sieve::cli::Voice::find();
                }
                catch (const std::exception& e)
                {
                    SDL_Log("Text to Voice: %s", e.what());
                }
            }
            if (!voice) continue;
            std::vector<int16_t> pcm = voice->speak(text);
            std::lock_guard<std::mutex> lock(mx);
            ready = std::move(pcm);
            has_ready = true;
        }
    }
};

SieveVoice& sieve_voice()
{
    static SieveVoice v;
    return v;
}

SDL_AudioStream* g_voice_stream = nullptr;

} // namespace

bool sieve_voice_available()
{
    static const bool there = [] {
        try
        {
            return sieve::cli::Voice::where().has_value();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }();
    return there;
}

void pump_speech()
{
    std::vector<int16_t> pcm;
    {
        SieveVoice& v = sieve_voice();
        std::lock_guard<std::mutex> lock(v.mx);
        if (!v.has_ready) return;
        pcm = std::move(v.ready);
        v.has_ready = false;
    }
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
    if (!g_voice_stream)
    {
        const SDL_AudioSpec spec{SDL_AUDIO_S16, 1, int(sieve::cli::Voice::kRate)};
        g_voice_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!g_voice_stream) return;
    }
    SDL_ClearAudioStream(g_voice_stream); // (the reply before, cut short)
    SDL_PutAudioStreamData(g_voice_stream, pcm.data(), int(pcm.size() * sizeof(int16_t)));
    SDL_FlushAudioStream(g_voice_stream);
    SDL_ResumeAudioStreamDevice(g_voice_stream);
}

void stop_speech()
{
    if (g_voice_stream)
    {
        SDL_DestroyAudioStream(g_voice_stream);
        g_voice_stream = nullptr;
    }
}

void set_read_aloud(bool on)
{
    g_on = on;
    {
        std::lock_guard<std::mutex> lock(g_mx);
        g_last.clear(); // (turned on, the item chosen is said at once)
    }
#ifdef _WIN32
    if (!on && g_started) voice().say(" "); // (turned off: what is being said is cut short)
#endif
}

void forget_announced()
{
    std::lock_guard<std::mutex> lock(g_mx);
    g_last.clear();
}

bool read_aloud() { return g_on; }

std::string spoken(const std::string& shown)
{
    std::string s = shown, state;
    if (s.rfind("[x] ", 0) == 0 || s.rfind("[ ] ", 0) == 0)
    {
        state = tr(s[1] == 'x' ? "value.on" : "value.off");
        s = s.substr(4);
    }
    std::string out;
    for (const char c : s)
        if (c != ' ' || (!out.empty() && out.back() != ' ')) out += c;
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (state.empty()) return out;
    // The state after the name, before what follows it ("max-run-v1, on, Filtered: 0.1%").
    const size_t gap = shown.find("  ", 4);
    if (gap == std::string::npos) return out + ", " + state;
    const std::string name = spoken(shown.substr(4, gap - 4)), rest = spoken(shown.substr(gap));
    return name + ", " + state + (rest.empty() ? "" : ", " + rest);
}

std::string last_announced()
{
    std::lock_guard<std::mutex> lock(g_mx);
    return g_last;
}

bool speech_available()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

void set_text_to_voice(bool on)
{
    g_voice_on = on;
#ifdef _WIN32
    if (!on && g_started) voice().say(" "); // (turned off: a reply being spoken is cut short)
#endif
    if (!on && g_voice_stream) SDL_ClearAudioStream(g_voice_stream);
}

bool text_to_voice() { return g_voice_on; }

namespace {
// A reply as it can be said: valid UTF-8 kept, control bytes and broken sequences as spaces (a
// model's reply is bytes, and an untrained one's are mostly not text), runs of spaces as one.
std::string sayable(const std::string& text)
{
    std::string out;
    auto space = [&] {
        if (!out.empty() && out.back() != ' ') out += ' ';
    };
    for (size_t i = 0; i < text.size();)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        const size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
        bool ok = n > 0 && i + n <= text.size() && (n > 1 || (c >= 0x20 && c != 0x7F));
        for (size_t k = 1; ok && k < n; ++k) ok = (static_cast<unsigned char>(text[i + k]) & 0xC0) == 0x80;
        if (!ok || c == ' ')
        {
            space();
            ++i;
            continue;
        }
        out.append(text, i, n);
        i += n;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}
} // namespace

void speak_reply(const std::string& reply)
{
    const std::string text = sayable(reply);
    if (!g_voice_on || text.empty()) return;
    {
        std::lock_guard<std::mutex> lock(g_mx);
        g_voiced = text;
        g_last.clear(); // (the menus' next item is said again after it)
    }
    // Sieve's own voice where it is here; else the system's (Windows).
    if (sieve_voice_available())
    {
        sieve_voice().say(text);
        return;
    }
#ifdef _WIN32
    if (voice().ok) voice().say(text);
#endif
}

std::string last_voiced()
{
    std::lock_guard<std::mutex> lock(g_mx);
    return g_voiced;
}

void announce(const std::string& text)
{
    if (!g_on || text.empty()) return;
    {
        std::lock_guard<std::mutex> lock(g_mx);
        if (text == g_last) return;
        g_last = text;
    }
#ifdef _WIN32
    if (voice().ok) voice().say(text);
#endif
}

} // namespace hallway
