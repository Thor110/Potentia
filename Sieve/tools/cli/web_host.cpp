#include "cli/web_host.hpp"

#include "cli/locate.hpp"
#include "sieve/json.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <istream>
#include <map>
#include <ostream>
#include <stdexcept>

namespace sieve::cli {

namespace {

constexpr char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string lower(std::string s)
{
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// The hash a link states, as hex: 64 hex digits, or an integrity attribute's "sha256-<base64>".
std::string expected_hex(const std::string& expected)
{
    std::string e = expected;
    while (!e.empty() && std::isspace(static_cast<unsigned char>(e.back()))) e.pop_back();
    if (e.rfind("sha256-", 0) == 0)
    {
        const std::vector<uint8_t> d = bytes_of_base64(e.substr(7));
        if (d.size() != 32) throw std::runtime_error("the link's hash is not a SHA-256 (" + std::to_string(d.size()) + " bytes)");
        static constexpr char kHex[] = "0123456789abcdef";
        std::string h;
        for (const uint8_t b : d) h += {kHex[b >> 4], kHex[b & 15]};
        return h;
    }
    if (e.size() != 64 || !std::all_of(e.begin(), e.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }))
        throw std::runtime_error("the link states no SHA-256 (64 hex digits, or sha256- and base64), so the file is refused");
    return lower(e);
}

} // namespace

std::string base64_of(const uint8_t* data, size_t n)
{
    std::string out;
    out.reserve((n + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 3 <= n; i += 3)
    {
        const uint32_t v = uint32_t(data[i]) << 16 | uint32_t(data[i + 1]) << 8 | data[i + 2];
        out += {kB64[v >> 18], kB64[v >> 12 & 63], kB64[v >> 6 & 63], kB64[v & 63]};
    }
    if (n - i == 1)
    {
        const uint32_t v = uint32_t(data[i]) << 16;
        out += {kB64[v >> 18], kB64[v >> 12 & 63], '=', '='};
    }
    else if (n - i == 2)
    {
        const uint32_t v = uint32_t(data[i]) << 16 | uint32_t(data[i + 1]) << 8;
        out += {kB64[v >> 18], kB64[v >> 12 & 63], kB64[v >> 6 & 63], '='};
    }
    return out;
}

std::vector<uint8_t> bytes_of_base64(const std::string& text)
{
    int value[256];
    std::fill(std::begin(value), std::end(value), -1);
    for (int k = 0; k < 64; ++k) value[static_cast<unsigned char>(kB64[k])] = k;
    std::vector<uint8_t> out;
    out.reserve(text.size() / 4 * 3);
    uint32_t acc = 0;
    int bits = 0;
    size_t pad = 0;
    for (const char c : text)
    {
        if (c == '=')
        {
            ++pad;
            continue;
        }
        const int v = value[static_cast<unsigned char>(c)];
        if (v < 0 || pad) throw std::runtime_error("not base64");
        acc = acc << 6 | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t(acc >> bits));
        }
    }
    if (pad > 2 || (text.size() % 4 != 0)) throw std::runtime_error("not base64 (its length is not a multiple of 4)");
    return out;
}

std::string media_type_of(const std::string& name)
{
    static const std::map<std::string, std::string> kTypes = {
        {"html", "text/html"},       {"htm", "text/html"},         {"css", "text/css"},          {"js", "text/javascript"},
        {"txt", "text/plain"},       {"md", "text/plain"},         {"json", "application/json"}, {"xml", "application/xml"},
        {"svg", "image/svg+xml"},    {"png", "image/png"},         {"jpg", "image/jpeg"},        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},        {"webp", "image/webp"},       {"bmp", "image/bmp"},         {"ico", "image/x-icon"},
        {"wav", "audio/wav"},        {"mp3", "audio/mpeg"},        {"ogg", "audio/ogg"},         {"flac", "audio/flac"},
        {"mid", "audio/midi"},       {"mp4", "video/mp4"},         {"webm", "video/webm"},       {"pdf", "application/pdf"},
        {"woff2", "font/woff2"},     {"woff", "font/woff"},        {"zip", "application/zip"},   {"7z", "application/x-7z-compressed"},
    };
    const size_t dot = name.rfind('.');
    if (dot == std::string::npos || name.find('/', dot) != std::string::npos) return "application/octet-stream";
    const auto it = kTypes.find(lower(name.substr(dot + 1)));
    return it == kTypes.end() ? "application/octet-stream" : it->second;
}

WebDecoded web_decode(const std::vector<uint8_t>& sieve, const std::string& expected, uint64_t max_bytes)
{
    // 1. The link's hash, against the instructions as they came: nothing else is looked at first.
    const std::string want = expected_hex(expected);
    const std::string got = sha256_hex(sieve);
    if (got != want) throw std::runtime_error("the .sieve's SHA-256 is " + got + ", not the " + want + " its link states: refused");
    // 2. Read as instructions, and only what the plugin shows.
    const Manifest m = manifest_of_installer(address_of_raw(sieve));
    if (!m.graph.empty()) throw std::runtime_error("it is a map (a graph part): open it in the hallway's node graph, not the browser");
    if (!m.lines.empty()) throw std::runtime_error("it carries items of Sieve's lines: install it with Sieve");
    if (!m.unpacked.empty()) throw std::runtime_error("it carries an archive as what it holds, not as the archive: install it with Sieve");
    const ManifestEntry* file = nullptr;
    size_t files = 0;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        ++files;
        file = &e;
        if (!e.remake.empty()) throw std::runtime_error("it remakes an archive (" + e.remake + "): install it with Sieve");
        if (e.way.rfind("item", 0) == 0) throw std::runtime_error("it carries items of Sieve's lines: install it with Sieve");
    }
    if (files != 1 || !file)
        throw std::runtime_error("it holds " + std::to_string(files) + " files: the plugin shows one file a link names (install the rest with Sieve)");
    if (file->size > max_bytes)
        throw std::runtime_error("it holds " + std::to_string(file->size) + " bytes, more than the " + std::to_string(max_bytes) + " the plugin unpacks");
    if (!m.with_contents || m.contents.size() != file->size) throw std::runtime_error("its contents are not the size its listing gives");
    WebDecoded d;
    d.bytes = m.contents;
    d.sha256 = sha256_hex(d.bytes);
    // Its listing names the file by its SHA-256, or (v7 on, "-") leaves that to the tree's, which
    // reading the instructions has checked already; either way, checked once more here.
    if (m.items ? tree_sha256(m) != m.tree : (file->sha256.empty() || file->sha256 == "-" || lower(file->sha256) != d.sha256))
        throw std::runtime_error("its file is not the one its listing names: refused");
    const size_t slash = file->path.rfind('/');
    d.name = slash == std::string::npos ? file->path : file->path.substr(slash + 1);
    d.type = media_type_of(d.name);
    return d;
}

namespace {

bool read_message(std::istream& in, std::string& text)
{
    uint8_t len[4];
    if (!in.read(reinterpret_cast<char*>(len), 4)) return false;
    uint32_t n = 0;
    std::memcpy(&n, len, 4); // this machine's byte order, as native messaging sends it
    if (n > (uint32_t(64) << 20)) throw std::runtime_error("a message of more than 64 MiB");
    text.assign(n, '\0');
    return bool(in.read(text.data(), std::streamsize(n)));
}

void write_message(std::ostream& out, const json::Value& v)
{
    const std::string text = json::write(v);
    const uint32_t n = uint32_t(text.size());
    char len[4];
    std::memcpy(len, &n, 4);
    out.write(len, 4);
    out.write(text.data(), std::streamsize(text.size()));
    out.flush();
}

json::Value num(uint64_t n) { return json::Value::make_number(std::to_string(n)); }

} // namespace

int run_web_host(std::istream& in, std::ostream& out, const std::string& version, uint64_t max_bytes)
{
    // A part is 699,999 bytes, 933,332 characters of base64: under the 1 MB a message may be.
    constexpr size_t kPart = 699999;
    std::string text;
    while (read_message(in, text))
    {
        json::Value id = num(0);
        try
        {
            const json::Value req = json::parse(text);
            if (const json::Value* i = req.find("id")) id = *i;
            const std::string op = req.at("op").string();
            if (op == "hello")
            {
                write_message(out, json::Value::make_object({{"id", id}, {"ok", json::Value::make_bool(true)}, {"sieve", json::Value::make_string(version)}}));
                continue;
            }
            if (op != "decode") throw std::runtime_error("no such request: " + op);
            const json::Value* h = req.find("sha256");
            const std::vector<uint8_t> given = bytes_of_base64(req.at("data").string());
            const auto t0 = std::chrono::steady_clock::now();
            const WebDecoded d = web_decode(given, h ? h->string() : std::string(), max_bytes);
            char seconds[32]; // how long checking and decoding took here, for the viewer's readout
            std::snprintf(seconds, sizeof seconds, "%.3f", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
            const size_t parts = (d.bytes.size() + kPart - 1) / kPart;
            write_message(out, json::Value::make_object({{"id", id},
                                                         {"ok", json::Value::make_bool(true)},
                                                         {"name", json::Value::make_string(d.name)},
                                                         {"type", json::Value::make_string(d.type)},
                                                         {"size", num(d.bytes.size())},
                                                         {"sha256", json::Value::make_string(d.sha256)},
                                                         {"seconds", json::Value::make_number(seconds)},
                                                         {"parts", num(parts)}}));
            for (size_t k = 0; k < parts; ++k)
            {
                const size_t at = k * kPart, n = std::min(kPart, d.bytes.size() - at);
                write_message(out, json::Value::make_object({{"id", id}, {"part", num(k)}, {"data", json::Value::make_string(base64_of(d.bytes.data() + at, n))}}));
            }
        }
        catch (const std::exception& e)
        {
            write_message(out, json::Value::make_object({{"id", id}, {"ok", json::Value::make_bool(false)}, {"error", json::Value::make_string(e.what())}}));
        }
    }
    return 0;
}

} // namespace sieve::cli
