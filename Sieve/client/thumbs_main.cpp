// sieve-thumbs.dll: the picture a .simage holds, as its thumbnail in Windows' Explorer (Edward, 5
// November 2026: "Could we pipe an image preview to windows just like a regular image rather than
// the standard application icon").
//
// A thumbnail handler (Windows' IThumbnailProvider, given the file as a stream by
// IInitializeWithStream). Explorer runs it in a process of its own (dllhost), so a fault here never
// takes Explorer with it. The .simage is read exactly as sieve-view reads it (picture_source.hpp):
// its listing first, refused there where it is not pictures within the memory free; then installed
// into a folder of its own, every file checked against its SHA-256, and the first picture's header
// checked before its pixels are made (a JPEG too large, at an eighth). Anything refused gives no
// thumbnail, and Explorer shows the file's icon instead, as before. The picture is scaled to the
// size asked for, its longer side to it, each pixel the mean of those it covers.
//
// Registered for the person alone, beside the rest of the right-click menu (context/
// install_context.bat): the class below under HKCU\Software\Classes\CLSID, and .simage's ShellEx
// for thumbnails ({e357fccd-a995-4576-b01f-234630154e96}) naming it.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>
#include <thumbcache.h>

#include "picture_source.hpp"

#include "cli/dictionaries.hpp" // set_executable_dir
#include "cli/items.hpp"        // use_item_maker

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// {DEA4E1A4-F2CC-427D-BD89-4A2E0231EB4B}: Sieve's picture thumbnails (made 5 November 2026; never
// to change, as the registry names it).
const CLSID kClsid = {0xDEA4E1A4, 0xF2CC, 0x427D, {0xBD, 0x89, 0x4A, 0x2E, 0x02, 0x31, 0xEB, 0x4B}};

HMODULE g_module = nullptr;
std::atomic<long> g_objects{0}, g_locks{0};

// This DLL's own folder: where the vault and the dictionaries are looked for (in dllhost, "the
// program's folder" would be Windows' own).
fs::path own_folder()
{
    std::wstring buf(32768, L'\0');
    const DWORD n = GetModuleFileNameW(g_module, buf.data(), DWORD(buf.size()));
    if (n == 0 || n >= buf.size()) return {};
    buf.resize(n);
    return fs::path(buf).parent_path();
}

// The stream's bytes, no more than `most` (a .simage larger than the memory budget is not read).
bool read_stream(IStream* s, uint64_t most, std::vector<uint8_t>& out)
{
    STATSTG st{};
    if (FAILED(s->Stat(&st, STATFLAG_NONAME)) || st.cbSize.QuadPart == 0 || st.cbSize.QuadPart > most) return false;
    out.resize(size_t(st.cbSize.QuadPart));
    LARGE_INTEGER zero{};
    s->Seek(zero, STREAM_SEEK_SET, nullptr);
    size_t got = 0;
    while (got < out.size())
    {
        ULONG n = 0;
        const ULONG want = ULONG(std::min<size_t>(out.size() - got, size_t(1) << 24));
        const HRESULT hr = s->Read(out.data() + got, want, &n);
        if (FAILED(hr) || n == 0) break;
        got += n;
    }
    return got == out.size();
}

// The picture made smaller (picture_source's shrink_to), as a 32-bit top-down DIB, blue green red
// alpha, premultiplied (as Explorer takes ARGB).
HBITMAP thumbnail_of(const sieve::RgbaImage& img, UINT side)
{
    const sieve::RgbaImage t = sieve::view::shrink_to(img, side);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = LONG(t.width);
    bi.bmiHeader.biHeight = -LONG(t.height); // (top-down)
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp || !bits) return nullptr;
    uint8_t* out = static_cast<uint8_t*>(bits);
    for (size_t i = 0; i < size_t(t.width) * t.height; ++i)
    {
        const uint8_t* p = t.rgba.data() + i * 4;
        uint8_t* q = out + i * 4;
        q[0] = uint8_t((p[2] * p[3] + 127) / 255), q[1] = uint8_t((p[1] * p[3] + 127) / 255), q[2] = uint8_t((p[0] * p[3] + 127) / 255), q[3] = p[3];
    }
    return bmp;
}

class Thumbnail final : public IThumbnailProvider, public IInitializeWithStream
{
public:
    Thumbnail() { ++g_objects; }
    ~Thumbnail()
    {
        if (stream_) stream_->Release();
        --g_objects;
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IThumbnailProvider) *ppv = static_cast<IThumbnailProvider*>(this);
        else if (riid == IID_IInitializeWithStream) *ppv = static_cast<IInitializeWithStream*>(this);
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return ULONG(++refs_); }
    IFACEMETHODIMP_(ULONG) Release() override
    {
        const long n = --refs_;
        if (n == 0) delete this;
        return ULONG(n);
    }

    IFACEMETHODIMP Initialize(IStream* stream, DWORD) override
    {
        if (stream_) return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        if (!stream) return E_INVALIDARG;
        stream_ = stream;
        stream_->AddRef();
        return S_OK;
    }

    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* alpha) override
    {
        if (!phbmp || !alpha || !stream_) return E_INVALIDARG;
        *phbmp = nullptr;
        // (a temporary copy of the file, as the reader reads a file; removed whatever happens)
        fs::path temp;
        try
        {
            // (once, for every thread Explorer asks on)
            static std::once_flag folder_set;
            std::call_once(folder_set, [] {
                sieve::cli::set_executable_dir(own_folder());
                // (every file Sieve's instructions carry made again: a JPEG from its brunsli form, a
                // PNG by stb_image_write, as sieve-view and sieve install make them)
                sieve::cli::use_item_maker();
                // (a thumbnail takes at most 1 GiB, or half the memory free where that is less: a
                // picture past it keeps its icon; a huge JPEG is still shown from its eighth)
                sieve::view::set_memory_budget(std::min<uint64_t>(sieve::view::memory_budget(), uint64_t(1) << 30));
            });
            std::vector<uint8_t> bytes;
            if (!read_stream(stream_, sieve::view::memory_budget(), bytes)) return E_FAIL;
            wchar_t dir[MAX_PATH + 1] = {};
            if (!GetTempPathW(MAX_PATH, dir)) return E_FAIL;
            static std::atomic<unsigned> serial{0};
            temp = fs::path(dir) / (L"sieve-thumb-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L"-" +
                                    std::to_wstring(serial++) + L".simage");
            {
                std::ofstream f(temp, std::ios::binary);
                f.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                if (!f) throw std::runtime_error("cannot write a temporary copy");
            }
            std::vector<sieve::view::Picture> pictures = sieve::view::pictures_of_sieve(temp);
            std::error_code ec;
            fs::remove(temp, ec);
            temp.clear();
            if (pictures.empty()) return E_FAIL;
            sieve::view::make_pixels(pictures.front());
            HBITMAP bmp = thumbnail_of(pictures.front().pixels, cx ? cx : 256);
            if (!bmp) return E_FAIL;
            *phbmp = bmp;
            *alpha = WTSAT_ARGB;
            return S_OK;
        }
        catch (...)
        {
            // (not a picture, refused by a limit, damaged: no thumbnail, and the file's icon shows)
            if (!temp.empty())
            {
                std::error_code ec;
                fs::remove(temp, ec);
            }
            return E_FAIL;
        }
    }

private:
    std::atomic<long> refs_{1};
    IStream* stream_ = nullptr;
};

class Factory final : public IClassFactory
{
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return 2; } // (one, for the DLL's life)
    IFACEMETHODIMP_(ULONG) Release() override { return 1; }
    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;
        Thumbnail* t = new (std::nothrow) Thumbnail();
        if (!t) return E_OUTOFMEMORY;
        const HRESULT hr = t->QueryInterface(riid, ppv);
        t->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL lock) override
    {
        if (lock) ++g_locks;
        else --g_locks;
        return S_OK;
    }
};

Factory g_factory;

} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

// (Exported by thumbs.def, by these names, as COM looks them up.)
extern "C" HRESULT __stdcall SieveThumbsGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    if (clsid != kClsid) return CLASS_E_CLASSNOTAVAILABLE;
    return g_factory.QueryInterface(riid, ppv);
}

extern "C" HRESULT __stdcall SieveThumbsCanUnloadNow() { return g_objects == 0 && g_locks == 0 ? S_OK : S_FALSE; }
