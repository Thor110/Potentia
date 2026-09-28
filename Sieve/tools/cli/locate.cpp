// The file locator and the manifest creator (locate.hpp).

#include "cli/locate.hpp"
#include "cli/vault.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace sieve::cli {

namespace fs = std::filesystem;

std::vector<uint8_t> read_file_bytes(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + file.generic_string());
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

BigUint binary_address(const std::vector<uint8_t>& bytes)
{
    // binary-v1: the bytes as one big-endian number, plus how many files are shorter, which is
    // "01" written once for each byte (binaryspace.hpp). Two hex conversions and an addition.
    static constexpr char kHex[] = "0123456789abcdef";
    std::string h, ones;
    h.reserve(bytes.size() * 2);
    ones.reserve(bytes.size() * 2);
    for (uint8_t b : bytes)
    {
        h += kHex[b >> 4];
        h += kHex[b & 15];
        ones += "01";
    }
    if (bytes.empty()) return BigUint();
    BigUint v = BigUint::from_hex(h);
    v += BigUint::from_hex(ones);
    return v;
}

std::vector<uint8_t> file_at(const BigUint& address)
{
    // Its length: the largest L with (256^L - 1) / 255 <= address, i.e. 256^L <= 255 a + 1.
    BigUint t = address;
    t.mul_small(255);
    t.add_small(1);
    const size_t length = (t.bit_length() - 1) / 8;
    std::vector<uint8_t> out(length);
    if (length == 0) return out;
    std::string ones;
    ones.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) ones += "01";
    BigUint v = address;
    v -= BigUint::from_hex(ones);
    const std::string h = v.to_hex(length * 2);
    auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
    for (size_t i = 0; i < length; ++i) out[i] = uint8_t(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
    return out;
}

std::string sha256_hex(const std::vector<uint8_t>& bytes)
{
    Sha256 s;
    s.update(std::span<const uint8_t>(bytes.data(), bytes.size()));
    return Sha256::hex(s.finish());
}

std::string sha256_file_hex(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + file.generic_string());
    Sha256 s;
    std::vector<char> buf(1 << 20);
    while (in)
    {
        in.read(buf.data(), std::streamsize(buf.size()));
        const std::streamsize got = in.gcount();
        if (got > 0) s.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf.data()), size_t(got)));
    }
    return Sha256::hex(s.finish());
}

std::string manifest_path(const fs::path& p, const fs::path& root)
{
    const std::u8string u = p.lexically_relative(root).generic_u8string();
    std::string out(u.begin(), u.end());
    if (out.find_first_of("\t\r\n") != std::string::npos)
        throw std::runtime_error("a path with a tab or a line break in it cannot be listed: " + out);
    return out;
}

std::string Manifest::text() const
{
    std::string t = std::string(with_contents ? kInstallManifestVersion : with_addresses ? kAddressManifestVersion : kManifestVersion) + "\n";
    t += "root " + root + "\n";
    t += "files " + std::to_string(files) + "\n";
    t += "bytes " + std::to_string(bytes) + "\n";
    for (const ManifestEntry& e : entries)
    {
        if (e.dir) t += "d\t" + e.path + "\n";
        else
            t += "f\t" + std::to_string(e.size) + "\t" + e.sha256 + "\t" + (with_addresses ? e.address + "\t" : std::string()) + e.path +
                 "\n";
    }
    t += "end\n";
    return t;
}

std::vector<uint8_t> Manifest::file() const
{
    const std::string t = text();
    std::vector<uint8_t> out;
    out.reserve(t.size() + contents.size());
    out.insert(out.end(), t.begin(), t.end());
    if (with_contents) out.insert(out.end(), contents.begin(), contents.end());
    return out;
}

Manifest manifest_of_file(const fs::path& file)
{
    if (!fs::is_regular_file(file)) throw std::runtime_error(file.generic_string() + " is not a file");
    Manifest m;
    const std::u8string name = fs::absolute(file).filename().u8string();
    m.root = std::string(name.begin(), name.end());
    ManifestEntry e;
    e.path = m.root;
    if (e.path.find_first_of("\t\r\n") != std::string::npos) throw std::runtime_error("a name with a tab or a line break in it cannot be listed: " + e.path);
    e.size = uint64_t(fs::file_size(file));
    e.sha256 = vault::hash_checked_file(file, e.path); // read once: its hash, and the vault's check
    m.entries.push_back(e);
    m.files = 1;
    m.bytes = e.size;
    return m;
}

Manifest walk_folder(const fs::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    if (!fs::is_directory(root)) throw std::runtime_error(root_in.generic_string() + " is not a folder");
    Manifest m;
    {
        // The folder's own name; for a path ending in a separator, the part before it.
        fs::path r = root;
        if (!r.has_filename()) r = r.parent_path();
        const std::u8string u = r.filename().generic_u8string();
        m.root = std::string(u.begin(), u.end());
    }
    for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::none); it != fs::recursive_directory_iterator(); ++it)
    {
        const fs::directory_entry& d = *it;
        if (d.is_symlink())
        {
            ++m.skipped;
            if (d.is_directory()) it.disable_recursion_pending();
            continue;
        }
        ManifestEntry e;
        if (d.is_directory())
        {
            e.dir = true;
            e.path = manifest_path(d.path(), root) + "/";
        }
        else if (d.is_regular_file())
        {
            e.path = manifest_path(d.path(), root);
            e.size = uint64_t(d.file_size());
            e.sha256 = vault::hash_checked_file(d.path(), e.path); // read once: its hash, and the vault's check
            ++m.files;
            m.bytes += e.size;
        }
        else
        {
            ++m.skipped; // devices, sockets and the like: not files anyone can install
            continue;
        }
        m.entries.push_back(std::move(e));
    }
    // By the paths' bytes, as unsigned bytes, so the order is the same on every system (char is
    // signed on some, which would put every non-ASCII name first).
    std::sort(m.entries.begin(), m.entries.end(), [](const ManifestEntry& a, const ManifestEntry& b) {
        return std::lexicographical_compare(a.path.begin(), a.path.end(), b.path.begin(), b.path.end(),
                                            [](char x, char y) { return uint8_t(x) < uint8_t(y); });
    });
    return m;
}

void add_addresses(Manifest& m, const std::filesystem::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    for (ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        const BigUint a = binary_address(read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end()))));
        e.address = a.is_zero() ? "0" : a.to_hex();
    }
    m.with_addresses = true;
}

void add_contents(Manifest& m, const std::filesystem::path& root_in)
{
    const fs::path root = fs::absolute(root_in).lexically_normal();
    m.contents.clear();
    m.contents.reserve(size_t(m.bytes));
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        const auto b = read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end())));
        if (b.size() != e.size || sha256_hex(b) != e.sha256) throw std::runtime_error(e.path + " changed while the folder was being read");
        m.contents.insert(m.contents.end(), b.begin(), b.end());
    }
    m.with_contents = true;
    m.with_addresses = false;
}

Manifest Manifest::parse(std::string_view whole)
{
    // Line by line, exactly as text() writes it, up to and including "end"; anything else is
    // refused rather than guessed at. After "end" a v3 manifest has its files' bytes, and the
    // others nothing.
    std::vector<std::string> lines;
    size_t after = std::string_view::npos;
    for (size_t at = 0; at < whole.size();)
    {
        const size_t nl = whole.find('\n', at);
        if (nl == std::string_view::npos) throw std::runtime_error("manifest: the last line has no line feed");
        lines.emplace_back(whole.substr(at, nl - at));
        at = nl + 1;
        if (lines.back() == "end")
        {
            after = at;
            break;
        }
    }
    if (after == std::string_view::npos) throw std::runtime_error("manifest: no 'end'");
    Manifest m;
    auto field = [](const std::string& line, const char* key) {
        const std::string k = std::string(key) + " ";
        if (line.rfind(k, 0) != 0) throw std::runtime_error(std::string("manifest: expected '") + key + "'");
        return line.substr(k.size());
    };
    if (lines.size() < 5) throw std::runtime_error("manifest: too short");
    if (lines[0] == kAddressManifestVersion) m.with_addresses = true;
    else if (lines[0] == kInstallManifestVersion) m.with_contents = true;
    else if (lines[0] != kManifestVersion) throw std::runtime_error("not a sieve manifest (" + lines[0].substr(0, 40) + ")");
    if (m.with_contents) m.contents.assign(whole.begin() + std::ptrdiff_t(after), whole.end());
    else if (after != whole.size()) throw std::runtime_error("manifest: something after 'end'");
    m.root = field(lines[1], "root");
    m.files = std::stoull(field(lines[2], "files"));
    m.bytes = std::stoull(field(lines[3], "bytes"));
    for (size_t i = 4; i + 1 < lines.size(); ++i)
    {
        std::vector<std::string> f;
        for (size_t at = 0;;)
        {
            const size_t tab = lines[i].find('\t', at);
            f.push_back(lines[i].substr(at, tab == std::string::npos ? std::string::npos : tab - at));
            if (tab == std::string::npos) break;
            at = tab + 1;
        }
        ManifestEntry e;
        if (f.size() == 2 && f[0] == "d")
        {
            e.dir = true;
            e.path = f[1];
        }
        else if (f[0] == "f" && f.size() == (m.with_addresses ? 5u : 4u))
        {
            e.size = std::stoull(f[1]);
            e.sha256 = f[2];
            if (m.with_addresses) e.address = f[3];
            e.path = f.back();
        }
        else throw std::runtime_error("manifest: line " + std::to_string(i + 1) + " is not an entry");
        // Nothing may point outside the folder being installed: no absolute path, no drive, no
        // backslash, and no part that is "." or ".." (a name with dots in it is fine).
        bool bad = e.path.empty() || e.path[0] == '/' || e.path.find(':') != std::string::npos || e.path.find('\\') != std::string::npos;
        for (size_t at = 0; !bad && at <= e.path.size();)
        {
            size_t slash = e.path.find('/', at);
            if (slash == std::string::npos) slash = e.path.size();
            const std::string part = e.path.substr(at, slash - at);
            bad = part == "." || part == ".." || (part.empty() && slash != e.path.size());
            at = slash + 1;
        }
        if (bad) throw std::runtime_error("manifest: refused path " + e.path);
        m.entries.push_back(std::move(e));
    }
    if (m.with_contents)
    {
        uint64_t sum = 0;
        for (const ManifestEntry& e : m.entries) sum += e.dir ? 0 : e.size;
        if (sum != m.bytes || m.contents.size() != m.bytes)
            throw std::runtime_error("manifest: its files' bytes are not the size its lines add up to");
    }
    return m;
}

BigUint read_address_file(const fs::path& file, bool hex)
{
    const auto bytes = read_file_bytes(file);
    if (hex)
    {
        std::string h(bytes.begin(), bytes.end());
        h.erase(std::remove_if(h.begin(), h.end(), [](char c) { return c == '\n' || c == '\r' || c == ' '; }), h.end());
        return h.empty() ? BigUint() : BigUint::from_hex(h);
    }
    return address_of_raw(bytes);
}

BigUint address_of_raw(const std::vector<uint8_t>& bytes)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string h;
    h.reserve(bytes.size() * 2);
    for (uint8_t b : bytes)
    {
        h += kHex[b >> 4];
        h += kHex[b & 15];
    }
    return h.empty() ? BigUint() : BigUint::from_hex(h);
}

fs::path own_executable(const char* argv0)
{
#ifdef _WIN32
    std::wstring w(1024, L'\0');
    for (;;)
    {
        const DWORD n = GetModuleFileNameW(nullptr, w.data(), DWORD(w.size()));
        if (n == 0) break;
        if (n < w.size())
        {
            w.resize(n);
            return fs::path(w);
        }
        w.resize(w.size() * 2);
    }
#else
    std::error_code ec;
    const fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return self;
#endif
    return fs::absolute(fs::path(argv0 ? argv0 : ""));
}

std::optional<fs::path> installer_program_beside(const fs::path& dir)
{
    // Beside the caller (a build folder, or sieve in tools\), else in the tools folder beside it
    // (the hallway, at the top of an installation).
    for (const fs::path& d : {dir, dir / "tools"})
        for (const char* name : {"sieve-install.exe", "sieve-install"})
        {
            std::error_code ec;
            if (fs::is_regular_file(d / name, ec)) return d / name;
        }
    return std::nullopt;
}

namespace {

constexpr size_t kMagic = sizeof(kAttachedMagic) - 1, kTrailer = 8 + kMagic;

// Where the program itself ends: its whole length, or where what is attached to it begins.
uint64_t program_length(const std::vector<uint8_t>& f, uint64_t* attached = nullptr)
{
    if (attached) *attached = 0;
    if (f.size() < kTrailer || std::memcmp(f.data() + f.size() - kMagic, kAttachedMagic, kMagic) != 0) return f.size();
    uint64_t n = 0;
    for (int i = 7; i >= 0; --i) n = n << 8 | f[f.size() - kTrailer + size_t(i)];
    if (n > f.size() - kTrailer) throw std::runtime_error("the installer attached to the program is damaged (its length is wrong)");
    if (attached) *attached = n;
    return f.size() - kTrailer - n;
}

} // namespace

void write_installer_program(const fs::path& program, const BigUint& address, const fs::path& out)
{
    const auto p = read_file_bytes(program);
    const uint64_t length = program_length(p);
    std::vector<uint8_t> raw;
    if (!address.is_zero())
    {
        std::string h = address.to_hex();
        if (h.size() % 2) h.insert(h.begin(), '0');
        auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
        raw.resize(h.size() / 2);
        for (size_t i = 0; i < raw.size(); ++i) raw[i] = uint8_t(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
    }
    {
        std::ofstream o(out, std::ios::binary);
        o.write(reinterpret_cast<const char*>(p.data()), std::streamsize(length));
        o.write(reinterpret_cast<const char*>(raw.data()), std::streamsize(raw.size()));
        uint64_t n = raw.size();
        for (int i = 0; i < 8; ++i, n >>= 8) o.put(char(n & 0xFF));
        o.write(kAttachedMagic, std::streamsize(kMagic));
        if (!o) throw std::runtime_error("cannot write " + out.generic_string());
    }
    // Runnable where that is a permission (it is not on Windows, where .exe says so).
    std::error_code ec;
    fs::permissions(out, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);
}

std::optional<BigUint> attached_address(const fs::path& program)
{
    std::error_code ec;
    if (!fs::is_regular_file(program, ec)) return std::nullopt;
    const auto f = read_file_bytes(program);
    uint64_t n = 0;
    const uint64_t at = program_length(f, &n);
    if (at == f.size()) return std::nullopt; // nothing attached
    return address_of_raw(std::vector<uint8_t>(f.begin() + std::ptrdiff_t(at), f.begin() + std::ptrdiff_t(at + n)));
}

void write_address_file(const fs::path& file, const BigUint& address, bool hex)
{
    std::ofstream out(file, std::ios::binary);
    if (hex) out << (address.is_zero() ? std::string("0") : address.to_hex()) << "\n";
    else if (!address.is_zero())
    {
        std::string h = address.to_hex();
        if (h.size() % 2) h.insert(h.begin(), '0');
        auto nib = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
        std::vector<char> raw(h.size() / 2);
        for (size_t i = 0; i < raw.size(); ++i) raw[i] = char(nib(h[2 * i]) << 4 | nib(h[2 * i + 1]));
        out.write(raw.data(), std::streamsize(raw.size()));
    }
    if (!out) throw std::runtime_error("cannot write " + file.generic_string());
}

Manifest manifest_of_installer(const BigUint& address)
{
    const auto bytes = file_at(address);
    Manifest m = Manifest::parse(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    if (!m.with_addresses && !m.with_contents) throw std::runtime_error("that is a manifest without the files in it (v1): nothing to install from");
    return m;
}

Manifest installable_manifest(const fs::path& file, bool hex)
{
    if (!hex)
    {
        if (const auto attached = attached_address(file)) return manifest_of_installer(*attached);
        std::ifstream in(file, std::ios::binary);
        char head[16] = {};
        in.read(head, sizeof head);
        if (in.gcount() == 16 && std::memcmp(head, "sieve-manifest-v", 16) == 0)
        {
            const auto bytes = read_file_bytes(file);
            Manifest m = Manifest::parse(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
            if (!m.with_addresses && !m.with_contents)
                throw std::runtime_error("that is a manifest without the files in it (v1): nothing to install from");
            return m;
        }
    }
    return manifest_of_installer(read_address_file(file, hex));
}

bool install_tree(const Manifest& m, const fs::path& dest, bool force, const InstallProgress& progress, const std::atomic<bool>* cancel)
{
    auto cancelled = [cancel] { return cancel && cancel->load(); };
    auto path_of = [&](const std::string& p) { return dest / fs::path(std::u8string(p.begin(), p.end())); };
    // Phase 0: every file cut from the contents (v3) or read back from its address (v2) and
    // checked, before anything is written.
    std::vector<std::pair<const ManifestEntry*, std::vector<uint8_t>>> files;
    for (const ManifestEntry& e : m.entries)
        if (!e.dir) files.emplace_back(&e, std::vector<uint8_t>{});
    size_t at = 0;
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (cancelled()) return false;
        const ManifestEntry& e = *files[i].first;
        std::vector<uint8_t> bytes;
        if (m.with_contents)
        {
            if (m.contents.size() - at < e.size) throw std::runtime_error("the installer ends before " + e.path);
            bytes.assign(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
            at += size_t(e.size);
        }
        else bytes = file_at(e.address == "0" ? BigUint() : BigUint::from_hex(e.address));
        if (bytes.size() != e.size || sha256_hex(bytes) != e.sha256)
            throw std::runtime_error("the installer does not give " + e.path + " (its size or SHA-256 is wrong)");
        vault::check_known(bytes, e.sha256, e.path); // the vault: nothing it withholds is ever written
        if (fs::exists(path_of(e.path)) && !force) throw std::runtime_error(e.path + " is already there");
        files[i].second = std::move(bytes);
        if (progress) progress(0, i + 1, files.size(), e.path);
    }
    // Phase 1: the folders, then the files, remembering what was made so a cancel can undo it.
    std::vector<fs::path> made_dirs, made_files;
    auto undo = [&] {
        for (auto it = made_files.rbegin(); it != made_files.rend(); ++it) fs::remove(*it);
        for (auto it = made_dirs.rbegin(); it != made_dirs.rend(); ++it)
        {
            std::error_code ec;
            fs::remove(*it, ec); // only if empty: a folder that held something before is kept
        }
    };
    auto make_dirs = [&](const fs::path& d) {
        std::vector<fs::path> chain;
        for (fs::path p = d; !p.empty() && !fs::exists(p); p = p.parent_path()) chain.push_back(p);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            fs::create_directory(*it);
            made_dirs.push_back(*it);
        }
    };
    try
    {
        make_dirs(dest);
        for (const ManifestEntry& e : m.entries)
            if (e.dir) make_dirs(path_of(e.path));
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (cancelled())
            {
                undo();
                return false;
            }
            const fs::path p = path_of(files[i].first->path);
            make_dirs(p.parent_path());
            const bool existed = fs::exists(p);
            std::ofstream out(p, std::ios::binary);
            out.write(reinterpret_cast<const char*>(files[i].second.data()), std::streamsize(files[i].second.size()));
            if (!out) throw std::runtime_error("cannot write " + p.generic_string());
            if (!existed) made_files.push_back(p);
            if (progress) progress(1, i + 1, files.size(), files[i].first->path);
        }
    }
    catch (...)
    {
        undo();
        throw;
    }
    return true;
}

} // namespace sieve::cli
