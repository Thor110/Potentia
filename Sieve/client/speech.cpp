// Reading the menus aloud (speech.hpp).
#include "speech.hpp"

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

} // namespace

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

bool speech_available()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
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
