// Sieve — the vault (vault.hpp): loading the entries, and the checks every part of Sieve calls.
//
// Loaded once, on the first check, and kept for the life of the process: the built-in entries,
// then every *.vault file in the vault folder. The folder is looked for beside the program, and in
// the folder above when the program sits in an installation's tools\ (as dictionaries are); running
// from the repository, data/vault too. A folder that is not there adds nothing; a file that is there
// but will not parse fails the vault closed.

#include "cli/vault.hpp"

#include "cli/image_io.hpp"
#include "cli/locate.hpp"
#include "cli/timings.hpp"
#include "cli/pdq_hash.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace sieve::cli::vault {

namespace fs = std::filesystem;

namespace {

// The built-in entries, compiled in. Only the tests, for now: the SHA-256 of the 16 bytes "sieve
// vault test", and the PDQ hash of the test picture (test_picture_rgba).
const char* const kBuiltin[] = {
    "3b19e41d21594052df12c58b539a105951c93be229ce6e1683b6db5a2e6de1f3",
};
const char* const kBuiltinPdq[] = {
    "69b3ded065e3895c02a91ddd969247727a186104d9166bd50e7957eb7184e4bd",
};

// The most pixels a picture found in bytes is decoded at, for PDQ (a frame of 8192 x 8192): past
// that, reading it would take more memory than a check should. Its exact hash is still checked.
constexpr uint64_t kMaxPicturePixels = uint64_t(8192) * 8192;

fs::path program_folder()
{
#ifdef _WIN32
    std::wstring buf(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) return {};
    buf.resize(n);
    return fs::path(buf).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) return {};
    std::error_code ec;
    const fs::path p = fs::weakly_canonical(fs::path(buf.c_str()), ec);
    return ec ? fs::path(buf.c_str()).parent_path() : p.parent_path();
#else
    std::error_code ec;
    const fs::path p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : p.parent_path();
#endif
}

bool is_hex64(const std::string& s)
{
    return s.size() == 64 && std::all_of(s.begin(), s.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

struct Vault
{
    std::set<std::string> sha256;
    std::set<pdq::Hash> pdq;
    Status st;
};

Vault load()
{
    Vault v;
    for (const char* h : kBuiltin) v.sha256.insert(h);
    for (const char* h : kBuiltinPdq) v.pdq.insert(*pdq::from_hex(h));
    v.st.builtin = v.sha256.size() + v.pdq.size();
    std::vector<fs::path> folders;
    const fs::path exe = program_folder();
    if (!exe.empty())
    {
        folders.push_back(exe / "vault");
        folders.push_back(exe.parent_path() / "vault"); // an installation's tools\ program
    }
    folders.push_back(fs::path("data") / "vault"); // running from the repository
    std::set<fs::path> seen;
    for (const fs::path& f : folders)
    {
        std::error_code ec;
        if (!fs::is_directory(f, ec)) continue;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(f, ec))
            if (e.path().extension() == ".vault") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const fs::path& file : files)
        {
            const fs::path key = fs::weakly_canonical(file, ec);
            if (!seen.insert(ec ? file : key).second) continue;
            try
            {
                std::ifstream in(file, std::ios::binary);
                if (!in) throw std::runtime_error("cannot be read");
                std::ostringstream s;
                s << in.rdbuf();
                const Entries e = parse(s.str());
                for (const std::string& h : e.sha256)
                    if (v.sha256.insert(h).second) ++v.st.loaded;
                for (const std::string& h : e.pdq)
                    if (v.pdq.insert(*pdq::from_hex(h)).second) ++v.st.loaded;
                const std::u8string u = file.u8string();
                v.st.files.emplace_back(u.begin(), u.end());
            }
            catch (const std::exception& e)
            {
                const std::u8string u = file.u8string();
                v.st.failed_closed = true;
                v.st.error = std::string(u.begin(), u.end()) + ": " + e.what();
            }
        }
    }
    v.st.sha256 = v.sha256.size();
    v.st.pdq = v.pdq.size();
    return v;
}

const Vault& the_vault()
{
    static std::once_flag once;
    static Vault v;
    std::call_once(once, [] {
        timings::Scope t("vault.load");
        v = load();
    });
    return v;
}

} // namespace

Entries parse(const std::string& text)
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream in(text);
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') throw std::runtime_error("line breaks must be line feeds only");
        lines.push_back(line);
    }
    if (lines.size() < 3 || (lines[0] != kVersion && lines[0] != kVersion1))
        throw std::runtime_error(std::string("not a vault (it must start with ") + kVersion + ")");
    const bool v1 = lines[0] == kVersion1;
    if (lines[1].rfind("entries ", 0) != 0) throw std::runtime_error("the second line must be: entries N");
    size_t n = 0;
    try { n = std::stoull(lines[1].substr(8)); } catch (...) { throw std::runtime_error("the entry count is not a number"); }
    if (lines.size() != n + 3 || lines.back() != "end") throw std::runtime_error("it must hold exactly its entries, then end");
    Entries out;
    for (size_t i = 0; i < n; ++i)
    {
        const std::string& l = lines[2 + i];
        const std::string sha = "v\tsha256\t", pdq_kind = "v\tpdq\t";
        const bool is_pdq = !v1 && l.rfind(pdq_kind, 0) == 0;
        if (!is_pdq && l.rfind(sha, 0) != 0)
            throw std::runtime_error("entry " + std::to_string(i + 1) + (v1 ? " is not: v<TAB>sha256<TAB>hash" : " is not: v<TAB>sha256<TAB>hash or v<TAB>pdq<TAB>hash"));
        const std::string h = l.substr(is_pdq ? pdq_kind.size() : sha.size());
        if (!is_hex64(h)) throw std::runtime_error("entry " + std::to_string(i + 1) + " is not 64 lower-case hex digits");
        (is_pdq ? out.pdq : out.sha256).push_back(h);
    }
    return out;
}

bool withheld_sha256(const std::string& h)
{
    const Vault& v = the_vault();
    return v.st.failed_closed || v.sha256.count(h) != 0;
}

bool withheld_picture(const uint8_t* rgba, uint32_t width, uint32_t height)
{
    timings::Scope t("vault.pdq");
    const Vault& v = the_vault();
    if (v.st.failed_closed) return true;
    if (v.pdq.empty()) return false;
    const auto h = pdq::hash_rgba(rgba, width, height);
    if (!h || h->quality < kPdqMinQuality) return false;
    for (const pdq::Hash& e : v.pdq)
        for (const pdq::Hash& o : h->orientations)
            if (pdq::distance(e, o) <= kPdqMaxDistance) return true;
    return false;
}

namespace {

// The picture half of the check: the bytes' frames by PDQ, if the bytes are a picture.
bool withheld_as_picture(const std::vector<uint8_t>& bytes)
{
    const Vault& v = the_vault();
    if (v.st.failed_closed) return true;
    uint32_t w = 0, h = 0;
    if (v.pdq.empty() || !image_info(bytes.data(), bytes.size(), w, h) || uint64_t(w) * h > kMaxPicturePixels) return false;
    std::vector<RgbaImage> frames;
    try
    {
        frames = decode_image_frames(bytes.data(), bytes.size(), "the picture");
    }
    catch (const std::exception&)
    {
        return false; // a header without a picture behind it: nothing to see
    }
    for (const RgbaImage& f : frames)
        if (withheld_picture(f.rgba.data(), f.width, f.height)) return true;
    return false;
}

// The first bytes of the picture formats stb_image reads (TGA has none: it goes by its name).
bool picture_magic(const uint8_t* b, size_t n)
{
    auto starts = [&](std::initializer_list<int> m) {
        if (n < m.size()) return false;
        size_t i = 0;
        for (int c : m)
            if (b[i++] != uint8_t(c)) return false;
        return true;
    };
    return starts({0x89, 'P', 'N', 'G'}) || starts({0xFF, 0xD8, 0xFF}) || starts({'G', 'I', 'F', '8'}) || starts({'B', 'M'}) ||
           starts({'8', 'B', 'P', 'S'}) || starts({0x53, 0x80, 0xF6, 0x34}) ||
           (n >= 2 && b[0] == 'P' && (b[1] == '5' || b[1] == '6'));
}

} // namespace

bool withheld_bytes(const std::vector<uint8_t>& bytes)
{
    timings::Scope t("vault.bytes");
    const Vault& v = the_vault();
    if (v.st.failed_closed) return true;
    if (v.sha256.count(sha256_hex(bytes)) != 0) return true;
    return withheld_as_picture(bytes); // a file that is a picture is also checked as one
}

void check_known(const std::vector<uint8_t>& bytes, const std::string& sha, const std::string& what)
{
    check_sha256(sha, what);
    if (withheld_as_picture(bytes)) throw VaultWithheld("withheld by the vault: " + what);
}

void check_file(const fs::path& file, const std::string& sha, const std::string& what)
{
    check_sha256(sha, what);
    if (the_vault().pdq.empty()) return;
    std::ifstream in(file, std::ios::binary);
    uint8_t head[8] = {};
    in.read(reinterpret_cast<char*>(head), sizeof head);
    std::string ext = file.extension().string();
    for (char& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
    if (!picture_magic(head, size_t(in.gcount())) && ext != ".tga") return;
    in.clear();
    in.seekg(0);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (withheld_as_picture(bytes)) throw VaultWithheld("withheld by the vault: " + what);
}

std::vector<PictureHash> picture_hashes(const std::vector<uint8_t>& bytes)
{
    std::vector<PictureHash> out;
    uint32_t w = 0, h = 0;
    if (!image_info(bytes.data(), bytes.size(), w, h)) return out;
    for (const RgbaImage& f : decode_image_frames(bytes.data(), bytes.size(), "the picture"))
        if (const auto ph = pdq::hash_rgba(f.rgba.data(), f.width, f.height)) out.push_back({pdq::to_hex(ph->orientations[0]), ph->quality});
    return out;
}

std::vector<uint8_t> test_picture_rgba()
{
    std::vector<uint8_t> shades;
    Sha256::Digest d = Sha256::hash("sieve vault test picture");
    for (int i = 0; i < 8; ++i)
    {
        shades.insert(shades.end(), d.begin(), d.end());
        d = Sha256::hash(std::string_view(reinterpret_cast<const char*>(d.data()), d.size()));
    }
    std::vector<uint8_t> px(size_t(64) * 64 * 4);
    for (size_t y = 0; y < 64; ++y)
        for (size_t x = 0; x < 64; ++x)
        {
            const uint8_t s = shades[(y / 4) * 16 + x / 4];
            uint8_t* p = &px[(y * 64 + x) * 4];
            p[0] = p[1] = p[2] = s;
            p[3] = 255;
        }
    return px;
}

bool withheld_text(const std::string& utf8) { return withheld_bytes(std::vector<uint8_t>(utf8.begin(), utf8.end())); }

void check_sha256(const std::string& h, const std::string& what)
{
    if (withheld_sha256(h)) throw VaultWithheld("withheld by the vault: " + what);
}

void check_bytes(const std::vector<uint8_t>& bytes, const std::string& what)
{
    if (withheld_bytes(bytes)) throw VaultWithheld("withheld by the vault: " + what);
}

Status status() { return the_vault().st; }

} // namespace sieve::cli::vault
