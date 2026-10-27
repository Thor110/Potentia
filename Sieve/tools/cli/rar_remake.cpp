// A RAR made again by the machine's own rar (rar_remake.hpp). The RAR 5 headers are read here, after
// RARLAB's published description of the format ("RAR 5.0 archive format", rarlab.com/technote.htm):
// only the headers, never the compressed data, which libarchive reads.

#include "cli/rar_remake.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace sieve::cli::rar {

namespace fs = std::filesystem;

namespace {

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

uint32_t crc32_of(const uint8_t* p, size_t n)
{
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// Reads a header's fields; throws at its end.
struct Reader
{
    const uint8_t* p;
    size_t n, at = 0;
    uint64_t vint()
    {
        uint64_t v = 0;
        for (int shift = 0; shift < 70; shift += 7)
        {
            if (at >= n) throw std::runtime_error("short");
            const uint8_t c = p[at++];
            v |= uint64_t(c & 0x7F) << shift;
            if (!(c & 0x80)) return v;
        }
        throw std::runtime_error("long");
    }
    uint64_t le(int bytes)
    {
        if (at + size_t(bytes) > n) throw std::runtime_error("short");
        uint64_t v = 0;
        for (int i = 0; i < bytes; ++i) v |= uint64_t(p[at + size_t(i)]) << (8 * i);
        at += size_t(bytes);
        return v;
    }
    std::string bytes(size_t k)
    {
        if (at + k > n) throw std::runtime_error("short");
        std::string s(reinterpret_cast<const char*>(p + at), k);
        at += k;
        return s;
    }
};

constexpr int64_t kFiletimeAt1970 = 116444736000000000LL; // 100 ns units from 1601 to 1970

std::string kib(uint64_t k) { return std::to_string(k) + "k"; }

} // namespace

std::optional<Archive> read_headers(const std::vector<uint8_t>& b)
{
    static const uint8_t sig[8] = {'R', 'a', 'r', '!', 0x1A, 0x07, 0x01, 0x00};
    if (b.size() < 8 || std::memcmp(b.data(), sig, 8) != 0) return std::nullopt;
    Archive a;
    bool solid = false, locked = false, recovery = false, quick_open = false, blake2 = false, owner = false;
    bool low_mtime = false, no_mtime = false, ctimes = false, atimes = false, unix_ns = false, unix_seconds = false;
    int method = -1;
    uint64_t dict = 0;
    bool ended = false;
    auto refuse = [&](const std::string& why) {
        if (a.refused.empty()) a.refused = why;
    };
    try
    {
        size_t pos = 8;
        while (pos < b.size() && !ended)
        {
            if (b.size() - pos < 6) return std::nullopt;
            const uint32_t crc = uint32_t(b[pos]) | uint32_t(b[pos + 1]) << 8 | uint32_t(b[pos + 2]) << 16 | uint32_t(b[pos + 3]) << 24;
            Reader size_r{b.data() + pos + 4, b.size() - pos - 4};
            const uint64_t hsize = size_r.vint();
            const size_t start = pos + 4 + size_r.at; // the header's fields, after its size
            if (hsize > b.size() - start) return std::nullopt;
            if (crc32_of(b.data() + pos + 4, size_r.at + size_t(hsize)) != crc) return std::nullopt;
            Reader r{b.data() + start, size_t(hsize)};
            const uint64_t type = r.vint(), flags = r.vint();
            const uint64_t extra = flags & 1 ? r.vint() : 0;
            const uint64_t data = flags & 2 ? r.vint() : 0;
            const size_t extra_at = size_t(hsize) - size_t(extra);
            if (extra > hsize) return std::nullopt;
            if (type == 1) // the main header
            {
                const uint64_t af = r.vint();
                if (af & 1) refuse("it is one volume of several");
                solid = af & 4;
                recovery = recovery || (af & 8);
                locked = af & 16;
            }
            else if (type == 2 || type == 3) // a file, or a service header (a comment, quick open, recovery)
            {
                const uint64_t ff = r.vint();
                r.vint(); // its size unpacked
                const uint64_t attributes = r.vint();
                std::optional<int64_t> mtime;
                if (ff & 2) mtime = int64_t(r.le(4)) * 1000000000LL;
                if (ff & 4) r.le(4); // its CRC32
                const uint64_t ci = r.vint(), host = r.vint(), name_len = r.vint();
                const std::string name = r.bytes(size_t(name_len));
                Member m;
                m.path = name;
                m.dir = ff & 1;
                m.attributes = attributes;
                bool hp_mtime = false;
                // Its extra area: its records.
                Reader x{b.data() + start + extra_at, size_t(extra)};
                while (x.at < x.n)
                {
                    const uint64_t rsize = x.vint();
                    const size_t rend = x.at + size_t(rsize);
                    if (rsize > x.n - x.at) return std::nullopt;
                    const uint64_t rtype = x.vint();
                    if (type == 2)
                    {
                        if (rtype == 1) refuse("it is encrypted");
                        else if (rtype == 2) blake2 = true;
                        else if (rtype == 3) // its times, to more than the second
                        {
                            const uint64_t tf = x.vint();
                            const bool unix_format = tf & 1;
                            std::optional<int64_t> t[3];
                            for (int k = 0; k < 3; ++k)
                                if (tf & (2u << k))
                                {
                                    if (unix_format) t[k] = int64_t(x.le(4)) * 1000000000LL;
                                    else t[k] = (int64_t(x.le(8)) - kFiletimeAt1970) * 100;
                                }
                            if (unix_format && (tf & 16))
                                for (int k = 0; k < 3; ++k)
                                    if (t[k]) *t[k] += int64_t(x.le(4) & 0x3FFFFFFF);
                            if (unix_format) (tf & 16 ? unix_ns : unix_seconds) = true;
                            if (t[0])
                            {
                                mtime = t[0];
                                hp_mtime = true;
                            }
                            if (t[1]) (m.ctime = t[1], ctimes = true);
                            if (t[2]) (m.atime = t[2], atimes = true);
                        }
                        else if (rtype == 4) refuse("it keeps versions of its files");
                        else if (rtype == 5) refuse("it holds links");
                        else if (rtype == 6) owner = true;
                    }
                    x.at = rend;
                }
                if (type == 3)
                {
                    if (name == "QO") quick_open = true;
                    else if (name == "RR") recovery = true;
                    else if (name == "CMT") refuse("it has a comment");
                    else refuse("it holds a " + name + " record");
                }
                else
                {
                    m.mtime = mtime;
                    if (!mtime) no_mtime = true;
                    else if (!hp_mtime) low_mtime = true;
                    if (a.host < 0) a.host = int(host);
                    else if (a.host != int(host)) refuse("its files were added on different systems");
                    if (!m.dir)
                    {
                        const int mth = int((ci >> 7) & 7);
                        if (method < 0) method = mth;
                        else if (method != mth) refuse("its files were packed at different levels");
                        if (mth) dict = std::max<uint64_t>(dict, uint64_t(128) << ((ci >> 10) & 31));
                    }
                    a.members.push_back(std::move(m));
                }
            }
            else if (type == 4) refuse("it is encrypted");
            else if (type == 5)
            {
                if (r.vint() & 1) refuse("it is one volume of several");
                ended = true;
            }
            pos = start + size_t(hsize) + size_t(data);
            if (data > b.size() || pos > b.size()) return std::nullopt;
        }
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
    if (!ended) return std::nullopt;
    if (recovery) refuse("it has a recovery record");
    if (a.members.empty()) refuse("it is empty");
    // The switches its headers give. (rar's own defaults are left alone where the headers agree
    // with them; -cfg- is given everywhere, so no rar.ini and no RAR variable take part.)
    std::vector<std::string> sw;
    if (method >= 0) sw.push_back("-m" + std::to_string(method));
    if (solid) sw.push_back("-s");
    if (locked) sw.push_back("-k");
    if (blake2) sw.push_back("-htb");
    if (owner) sw.push_back("-ow");
    if (no_mtime) sw.push_back("-tsm-");
    else if (low_mtime) sw.push_back("-tsm1");
    if (ctimes) sw.push_back("-tsc");
    if (atimes) sw.push_back("-tsa");
    sw.push_back(quick_open ? "-qo+" : "-qo-");
    auto joined = [](const std::vector<std::string>& v) {
        std::string s;
        for (const std::string& w : v) s += (s.empty() ? "" : " ") + w;
        return s;
    };
    a.switches = joined(sw);
    // Others to try: the dictionary named (rar may pick a smaller one for small files itself), and
    // quick open left to rar.
    std::vector<std::string> with_dict = sw;
    if (dict) with_dict.insert(with_dict.begin() + 1, "-md" + kib(dict));
    std::vector<std::string> qo_default;
    for (const std::string& w : sw)
        if (w.rfind("-qo", 0) != 0) qo_default.push_back(w);
    if (dict) a.alternatives.push_back(joined(with_dict));
    a.alternatives.push_back(joined(qo_default));
    (void)unix_ns;
    (void)unix_seconds;
    return a;
}

// ---- the program

namespace {

#ifdef _WIN32
std::wstring quoted(const std::wstring& s)
{
    if (!s.empty() && s.find_first_of(L" \t\"") == std::wstring::npos) return s;
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

// A program run in `cwd` with its arguments (no shell between), what it prints (both outputs) in
// `said`; its exit status, or -1 if it could not be run.
int run(const std::vector<std::string>& args, const fs::path& cwd, std::string* said)
{
    static std::random_device rd;
    const fs::path log = fs::temp_directory_path() / ("sieve-rar-" + std::to_string(rd()) + std::to_string(rd()) + ".txt");
    int status = -1;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    const HANDLE out = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, 0, nullptr);
    const HANDLE in = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
    std::wstring line;
    for (const std::string& a : args) line += (line.empty() ? L"" : L" ") + quoted(from_u8(a).wstring());
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in;
    si.hStdOutput = out;
    si.hStdError = out;
    PROCESS_INFORMATION pi{};
    const std::wstring dir = cwd.wstring();
    if (out != INVALID_HANDLE_VALUE && in != INVALID_HANDLE_VALUE &&
        CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, dir.empty() ? nullptr : dir.c_str(), &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(pi.hProcess, &code);
        status = int(code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    if (out != INVALID_HANDLE_VALUE) CloseHandle(out);
    if (in != INVALID_HANDLE_VALUE) CloseHandle(in);
#else
    const std::string log8 = u8(log), cwd8 = u8(cwd);
    std::vector<char*> argv;
    for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    const pid_t pid = fork();
    if (pid == 0)
    {
        const int o = open(log8.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644), i = open("/dev/null", O_RDONLY);
        if (o >= 0) (dup2(o, 1), dup2(o, 2));
        if (i >= 0) dup2(i, 0);
        if (!cwd8.empty() && chdir(cwd8.c_str()) != 0) _exit(127);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    if (pid > 0)
    {
        int st = 0;
        while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {}
        status = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
        if (status == 127) status = -1;
    }
#endif
    if (said)
    {
        std::ifstream f(log, std::ios::binary);
        std::ostringstream s;
        s << f.rdbuf();
        *said = s.str();
    }
    std::error_code ec;
    fs::remove(log, ec);
    return status;
}

fs::path own_folder()
{
#ifdef _WIN32
    wchar_t buf[32768];
    const DWORD n = GetModuleFileNameW(nullptr, buf, 32768);
    return n ? fs::path(std::wstring(buf, n)).parent_path() : fs::path();
#else
    std::error_code ec;
    const fs::path p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : p.parent_path();
#endif
}

#ifdef _WIN32
const char* kNames[] = {"rar.exe", "Rar.exe"};
#else
const char* kNames[] = {"rar"};
#endif

std::vector<fs::path> path_folders()
{
    std::vector<fs::path> out;
    const char* p = std::getenv("PATH");
    if (!p) return out;
#ifdef _WIN32
    const char sep = ';';
#else
    const char sep = ':';
#endif
    std::string all = p;
    for (size_t at = 0; at <= all.size();)
    {
        size_t e = all.find(sep, at);
        if (e == std::string::npos) e = all.size();
        if (e > at) out.push_back(fs::path(all.substr(at, e - at)));
        at = e + 1;
    }
    return out;
}

} // namespace

std::optional<fs::path> find_program()
{
    std::error_code ec;
    if (const char* s = std::getenv("SIEVE_RAR"); s && *s) return fs::is_regular_file(from_u8(s), ec) ? std::optional<fs::path>(from_u8(s)) : std::nullopt;
    std::vector<fs::path> where{own_folder()};
    for (const fs::path& d : path_folders()) where.push_back(d);
#ifdef _WIN32
    for (const char* v : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"})
        if (const char* pf = std::getenv(v)) where.push_back(fs::path(pf) / "WinRAR");
#endif
    for (const fs::path& d : where)
        for (const char* n : kNames)
            if (!d.empty() && fs::is_regular_file(d / n, ec)) return d / n;
    return std::nullopt;
}

std::string looked()
{
#ifdef _WIN32
    return "looked in SIEVE_RAR, beside Sieve, on the PATH and in WinRAR's folder";
#else
    return "looked in SIEVE_RAR, beside Sieve and on the PATH";
#endif
}

std::string program_version(const fs::path& program)
{
    std::string said;
    run({u8(program)}, fs::path(), &said);
    // Its banner: "RAR 7.01 x64   Copyright (c) 1993-2024 Alexander Roshal   12 May 2024".
    std::istringstream in(said);
    std::string line;
    while (std::getline(in, line))
    {
        const size_t c = line.find("Copyright");
        if (line.rfind("RAR ", 0) != 0 || c == std::string::npos) continue;
        std::string v = line.substr(0, c);
        while (!v.empty() && (v.back() == ' ' || v.back() == '\t' || v.back() == '\r')) v.pop_back();
        return v;
    }
    return "";
}

bool set_times(const fs::path& folder, const std::vector<Member>& members, std::string& why)
{
    // Folders last, the deepest first: writing a file inside one changes its time.
    std::vector<const Member*> order;
    for (const Member& m : members)
        if (!m.dir) order.push_back(&m);
    std::vector<const Member*> dirs;
    for (const Member& m : members)
        if (m.dir) dirs.push_back(&m);
    std::sort(dirs.begin(), dirs.end(), [](const Member* x, const Member* y) { return x->path.size() > y->path.size(); });
    order.insert(order.end(), dirs.begin(), dirs.end());
    for (const Member* m : order)
    {
        const fs::path p = folder / from_u8(m->path);
#ifdef _WIN32
        if (!SetFileAttributesW(p.c_str(), DWORD(m->attributes))) return (why = "cannot set the attributes of " + m->path, false);
        const HANDLE h = CreateFileW(p.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                     OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE) return (why = "cannot set the times of " + m->path, false);
        auto ft = [](const std::optional<int64_t>& t, FILETIME& f) {
            if (!t) return false;
            const uint64_t v = uint64_t(*t / 100 + kFiletimeAt1970);
            f.dwLowDateTime = DWORD(v);
            f.dwHighDateTime = DWORD(v >> 32);
            return true;
        };
        FILETIME c{}, a{}, w{};
        const bool hc = ft(m->ctime, c), ha = ft(m->atime, a), hw = ft(m->mtime, w);
        const bool ok = SetFileTime(h, hc ? &c : nullptr, ha ? &a : nullptr, hw ? &w : nullptr);
        CloseHandle(h);
        if (!ok) return (why = "cannot set the times of " + m->path, false);
#else
        if (m->ctime) return (why = "it keeps creation times, which cannot be set here", false);
        if (::chmod(u8(p).c_str(), mode_t(m->attributes & 07777)) != 0) return (why = "cannot set the mode of " + m->path, false);
        struct timespec ts[2];
        ts[0].tv_nsec = UTIME_OMIT;
        ts[1].tv_nsec = UTIME_OMIT;
        if (m->atime) (ts[0].tv_sec = time_t(*m->atime / 1000000000LL), ts[0].tv_nsec = long(*m->atime % 1000000000LL));
        if (m->mtime) (ts[1].tv_sec = time_t(*m->mtime / 1000000000LL), ts[1].tv_nsec = long(*m->mtime % 1000000000LL));
        if (utimensat(AT_FDCWD, u8(p).c_str(), ts, 0) != 0) return (why = "cannot set the times of " + m->path, false);
#endif
    }
    return true;
}

bool switches_known(const std::string& switches)
{
    static const std::set<std::string> fixed = {"-s", "-k", "-htb", "-ow", "-tsm-", "-tsm1", "-tsc", "-tsa", "-qo+", "-qo-"};
    auto digits = [](const std::string& t) { return !t.empty() && t.size() <= 10 && t.find_first_not_of("0123456789") == std::string::npos; };
    std::istringstream in(switches);
    for (std::string w; in >> w;)
    {
        if (fixed.count(w)) continue;
        if (w.size() == 3 && w.compare(0, 2, "-m") == 0 && w[2] >= '0' && w[2] <= '9') continue; // -m3
        if (w.size() > 4 && w.compare(0, 3, "-md") == 0 && w.back() == 'k' && digits(w.substr(3, w.size() - 4))) continue; // -md4096k
        return false;
    }
    return true;
}

int make(const fs::path& program, const fs::path& folder, const std::vector<Member>& members, const std::string& how, const std::string& switches,
         const fs::path& out_in, std::string* said)
{
    const fs::path out = fs::absolute(out_in); // (rar runs in the folder)
    std::vector<std::string> args{u8(program), "a", "-cfg-", "-idq", "-y"};
    std::istringstream sw(switches);
    for (std::string w; sw >> w;) args.push_back(w);
    fs::path list;
    if (how == "top")
    {
        args.push_back("-r");
        args.push_back("--");
        args.push_back(u8(out));
        std::set<std::string> seen;
        for (const Member& m : members)
        {
            const std::string top = m.path.substr(0, m.path.find('/'));
            if (seen.insert(top).second) args.push_back(top);
        }
    }
    else // "list": every member, in the archive's order, in a list file (UTF-8)
    {
        list = out;
        list += ".list";
        std::ofstream f(list, std::ios::binary | std::ios::trunc);
        for (const Member& m : members)
        {
            std::string p = m.path;
#ifdef _WIN32
            std::replace(p.begin(), p.end(), '/', '\\');
#endif
            f << p << "\n";
        }
        f.close();
        args.push_back("-scfl");
        args.push_back("--");
        args.push_back(u8(out));
        args.push_back("@" + u8(list));
    }
    std::error_code ec;
    fs::remove(out, ec);
    const int status = run(args, folder, said);
    if (!list.empty()) fs::remove(list, ec);
    return status;
}

} // namespace sieve::cli::rar
