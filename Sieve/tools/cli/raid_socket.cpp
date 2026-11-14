// Sieve — the RAID's protocol over TCP (see raid_socket.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "raid_socket.hpp"

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
using socket_t = SOCKET;
constexpr socket_t kNoSocket = INVALID_SOCKET;
#else
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t kNoSocket = -1;
#endif

namespace sieve_cli {

namespace {

constexpr size_t kAtOnce = 32; // connections answered at once; more wait to be accepted

void close_socket(socket_t s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

// Sockets need starting once on Windows; nothing elsewhere.
void start_sockets()
{
#ifdef _WIN32
    static const bool started = [] {
        WSADATA w;
        return WSAStartup(MAKEWORD(2, 2), &w) == 0;
    }();
    if (!started) throw std::runtime_error("Windows sockets could not be started");
#endif
}

void set_timeouts(socket_t s, int timeout_s)
{
    // Each request and answer goes at once, not held back to be joined with what follows (Nagle's
    // rule against a peer that delays its acknowledgement cost a put about 9 ms; review 1, 14 November
    // 2026). The bytes sent are the same.
    {
        const int on = 1;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof on);
    }
#ifdef _WIN32
    const DWORD ms = DWORD(timeout_s) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&ms), sizeof ms);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&ms), sizeof ms);
#else
    timeval tv{};
    tv.tv_sec = timeout_s;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
#ifdef SO_NOSIGPIPE
    const int one = 1; // macOS: a closed peer is an error, not a signal
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
#endif
#endif
}

class SocketStream : public sieve::RaidStream
{
public:
    explicit SocketStream(socket_t s) : s_(s) {}
    ~SocketStream() override { close_socket(s_); }
    bool read(char* to, size_t n) override
    {
        while (n)
        {
            const int want = int(n > (size_t(1) << 20) ? (size_t(1) << 20) : n);
            const auto got = recv(s_, to, want, 0);
            if (got <= 0) return false;
            to += got;
            n -= size_t(got);
        }
        return true;
    }
    bool write(std::string_view bytes) override
    {
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL; // Linux: a closed peer is an error, not a signal
#else
        const int flags = 0;
#endif
        while (!bytes.empty())
        {
            const int want = int(bytes.size() > (size_t(1) << 20) ? (size_t(1) << 20) : bytes.size());
            const auto put = send(s_, bytes.data(), want, flags);
            if (put <= 0) return false;
            bytes.remove_prefix(size_t(put));
        }
        return true;
    }

private:
    socket_t s_;
};

struct Addresses
{
    addrinfo* list = nullptr;
    ~Addresses()
    {
        if (list) freeaddrinfo(list);
    }
};

} // namespace

std::unique_ptr<sieve::RaidStream> raid_connect(const std::string& address, int timeout_s)
{
    start_sockets();
    const auto [host, port] = sieve::raid_split_address(address);
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    Addresses a;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &a.list) != 0) throw std::runtime_error("cannot find " + address);
    for (addrinfo* i = a.list; i; i = i->ai_next)
    {
        const socket_t s = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (s == kNoSocket) continue;
        set_timeouts(s, timeout_s);
        if (connect(s, i->ai_addr, int(i->ai_addrlen)) == 0) return std::make_unique<SocketStream>(s);
        close_socket(s);
    }
    throw std::runtime_error("cannot connect to " + address);
}

void raid_listen(const std::string& address, size_t connections, int timeout_s, const std::function<void()>& ready,
                 const std::function<void(sieve::RaidStream&, const std::string&)>& on, const std::atomic<bool>* stop)
{
    start_sockets();
    const auto [host, port] = sieve::raid_split_address(address);
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    Addresses a;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &a.list) != 0 || !a.list) throw std::runtime_error("cannot find " + address);
    const socket_t l = socket(a.list->ai_family, a.list->ai_socktype, a.list->ai_protocol);
    if (l == kNoSocket) throw std::runtime_error("cannot make a socket for " + address);
    const int one = 1;
    setsockopt(l, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof one);
    if (bind(l, a.list->ai_addr, int(a.list->ai_addrlen)) != 0 || listen(l, 8) != 0)
    {
        close_socket(l);
        throw std::runtime_error("cannot listen on " + address + " (in use, or not this machine's address)");
    }
    ready();
    // Each connection on a thread of its own, so one slow or silent peer does not keep the others
    // waiting (until its timeout); at most kAtOnce at a time, the next accepted as one finishes.
    struct Worker
    {
        std::thread t;
        std::shared_ptr<std::atomic<bool>> done;
    };
    std::list<Worker> workers;
    std::mutex mx;
    std::condition_variable cv;
    size_t running = 0;
    auto reap = [&] {
        for (auto i = workers.begin(); i != workers.end();)
            if (i->done->load())
            {
                i->t.join();
                i = workers.erase(i);
            }
            else ++i;
    };
    for (size_t n = 0; connections == 0 || n < connections; ++n)
    {
        {
            std::unique_lock<std::mutex> lock(mx);
            cv.wait(lock, [&] { return running < kAtOnce; });
        }
        reap();
        if (stop)
        {
            // Waiting for a connection a second at a time, so a stop is seen.
            bool ready_now = false;
            while (!stop->load() && !ready_now)
            {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(l, &fds);
                timeval tv{};
                tv.tv_sec = 1;
                ready_now = select(int(l) + 1, &fds, nullptr, nullptr, &tv) > 0;
            }
            if (!ready_now) break;
        }
        sockaddr_storage from{};
        socklen_t from_len = sizeof from;
        const socket_t c = accept(l, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (c == kNoSocket) continue;
        char ip[NI_MAXHOST] = {};
        if (getnameinfo(reinterpret_cast<sockaddr*>(&from), from_len, ip, sizeof ip, nullptr, 0, NI_NUMERICHOST) != 0) ip[0] = '\0';
        set_timeouts(c, timeout_s);
        {
            std::lock_guard<std::mutex> lock(mx);
            ++running;
        }
        auto done = std::make_shared<std::atomic<bool>>(false);
        workers.push_back({std::thread([&on, &mx, &cv, &running, c, done, address = std::string(ip)] {
                               {
                                   SocketStream s(c);
                                   try
                                   {
                                       on(s, address);
                                   }
                                   catch (const std::exception&)
                                   {
                                       // one connection's trouble is not the listener's: the others are answered
                                   }
                               }
                               {
                                   std::lock_guard<std::mutex> lock(mx);
                                   --running;
                               }
                               done->store(true);
                               cv.notify_all();
                           }),
                           done});
    }
    for (auto& w : workers) w.t.join(); // (the last connections answered before it stops)
    close_socket(l);
}

} // namespace sieve_cli
