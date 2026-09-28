// The installer's 7z unpacking (unpack_7z.hpp): the LZMA SDK's decoder over the archive in memory.
//
// The archive is already in memory (it is the file the installer carries, cut from its manifest and
// checked against its SHA-256), so the decoder reads it through a look-in stream that hands out
// pointers into that buffer, with no copying. A solid archive decodes a block at a time; the SDK
// keeps the block last decoded, so its files are then only copied out. Names are checked before
// anything is written, and files already at the destination are refused (unless replacing) before
// anything is written too; after that, a failure or a cancel removes what was written.

#include "unpack_7z.hpp"

#include "cli/vault.hpp"

#include "7z.h"
#include "7zCrc.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <stdexcept>

namespace sieve::install {

namespace fs = std::filesystem;

namespace {

// ---- the archive in memory, as the SDK's look-in stream
struct MemLook
{
    ILookInStream vt;
    const uint8_t* data;
    size_t size;
    size_t pos;
};

MemLook* self(ILookInStreamPtr p) { return const_cast<MemLook*>(reinterpret_cast<const MemLook*>(p)); }

SRes mem_look(ILookInStreamPtr p, const void** buf, size_t* size)
{
    MemLook* m = self(p);
    const size_t left = m->size - m->pos;
    if (*size > left) *size = left;
    *buf = m->data + m->pos;
    return SZ_OK;
}

SRes mem_skip(ILookInStreamPtr p, size_t offset)
{
    MemLook* m = self(p);
    m->pos += offset;
    return SZ_OK;
}

SRes mem_read(ILookInStreamPtr p, void* buf, size_t* size)
{
    MemLook* m = self(p);
    const size_t left = m->size - m->pos;
    if (*size > left) *size = left;
    if (*size) std::memcpy(buf, m->data + m->pos, *size);
    m->pos += *size;
    return SZ_OK;
}

SRes mem_seek(ILookInStreamPtr p, Int64* pos, ESzSeek origin)
{
    MemLook* m = self(p);
    Int64 base = 0;
    if (origin == SZ_SEEK_CUR) base = Int64(m->pos);
    else if (origin == SZ_SEEK_END) base = Int64(m->size);
    const Int64 to = base + *pos;
    if (to < 0 || to > Int64(m->size)) return SZ_ERROR_PARAM;
    m->pos = size_t(to);
    *pos = to;
    return SZ_OK;
}

void* sz_alloc(ISzAllocPtr, size_t size) { return size ? std::malloc(size) : nullptr; }
void sz_free(ISzAllocPtr, void* address) { std::free(address); }
const ISzAlloc kAlloc = {sz_alloc, sz_free};

std::string sz_error(SRes r)
{
    switch (r)
    {
    case SZ_ERROR_DATA: return "its data is damaged";
    case SZ_ERROR_MEM: return "there is not enough memory to unpack it";
    case SZ_ERROR_CRC: return "a file in it fails its CRC check";
    case SZ_ERROR_UNSUPPORTED: return "it uses a method this installer cannot unpack (or is encrypted)";
    case SZ_ERROR_INPUT_EOF: return "it is cut short";
    case SZ_ERROR_NO_ARCHIVE: return "it is not a 7z archive";
    case SZ_ERROR_ARCHIVE: return "its listing is damaged";
    default: return "the 7z decoder failed (error " + std::to_string(r) + ")";
    }
}

std::string utf8_of(const std::vector<UInt16>& w)
{
    std::string s;
    for (size_t i = 0; i < w.size() && w[i]; ++i)
    {
        uint32_t c = w[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < w.size() && w[i + 1] >= 0xDC00 && w[i + 1] < 0xE000)
            c = 0x10000 + ((c - 0xD800) << 10) + (w[++i] - 0xDC00);
        if (c < 0x80) s += char(c);
        else if (c < 0x800) s += char(0xC0 | (c >> 6)), s += char(0x80 | (c & 63));
        else if (c < 0x10000) s += char(0xE0 | (c >> 12)), s += char(0x80 | ((c >> 6) & 63)), s += char(0x80 | (c & 63));
        else
            s += char(0xF0 | (c >> 18)), s += char(0x80 | ((c >> 12) & 63)), s += char(0x80 | ((c >> 6) & 63)),
                s += char(0x80 | (c & 63));
    }
    return s;
}

// A name's parts, or throws if it could reach outside the folder it unpacks into.
std::vector<std::string> parts_of(const std::string& name)
{
    std::vector<std::string> parts;
    std::string cur;
    auto push = [&] {
        if (cur == "..") throw std::runtime_error("the archive has a name that leaves its folder: " + name);
        if (!cur.empty() && cur != ".") parts.push_back(cur);
        cur.clear();
    };
    if (!name.empty() && (name[0] == '/' || name[0] == '\\')) throw std::runtime_error("the archive has an absolute name: " + name);
    for (char ch : name)
    {
        if (ch == '/' || ch == '\\') push();
        else if (ch == ':') throw std::runtime_error("the archive has a name with a drive or a stream in it: " + name);
        else cur += ch;
    }
    push();
    if (parts.empty()) throw std::runtime_error("the archive has an empty name");
    return parts;
}

std::string joined(const std::vector<std::string>& parts, size_t from)
{
    std::string s;
    for (size_t i = from; i < parts.size(); ++i) s += (i > from ? "/" : "") + parts[i];
    return s;
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// The archive opened; closed again when this goes.
struct Open7z
{
    MemLook look{};
    CSzArEx db{};
    Open7z(const std::vector<uint8_t>& archive)
    {
        static std::once_flag crc;
        std::call_once(crc, [] { CrcGenerateTable(); });
        look.vt.Look = mem_look;
        look.vt.Skip = mem_skip;
        look.vt.Read = mem_read;
        look.vt.Seek = mem_seek;
        look.data = archive.data();
        look.size = archive.size();
        look.pos = 0;
        SzArEx_Init(&db);
        const SRes r = SzArEx_Open(&db, &look.vt, &kAlloc, &kAlloc);
        if (r != SZ_OK)
        {
            SzArEx_Free(&db, &kAlloc);
            throw std::runtime_error("cannot unpack the archive: " + sz_error(r));
        }
    }
    ~Open7z() { SzArEx_Free(&db, &kAlloc); }
    Open7z(const Open7z&) = delete;
    Open7z& operator=(const Open7z&) = delete;

    std::string name(UInt32 i) const
    {
        const size_t n = SzArEx_GetFileNameUtf16(&db, i, nullptr);
        std::vector<UInt16> w(n + 1, 0);
        SzArEx_GetFileNameUtf16(&db, i, w.data());
        return utf8_of(w);
    }
};

// The listing, with the common top folder found and removed; `index` is each entry's in the archive.
SevenZip listing(const Open7z& a, std::vector<UInt32>* index)
{
    std::vector<std::vector<std::string>> all;
    std::vector<bool> dirs;
    for (UInt32 i = 0; i < a.db.NumFiles; ++i)
    {
        all.push_back(parts_of(a.name(i)));
        dirs.push_back(SzArEx_IsDir(&a.db, i) != 0);
    }
    // One top folder: every entry is it, or inside it, and at least one is inside it.
    std::string top;
    bool one = !all.empty();
    bool inside = false;
    for (size_t i = 0; i < all.size() && one; ++i)
    {
        if (all[i][0] != all[0][0] || (all[i].size() == 1 && !dirs[i])) one = false;
        if (all[i].size() > 1) inside = true;
    }
    if (one && inside) top = all[0][0];
    SevenZip z;
    z.top = top;
    for (UInt32 i = 0; i < all.size(); ++i)
    {
        const size_t from = top.empty() ? 0 : 1;
        if (all[i].size() <= from) continue; // the top folder itself
        Unpacked u;
        u.path = joined(all[i], from);
        u.dir = dirs[i];
        u.size = u.dir ? 0 : SzArEx_GetFileSize(&a.db, i);
        if (!u.dir)
        {
            ++z.files;
            z.bytes += u.size;
        }
        z.entries.push_back(u);
        if (index) index->push_back(i);
    }
    return z;
}

} // namespace

bool is_7z(const std::vector<uint8_t>& bytes)
{
    static const uint8_t sig[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
    return bytes.size() >= 32 && std::memcmp(bytes.data(), sig, 6) == 0;
}

SevenZip list_7z(const std::vector<uint8_t>& archive)
{
    const Open7z a(archive);
    return listing(a, nullptr);
}

bool unpack_7z(const std::vector<uint8_t>& archive, const fs::path& dest, bool force,
               const std::function<void(size_t, size_t, const std::string&)>& progress, const std::atomic<bool>* cancel)
{
    Open7z a(archive);
    std::vector<UInt32> index;
    const SevenZip z = listing(a, &index);
    if (!force)
        for (const Unpacked& u : z.entries)
            if (!u.dir && fs::exists(dest / from_u8(u.path)))
                throw std::runtime_error("already there (tick Replace to write over it): " + u.path);

    std::vector<fs::path> made_dirs, written;
    auto make_dirs = [&](const fs::path& d) {
        std::vector<fs::path> missing;
        for (fs::path p = d; !p.empty() && !fs::exists(p); p = p.parent_path())
        {
            missing.push_back(p);
            if (p == p.parent_path()) break;
        }
        for (auto it = missing.rbegin(); it != missing.rend(); ++it)
        {
            fs::create_directory(*it);
            made_dirs.push_back(*it);
        }
    };
    auto undo = [&] {
        std::error_code ec;
        for (const fs::path& f : written) fs::remove(f, ec);
        for (auto it = made_dirs.rbegin(); it != made_dirs.rend(); ++it) fs::remove(*it, ec); // only if empty
    };

    UInt32 block = 0xFFFFFFFF;
    Byte* out = nullptr;
    size_t out_size = 0;
    size_t done = 0;
    try
    {
        make_dirs(dest);
        for (size_t k = 0; k < z.entries.size(); ++k)
        {
            if (cancel && *cancel)
            {
                std::free(out);
                undo();
                return false;
            }
            const Unpacked& u = z.entries[k];
            const fs::path to = dest / from_u8(u.path);
            if (u.dir)
            {
                make_dirs(to);
                continue;
            }
            make_dirs(to.parent_path());
            size_t offset = 0, got = 0;
            const SRes r = SzArEx_Extract(&a.db, &a.look.vt, index[k], &block, &out, &out_size, &offset, &got, &kAlloc, &kAlloc);
            if (r != SZ_OK) throw std::runtime_error("cannot unpack " + u.path + ": " + sz_error(r));
            // The vault: nothing it withholds is written, whatever archive it arrives in.
            sieve::cli::vault::check_bytes(std::vector<uint8_t>(out + offset, out + offset + got), u.path);
            std::ofstream f(to, std::ios::binary | std::ios::trunc);
            if (!f) throw std::runtime_error("cannot write " + u.path);
            written.push_back(to);
            if (got) f.write(reinterpret_cast<const char*>(out + offset), std::streamsize(got));
            f.close();
            if (!f) throw std::runtime_error("cannot write " + u.path);
            // A Unix mode, where the archive recorded one (7-Zip on Linux or macOS does, in the
            // attributes' top half): kept, so a program unpacked there can still be run. Windows
            // has no such bits; there the attributes are left alone.
            const UInt32 i = index[k];
            if (SzBitWithVals_Check(&a.db.Attribs, i) && (a.db.Attribs.Vals[i] & 0x8000u))
            {
                std::error_code ec;
                fs::permissions(to, fs::perms((a.db.Attribs.Vals[i] >> 16) & 0777u), fs::perm_options::replace, ec);
            }
            ++done;
            if (progress) progress(done, size_t(z.files), u.path);
        }
    }
    catch (...)
    {
        std::free(out);
        undo();
        throw;
    }
    std::free(out);
    return true;
}

} // namespace sieve::install
