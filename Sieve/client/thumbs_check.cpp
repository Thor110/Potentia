// sieve-view --thumbnail-check FILE (Edward, 5 November 2026: the thumbnails did not show, the
// viewer's icon did). Each step Explorer takes for a thumbnail, taken here one at a time and said
// as it is taken, so the one that fails is named: the registry's handler for the extension, the
// DLL its class names, that DLL loaded in this process and given the file as a stream (as dllhost
// gives it), and the shell asked as Explorer asks.

#include "thumbs_check.hpp"

#include <cstdio>
#include <string>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <thumbcache.h>

namespace sieve::view {

namespace {

// (the shell's key for thumbnail handlers, under an extension's or a class's ShellEx)
const wchar_t* const kThumbKey = L"{e357fccd-a995-4576-b01f-234630154e96}";

std::string narrow(const std::wstring& w)
{
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(size_t(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

// A key's default value under HKEY_CLASSES_ROOT (the person's classes over the machine's), or
// empty where there is none.
std::wstring class_value(const std::wstring& key, const wchar_t* name = nullptr)
{
    DWORD size = 0;
    if (RegGetValueW(HKEY_CLASSES_ROOT, key.c_str(), name, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size < sizeof(wchar_t))
        return {};
    std::wstring v(size / sizeof(wchar_t), L'\0');
    if (RegGetValueW(HKEY_CLASSES_ROOT, key.c_str(), name, RRF_RT_REG_SZ, nullptr, v.data(), &size) != ERROR_SUCCESS) return {};
    v.resize(wcsnlen(v.c_str(), v.size()));
    return v;
}

void said_hr(const char* what, HRESULT hr)
{
    char* text = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, DWORD(hr), 0,
                   reinterpret_cast<char*>(&text), 0, nullptr);
    std::string t = text ? text : "";
    if (text) LocalFree(text);
    while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back();
    std::printf("%s: %s (0x%08lX%s%s)\n", what, SUCCEEDED(hr) ? "yes" : "no", static_cast<unsigned long>(hr), t.empty() ? "" : ": ", t.c_str());
}

void said_bitmap(const char* what, HBITMAP bmp)
{
    BITMAP b{};
    if (bmp && GetObjectW(bmp, sizeof b, &b)) std::printf("%s: %ld by %ld pixels, %u bits\n", what, b.bmWidth, b.bmHeight, unsigned(b.bmBitsPixel));
}


// Whether this program runs as administrator (elevated).
bool elevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION e{};
    DWORD size = 0;
    const bool yes = GetTokenInformation(token, TokenElevation, &e, sizeof e, &size) && e.TokenIsElevated;
    CloseHandle(token);
    return yes;
}

// The handler given the file as a stream (as dllhost gives it) and asked for a 256-pixel
// thumbnail, each step said; true when it gave one.
bool ask(IThumbnailProvider* provider, const std::filesystem::path& file)
{
    bool made = false;
    IInitializeWithStream* init = nullptr;
    HRESULT hr = provider->QueryInterface(IID_PPV_ARGS(&init));
    said_hr("  handler takes a stream", hr);
    IStream* stream = nullptr;
    if (SUCCEEDED(hr)) hr = SHCreateStreamOnFileEx(file.c_str(), STGM_READ | STGM_SHARE_DENY_NONE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream);
    if (SUCCEEDED(hr)) hr = init->Initialize(stream, STGM_READ);
    said_hr("  handler given the file", hr);
    if (SUCCEEDED(hr))
    {
        HBITMAP bmp = nullptr;
        WTS_ALPHATYPE alpha = WTSAT_UNKNOWN;
        hr = provider->GetThumbnail(256, &bmp, &alpha);
        said_hr("  handler's thumbnail", hr);
        said_bitmap("    its size", bmp);
        if (bmp)
        {
            DeleteObject(bmp);
            made = SUCCEEDED(hr);
        }
    }
    if (stream) stream->Release();
    if (init) init->Release();
    return made;
}

} // namespace

int thumbnail_check(const std::filesystem::path& file)
{
    const std::wstring ext = file.extension().wstring();
    std::printf("file: %s\n", narrow(file.wstring()).c_str());

    // 1. The handler named for the extension, and for its class (Explorer looks at both).
    const std::wstring progid = class_value(ext);
    std::printf("class of %s: %s\n", narrow(ext).c_str(), progid.empty() ? "(none)" : narrow(progid).c_str());
    std::wstring clsid = class_value(ext + L"\\ShellEx\\" + kThumbKey);
    std::printf("thumbnail handler for %s: %s\n", narrow(ext).c_str(), clsid.empty() ? "(none)" : narrow(clsid).c_str());
    if (!progid.empty())
    {
        const std::wstring by_class = class_value(progid + L"\\ShellEx\\" + kThumbKey);
        std::printf("thumbnail handler for %s: %s\n", narrow(progid).c_str(), by_class.empty() ? "(none)" : narrow(by_class).c_str());
        if (clsid.empty()) clsid = by_class;
    }
    // (an "open with" choice the person made in Explorer, which takes the place of the class)
    {
        HKEY k = nullptr;
        const std::wstring choice = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + ext + L"\\UserChoice";
        if (RegOpenKeyExW(HKEY_CURRENT_USER, choice.c_str(), 0, KEY_READ, &k) == ERROR_SUCCESS)
        {
            wchar_t v[512] = {};
            DWORD size = sizeof v;
            if (RegGetValueW(k, nullptr, L"ProgId", RRF_RT_REG_SZ, nullptr, v, &size) == ERROR_SUCCESS)
            {
                std::printf("chosen in Explorer for %s: %s\n", narrow(ext).c_str(), narrow(v).c_str());
                const std::wstring by_choice = class_value(std::wstring(v) + L"\\ShellEx\\" + kThumbKey);
                std::printf("thumbnail handler for %s: %s\n", narrow(v).c_str(), by_choice.empty() ? "(none)" : narrow(by_choice).c_str());
            }
            RegCloseKey(k);
        }
    }
    if (clsid.empty())
    {
        std::printf("no thumbnail handler is registered: run context\\install_context.bat with sieve-thumbs.dll beside sieve-view\n");
        return 1;
    }

    // 2. The DLL the class names.
    const std::wstring dll = class_value(L"CLSID\\" + clsid + L"\\InprocServer32");
    const std::wstring model = class_value(L"CLSID\\" + clsid + L"\\InprocServer32", L"ThreadingModel");
    std::printf("its DLL: %s (threading: %s)\n", dll.empty() ? "(none)" : narrow(dll).c_str(), model.empty() ? "(none)" : narrow(model).c_str());
    if (!dll.empty()) std::printf("that DLL is there: %s\n", GetFileAttributesW(dll.c_str()) != INVALID_FILE_ATTRIBUTES ? "yes" : "no");

    // 3. The DLL itself: loaded (any DLL it needs that is missing fails here), its class asked of
    // it directly (as COM asks it, without the registry), then made by COM from the registry. COM
    // reports a DLL that refuses its class as "Class not registered" too, so the two are told apart
    // here; and a program run as administrator is not given classes registered for the person
    // alone (install_context.bat registers them so).
    const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::printf("run as administrator: %s\n", elevated() ? "yes (COM then ignores classes registered for your user alone: run this from an ordinary prompt)" : "no");
    bool made = false;
    CLSID id{};
    const bool have_id = SUCCEEDED(CLSIDFromString(clsid.c_str(), &id));
    if (!have_id) std::printf("the handler's class is not a class id: %s\n", narrow(clsid).c_str());
    if (!dll.empty())
    {
        HMODULE m = LoadLibraryExW(dll.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        said_hr("DLL loads", m ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
        if (m)
        {
            using GetClassObject = HRESULT(__stdcall*)(REFCLSID, REFIID, void**);
            const auto get = reinterpret_cast<GetClassObject>(reinterpret_cast<void*>(GetProcAddress(m, "DllGetClassObject")));
            std::printf("DLL exports DllGetClassObject: %s\n", get ? "yes" : "no");
            std::printf("DLL exports DllCanUnloadNow: %s\n", GetProcAddress(m, "DllCanUnloadNow") ? "yes" : "no");
            if (get && have_id)
            {
                IClassFactory* factory = nullptr;
                HRESULT hr = get(id, IID_IClassFactory, reinterpret_cast<void**>(&factory));
                said_hr("DLL gives its class (asked directly)", hr);
                if (SUCCEEDED(hr))
                {
                    IThumbnailProvider* provider = nullptr;
                    hr = factory->CreateInstance(nullptr, IID_PPV_ARGS(&provider));
                    said_hr("handler made (directly)", hr);
                    if (SUCCEEDED(hr))
                    {
                        made = ask(provider, file);
                        provider->Release();
                    }
                    factory->Release();
                }
            }
        }
    }
    if (have_id)
    {
        IThumbnailProvider* provider = nullptr;
        const HRESULT hr = CoCreateInstance(id, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&provider));
        said_hr("handler made by COM from the registry", hr);
        if (SUCCEEDED(hr))
        {
            ask(provider, file);
            provider->Release();
        }
    }

    // 4. The shell, asked as Explorer asks (in a process of its own; from its cache where it has one).
    {
        IShellItemImageFactory* factory = nullptr;
        HRESULT hr = SHCreateItemFromParsingName(std::filesystem::absolute(file).c_str(), nullptr, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr))
        {
            HBITMAP bmp = nullptr;
            hr = factory->GetImage(SIZE{256, 256}, SIIGBF_THUMBNAILONLY, &bmp);
            said_hr("Windows gives a thumbnail", hr);
            said_bitmap("  its size", bmp);
            if (bmp) DeleteObject(bmp);
            factory->Release();
        }
        else said_hr("Windows finds the file", hr);
    }
    if (SUCCEEDED(co)) CoUninitialize();
    std::printf("the DLL's own notes (each thumbnail it was asked for): %%TEMP%%\\sieve-thumbs.log\n");
    return made ? 0 : 1;
}

} // namespace sieve::view

#else

namespace sieve::view {

int thumbnail_check(const std::filesystem::path&)
{
    std::printf("thumbnails in Explorer are Windows' alone: nothing to check here\n");
    return 1;
}

} // namespace sieve::view

#endif
