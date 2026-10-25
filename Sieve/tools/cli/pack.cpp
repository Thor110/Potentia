// Sieve CLI -- an installer's files packed (pack.hpp).

#include "cli/pack.hpp"
#include "cli/timings.hpp"

#include "sieve/filekind.hpp"
#include "sieve/rope_wave.hpp"
#include "sieve/sha256.hpp"
#include "sieve/stop.hpp"
#include "unpack_7z.hpp"
#include "cli/vault.hpp"

// bzlib.h includes <windows.h> on Windows: without these first, its min and max macros break
// std::min and std::max throughout this file.
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <bzlib.h>
#include <lzma.h>
#include <zlib.h>
#define ZSTD_STATIC_LINKING_ONLY // its frame header's reader (zstd is linked statically, at a pinned version)
#include <zstd.h>
#include <brotli/decode.h>
#include <brotli/encode.h>
#include <lz4frame.h>
#include "cli/preflate_glue.hpp"
#include <archive.h>
#include <archive_entry.h>

#include "Bra.h"
#include "Lzma2Enc.h"
#include "LzmaEnc.h"
#include "7z.h"
#include "7zCrc.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <thread>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

// ---- threads (IDEAS §19.99): work made of separate trials runs them at once, each scored as one
// thread scores it and the best taken in the same order (ties to the earlier), so the instructions
// are the same at any thread count. SIEVE_PACK_THREADS sets how many (1: one, as before); unless
// given, every core.
unsigned pack_threads()
{
    static const unsigned n = [] {
        if (const char* e = std::getenv("SIEVE_PACK_THREADS"))
            if (const int v = std::atoi(e); v >= 1) return unsigned(v);
        return std::max(1u, std::thread::hardware_concurrency());
    }();
    return n;
}

// Memory free now, in bytes, or 0 where it cannot be known.
uint64_t memory_free()
{
#if defined(_WIN32)
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof m;
    return GlobalMemoryStatusEx(&m) ? uint64_t(m.ullAvailPhys) : 0;
#elif defined(__linux__) && defined(_SC_AVPHYS_PAGES)
    const long pages = sysconf(_SC_AVPHYS_PAGES), size = sysconf(_SC_PAGESIZE);
    return pages > 0 && size > 0 ? uint64_t(pages) * uint64_t(size) : 0;
#else
    return 0;
#endif
}

// How many LZMA2 passes of dictionary `dict` may run at once: the threads, and no more than half
// the memory free holds (a pass at preset 9 takes about twelve times its dictionary); where the
// memory free cannot be known, two for a dictionary above 16 MiB.
unsigned passes_at_once(uint32_t dict)
{
    const unsigned t = pack_threads();
    const uint64_t each = uint64_t(dict) * 12 + (uint64_t(16) << 20), free = memory_free();
    if (free == 0) return dict > (16u << 20) ? std::min(t, 2u) : t;
    return unsigned(std::clamp<uint64_t>(free / 2 / each, 1, t));
}

// f(i) for every i below n, on up to `threads` threads (this one among them), each taking the next
// index there is; the threads take on this thread's flag to stop.
template <class F>
void for_each_index(size_t n, unsigned threads, F&& f)
{
    threads = unsigned(std::min<size_t>(std::max(1u, threads), n));
    if (threads <= 1)
    {
        for (size_t i = 0; i < n; ++i) f(i);
        return;
    }
    std::atomic<size_t> next{0};
    std::exception_ptr failed;
    std::mutex failed_mx;
    const std::atomic<int>* stop = current_stop();
    auto work = [&] {
        try
        {
            for (size_t i; (i = next.fetch_add(1)) < n;) f(i);
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(failed_mx);
            if (!failed) failed = std::current_exception();
            next = n; // the rest left undone
        }
    };
    std::vector<std::thread> pool;
    for (unsigned k = 1; k < threads; ++k)
        pool.emplace_back([&, stop] {
            const StopScope scope(stop, {});
            work();
        });
    work();
    for (std::thread& t : pool) t.join();
    if (failed) std::rethrow_exception(failed);
}

// The smallest LZMA2 dictionary size that holds `n` bytes: 2 or 3 times a power of two, at least
// 4 KiB, at most 64 MiB (preset 9's).
uint32_t dictionary_for(uint64_t n)
{
    for (uint32_t p = 0; p < 40; ++p)
    {
        const uint64_t d = uint64_t(2 | (p & 1)) << (p / 2 + 11);
        if (d >= 4096 && (d >= n || d >= (uint64_t(64) << 20))) return uint32_t(d);
    }
    return uint32_t(64) << 20;
}

// Raw LZMA2 (after the x86 filter, with `x86`) at `preset`, with dictionary `dict`; `lclppb` its
// literal and position settings (lc * 100 + lp * 10 + pb; -1: the preset's own, lc 3 lp 0 pb 2), and
// `nice` its match length (0: the preset's own).
std::vector<uint8_t> encode_lzma2(const std::vector<uint8_t>& data, uint32_t dict, bool x86, uint32_t preset, int lclppb = -1,
                                  uint32_t nice = 0, bool abortable = true)
{
    if (data.empty()) return {};
    lzma_options_lzma opt;
    if (lzma_lzma_preset(&opt, preset)) throw std::runtime_error("liblzma: no such preset");
    opt.dict_size = dict;
    if (lclppb >= 0)
    {
        opt.lc = uint32_t(lclppb / 100);
        opt.lp = uint32_t(lclppb / 10 % 10);
        opt.pb = uint32_t(lclppb % 10);
    }
    if (nice) opt.nice_len = nice;
    const lzma_filter plain[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    const lzma_filter branch[] = {{LZMA_FILTER_X86, nullptr}, {LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    // Fed a MiB at a time (the same stream as at once), so that a force stop (sieve/stop.hpp)
    // abandons it within one: then nothing is returned for non-empty data (Abandoned).
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_raw_encoder(&z, x86 ? branch : plain) != LZMA_OK) throw std::runtime_error("liblzma: cannot start");
    std::vector<uint8_t> out(data.size() + data.size() / 16 + 4096);
    constexpr size_t kPiece = size_t(1) << 20;
    size_t at = 0;
    z.next_out = out.data();
    z.avail_out = out.size();
    lzma_ret r = LZMA_OK;
    while (r != LZMA_STREAM_END)
    {
        if (abortable && force_requested())
        {
            lzma_end(&z);
            return {};
        }
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = data.data() + at;
        z.avail_in = n;
        at += n;
        const lzma_action action = at == data.size() ? LZMA_FINISH : LZMA_RUN;
        do
        {
            if (z.avail_out == 0)
            {
                const size_t done = out.size();
                out.resize(out.size() * 2);
                z.next_out = out.data() + done;
                z.avail_out = out.size() - done;
            }
            r = lzma_code(&z, action);
            if (r != LZMA_OK && r != LZMA_STREAM_END)
            {
                lzma_end(&z);
                throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
            }
        } while (z.avail_in > 0 || z.avail_out == 0 || (action == LZMA_FINISH && r != LZMA_STREAM_END));
    }
    out.resize(size_t(z.total_out));
    lzma_end(&z);
    return out;
}

void* sdk_alloc(ISzAllocPtr, size_t size) { return size ? std::malloc(size) : nullptr; }
void sdk_free(ISzAllocPtr, void* address) { std::free(address); }
const ISzAlloc kSdkAlloc = {sdk_alloc, sdk_free};

// The same raw LZMA2 by 7-Zip's own encoder (the LZMA SDK's, single-threaded): level 9, the
// longest match (273), its literal and position settings `lclppb`, dictionary `dict`; for the x86
// stream, after the same x86 filter (the SDK's, as liblzma's: start 0). Its stream is LZMA2 as
// liblzma's is, read by the same readers; it parses differently, so it is sometimes the smaller.
std::vector<uint8_t> encode_lzma2_sdk(const std::vector<uint8_t>& data, uint32_t dict, bool x86, int lclppb)
{
    if (data.empty()) return {};
    std::vector<uint8_t> in = data;
    if (x86)
    {
        UInt32 state = Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL;
        z7_BranchConvSt_X86_Enc(in.data(), in.size(), 0, &state);
    }
    CLzma2EncHandle h = Lzma2Enc_Create(&kSdkAlloc, &kSdkAlloc);
    if (!h) throw std::runtime_error("LZMA SDK: out of memory");
    CLzma2EncProps p;
    Lzma2EncProps_Init(&p);
    p.lzmaProps.level = 9;
    p.lzmaProps.dictSize = dict;
    p.lzmaProps.lc = lclppb / 100;
    p.lzmaProps.lp = lclppb / 10 % 10;
    p.lzmaProps.pb = lclppb % 10;
    p.lzmaProps.fb = 273;
    p.lzmaProps.algo = 1;
    p.lzmaProps.btMode = 1;
    p.lzmaProps.numHashBytes = 4;
    p.lzmaProps.numThreads = 1;
    p.blockSize = LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID;
    p.numBlockThreads_Max = p.numBlockThreads_Reduced = p.numTotalThreads = 1;
    Lzma2EncProps_Normalize(&p);
    std::vector<uint8_t> out(in.size() + in.size() / 16 + 4096);
    size_t n = out.size();
    SRes r = Lzma2Enc_SetProps(h, &p);
    if (r == SZ_OK)
    {
        Lzma2Enc_SetDataSize(h, in.size());
        // A force stop (sieve/stop.hpp) breaks it off at its next word of progress.
        static const ICompressProgress kForceStop = {[](ICompressProgressPtr, UInt64, UInt64) -> SRes {
            return force_requested() ? SZ_ERROR_PROGRESS : SZ_OK;
        }};
        r = Lzma2Enc_Encode2(h, nullptr, out.data(), &n, nullptr, in.data(), in.size(), &kForceStop);
    }
    Lzma2Enc_Destroy(h);
    if (r == SZ_ERROR_PROGRESS) return {}; // abandoned
    if (r != SZ_OK) throw std::runtime_error("LZMA SDK: could not compress (" + std::to_string(int(r)) + ")");
    out.resize(n);
    return out;
}

// LZMA2's settings chosen for the stream (`tune`): its literal and position settings (lc, lp, pb)
// tried on its first MiB at preset 6, a few that suit different kinds of bytes (text, bytes with no
// alignment, 2- and 4-byte samples), and the best of them used on the whole stream at preset 9
// extreme, with the preset's match length and a shorter one, and by 7-Zip's own encoder too. The
// stream as the preset makes it is one of the candidates, so a stream never comes out larger than
// without this. The reader needs nothing new: LZMA2 writes lc, lp and pb into its own stream, and
// every reader takes them from there (only the dictionary is in the instructions).
// `abortable`: a force stop may abandon its first pass too, and then nothing is returned (the
// caller has a way round it); every later pass may always be abandoned, the best before it kept.
std::vector<uint8_t> pack_stream(const std::vector<uint8_t>& data, uint32_t dict, bool x86, uint32_t preset = 9 | LZMA_PRESET_EXTREME,
                                 bool tune = false, bool abortable = false)
{
    if (!tune || data.empty())
    {
        const timings::Scope t("pack.lzma2.quick");
        return encode_lzma2(data, dict, x86, preset, -1, 0, abortable); // (empty for data: abandoned by a force stop)
    }
    // How it goes, for whoever is watching (sieve/stop.hpp), and asked to stop: the best so far kept.
    const std::string what = std::string(x86 ? "the x86 stream (" : "the lzma2 stream (") + std::to_string(data.size()) + " bytes)";
    std::vector<uint8_t> best;
    auto said = [&](const std::string& next) {
        progress(what + ": best so far " + std::to_string(best.size()) + " bytes; " + next, double(best.size()), double(data.size()));
    };
    // The preset's own pass, and LZMA2's settings tried on the first MiB at preset 6, at once.
    progress(what + ": packing it, and trying LZMA2's settings on its first MiB");
    constexpr size_t kSample = size_t(1) << 20;
    const std::vector<uint8_t> sample(data.begin(), data.begin() + std::ptrdiff_t(std::min(data.size(), kSample)));
    const uint32_t sample_dict = dictionary_for(sample.size());
    int chosen = 302;
    for_each_index(2, passes_at_once(dict), [&](size_t task) {
        if (task == 0)
        {
            const timings::Scope t("pack.lzma2.first-pass");
            best = encode_lzma2(data, dict, x86, preset, -1, 0, abortable);
            return;
        }
        const timings::Scope ts("pack.lzma2.settings-trial");
        const std::vector<int> tried = {302, 0, 400, 200, 22, 402, 111};
        std::vector<size_t> sizes(tried.size(), 0);
        for_each_index(tried.size(), pack_threads(), [&](size_t k) {
            if (!stop_requested()) sizes[k] = encode_lzma2(sample, sample_dict, x86, 6, tried[k]).size();
        });
        size_t chosen_size = sizes[0];
        for (size_t k = 1; k < tried.size(); ++k)
            if (sizes[k] && sizes[k] < chosen_size) // in order, ties to the earlier: as one thread chose
            {
                chosen = tried[k];
                chosen_size = sizes[k];
            }
    });
    if (best.empty() || stop_requested()) return best; // (empty: the first pass abandoned by a force stop)
    // Then the stream at the settings chosen, at the preset's match length and at 64, and by
    // 7-Zip's own encoder (22 October 2026), at once; each kept where it is smaller, in that order.
    said("packing it at lc" + std::to_string(chosen / 100) + " lp" + std::to_string(chosen / 10 % 10) + " pb" + std::to_string(chosen % 10) +
         ", at two match lengths and with 7-Zip's encoder");
    struct Pass
    {
        uint32_t nice;
        bool sdk;
    };
    std::vector<Pass> passes;
    for (const uint32_t nice : {0u, 64u})
        if (!(chosen == 302 && nice == 0)) passes.push_back({nice, false}); // the preset's own: already made
    passes.push_back({0, true});
    std::vector<std::vector<uint8_t>> made(passes.size());
    for_each_index(passes.size(), passes_at_once(dict), [&](size_t k) {
        if (stop_requested()) return;
        if (passes[k].sdk)
        {
            const timings::Scope tk("pack.lzma2.7zip-pass");
            made[k] = encode_lzma2_sdk(data, dict, x86, chosen);
        }
        else
        {
            const timings::Scope tp("pack.lzma2.tuned-pass");
            made[k] = encode_lzma2(data, dict, x86, preset, chosen, passes[k].nice); // (empty: abandoned)
        }
    });
    for (std::vector<uint8_t>& t : made)
        if (!t.empty() && t.size() < best.size()) best = std::move(t);
    said("done");
    return best;
}

// What a stream of `data` packs to, quickly (preset 6): for choosing between two ways.
// (Abandoned by a force stop, it gives 0: whoever asked checks force_requested() before using it.)
std::atomic<uint32_t> g_sample_floor{0};

uint64_t estimate(const std::vector<uint8_t>& data) { return data.empty() ? 0 : pack_stream(data, dictionary_for(data.size()), false, 6, false, true).size(); }

// v13: a file cut into pieces, each coded by the way that suits it: LZMA2 (z, carried in the lzma2
// stream) where its bytes repeat, the rope (r, all of them joined and coded as one) where they do
// not repeat but are unevenly spread. A phone's photo: its metadata to LZMA2, its picture to the
// rope. The pieces in order, each its kind and length; empty where one way would take it all.
struct Pieces
{
    std::vector<std::pair<char, uint64_t>> list;
    std::vector<uint8_t> z, r;
};

Pieces pieces_of_uncached(const std::vector<uint8_t>& b);

// The cuts made in this run, by the file's SHA-256: the File Locator packs a file and weighs it, and
// both cut it into pieces; on a 52 MB film each cut took 170 s (IDEAS §19.97). At most four kept, and
// at most 1 GiB of them; a cut a stop ended is not kept.
std::mutex g_pieces_mx;
std::vector<std::pair<std::string, std::shared_ptr<const Pieces>>> g_pieces;

Pieces pieces_of(const std::vector<uint8_t>& b)
{
    Sha256 h;
    h.update(std::span<const uint8_t>(b.data(), b.size()));
    const std::string key = Sha256::hex(h.finish());
    {
        std::lock_guard<std::mutex> lock(g_pieces_mx);
        for (const auto& [k, p] : g_pieces)
            if (k == key) return *p;
    }
    Pieces p = pieces_of_uncached(b);
    if (stop_requested()) return p;
    std::lock_guard<std::mutex> lock(g_pieces_mx);
    uint64_t held = b.size();
    for (const auto& [k, q] : g_pieces) held += q->z.size() + q->r.size();
    while (!g_pieces.empty() && (g_pieces.size() >= 4 || held > (uint64_t(1) << 30)))
    {
        held -= g_pieces.front().second->z.size() + g_pieces.front().second->r.size();
        g_pieces.erase(g_pieces.begin());
    }
    if (held <= (uint64_t(1) << 30)) g_pieces.emplace_back(key, std::make_shared<const Pieces>(p));
    return p;
}

Pieces pieces_of_uncached(const std::vector<uint8_t>& b)
{
    constexpr size_t B = 4096;
    Pieces out;
    if (b.size() < 4 * B) return out;
    // Each byte's cost under the whole file's counts (what the rope pays at least, near enough), and
    // each block's under LZMA2 alone: a block is the rope's unless LZMA2 codes it smaller.
    std::vector<uint64_t> count(256, 0);
    for (const uint8_t v : b) ++count[v];
    std::vector<double> bits(256, 0);
    for (int v = 0; v < 256; ++v)
        if (count[size_t(v)]) bits[size_t(v)] = std::log2(double(b.size()) / double(count[size_t(v)]));
    auto rope_cost = [&](size_t from, size_t to) {
        double c = 0;
        for (size_t i = from; i < to; ++i) c += bits[b[i]];
        return c / 8;
    };
    auto lzma_cost = [&](size_t from, size_t to) {
        const std::vector<uint8_t> part(b.begin() + std::ptrdiff_t(from), b.begin() + std::ptrdiff_t(to));
        return double(pack_stream(part, dictionary_for(part.size()), false, 6, false, true).size());
    };
    const size_t blocks = (b.size() + B - 1) / B;
    std::vector<char> kind(blocks);
    std::vector<double> gain(blocks); // what LZMA2 saves over the rope on the block (negative: it costs)
    progress("cutting it into pieces: weighing its " + std::to_string(blocks) + " blocks");
    std::optional<timings::Scope> t_blocks(std::in_place, "pack.pieces.blocks");
    for_each_index(blocks, pack_threads(), [&](size_t k) { // each block on its own: the same at any thread count
        if (stop_requested()) return;
        const size_t from = k * B, to = std::min(b.size(), from + B);
        gain[k] = rope_cost(from, to) - lzma_cost(from, to);
        kind[k] = gain[k] > 0 ? 'z' : 'r';
    });
    t_blocks.reset();
    if (stop_requested()) return out; // asked to stop: not cut (the file whole is still a way)
    // Runs: one that saves less than 64 bytes over the other way goes to its neighbours' way.
    for (int pass = 0; pass < 2; ++pass)
        for (size_t a = 0; a < blocks;)
        {
            size_t e = a;
            double g = 0;
            while (e < blocks && kind[e] == kind[a]) g += kind[a] == 'z' ? gain[e] : -gain[e], ++e;
            const bool inner = a > 0 && e < blocks;
            if (g < 64 && (inner || pass == 1) && !(a == 0 && e == blocks))
                for (size_t k = a; k < e; ++k) kind[k] = kind[a] == 'z' ? 'r' : 'z';
            a = e;
        }
    std::vector<std::pair<char, uint64_t>> runs;
    for (size_t k = 0; k < blocks; ++k)
    {
        const uint64_t len = std::min(b.size(), (k + 1) * B) - k * B;
        if (!runs.empty() && runs.back().first == kind[k]) runs.back().second += len;
        else runs.emplace_back(kind[k], len);
    }
    if (runs.size() < 2) return out;
    // Each boundary moved, up to a block either way, to where the two ways cost least: the LZMA2
    // side packed again at each step (where its run is no longer than 1 MiB), over a window of at
    // most 16 KiB beyond the steps, fixed while the boundary moves (so every step is weighed over
    // the same bytes but its own; the whole run, packed at every step, made a file of many runs take
    // hours: IDEAS §19.97), the rope's side counted over the two blocks around the boundary. The
    // steps are 512 bytes first, then 64 within 448 of the best of those (IDEAS §19.99: on every
    // file measured, the same places as every 64 bytes over 64 KiB, in a twelfth of the time).
    const timings::Scope t_bounds("pack.pieces.boundaries");
    constexpr uint64_t kWindow = uint64_t(16) << 10;
    uint64_t start = 0;
    for (size_t j = 0; j + 1 < runs.size(); ++j)
    {
        const uint64_t edge = start + runs[j].second, end = edge + runs[j + 1].second;
        const bool z_left = runs[j].first == 'z';
        const uint64_t zlen = z_left ? runs[j].second : runs[j + 1].second;
        if (j % 16 == 0) progress("cutting it into pieces: moving boundary " + std::to_string(j + 1) + " of " + std::to_string(runs.size() - 1));
        if (stop_requested()) // asked to stop: this boundary and the rest stay where the blocks put them
        {
            start += runs[j].second;
            continue;
        }
        const uint64_t lo0 = std::max(start + 64, edge >= B ? edge - B : 0), hi0 = std::min(end - 64, edge + B);
        if (zlen <= (uint64_t(1) << 20) && lo0 <= edge && edge <= hi0)
        {
            const uint64_t lo = std::max(start + 64, edge >= B ? edge - B : 0), hi = std::min(end - 64, edge + B);
            const uint64_t wlo = edge >= B ? edge - B : 0, whi = std::min(uint64_t(b.size()), edge + B);
            const uint64_t zfrom = std::max(start, lo >= kWindow ? lo - kWindow : 0), zto = std::min(end, hi + kWindow);
            // Every step weighed at once, then the cheapest taken in order (ties to the earlier).
            auto weigh = [&](uint64_t first, uint64_t last, uint64_t step, uint64_t& best_e) {
                const size_t steps = size_t((last - first) / step + 1);
                std::vector<double> cost(steps);
                for_each_index(steps, pack_threads(), [&](size_t k) {
                    const uint64_t e = first + step * uint64_t(k);
                    cost[k] = z_left ? lzma_cost(size_t(zfrom), size_t(e)) + rope_cost(size_t(e), size_t(whi))
                                     : lzma_cost(size_t(e), size_t(zto)) + rope_cost(size_t(wlo), size_t(e));
                });
                double best = -1;
                for (size_t k = 0; k < steps; ++k)
                    if (best < 0 || cost[k] < best)
                    {
                        best = cost[k];
                        best_e = first + step * uint64_t(k);
                    }
            };
            const uint64_t first = lo + (edge - lo) % 64;
            const uint64_t last = first + (hi - first) / 64 * 64;
            uint64_t best_e = edge;
            weigh(first, last, 512, best_e);
            weigh(std::max(first, best_e >= 448 ? best_e - 448 : 0), std::min(last, best_e + 448), 64, best_e);
            runs[j].second = best_e - start;
            runs[j + 1].second = end - best_e;
        }
        start += runs[j].second;
    }
    uint64_t pos = 0;
    for (const auto& [k, len] : runs)
    {
        (k == 'z' ? out.z : out.r).insert((k == 'z' ? out.z : out.r).end(), b.begin() + std::ptrdiff_t(pos), b.begin() + std::ptrdiff_t(pos + len));
        pos += len;
    }
    out.list = std::move(runs);
    return out;
}

std::string pieces_text(const std::vector<std::pair<char, uint64_t>>& list)
{
    std::string t;
    for (const auto& [k, len] : list) t += std::string(t.empty() ? "" : " ") + k + std::to_string(len);
    return t;
}

// ---- v14: archives ("Optimise Archive Formats")

bool is_xz(const std::vector<uint8_t>& b)
{
    static const uint8_t sig[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
    return b.size() >= 32 && std::equal(sig, sig + 6, b.begin());
}

// What an xz file holds, or nothing if liblzma cannot read it (or it is not one stream).
std::optional<std::vector<uint8_t>> xz_inner(const std::vector<uint8_t>& b)
{
    lzma_stream z = LZMA_STREAM_INIT;
    // (Concatenated: streams one after another, as `cat a.xz b.xz` makes, are read as one; such a
    // file is never remade, the recipe being for one stream, and is carried as what it holds.)
    if (lzma_stream_decoder(&z, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK) return std::nullopt;
    std::vector<uint8_t> out;
    std::vector<uint8_t> buf(1 << 20);
    z.next_in = b.data();
    z.avail_in = b.size();
    lzma_ret r = LZMA_OK;
    while (r == LZMA_OK)
    {
        z.next_out = buf.data();
        z.avail_out = buf.size();
        r = lzma_code(&z, LZMA_FINISH);
        out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
    }
    const bool whole = r == LZMA_STREAM_END && z.avail_in == 0;
    lzma_end(&z);
    if (!whole) return std::nullopt;
    return out;
}

// ---- gzip and zip (v17's gzip remade exactly; both carried as their contents)

bool is_gzip(const std::vector<uint8_t>& b) { return b.size() >= 18 && b[0] == 0x1F && b[1] == 0x8B && b[2] == 8; }

// One gzip member at `start`: its header's length (to the deflate), or 0 if it is not one this reads.
size_t gzip_header_size(const std::vector<uint8_t>& b, size_t start = 0)
{
    if (b.size() < start + 18 || b[start] != 0x1F || b[start + 1] != 0x8B || b[start + 2] != 8) return 0;
    const uint8_t flg = b[start + 3];
    if (flg & 0xE0) return 0;
    size_t at = start + 10;
    if (flg & 4)
    {
        if (at + 2 > b.size()) return 0;
        at += 2 + size_t(b[at] | b[at + 1] << 8);
    }
    for (const uint8_t bit : {uint8_t(8), uint8_t(16)})
        if (flg & bit)
        {
            while (at < b.size() && b[at]) ++at;
            ++at;
        }
    if (flg & 2) at += 2;
    return at < b.size() ? at - start : 0;
}

// Raw deflate undone (zlib), all of it, with how much input it took; nothing if it is not whole.
std::optional<std::vector<uint8_t>> inflate_raw(const uint8_t* in, size_t n, size_t* used, uint64_t expect = UINT64_MAX)
{
    z_stream z{};
    if (inflateInit2(&z, -15) != Z_OK) return std::nullopt;
    std::vector<uint8_t> out;
    std::vector<uint8_t> buf(1 << 20);
    z.next_in = const_cast<Bytef*>(in);
    z.avail_in = uInt(std::min<size_t>(n, 0xFFFFFFFFu));
    int r = Z_OK;
    while (r == Z_OK)
    {
        z.next_out = buf.data();
        z.avail_out = uInt(buf.size());
        r = inflate(&z, Z_NO_FLUSH);
        out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
        if (out.size() > expect) break;
        if (r == Z_BUF_ERROR && z.avail_in == 0) break;
    }
    const bool whole = r == Z_STREAM_END;
    if (used) *used = n - z.avail_in;
    inflateEnd(&z);
    if (!whole) return std::nullopt;
    return out;
}

// What a gzip file holds: every member of it (pigz -i and `cat a.gz b.gz` make several), each one's
// CRC and length checked, nothing after the last; and how many members.
std::optional<std::vector<uint8_t>> gzip_inner(const std::vector<uint8_t>& b, size_t* members = nullptr)
{
    std::vector<uint8_t> all;
    size_t at = 0, n = 0;
    while (at < b.size())
    {
        const size_t h = gzip_header_size(b, at);
        if (!h || b.size() < at + h + 8) return std::nullopt;
        size_t used = 0;
        auto out = inflate_raw(b.data() + at + h, b.size() - at - h, &used);
        if (!out || at + h + used + 8 > b.size()) return std::nullopt;
        const uint8_t* t = b.data() + at + h + used;
        const uint32_t crc = uint32_t(t[0]) | uint32_t(t[1]) << 8 | uint32_t(t[2]) << 16 | uint32_t(t[3]) << 24;
        const uint32_t len = uint32_t(t[4]) | uint32_t(t[5]) << 8 | uint32_t(t[6]) << 16 | uint32_t(t[7]) << 24;
        if (crc32(0, out->data(), uInt(out->size())) != crc || uint32_t(out->size()) != len) return std::nullopt;
        if (all.empty()) all = std::move(*out);
        else all.insert(all.end(), out->begin(), out->end());
        at += h + used + 8;
        ++n;
    }
    if (members) *members = n;
    return all;
}

std::vector<uint8_t> deflate_raw(const std::vector<uint8_t>& in, int level, int mem)
{
    z_stream z{};
    if (deflateInit2(&z, level, Z_DEFLATED, -15, mem, Z_DEFAULT_STRATEGY) != Z_OK) throw std::runtime_error("zlib could not start");
    std::vector<uint8_t> out(deflateBound(&z, uLong(in.size())) + 16);
    z.next_in = const_cast<Bytef*>(in.data());
    z.avail_in = uInt(in.size());
    z.next_out = out.data();
    z.avail_out = uInt(out.size());
    const int r = deflate(&z, Z_FINISH);
    const size_t n = out.size() - z.avail_out;
    deflateEnd(&z);
    if (r != Z_STREAM_END) throw std::runtime_error("zlib could not pack");
    out.resize(n);
    return out;
}

std::string hex_of(const uint8_t* p, size_t n)
{
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) (s += d[p[i] >> 4]) += d[p[i] & 15];
    return s;
}

// A zip's entries, read into memory: stored and deflated ones, each checked against its CRC; the
// one top folder left out as a 7z's is. Nothing where any entry is encrypted, in another method,
// zip64, or named so it could leave its folder.
std::optional<std::vector<sieve::install::InMemory>> zip_entries(const std::vector<uint8_t>& b, bool leave_out_top = true)
{
    auto u16 = [&](size_t at) { return uint32_t(b[at]) | uint32_t(b[at + 1]) << 8; };
    auto u32 = [&](size_t at) { return u16(at) | u16(at + 2) << 16; };
    if (b.size() < 22 || u32(0) != 0x04034b50) return std::nullopt;
    size_t eocd = std::string::npos;
    for (size_t at = b.size() - 22 + 1; at-- > 0 && b.size() - at <= 65557;)
        if (u32(at) == 0x06054b50)
        {
            eocd = at;
            break;
        }
    if (eocd == std::string::npos) return std::nullopt;
    const uint32_t count = u16(eocd + 10), cd_size = u32(eocd + 12), cd_at = u32(eocd + 16);
    if (count == 0xFFFF || cd_at == 0xFFFFFFFFu || size_t(cd_at) + cd_size > eocd) return std::nullopt;
    std::vector<sieve::install::InMemory> out;
    size_t at = cd_at;
    for (uint32_t k = 0; k < count; ++k)
    {
        if (at + 46 > b.size() || u32(at) != 0x02014b50) return std::nullopt;
        const uint32_t flags = u16(at + 8), method = u16(at + 10), crc = u32(at + 16), csize = u32(at + 20), usize = u32(at + 24);
        const uint32_t nlen = u16(at + 28), xlen = u16(at + 30), clen = u16(at + 32), local = u32(at + 42);
        if (at + 46 + nlen > b.size()) return std::nullopt;
        std::string name(b.begin() + std::ptrdiff_t(at + 46), b.begin() + std::ptrdiff_t(at + 46 + nlen));
        at += 46 + nlen + xlen + clen;
        if ((flags & 1) || (method != 0 && method != 8) || csize == 0xFFFFFFFFu || usize == 0xFFFFFFFFu) return std::nullopt;
        // Its name: relative, '/' between parts, no "." or ".." part, no drive, no backslash.
        if (name.empty() || name[0] == '/' || name.find('\\') != std::string::npos || name.find(':') != std::string::npos) return std::nullopt;
        for (size_t p = 0; p <= name.size();)
        {
            size_t slash = name.find('/', p);
            if (slash == std::string::npos) slash = name.size();
            const std::string part = name.substr(p, slash - p);
            if (part == "." || part == ".." || (part.empty() && slash != name.size() && slash + 1 != name.size())) return std::nullopt;
            p = slash + 1;
        }
        sieve::install::InMemory m;
        m.entry.dir = name.back() == '/';
        m.entry.path = m.entry.dir ? name.substr(0, name.size() - 1) : name;
        if (!m.entry.dir)
        {
            if (size_t(local) + 30 > b.size() || u32(local) != 0x04034b50) return std::nullopt;
            const size_t data = size_t(local) + 30 + u16(local + 26) + u16(local + 28);
            if (data + csize > b.size()) return std::nullopt;
            if (method == 0)
            {
                if (csize != usize) return std::nullopt;
                m.bytes.assign(b.begin() + std::ptrdiff_t(data), b.begin() + std::ptrdiff_t(data + csize));
            }
            else
            {
                size_t used = 0;
                auto got = inflate_raw(b.data() + data, csize, &used, usize);
                if (!got || got->size() != usize) return std::nullopt;
                m.bytes = std::move(*got);
            }
            if (crc32(0, m.bytes.data(), uInt(m.bytes.size())) != crc) return std::nullopt;
            m.entry.size = m.bytes.size();
        }
        out.push_back(std::move(m));
    }
    // One top folder: every entry is it or inside it, at least one inside: left out.
    if (!out.empty() && leave_out_top)
    {
        const std::string top = out[0].entry.path.substr(0, out[0].entry.path.find('/'));
        bool one = true, inside = false;
        for (const auto& m : out)
        {
            const std::string& p = m.entry.path;
            if (p.substr(0, p.find('/')) != top || (p == top && !m.entry.dir)) one = false;
            if (p.size() > top.size()) inside = true;
        }
        if (one && inside)
        {
            std::vector<sieve::install::InMemory> kept;
            for (auto& m : out)
                if (m.entry.path != top)
                {
                    m.entry.path = m.entry.path.substr(top.size() + 1);
                    kept.push_back(std::move(m));
                }
            out = std::move(kept);
        }
    }
    return out;
}

// ---- The archives libarchive reads (v20): RAR (4 and 5), cab, ISO 9660, LHA, cpio, ar and WARC,
// read whole into memory, as zip's are, and carried as their contents (never remade: Sieve has none
// of their encoders). Nothing where an entry is a link, a device, encrypted, named twice, or named so
// it could leave its folder; or where libarchive cannot read every entry. Its format's name, the "u"
// line's, in `format`.
// Whether a name is well-formed UTF-8 (no overlong forms, surrogates or values past U+10FFFF).
bool valid_utf8(std::string_view s)
{
    for (size_t i = 0; i < s.size();)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        const size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (!n || i + n > s.size()) return false;
        uint32_t v = n == 1 ? c : n == 2 ? c & 31 : n == 3 ? c & 15 : c & 7;
        for (size_t k = 1; k < n; ++k)
        {
            const unsigned char d = static_cast<unsigned char>(s[i + k]);
            if ((d >> 6) != 2) return false;
            v = v << 6 | (d & 63);
        }
        if ((n == 2 && v < 0x80) || (n == 3 && v < 0x800) || (n == 4 && v < 0x10000) || v > 0x10FFFF || (v >= 0xD800 && v < 0xE000)) return false;
        i += n;
    }
    return true;
}

std::optional<std::vector<sieve::install::InMemory>> libarchive_entries(const std::vector<uint8_t>& b, std::string& format, size_t* links_left_out = nullptr)
{
    // Extracting (links_left_out given: read_any_archive): tar and zip too, a link left out rather
    // than the archive refused, and the top folder kept, as an archiver's Extract does.
    const bool extract = links_left_out != nullptr;
    struct archive* a = archive_read_new();
    if (!a) return std::nullopt;
    archive_read_support_format_rar(a);
    archive_read_support_format_rar5(a);
    archive_read_support_format_cab(a);
    archive_read_support_format_iso9660(a);
    archive_read_support_format_lha(a);
    archive_read_support_format_cpio(a);
    archive_read_support_format_ar(a);
    archive_read_support_format_warc(a);
    if (extract)
    {
        archive_read_support_format_tar(a);
        archive_read_support_format_zip(a);
    }
    std::vector<sieve::install::InMemory> out;
    std::unordered_set<std::string> names;
    bool ok = archive_read_open_memory(a, b.data(), b.size()) == ARCHIVE_OK;
    struct archive_entry* en = nullptr;
    while (ok)
    {
        const int r = archive_read_next_header(a, &en);
        if (r == ARCHIVE_EOF) break;
        // (A warning, extracting, is a name it could not convert: taken as held, below.)
        if (r != ARCHIVE_OK && !(extract && r == ARCHIVE_WARN)) { ok = false; break; }
        // (A name libarchive cannot give as UTF-8, one in a code page, is refused: the listing's names are UTF-8.)
        const char* raw = archive_entry_pathname_utf8(en);
        std::string name = raw ? raw : "";
        // Extracting: where libarchive cannot convert a name (a tar's, in the "C" locale this runs
        // in), the name as the archive holds it, where that is UTF-8 already, as a tar's nearly
        // always is.
        if (!raw && extract)
            if (const char* as_held = archive_entry_pathname(en); as_held && valid_utf8(as_held)) name = as_held;
        while (name.size() > 2 && name.compare(0, 2, "./") == 0) name.erase(0, 2);
        const auto type = archive_entry_filetype(en); // (auto: libarchive's own mode type, mode_t having none on Windows)
        const bool dir = type == AE_IFDIR;
        if (dir && (name == "." || name == "./")) continue; // the root's own entry (an ISO's)
        if (dir && !name.empty() && name.back() == '/') name.pop_back();
        if (extract && (type == AE_IFLNK || archive_entry_hardlink(en)))
        {
            ++*links_left_out;
            continue;
        }
        if ((type != AE_IFREG && !dir) || archive_entry_hardlink(en) || archive_entry_is_encrypted(en) || name.empty() || name[0] == '/' ||
            name.find('\\') != std::string::npos || name.find(':') != std::string::npos)
        {
            ok = false;
            break;
        }
        for (size_t p = 0; ok && p <= name.size();)
        {
            size_t slash = name.find('/', p);
            if (slash == std::string::npos) slash = name.size();
            const std::string part = name.substr(p, slash - p);
            if (part.empty() || part == "." || part == "..") ok = false;
            p = slash + 1;
        }
        if (!ok || !names.insert(name).second) { ok = false; break; }
        sieve::install::InMemory m;
        m.entry.dir = dir;
        m.entry.path = name;
        if (!dir)
        {
            std::vector<uint8_t> buf(size_t(1) << 20);
            for (;;)
            {
                const la_ssize_t n = archive_read_data(a, buf.data(), buf.size());
                if (n < 0) { ok = false; break; }
                if (n == 0) break;
                m.bytes.insert(m.bytes.end(), buf.data(), buf.data() + n);
            }
            if (ok && archive_entry_size_is_set(en) && uint64_t(archive_entry_size(en)) != m.bytes.size()) ok = false;
        }
        if (ok) out.push_back(std::move(m));
    }
    const int fmt = archive_format(a) & ARCHIVE_FORMAT_BASE_MASK;
    archive_read_free(a);
    if (!ok || out.empty()) return std::nullopt;
    format = fmt == ARCHIVE_FORMAT_RAR || fmt == ARCHIVE_FORMAT_RAR_V5 ? "rar" : fmt == ARCHIVE_FORMAT_CAB ? "cab" : fmt == ARCHIVE_FORMAT_ISO9660 ? "iso" :
             fmt == ARCHIVE_FORMAT_LHA ? "lha" : fmt == ARCHIVE_FORMAT_CPIO ? "cpio" : fmt == ARCHIVE_FORMAT_AR ? "ar" : fmt == ARCHIVE_FORMAT_WARC ? "warc" :
             extract && fmt == ARCHIVE_FORMAT_TAR ? "tar" : extract && fmt == ARCHIVE_FORMAT_ZIP ? "zip" : "";
    if (format.empty()) return std::nullopt;
    if (extract) return out;
    // One top folder: every entry is it or inside it, at least one inside: left out (as a zip's).
    const std::string top = out[0].entry.path.substr(0, out[0].entry.path.find('/'));
    bool one = true, inside = false;
    for (const auto& m : out)
    {
        const std::string& p = m.entry.path;
        if (p.substr(0, p.find('/')) != top || (p == top && !m.entry.dir)) one = false;
        if (p.size() > top.size()) inside = true;
    }
    if (one && inside)
    {
        std::vector<sieve::install::InMemory> kept;
        for (auto& m : out)
            if (m.entry.path != top)
            {
                m.entry.path = m.entry.path.substr(top.size() + 1);
                kept.push_back(std::move(m));
            }
        out = std::move(kept);
    }
    return out;
}

// ---- Deflate made again exactly, whoever made it (v21, 25 October 2026): preflate (pinned in
// CMakeLists.txt) splits a deflate stream into what it holds and its difference from preflate's own
// deflate, a few bytes where zlib made it, about a tenth of the stream where 7-Zip did; and makes the
// stream again from the two. A file holding deflate streams (a gzip, a zip) is carried as a skeleton
// and the data it holds: the skeleton a varint of its segments' count, then each segment, a byte and
// its numbers: 0 bytes as they are (a varint of how many, then them); 1 data stored (a varint of how
// much of the data); 2 a deflate stream (a varint of how much of the data it holds, a varint of its
// difference's length, then the difference). The data is what follows the skeleton, taken in order.

// LZMA2's dictionary from its byte (0 to 40).
uint32_t lzma2_dict(uint8_t b) { return b >= 40 ? 0xFFFFFFFFu : (2u | (b & 1u)) << (b / 2 + 11); }

// The block size 7-Zip's LZMA2 uses when it encodes in blocks (its threads': Lzma2EncProps_Normalize).
uint64_t lzma2_auto_block(uint8_t dict_byte)
{
    const uint64_t dict = lzma2_dict(dict_byte);
    uint64_t bs = dict << 2;
    bs = std::clamp<uint64_t>(bs, uint64_t(1) << 20, uint64_t(1) << 28);
    if (bs < dict) bs = dict;
    bs += (uint64_t(1) << 20) - 1;
    bs &= ~((uint64_t(1) << 20) - 1);
    return bs;
}

// An LZMA2 (dictionary byte `props[0]`) or LZMA (`props` its 5 bytes: lc/lp/pb and dictionary) stream
// by the LZMA SDK at `level`, single-threaded, in blocks of `block` bytes (0: solid), after the x86
// filter where `x86`: as 7-Zip writes a 7z's (LZMA with no end mark, its size known).
std::vector<uint8_t> sdk_stream(std::vector<uint8_t> in, bool lzma2, const uint8_t* props, int level, uint64_t block, bool x86)
{
    if (x86)
    {
        UInt32 state = Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL;
        z7_BranchConvSt_X86_Enc(in.data(), in.size(), 0, &state);
    }
    std::vector<uint8_t> out(in.size() + in.size() / 2 + 65536);
    size_t n = out.size();
    SRes r;
    if (lzma2)
    {
        CLzma2EncHandle h = Lzma2Enc_Create(&kSdkAlloc, &kSdkAlloc);
        if (!h) throw std::runtime_error("LZMA SDK: out of memory");
        CLzma2EncProps p;
        Lzma2EncProps_Init(&p);
        p.lzmaProps.level = level;
        p.lzmaProps.dictSize = lzma2_dict(props[0]);
        p.lzmaProps.numThreads = 1;
        p.blockSize = block ? block : LZMA2_ENC_PROPS_BLOCK_SIZE_SOLID;
        r = Lzma2Enc_SetProps(h, &p);
        if (r == SZ_OK)
        {
            Lzma2Enc_SetDataSize(h, in.size());
            r = Lzma2Enc_Encode2(h, nullptr, out.data(), &n, nullptr, in.data(), in.size(), nullptr);
        }
        Lzma2Enc_Destroy(h);
    }
    else
    {
        CLzmaEncProps p;
        LzmaEncProps_Init(&p);
        p.level = level;
        p.dictSize = uint32_t(props[1]) | uint32_t(props[2]) << 8 | uint32_t(props[3]) << 16 | uint32_t(props[4]) << 24;
        p.lc = props[0] % 9;
        p.lp = props[0] / 9 % 5;
        p.pb = props[0] / 45;
        p.numThreads = 1;
        Byte pr[5];
        SizeT ps = 5, on = n;
        r = props[0] < 225 ? LzmaEncode(out.data(), &on, in.data(), in.size(), &p, pr, &ps, 0, nullptr, &kSdkAlloc, &kSdkAlloc) : SZ_ERROR_PARAM;
        n = on;
    }
    if (r != SZ_OK) throw std::runtime_error("LZMA SDK: could not compress (" + std::to_string(int(r)) + ")");
    out.resize(n);
    return out;
}

void sk_put_varint(std::vector<uint8_t>& out, uint64_t v)
{
    for (; v >= 0x80; v >>= 7) out.push_back(uint8_t(v | 0x80));
    out.push_back(uint8_t(v));
}

uint64_t sk_get_varint(std::span<const uint8_t> in, size_t& at)
{
    uint64_t v = 0;
    for (int shift = 0; shift < 64; shift += 7)
    {
        if (at >= in.size()) throw std::runtime_error("a deflate skeleton ends early");
        const uint8_t c = in[at++];
        v |= uint64_t(c & 0x7F) << shift;
        if (!(c & 0x80)) return v;
    }
    throw std::runtime_error("a deflate skeleton's number is too long");
}

struct DeflateSkeleton
{
    std::vector<uint8_t> head; // the segments, without their count
    std::vector<uint8_t> data;
    uint64_t segments = 0;
    void literal(const uint8_t* p, size_t n)
    {
        if (!n) return;
        head.push_back(0);
        sk_put_varint(head, n);
        head.insert(head.end(), p, p + n);
        ++segments;
    }
    void stored(const uint8_t* p, size_t n)
    {
        head.push_back(1);
        sk_put_varint(head, n);
        data.insert(data.end(), p, p + n);
        ++segments;
    }
    bool deflated(const uint8_t* p, size_t n)
    {
        const std::vector<uint8_t> raw(p, p + n);
        std::vector<uint8_t> un, diff;
        if (!preflate_split(raw, un, diff)) return false;
        head.push_back(2);
        sk_put_varint(head, un.size());
        sk_put_varint(head, diff.size());
        head.insert(head.end(), diff.begin(), diff.end());
        data.insert(data.end(), un.begin(), un.end());
        ++segments;
        return true;
    }
    // An LZMA2 (kind 3) or LZMA (kind 4) stream, as 7-Zip writes them in a 7z, made again by the LZMA
    // SDK's encoder: `un` what it holds (before any x86 filter), its settings as the 7z names them, the
    // level found by trying (each first on its first MiB, where it is longer); false if none gives it
    // back.
    bool lzma_stream(const uint8_t* p, size_t n, const std::vector<uint8_t>& un, bool lzma2, const uint8_t* props, bool x86)
    {
        for (const int level : {5, 7, 9, 0, 1, 3, 6, 4, 2, 8})
            for (const uint64_t block : {uint64_t(0), uint64_t(1)})
            {
                if (!lzma2 && block) continue;
                const uint64_t bs = block ? lzma2_auto_block(props[0]) : 0;
                if (un.size() > (size_t(2) << 20))
                {
                    const std::vector<uint8_t> part(un.begin(), un.begin() + (std::ptrdiff_t(1) << 20));
                    const std::vector<uint8_t> f = sdk_stream(part, lzma2, props, level, bs, x86);
                    const size_t k = std::min<size_t>(f.size(), 16384) / 2;
                    if (k == 0 || k > n || !std::equal(f.begin(), f.begin() + std::ptrdiff_t(k), p)) continue;
                }
                const std::vector<uint8_t> f = sdk_stream(un, lzma2, props, level, bs, x86);
                if (f.size() != n || !std::equal(f.begin(), f.end(), p)) continue;
                head.push_back(lzma2 ? 3 : 4);
                sk_put_varint(head, un.size());
                head.insert(head.end(), props, props + (lzma2 ? 1 : 5));
                head.push_back(uint8_t(level));
                head.push_back(uint8_t((block ? 1 : 0) | (x86 ? 2 : 0)));
                data.insert(data.end(), un.begin(), un.end());
                ++segments;
                return true;
            }
        return false;
    }
    // The inner bytes: the segments' count, the segments, then the data; and how long the skeleton is.
    std::vector<uint8_t> inner(size_t& skeleton) const
    {
        std::vector<uint8_t> out;
        sk_put_varint(out, segments);
        out.insert(out.end(), head.begin(), head.end());
        skeleton = out.size();
        out.insert(out.end(), data.begin(), data.end());
        return out;
    }
};

// A file made again from its skeleton and data (inner: the skeleton's `skeleton` bytes, then the data).
std::vector<uint8_t> deflate_skeleton_remake(const std::vector<uint8_t>& inner, size_t skeleton)
{
    if (skeleton > inner.size()) throw std::runtime_error("a deflate skeleton longer than what holds it");
    size_t at = 0, data = skeleton;
    const std::span<const uint8_t> sk(inner.data(), skeleton);
    auto take_data = [&](uint64_t n) {
        if (n > inner.size() - data) throw std::runtime_error("a deflate skeleton asks for more data than there is");
        const size_t from = data;
        data += size_t(n);
        return from;
    };
    std::vector<uint8_t> out;
    const uint64_t count = sk_get_varint(sk, at);
    for (uint64_t k = 0; k < count; ++k)
    {
        if (at >= sk.size()) throw std::runtime_error("a deflate skeleton ends early");
        const uint8_t kind = sk[at++];
        const uint64_t n = sk_get_varint(sk, at);
        if (kind == 0)
        {
            if (n > sk.size() - at) throw std::runtime_error("a deflate skeleton ends early");
            out.insert(out.end(), sk.begin() + std::ptrdiff_t(at), sk.begin() + std::ptrdiff_t(at + n));
            at += size_t(n);
        }
        else if (kind == 1)
        {
            const size_t from = take_data(n);
            out.insert(out.end(), inner.begin() + std::ptrdiff_t(from), inner.begin() + std::ptrdiff_t(from + n));
        }
        else if (kind == 2)
        {
            const uint64_t d = sk_get_varint(sk, at);
            if (d > sk.size() - at) throw std::runtime_error("a deflate skeleton ends early");
            const std::vector<uint8_t> diff(sk.begin() + std::ptrdiff_t(at), sk.begin() + std::ptrdiff_t(at + d));
            at += size_t(d);
            const size_t from = take_data(n);
            const std::vector<uint8_t> un(inner.begin() + std::ptrdiff_t(from), inner.begin() + std::ptrdiff_t(from + n));
            std::vector<uint8_t> raw;
            if (!preflate_join(un, diff, raw)) throw std::runtime_error("preflate could not make a deflate stream again");
            out.insert(out.end(), raw.begin(), raw.end());
        }
        else if (kind == 3 || kind == 4)
        {
            const size_t np = kind == 3 ? 1 : 5;
            if (np + 2 > sk.size() - at) throw std::runtime_error("a skeleton ends early");
            const uint8_t* props = sk.data() + at;
            const int level = sk[at + np];
            const uint8_t flags = sk[at + np + 1];
            at += np + 2;
            if (level > 9 || (flags & ~3) || (kind == 4 && (flags & 1)) || (kind == 3 && props[0] > 40))
                throw std::runtime_error("a skeleton's LZMA settings this Sieve does not know");
            const size_t from = take_data(n);
            const std::vector<uint8_t> un(inner.begin() + std::ptrdiff_t(from), inner.begin() + std::ptrdiff_t(from + n));
            const std::vector<uint8_t> f = sdk_stream(un, kind == 3, props, level, (flags & 1) ? lzma2_auto_block(props[0]) : 0, (flags & 2) != 0);
            out.insert(out.end(), f.begin(), f.end());
        }
        else throw std::runtime_error("a deflate skeleton's segment this Sieve does not know");
    }
    if (at != sk.size() || data != inner.size()) throw std::runtime_error("a deflate skeleton that does not use all it is given");
    return out;
}

// A gzip of one member or more as a skeleton: each member's header and trailer as they are, its
// deflate made again by preflate. Nothing where a member is not one this reads.
std::optional<DeflateSkeleton> gzip_skeleton(const std::vector<uint8_t>& b)
{
    DeflateSkeleton s;
    size_t at = 0;
    while (at < b.size())
    {
        const size_t h = gzip_header_size(b, at);
        if (!h || b.size() < at + h + 8) return std::nullopt;
        size_t used = 0;
        if (!inflate_raw(b.data() + at + h, b.size() - at - h, &used) || at + h + used + 8 > b.size()) return std::nullopt;
        s.literal(b.data() + at, h);
        if (!s.deflated(b.data() + at + h, used)) return std::nullopt;
        s.literal(b.data() + at + h + used, 8);
        at += h + used + 8;
    }
    return s;
}

// A 7z as a skeleton: its start header, its compressed header and any block of data it cannot make
// again as they are; each block of one LZMA or LZMA2 coder (or that after 7-Zip's x86 filter) made
// again by the LZMA SDK. Nothing where the LZMA SDK cannot read it or makes no block of it again.
std::optional<DeflateSkeleton> sevenz_skeleton(const std::vector<uint8_t>& b)
{
    struct Mem
    {
        ISeekInStream vt;
        const std::vector<uint8_t>* d;
        size_t at;
    } mem{};
    mem.d = &b;
    mem.vt.Read = [](ISeekInStreamPtr pp, void* buf, size_t* size) -> SRes {
        Mem* m = (Mem*)pp;
        const size_t k = std::min(*size, m->d->size() - m->at);
        std::memcpy(buf, m->d->data() + m->at, k);
        m->at += k;
        *size = k;
        return SZ_OK;
    };
    mem.vt.Seek = [](ISeekInStreamPtr pp, Int64* pos, ESzSeek origin) -> SRes {
        Mem* m = (Mem*)pp;
        Int64 base = origin == SZ_SEEK_SET ? 0 : origin == SZ_SEEK_CUR ? Int64(m->at) : Int64(m->d->size());
        base += *pos;
        if (base < 0 || uint64_t(base) > m->d->size()) return SZ_ERROR_READ;
        m->at = size_t(base);
        *pos = base;
        return SZ_OK;
    };
    static const bool crc_ready = (CrcGenerateTable(), true);
    (void)crc_ready;
    std::vector<Byte> look_buf(size_t(1) << 18);
    CLookToRead2 look;
    LookToRead2_CreateVTable(&look, False);
    look.buf = look_buf.data();
    look.bufSize = look_buf.size();
    look.realStream = &mem.vt;
    LookToRead2_INIT(&look);
    CSzArEx db;
    SzArEx_Init(&db);
    struct Free
    {
        CSzArEx* db;
        ~Free() { SzArEx_Free(db, &kSdkAlloc); }
    } free_db{&db};
    if (SzArEx_Open(&db, &look.vt, &kSdkAlloc, &kSdkAlloc) != SZ_OK) return std::nullopt;
    struct Block
    {
        uint64_t pos, size;
        uint32_t folder;
    };
    std::vector<Block> blocks;
    for (uint32_t i = 0; i < db.db.NumFolders; ++i)
    {
        const uint32_t ps = db.db.FoStartPackStreamIndex[i];
        if (db.db.FoStartPackStreamIndex[i + 1] != ps + 1) return std::nullopt; // several packed streams (BCJ2)
        blocks.push_back({db.dataPos + db.db.PackPositions[ps], db.db.PackPositions[ps + 1] - db.db.PackPositions[ps], i});
    }
    std::sort(blocks.begin(), blocks.end(), [](const Block& x, const Block& y) { return x.pos < y.pos; });
    DeflateSkeleton s;
    uint64_t cur = 0;
    size_t remade = 0;
    for (const Block& k : blocks)
    {
        if (k.pos < cur || k.pos + k.size > b.size()) return std::nullopt;
        s.literal(b.data() + cur, size_t(k.pos - cur));
        cur = k.pos + k.size;
        CSzFolder fo;
        CSzData sd;
        sd.Data = db.db.CodersData + db.db.FoCodersOffsets[k.folder];
        sd.Size = db.db.FoCodersOffsets[k.folder + 1] - db.db.FoCodersOffsets[k.folder];
        bool done = false;
        if (SzGetNextFolderItem(&fo, &sd) == SZ_OK && (fo.NumCoders == 1 || fo.NumCoders == 2))
        {
            const CSzCoderInfo& c = fo.Coders[0];
            const bool x86 = fo.NumCoders == 2 && fo.Coders[1].MethodID == 0x03030103 && fo.Coders[1].PropsSize == 0;
            const bool lzma2 = c.MethodID == 0x21 && c.PropsSize == 1, lzma = c.MethodID == 0x030101 && c.PropsSize == 5;
            const uint64_t un_size = SzAr_GetFolderUnpackSize(&db.db, k.folder);
            if ((lzma2 || lzma) && (fo.NumCoders == 1 || x86) && un_size <= (uint64_t(1) << 32))
            {
                std::vector<uint8_t> un = std::vector<uint8_t>(size_t(un_size));
                if (SzAr_DecodeFolder(&db.db, k.folder, &look.vt, db.dataPos, un.data(), un.size(), &kSdkAlloc) == SZ_OK)
                    done = s.lzma_stream(b.data() + k.pos, size_t(k.size), un, lzma2, db.db.CodersData + db.db.FoCodersOffsets[k.folder] + c.PropsOffset, x86);
            }
        }
        if (done) ++remade;
        else s.literal(b.data() + k.pos, size_t(k.size));
    }
    if (!remade) return std::nullopt;
    s.literal(b.data() + cur, size_t(b.size() - cur));
    return s;
}

// A zip as a skeleton: everything but its entries' data as it is (local headers, data descriptors,
// the central directory, the end record), each entry's data stored or, where deflated, made again by
// preflate; an entry in any other method kept as it is too. Nothing for an encrypted entry, zip64,
// entries that overlap, or a deflate preflate cannot read.
std::optional<DeflateSkeleton> zip_skeleton(const std::vector<uint8_t>& b)
{
    auto u16 = [&](size_t at) { return uint32_t(b[at]) | uint32_t(b[at + 1]) << 8; };
    auto u32 = [&](size_t at) { return u16(at) | u16(at + 2) << 16; };
    if (b.size() < 22 || u32(0) != 0x04034b50) return std::nullopt;
    size_t eocd = std::string::npos;
    for (size_t at = b.size() - 22 + 1; at-- > 0 && b.size() - at <= 22 + 65535;)
        if (u32(at) == 0x06054b50) { eocd = at; break; }
    if (eocd == std::string::npos) return std::nullopt;
    const uint32_t count = u16(eocd + 10), cd_size = u32(eocd + 12), cd_at = u32(eocd + 16);
    if (count == 0xFFFF || cd_at == 0xFFFFFFFFu || size_t(cd_at) + cd_size > eocd) return std::nullopt;
    struct Entry { size_t data, csize; uint32_t method; };
    std::vector<Entry> es;
    for (size_t at = cd_at, k = 0; k < count; ++k)
    {
        if (at + 46 > b.size() || u32(at) != 0x02014b50) return std::nullopt;
        const uint32_t flags = u16(at + 8), method = u16(at + 10), csize = u32(at + 20), nlen = u16(at + 28), xlen = u16(at + 30), clen = u16(at + 32);
        const uint32_t local = u32(at + 42);
        if ((flags & 1) || csize == 0xFFFFFFFFu || local == 0xFFFFFFFFu) return std::nullopt;
        if (size_t(local) + 30 > b.size() || u32(local) != 0x04034b50) return std::nullopt;
        const size_t data = size_t(local) + 30 + u16(local + 26) + u16(local + 28);
        if (data + csize > b.size()) return std::nullopt;
        es.push_back({data, csize, method});
        at += 46 + nlen + xlen + clen;
    }
    std::sort(es.begin(), es.end(), [](const Entry& x, const Entry& y) { return x.data < y.data; });
    DeflateSkeleton s;
    size_t cur = 0;
    for (const Entry& e : es)
    {
        if (e.data < cur) return std::nullopt; // overlapping entries
        s.literal(b.data() + cur, e.data - cur);
        if (e.method == 8 && e.csize > 0)
        {
            if (!s.deflated(b.data() + e.data, e.csize)) return std::nullopt;
        }
        else if (e.method == 0) s.stored(b.data() + e.data, e.csize);
        else s.literal(b.data() + e.data, e.csize);
        cur = e.data + e.csize;
    }
    s.literal(b.data() + cur, b.size() - cur);
    return s;
}

const char* xz_check_name(int id) { return id == 0 ? "none" : id == 1 ? "crc32" : id == 4 ? "crc64" : id == 10 ? "sha256" : nullptr; }

// ---- bzip2 and zstd (v19, 25 October 2026): read with their own libraries (bzip2 1.0.8, zstd
// 1.5.7, pinned in CMakeLists.txt), carried as their contents, and remade exactly where the same
// library makes the same bytes.

bool is_bz2(const std::vector<uint8_t>& b) { return b.size() >= 14 && b[0] == 'B' && b[1] == 'Z' && b[2] == 'h' && b[3] >= '1' && b[3] <= '9'; }

// What a bzip2 file holds, every stream of it (pbzip2 writes one a block), and how many streams;
// nothing if libbzip2 cannot read it whole.
std::optional<std::vector<uint8_t>> bz2_inner(const std::vector<uint8_t>& b, size_t* streams = nullptr)
{
    std::vector<uint8_t> out;
    std::vector<char> buf(1 << 20);
    size_t at = 0, n = 0;
    while (at < b.size())
    {
        if (b.size() - at < 14 || b[at] != 'B' || b[at + 1] != 'Z' || b[at + 2] != 'h') return std::nullopt; // not another stream
        bz_stream z{};
        if (BZ2_bzDecompressInit(&z, 0, 0) != BZ_OK) return std::nullopt;
        int r = BZ_OK;
        size_t fed = at;
        while (r == BZ_OK)
        {
            if (z.avail_in == 0 && fed < b.size())
            {
                const size_t take = std::min<size_t>(b.size() - fed, size_t(1) << 30);
                z.next_in = const_cast<char*>(reinterpret_cast<const char*>(b.data() + fed));
                z.avail_in = unsigned(take);
                fed += take;
            }
            z.next_out = buf.data();
            z.avail_out = unsigned(buf.size());
            r = BZ2_bzDecompress(&z);
            out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
            if (r == BZ_OK && z.avail_in == 0 && fed >= b.size() && z.avail_out == buf.size()) r = BZ_UNEXPECTED_EOF;
        }
        at = fed - z.avail_in;
        BZ2_bzDecompressEnd(&z);
        if (r != BZ_STREAM_END) return std::nullopt;
        ++n;
    }
    if (streams) *streams = n;
    return out;
}

// libbzip2's compressor at a block size (1 to 9), as the bzip2 program uses it.
std::vector<uint8_t> bz2_pack(const std::vector<uint8_t>& in, int level)
{
    bz_stream z{};
    if (BZ2_bzCompressInit(&z, level, 0, 30) != BZ_OK) throw std::runtime_error("libbzip2 could not start");
    std::vector<uint8_t> out;
    std::vector<char> buf(1 << 20);
    size_t fed = 0;
    int r = BZ_RUN_OK;
    for (;;)
    {
        if (z.avail_in == 0 && fed < in.size())
        {
            const size_t take = std::min<size_t>(in.size() - fed, size_t(1) << 30);
            z.next_in = const_cast<char*>(reinterpret_cast<const char*>(in.data() + fed));
            z.avail_in = unsigned(take);
            fed += take;
        }
        z.next_out = buf.data();
        z.avail_out = unsigned(buf.size());
        r = BZ2_bzCompress(&z, fed >= in.size() ? BZ_FINISH : BZ_RUN);
        out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
        if (r == BZ_STREAM_END) break;
        if (r != BZ_RUN_OK && r != BZ_FINISH_OK) break;
    }
    BZ2_bzCompressEnd(&z);
    if (r != BZ_STREAM_END) throw std::runtime_error("libbzip2 could not pack");
    return out;
}

bool is_zstd(const std::vector<uint8_t>& b) { return b.size() >= 9 && b[0] == 0x28 && b[1] == 0xB5 && b[2] == 0x2F && b[3] == 0xFD; }

// What a zstd file holds, every frame of it, and how many frames; nothing if libzstd cannot read
// it whole (windows up to 2 GiB are allowed, as zstd --long=31 makes).
std::optional<std::vector<uint8_t>> zstd_inner(const std::vector<uint8_t>& b, size_t* frames = nullptr)
{
    size_t n = 0;
    for (size_t at = 0; at < b.size();)
    {
        const size_t f = ZSTD_findFrameCompressedSize(b.data() + at, b.size() - at);
        if (ZSTD_isError(f) || f == 0) return std::nullopt;
        at += f;
        ++n;
    }
    ZSTD_DCtx* d = ZSTD_createDCtx();
    if (!d) return std::nullopt;
    ZSTD_DCtx_setParameter(d, ZSTD_d_windowLogMax, 31);
    std::vector<uint8_t> out, buf(ZSTD_DStreamOutSize());
    ZSTD_inBuffer in{b.data(), b.size(), 0};
    size_t r = 1;
    while (in.pos < in.size)
    {
        ZSTD_outBuffer o{buf.data(), buf.size(), 0};
        r = ZSTD_decompressStream(d, &o, &in);
        if (ZSTD_isError(r)) break;
        out.insert(out.end(), buf.data(), buf.data() + o.pos);
        if (o.pos == 0 && in.pos == in.size) break;
    }
    // (Its last output, if the output buffer was full when the input ran out.)
    for (; !ZSTD_isError(r) && r != 0;)
    {
        ZSTD_outBuffer o{buf.data(), buf.size(), 0};
        r = ZSTD_decompressStream(d, &o, &in);
        if (ZSTD_isError(r) || o.pos == 0) break;
        out.insert(out.end(), buf.data(), buf.data() + o.pos);
    }
    ZSTD_freeDCtx(d);
    if (ZSTD_isError(r) || r != 0) return std::nullopt;
    if (frames) *frames = n;
    return out;
}

// libzstd's compressor at a level, with the checksum and the content size written or not, as the
// zstd program (single-threaded) writes a frame; `prefix`: only that much of the input, but the frame
// as for the whole (its header and its first blocks), to test a level quickly.
std::vector<uint8_t> zstd_pack(const std::vector<uint8_t>& in, int level, bool checksum, bool size_flag, size_t prefix = SIZE_MAX)
{
    ZSTD_CCtx* c = ZSTD_createCCtx();
    if (!c) throw std::runtime_error("libzstd could not start");
    ZSTD_CCtx_setParameter(c, ZSTD_c_compressionLevel, level);
    ZSTD_CCtx_setParameter(c, ZSTD_c_checksumFlag, checksum ? 1 : 0);
    ZSTD_CCtx_setParameter(c, ZSTD_c_contentSizeFlag, size_flag ? 1 : 0);
    ZSTD_CCtx_setPledgedSrcSize(c, in.size());
    const size_t n = std::min(prefix, in.size());
    std::vector<uint8_t> out(ZSTD_compressBound(in.size()) + 64);
    ZSTD_inBuffer i{in.data(), n, 0};
    ZSTD_outBuffer o{out.data(), out.size(), 0};
    size_t r = 0;
    do
        r = ZSTD_compressStream2(c, &o, &i, n == in.size() ? ZSTD_e_end : ZSTD_e_flush);
    while (!ZSTD_isError(r) && r != 0);
    ZSTD_freeCCtx(c);
    if (ZSTD_isError(r)) throw std::runtime_error(std::string("libzstd could not pack: ") + ZSTD_getErrorName(r));
    out.resize(o.pos);
    return out;
}

// ---- lz4, brotli, lzma and lzip (v20, 25 October 2026): lz4 1.10.0 and brotli 1.2.1 pinned in
// CMakeLists.txt, lzma and lzip read by liblzma. lz4, brotli and lzma files are remade exactly where
// the library gives them back (measured: the lz4, brotli and lzma programs' files, every one tried);
// lzip's own encoder is not liblzma's, so an lzip file is carried as what it holds.

bool is_lz4(const std::vector<uint8_t>& b) { return b.size() >= 11 && b[0] == 0x04 && b[1] == 0x22 && b[2] == 0x4D && b[3] == 0x18; }

// What an lz4 file holds, every frame of it (skippable frames passed over), and how many frames;
// nothing if liblz4 cannot read it whole.
std::optional<std::vector<uint8_t>> lz4_inner(const std::vector<uint8_t>& b, size_t* frames = nullptr)
{
    LZ4F_dctx* d = nullptr;
    if (LZ4F_isError(LZ4F_createDecompressionContext(&d, LZ4F_VERSION))) return std::nullopt;
    std::vector<uint8_t> out, buf(size_t(4) << 20);
    size_t at = 0, n = 0, hint = 1;
    bool ok = true;
    while (at < b.size())
    {
        size_t src = b.size() - at, dst = buf.size();
        hint = LZ4F_decompress(d, buf.data(), &dst, b.data() + at, &src, nullptr);
        if (LZ4F_isError(hint)) { ok = false; break; }
        out.insert(out.end(), buf.data(), buf.data() + dst);
        at += src;
        if (hint == 0) ++n; // a frame ended
        if (src == 0 && dst == 0) { ok = false; break; }
    }
    // (Output still held when the input ran out.)
    while (ok && hint != 0)
    {
        size_t src = 0, dst = buf.size();
        hint = LZ4F_decompress(d, buf.data(), &dst, nullptr, &src, nullptr);
        if (LZ4F_isError(hint) || dst == 0) { ok = false; break; }
        out.insert(out.end(), buf.data(), buf.data() + dst);
        if (hint == 0) ++n;
    }
    LZ4F_freeDecompressionContext(d);
    if (!ok || hint != 0 || n == 0) return std::nullopt;
    if (frames) *frames = n;
    return out;
}

// One lz4 frame as liblz4 makes it, as the lz4 program does: its level (above 2, LZ4 HC; 0 to 2,
// the fast coder; below 0, faster still), the largest block (4: 64 KiB to 7: 4 MiB), blocks linked
// or not, and the three checks/sizes written or not.
std::vector<uint8_t> lz4_pack(const std::vector<uint8_t>& in, int level, int block, bool linked, bool block_sum, bool content_sum, bool size_flag)
{
    LZ4F_preferences_t p{};
    p.frameInfo.blockSizeID = LZ4F_blockSizeID_t(block);
    p.frameInfo.blockMode = linked ? LZ4F_blockLinked : LZ4F_blockIndependent;
    p.frameInfo.contentChecksumFlag = content_sum ? LZ4F_contentChecksumEnabled : LZ4F_noContentChecksum;
    p.frameInfo.blockChecksumFlag = block_sum ? LZ4F_blockChecksumEnabled : LZ4F_noBlockChecksum;
    p.frameInfo.contentSize = size_flag ? in.size() : 0;
    p.compressionLevel = level;
    std::vector<uint8_t> out(LZ4F_compressFrameBound(in.size(), &p));
    const size_t n = LZ4F_compressFrame(out.data(), out.size(), in.data(), in.size(), &p);
    if (LZ4F_isError(n)) throw std::runtime_error(std::string("liblz4 could not pack: ") + LZ4F_getErrorName(n));
    out.resize(n);
    return out;
}

// What a brotli file holds; nothing if it is not one stream that ends where the file does. (Brotli
// has no mark of its own: only a file named .br is tried.)
std::optional<std::vector<uint8_t>> brotli_inner(const std::vector<uint8_t>& b)
{
    BrotliDecoderState* d = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!d) return std::nullopt;
    std::vector<uint8_t> out, buf(size_t(1) << 20);
    const uint8_t* next_in = b.data();
    size_t avail_in = b.size();
    BrotliDecoderResult r = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
    while (r == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
    {
        uint8_t* next_out = buf.data();
        size_t avail_out = buf.size();
        r = BrotliDecoderDecompressStream(d, &avail_in, &next_in, &avail_out, &next_out, nullptr);
        out.insert(out.end(), buf.data(), next_out);
    }
    BrotliDecoderDestroyInstance(d);
    if (r != BROTLI_DECODER_RESULT_SUCCESS || avail_in != 0) return std::nullopt;
    return out;
}

// A brotli stream's window, from its first bits (10 to 24); 0 for the large window or none.
int brotli_window(const std::vector<uint8_t>& b)
{
    if (b.empty()) return 0;
    const uint32_t v = b[0] | (b.size() > 1 ? uint32_t(b[1]) << 8 : 0);
    if (!(v & 1)) return 16;
    const uint32_t n = (v >> 1) & 7;
    if (n) return int(17 + n);
    const uint32_t m = (v >> 4) & 7;
    if (m == 1) return 0; // the large window (an extension the brotli program does not write by default)
    return m ? int(8 + m) : 17;
}

// Brotli as its library makes it, as the brotli program does: one stream at a quality (0 to 11) and
// window (10 to 24), the generic mode, the input's size given beforehand. `against`: stop, and return
// what was made so far, at the first byte that is not that file's (a quality is ruled out quickly).
std::vector<uint8_t> brotli_pack(const std::vector<uint8_t>& in, int quality, int window, const std::vector<uint8_t>* against = nullptr)
{
    BrotliEncoderState* e = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
    if (!e) throw std::runtime_error("brotli could not start");
    BrotliEncoderSetParameter(e, BROTLI_PARAM_QUALITY, uint32_t(quality));
    BrotliEncoderSetParameter(e, BROTLI_PARAM_LGWIN, uint32_t(window));
    BrotliEncoderSetParameter(e, BROTLI_PARAM_MODE, BROTLI_MODE_GENERIC);
    BrotliEncoderSetParameter(e, BROTLI_PARAM_SIZE_HINT, uint32_t(std::min<size_t>(in.size(), 1u << 30)));
    std::vector<uint8_t> out;
    const uint8_t* next_in = in.data();
    size_t avail_in = in.size();
    bool ok = true;
    for (;;)
    {
        size_t avail_out = 0;
        if (!BrotliEncoderCompressStream(e, BROTLI_OPERATION_FINISH, &avail_in, &next_in, &avail_out, nullptr, nullptr)) { ok = false; break; }
        size_t got = 0;
        const uint8_t* o = BrotliEncoderTakeOutput(e, &got);
        out.insert(out.end(), o, o + got);
        if (against && (out.size() > against->size() || !std::equal(out.begin(), out.end(), against->begin()))) break;
        if (BrotliEncoderIsFinished(e) && !BrotliEncoderHasMoreOutput(e)) break;
    }
    BrotliEncoderDestroyInstance(e);
    if (!ok) throw std::runtime_error("brotli could not pack");
    return out;
}

// An .lzma file (LZMA_Alone, as the lzma program writes): its properties byte and dictionary, then
// its size (all ones: not given, the stream marking its own end).
bool is_lzma_alone(const std::vector<uint8_t>& b) { return b.size() >= 14 && b[0] < 225 && b[1] == 0 && b[2] < 0x80; }

std::optional<std::vector<uint8_t>> liblzma_inner(const std::vector<uint8_t>& b, bool lzip)
{
    lzma_stream z = LZMA_STREAM_INIT;
    if ((lzip ? lzma_lzip_decoder(&z, UINT64_MAX, LZMA_CONCATENATED) : lzma_alone_decoder(&z, UINT64_MAX)) != LZMA_OK) return std::nullopt;
    std::vector<uint8_t> out, buf(size_t(1) << 20);
    z.next_in = b.data();
    z.avail_in = b.size();
    lzma_ret r = LZMA_OK;
    while (r == LZMA_OK)
    {
        z.next_out = buf.data();
        z.avail_out = buf.size();
        r = lzma_code(&z, LZMA_FINISH);
        out.insert(out.end(), buf.data(), buf.data() + (buf.size() - z.avail_out));
    }
    const bool whole = r == LZMA_STREAM_END && z.avail_in == 0;
    lzma_end(&z);
    if (!whole) return std::nullopt;
    return out;
}

bool is_lzip(const std::vector<uint8_t>& b) { return b.size() >= 26 && b[0] == 'L' && b[1] == 'Z' && b[2] == 'I' && b[3] == 'P' && b[4] == 1; }

// An .lzma file as liblzma makes it, as the lzma program does, at a preset.
std::vector<uint8_t> lzma_alone_pack(const std::vector<uint8_t>& in, uint32_t preset)
{
    lzma_options_lzma opt{};
    if (lzma_lzma_preset(&opt, preset)) throw std::runtime_error("liblzma has no such preset");
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_alone_encoder(&z, &opt) != LZMA_OK) throw std::runtime_error("liblzma could not start");
    std::vector<uint8_t> out(in.size() + in.size() / 2 + 4096);
    z.next_in = in.data();
    z.avail_in = in.size();
    z.next_out = out.data();
    z.avail_out = out.size();
    lzma_ret r = LZMA_OK;
    while (r == LZMA_OK)
    {
        if (z.avail_out == 0)
        {
            const size_t had = out.size();
            out.resize(had * 2);
            z.next_out = out.data() + had;
            z.avail_out = out.size() - had;
        }
        r = lzma_code(&z, LZMA_FINISH);
    }
    const size_t pos = size_t(z.next_out - out.data());
    lzma_end(&z);
    if (r != LZMA_STREAM_END) throw std::runtime_error("liblzma could not pack");
    out.resize(pos);
    return out;
}

} // namespace

std::vector<uint8_t> remake_archive(const std::string& recipe, const std::vector<uint8_t>& inner)
{
    if (recipe.rfind("sk ", 0) == 0)
    {
        // "sk <skeleton size> <inner size>" (v21): a gzip, zip or 7z made again from its skeleton and the
        // data it holds, its deflate streams by preflate.
        unsigned long long skeleton = 0, size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "sk %llu %llu%c", &skeleton, &size, &extra) != 2 || size != inner.size() || skeleton > size)
            throw std::runtime_error("a deflate recipe this Sieve does not know: " + recipe);
        return deflate_skeleton_remake(inner, size_t(skeleton));
    }
    if (recipe.rfind("lz4 ", 0) == 0)
    {
        // "lz4 <level> <block size 4-7> <linked 0|1> <block checksum 0|1> <content checksum 0|1>
        // <content size 0|1> <inner size>" (v20): one frame, as the lz4 program writes it.
        int level = 0, block = 0, linked = 0, bsum = 0, csum = 0, sflag = 0;
        unsigned long long size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "lz4 %d %d %d %d %d %d %llu%c", &level, &block, &linked, &bsum, &csum, &sflag, &size, &extra) != 7 ||
            level < -65537 || level > 12 || block < 4 || block > 7 || (linked | bsum | csum | sflag) & ~1 || size != inner.size())
            throw std::runtime_error("an lz4 recipe this Sieve does not know: " + recipe);
        return lz4_pack(inner, level, block, linked != 0, bsum != 0, csum != 0, sflag != 0);
    }
    if (recipe.rfind("br ", 0) == 0)
    {
        // "br <quality 0-11> <window 10-24> <inner size>" (v20): brotli 1.2.1, as the brotli program writes.
        int quality = 0, window = 0;
        unsigned long long size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "br %d %d %llu%c", &quality, &window, &size, &extra) != 3 || quality < 0 || quality > 11 || window < 10 ||
            window > 24 || size != inner.size())
            throw std::runtime_error("a brotli recipe this Sieve does not know: " + recipe);
        return brotli_pack(inner, quality, window);
    }
    if (recipe.rfind("lzma ", 0) == 0)
    {
        // "lzma <preset>[e] <inner size>" (v20): liblzma's LZMA_Alone encoder, as the lzma program writes.
        char preset[8] = {0};
        unsigned long long size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "lzma %7s %llu%c", preset, &size, &extra) != 2 || size != inner.size())
            throw std::runtime_error("an lzma recipe this Sieve does not know: " + recipe);
        const std::string ps(preset);
        if (ps.empty() || ps[0] < '0' || ps[0] > '9' || ps.size() > 2 || (ps.size() == 2 && ps[1] != 'e'))
            throw std::runtime_error("an lzma preset this Sieve does not know: " + recipe);
        return lzma_alone_pack(inner, uint32_t(ps[0] - '0') | (ps.size() == 2 ? LZMA_PRESET_EXTREME : 0));
    }
    if (recipe.rfind("bz2 ", 0) == 0)
    {
        // "bz2 <block size> <inner size>" (v19): libbzip2 1.0.8, as the bzip2 program writes one stream.
        int level = 0;
        unsigned long long size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "bz2 %d %llu%c", &level, &size, &extra) != 2 || level < 1 || level > 9 || size != inner.size())
            throw std::runtime_error("a bzip2 recipe this Sieve does not know: " + recipe);
        return bz2_pack(inner, level);
    }
    if (recipe.rfind("zst ", 0) == 0)
    {
        // "zst <libzstd version> <level> <checksum 0|1> <content size 0|1> <inner size>" (v19): remade
        // only by the version that made it, the one this Sieve was built with.
        char version[32] = {0};
        int level = 0, checksum = 0, size_flag = 0;
        unsigned long long size = 0;
        char extra = 0;
        if (std::sscanf(recipe.c_str(), "zst %31s %d %d %d %llu%c", version, &level, &checksum, &size_flag, &size, &extra) != 5 ||
            level < ZSTD_minCLevel() || level > ZSTD_maxCLevel() || checksum < 0 || checksum > 1 || size_flag < 0 || size_flag > 1 || size != inner.size())
            throw std::runtime_error("a zstd recipe this Sieve does not know: " + recipe);
        if (std::string(version) != ZSTD_versionString())
            throw std::runtime_error(std::string("a zstd file made with libzstd ") + version + ", which only that version remakes exactly; this Sieve has " +
                                     ZSTD_versionString());
        return zstd_pack(inner, level, checksum != 0, size_flag != 0);
    }
    if (recipe.rfind("gz ", 0) == 0)
    {
        // "gz <level> <memory level> <header in hex> <inner size>" (v17): the header as it was, the
        // deflate made again by zlib, then the CRC-32 and the length, little-endian.
        char hexbuf[8192] = {0};
        int level = 0, mem = 0;
        unsigned long long size = 0;
        char extra = 0;
        if (recipe.size() > 8000 || std::sscanf(recipe.c_str(), "gz %d %d %8191s %llu%c", &level, &mem, hexbuf, &size, &extra) != 4 || level < 1 ||
            level > 9 || mem < 1 || mem > 9 || size != inner.size())
            throw std::runtime_error("a gzip recipe this Sieve does not know: " + recipe.substr(0, 80));
        const std::string hx(hexbuf);
        if (hx.size() < 20 || hx.size() % 2 || hx.find_first_not_of("0123456789abcdef") != std::string::npos)
            throw std::runtime_error("a gzip recipe's header is not hex");
        std::vector<uint8_t> out;
        for (size_t i = 0; i < hx.size(); i += 2) out.push_back(uint8_t(std::stoul(hx.substr(i, 2), nullptr, 16)));
        const std::vector<uint8_t> d = deflate_raw(inner, level, mem);
        out.insert(out.end(), d.begin(), d.end());
        const uint32_t crc = uint32_t(crc32(0, inner.data(), uInt(inner.size()))), len = uint32_t(inner.size());
        for (int k = 0; k < 4; ++k) out.push_back(uint8_t(crc >> (8 * k)));
        for (int k = 0; k < 4; ++k) out.push_back(uint8_t(len >> (8 * k)));
        return out;
    }
    // "xz <preset>[e] <check> <inner size>": liblzma's single-call encoder, as xz writes one stream.
    char kind[8] = {0}, preset[8] = {0}, check[8] = {0};
    unsigned long long size = 0;
    char extra = 0;
    if (std::sscanf(recipe.c_str(), "%7s %7s %7s %llu%c", kind, preset, check, &size, &extra) != 4 || std::string(kind) != "xz" || size != inner.size())
        throw std::runtime_error("an archive's recipe this Sieve does not know: " + recipe);
    const std::string ps(preset), cs(check);
    if (ps.empty() || ps[0] < '0' || ps[0] > '9' || (ps.size() == 2 && ps[1] != 'e') || ps.size() > 2)
        throw std::runtime_error("an xz preset this Sieve does not know: " + recipe);
    const uint32_t level = uint32_t(ps[0] - '0') | (ps.size() == 2 ? LZMA_PRESET_EXTREME : 0);
    int id = -1;
    for (const int c : {0, 1, 4, 10})
        if (cs == xz_check_name(c)) id = c;
    if (id < 0) throw std::runtime_error("an xz check this Sieve does not know: " + recipe);
    // The streaming encoder, as the xz program uses: the single-call one writes each block's sizes
    // in its header, which xz does not, so its files would not come out the same.
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_easy_encoder(&z, level, lzma_check(id)) != LZMA_OK) throw std::runtime_error("liblzma could not remake an xz file");
    std::vector<uint8_t> out(lzma_stream_buffer_bound(inner.size()));
    z.next_in = inner.data();
    z.avail_in = inner.size();
    z.next_out = out.data();
    z.avail_out = out.size();
    const lzma_ret r = lzma_code(&z, LZMA_FINISH);
    const size_t pos = out.size() - z.avail_out;
    lzma_end(&z);
    if (r != LZMA_STREAM_END) throw std::runtime_error("liblzma could not remake an xz file");
    out.resize(pos);
    return out;
}

namespace {

// The recipe that remakes an xz file exactly from what it holds ("xz 6 crc64"), or "" if none does:
// the presets whose dictionary is the one its first block names, each with and without extreme.
std::string xz_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner)
{
    const timings::Scope t("pack.archives.recipe");
    const char* check = xz_check_name(b[7]);
    if (b[6] != 0 || !check) return "";
    // The first block's header: its filters, the last LZMA2's dictionary byte.
    uint32_t dict = 0;
    {
        size_t at = 12;
        const size_t hsize = (size_t(b[at]) + 1) * 4;
        const uint8_t flags = b[at + 1];
        size_t p = at + 2;
        auto vli = [&]() {
            uint64_t v = 0;
            for (int k = 0; k < 9 && p < at + hsize; ++k)
            {
                const uint8_t c = b[p++];
                v |= uint64_t(c & 0x7F) << (7 * k);
                if (!(c & 0x80)) break;
            }
            return v;
        };
        if (b[at] != 0 && at + hsize <= b.size())
        {
            if (flags & 0x40) vli();
            if (flags & 0x80) vli();
            for (int f = 0; f <= (flags & 3); ++f)
            {
                const uint64_t id = vli(), n = vli();
                if (id == 0x21 && n == 1 && p < b.size() && b[p] <= 40) dict = b[p] == 40 ? 0xFFFFFFFFu : (2u | (b[p] & 1u)) << (b[p] / 2 + 11);
                p += size_t(n);
            }
        }
    }
    static const uint32_t preset_dict[10] = {1u << 18, 1u << 20, 1u << 21, 1u << 22, 1u << 22, 1u << 23, 1u << 23, 1u << 24, 1u << 25, 1u << 26};
    for (const int level : {6, 9, 5, 7, 8, 4, 3, 2, 1, 0})
        for (const bool extreme : {false, true})
        {
            if (dict && preset_dict[level] != dict) continue;
            const std::string recipe = "xz " + std::to_string(level) + (extreme ? "e" : "") + " " + check + " " + std::to_string(inner.size());
            if (remake_archive(recipe, inner) == b) return recipe;
        }
    return "";
}

// The recipe that remakes a gzip file exactly ("gz <level> <memory level> <its header in hex>"),
// or "" if zlib at no level gives its deflate back: the header is kept as it is, the deflate made
// again and its CRC and length after it.
std::string gzip_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner)
{
    const timings::Scope t("pack.archives.recipe");
    const size_t h = gzip_header_size(b);
    if (!h) return "";
    const std::vector<uint8_t> stream(b.begin() + std::ptrdiff_t(h), b.end() - 8);
    for (const int level : {6, 9, 1, 2, 3, 4, 5, 7, 8})
        for (const int mem : {8, 9})
            if (deflate_raw(inner, level, mem) == stream)
                return "gz " + std::to_string(level) + " " + std::to_string(mem) + " " + hex_of(b.data(), h) + " " + std::to_string(inner.size());
    return "";
}

// The recipe that remakes a bzip2 file exactly ("bz2 9 <size>"), or "" if libbzip2 does not give it
// back: one stream, at the block size its header names.
std::string bz2_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner, size_t streams)
{
    const timings::Scope t("pack.archives.recipe");
    if (streams != 1) return "";
    const int level = b[3] - '0';
    if (bz2_pack(inner, level) != b) return "";
    return "bz2 " + std::to_string(level) + " " + std::to_string(inner.size());
}

// The recipe that remakes a zstd file exactly ("zst 1.5.7 3 1 1 <size>"), or "" if no level of this
// libzstd gives it back: one frame, its checksum and content size as its header says, each level
// tried first on the frame's start (its header and first 128 KiB), and in full only where that
// matches.
std::string zstd_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner, size_t frames)
{
    const timings::Scope t("pack.archives.recipe");
    if (frames != 1) return "";
    ZSTD_frameHeader h{};
    if (ZSTD_getFrameHeader(&h, b.data(), b.size()) != 0 || h.frameType != ZSTD_frame || h.dictID != 0) return "";
    const bool checksum = h.checksumFlag != 0, size_flag = h.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN;
    if (size_flag && h.frameContentSize != inner.size()) return "";
    std::vector<int> levels = {3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
    for (int l = -1; l >= -7; --l) levels.push_back(l);
    constexpr size_t kStart = size_t(128) << 10;
    for (const int level : levels)
    {
        if (inner.size() > kStart)
        {
            const std::vector<uint8_t> start = zstd_pack(inner, level, checksum, size_flag, kStart);
            if (start.size() > b.size() || !std::equal(start.begin(), start.end(), b.begin())) continue;
        }
        if (zstd_pack(inner, level, checksum, size_flag) == b)
            return std::string("zst ") + ZSTD_versionString() + " " + std::to_string(level) + " " + (checksum ? "1" : "0") + " " + (size_flag ? "1" : "0") +
                   " " + std::to_string(inner.size());
    }
    return "";
}

// The recipe that remakes an lz4 file exactly ("lz4 1 7 0 0 1 0 <size>"), or "" if no level of
// liblz4 gives it back: one frame, its settings as its header says, each level tried first on the
// frame's first block, and in full only where that matches.
std::string lz4_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner, size_t frames)
{
    const timings::Scope t("pack.archives.recipe");
    if (frames != 1 || b.size() < 7) return "";
    const uint8_t flg = b[4], bd = b[5];
    if ((flg >> 6) != 1 || (flg & 0x03) || (bd & 0x8F)) return ""; // version 01, no dictionary, reserved bits clear
    const int block = (bd >> 4) & 7;
    if (block < 4) return "";
    const bool linked = !(flg & 0x20), bsum = (flg & 0x10) != 0, sflag = (flg & 0x08) != 0, csum = (flg & 0x04) != 0;
    const size_t head = sflag ? 15 : 7;
    if (b.size() < head + 4) return "";
    // The first block: its 4-byte size (the top bit: stored as it is) and its bytes.
    const uint32_t first = (uint32_t(b[head]) | uint32_t(b[head + 1]) << 8 | uint32_t(b[head + 2]) << 16 | uint32_t(b[head + 3]) << 24) & 0x7FFFFFFFu;
    const size_t block_bytes = size_t(64) << (2 * (block - 4)) << 10;
    std::vector<int> levels = {1, 9, 12, 3, 4, 5, 6, 7, 8, 10, 11};
    for (int l = -1; l >= -16; --l) levels.push_back(l);
    for (const int level : levels)
    {
        if (inner.size() > block_bytes && first > 0 && head + 4 + first <= b.size())
        {
            const std::vector<uint8_t> part(inner.begin(), inner.begin() + std::ptrdiff_t(block_bytes));
            const std::vector<uint8_t> f = lz4_pack(part, level, block, linked, false, false, false);
            if (f.size() < 7 + 4 + first || !std::equal(f.begin() + 7, f.begin() + 7 + 4 + std::ptrdiff_t(first), b.begin() + std::ptrdiff_t(head))) continue;
        }
        if (lz4_pack(inner, level, block, linked, bsum, csum, sflag) == b)
            return "lz4 " + std::to_string(level) + " " + std::to_string(block) + " " + (linked ? "1" : "0") + " " + (bsum ? "1" : "0") + " " +
                   (csum ? "1" : "0") + " " + (sflag ? "1" : "0") + " " + std::to_string(inner.size());
    }
    return "";
}

// The recipe that remakes a brotli file exactly ("br 11 22 <size>"), or "" if no quality gives it
// back: its window from its first bits, each quality stopped at its first byte that differs.
std::string brotli_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner)
{
    const timings::Scope t("pack.archives.recipe");
    const int window = brotli_window(b);
    if (window < 10 || window > 24) return "";
    for (const int q : {11, 9, 5, 6, 4, 10, 7, 8, 3, 2, 1, 0})
        if (brotli_pack(inner, q, window, &b) == b) return "br " + std::to_string(q) + " " + std::to_string(window) + " " + std::to_string(inner.size());
    return "";
}

// The recipe that remakes an .lzma file exactly ("lzma 6 <size>"), or "" if no preset of liblzma
// does: the presets whose dictionary is the one its header names, each with and without extreme.
std::string lzma_recipe(const std::vector<uint8_t>& b, const std::vector<uint8_t>& inner)
{
    const timings::Scope t("pack.archives.recipe");
    if (b.size() < 13 || b[0] != 0x5D) return ""; // lc 3, lp 0, pb 2: every preset's
    const uint32_t dict = uint32_t(b[1]) | uint32_t(b[2]) << 8 | uint32_t(b[3]) << 16 | uint32_t(b[4]) << 24;
    for (int k = 5; k < 13; ++k)
        if (b[size_t(k)] != 0xFF) return ""; // the lzma program writes no size
    static const uint32_t preset_dict[10] = {1u << 18, 1u << 20, 1u << 21, 1u << 22, 1u << 22, 1u << 23, 1u << 23, 1u << 24, 1u << 25, 1u << 26};
    for (const int level : {6, 9, 5, 7, 8, 4, 3, 2, 1, 0})
        for (const bool extreme : {false, true})
        {
            if (preset_dict[level] != dict) continue;
            if (lzma_alone_pack(inner, uint32_t(level) | (extreme ? LZMA_PRESET_EXTREME : 0)) == b)
                return "lzma " + std::to_string(level) + (extreme ? "e" : "") + " " + std::to_string(inner.size());
        }
    return "";
}

// An archive's contents' folder (or file): its name without the extension, else with ".contents".
std::string contents_name(const std::string& path, const char* ext)
{
    const std::string e(ext);
    if (!e.empty() && path.size() > e.size() && path.compare(path.size() - e.size(), e.size(), e) == 0) return path.substr(0, path.size() - e.size());
    return path + ".contents";
}

} // namespace

SampleAsWhole::SampleAsWhole(uint64_t whole_bytes) { g_sample_floor = dictionary_for(whole_bytes); }
SampleAsWhole::~SampleAsWhole() { g_sample_floor = 0; }

FileWays file_ways(const std::vector<uint8_t>& bytes, double rope_seconds, uint32_t rope_newest, bool pieces)
{
    auto varint_of = [](uint64_t n) {
        uint64_t v = 1;
        while (n >>= 7) ++v;
        return v;
    };
    FileWays w;
    w.lzma2 = estimate(bytes);
    const RopeSearch found = [&] {
        const timings::Scope t("pack.rope.search");
        return rope_search(bytes, rope_seconds, rope_newest);
    }();
    w.tried += found.tried;
    w.rope = found.best;
    if (force_requested()) // abandoned: no way but LZMA2's (the caller leaves the file as it is)
    {
        w.forced = true;
        return w;
    }
    {
        const timings::Scope t("pack.rope.code");
        w.rope_code = rope_code(bytes, found.best);
    }
    w.rope_bytes = w.rope_code.size() + varint_of(w.rope_code.size());
    if (pieces && !stop_requested())
    {
        Pieces p = [&] {
            const timings::Scope t("pack.pieces.cut");
            return pieces_of(bytes);
        }();
        if (force_requested()) // its costs may be from passes abandoned part way: not used
        {
            w.forced = true;
            return w;
        }
        if (!p.list.empty())
        {
            const timings::Scope t("pack.pieces.rope");
            const RopeSearch pr = rope_search(p.r, rope_seconds, rope_newest);
            w.tried += pr.tried;
            w.has_pieces = true;
            w.pieces_rope = pr.best;
            w.pieces_code = rope_code(p.r, pr.best);
            w.pieces_way = "pieces " + pieces_text(p.list) + " rope " + pr.best.text();
            w.pieces_bytes = estimate(p.z) + w.pieces_code.size() + varint_of(w.pieces_code.size()) + w.pieces_way.size();
            w.pieces_z = std::move(p.z);
        }
    }
    return w;
}

namespace {

// A text's lines, as locate.cpp's unpacking splits them (at each line feed).
std::vector<std::string_view> lines_of(const std::vector<uint8_t>& t)
{
    std::vector<std::string_view> out;
    const char* p = reinterpret_cast<const char*>(t.data());
    size_t at = 0;
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i] == '\n')
        {
            out.emplace_back(p + at, i - at);
            at = i + 1;
        }
    out.emplace_back(p + at, t.size() - at);
    return out;
}

// The mask of `base`'s lines that makes `text` (its lines in base's order), or nothing.
std::optional<std::vector<uint8_t>> mask_of(const std::vector<std::string_view>& text, const std::vector<std::string_view>& base)
{
    if (text.size() > base.size()) return std::nullopt;
    std::vector<uint8_t> mask((base.size() + 7) / 8, 0);
    size_t j = 0;
    for (size_t i = 0; i < base.size() && j < text.size(); ++i)
        if (base[i] == text[j])
        {
            mask[i / 8] |= uint8_t(1u << (i % 8));
            ++j;
        }
    if (j != text.size()) return std::nullopt;
    return mask;
}

bool is_program(const std::vector<uint8_t>& b)
{
    const std::string k = file_kind(b, b.size());
    return k == "EXE" || k == "ELF";
}

} // namespace

namespace {

// The line an item's way ("item K <place>") names.
size_t item_line_of_way(const std::string& way) { return size_t(std::stoull(way.substr(5, way.find(' ', 5) - 5))); }

// A place (hex) as `width` raw bytes, most significant first.
std::vector<uint8_t> place_bytes_of(const std::string& hex, uint32_t width)
{
    std::string h = BigUint::from_hex(hex).to_hex(size_t(width) * 2);
    std::vector<uint8_t> out(width);
    auto digit = [](char c) { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; };
    for (uint32_t i = 0; i < width && 2 * i + 1 < h.size(); ++i) out[i] = uint8_t(digit(h[2 * i]) * 16 + digit(h[2 * i + 1]));
    return out;
}

} // namespace

void add_packed(Manifest& m, const fs::path& root_in, PackReport* report, const ItemFinder* items, bool item_hashes, const PlaceCoder* coder,
                double rope_seconds, uint32_t rope_newest, bool trim, bool pieces, bool optimise_archives, bool binary, bool gzip,
                bool v20_on, bool v21_on)
{
    const timings::Scope t_all("pack.all");
    // Every file's bytes, checked against the walk: from the contents where a v3 manifest has them,
    // else read from the folder (the current one, for a file named without one).
    std::vector<ManifestEntry*> files;
    for (ManifestEntry& e : m.entries)
        if (!e.dir) files.push_back(&e);
    std::vector<std::vector<uint8_t>> bytes(files.size());
    size_t at = 0;
    const bool have = m.with_contents && !m.packed && m.contents.size() == m.bytes;
    const fs::path root = have ? fs::path() : fs::absolute(root_in.empty() ? fs::path(".") : root_in).lexically_normal();
    for (size_t i = 0; i < files.size(); ++i)
    {
        const ManifestEntry& e = *files[i];
        if (have)
        {
            bytes[i].assign(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
            at += size_t(e.size);
        }
        else bytes[i] = read_file_bytes(root / fs::path(std::u8string(e.path.begin(), e.path.end())));
        if (bytes[i].size() != e.size || sha256_hex(bytes[i]) != e.sha256) throw std::runtime_error(e.path + " changed while the folder was being read");
    }

    // v14, "Optimise Archive Formats" (asked for, never by default): an xz file remade exactly from
    // what it holds where liblzma does that, its contents carried; else, and a 7z always (Sieve has
    // no 7-Zip encoder), the archive carried as its contents, in a folder (or file) named after it,
    // and said so. Where that name is taken, or the archive cannot be read, it stays as it is.
    std::vector<std::vector<uint8_t>> original(files.size()); // a remade file's own bytes
    m.unpacked.clear();
    m.archives = false;
    const bool gzip_remake = binary && gzip; // v17's (a binary head's): gzip remade exactly
    const bool newer_archives = gzip_remake;  // v19's, with the newest (bzip2 and zstd)
    const bool v20 = gzip_remake && v20_on; // v20's (lz4, brotli, lzma, lzip, libarchive's)
    const bool v21 = v20 && v21_on;         // v21's (gzip, zip and 7z made again from a skeleton)
    m.remake_gz = false;
    m.more_archives = false;
    m.v20 = false;
    m.v21 = false;
    // (SIEVE_ARCHIVES_ALWAYS=1 in the environment: every archive Sieve can read is carried as its contents
    // or remade, smaller or not, so the CI can test each reader on the small archives it has.)
    const char* always_env = std::getenv("SIEVE_ARCHIVES_ALWAYS");
    const bool archives_always = always_env && std::string(always_env) == "1";
    if (optimise_archives && m.compact)
    {
        std::unordered_set<std::string> taken;
        for (const ManifestEntry& e : m.entries) taken.insert(e.dir && !e.path.empty() && e.path.back() == '/' ? e.path.substr(0, e.path.size() - 1) : e.path);
        const bool one = m.one_file_root();
        std::vector<ManifestEntry> entries;
        std::vector<std::vector<uint8_t>> carried, kept;
        size_t fi = 0;
        auto add_file = [&](ManifestEntry e, std::vector<uint8_t> b, std::vector<uint8_t> orig) {
            e.size = (orig.empty() ? b : orig).size();
            e.sha256 = sha256_hex(orig.empty() ? b : orig);
            entries.push_back(std::move(e));
            carried.push_back(std::move(b));
            kept.push_back(std::move(orig));
        };
        std::unordered_set<std::string> dirs; // the folders listed, each ending in '/'
        for (const ManifestEntry& e : m.entries)
            if (e.dir) dirs.insert(e.path);
        auto add_dir = [&](const std::string& d) { // d ends in '/'
            if (!dirs.insert(d).second) return;
            ManifestEntry de;
            de.dir = true;
            de.path = d;
            entries.push_back(de);
            carried.emplace_back();
            kept.emplace_back();
        };
        auto add_dirs = [&](const std::string& path) { // every folder above path
            for (size_t at = path.find('/'); at != std::string::npos; at = path.find('/', at + 1)) add_dir(path.substr(0, at + 1));
        };
        for (ManifestEntry& e : m.entries)
        {
            if (e.dir)
            {
                entries.push_back(e); // listed already in dirs
                carried.emplace_back();
                kept.emplace_back();
                continue;
            }
            std::vector<uint8_t>& b = bytes[fi++];
            // Only where it comes out clearly smaller: the archive as it is (packed, if that is smaller)
            // against what it holds, packed by LZMA2 or pulled tight by the rope (the search is
            // remembered, so packing it later does not search again), its names and recipe counted.
            const uint64_t as_is = std::min<uint64_t>(b.size(), estimate(b));
            auto worth = [&](const std::vector<uint8_t>& inner, uint64_t extra, bool one_file = true) {
                if (archives_always) return true; // SIEVE_ARCHIVES_ALWAYS: every archive read, for testing the readers
                uint64_t cost = estimate(inner);
                if (one_file && rope_seconds >= 0 && inner.size() >= 512) cost = std::min<uint64_t>(cost, uint64_t(rope_search(inner, rope_seconds, rope_newest).estimate));
                // By a clear margin (64 bytes and 0.5%): the estimate is LZMA2 at preset 6, the stream
                // preset 9, and a near thing is not worth giving up the archive as it was.
                return cost + extra + 64 + as_is / 200 < as_is;
            };
            // A single-stream archive (xz, gzip): remade exactly where a recipe gives it back, else
            // carried as what it holds, under its name without the extension.
            auto single = [&](const char* format, std::optional<std::vector<uint8_t>> inner, const std::string& recipe, const std::string& name,
                              uint64_t extra) {
                if (!inner || !worth(*inner, extra)) return false;
                if (!recipe.empty())
                {
                    ManifestEntry r = e;
                    r.remake = recipe;
                    add_file(r, std::move(*inner), std::move(b));
                    return true;
                }
                if (taken.count(name)) return false;
                taken.insert(name);
                m.unpacked.push_back({format, e.path, name, e.size});
                ManifestEntry c = e;
                c.path = name;
                if (one) m.root = name; // one archive located alone: what it holds is the root
                add_file(c, std::move(*inner), {});
                return true;
            };
            // An archive of files (7z, zip): carried as its contents, in a folder named after it.
            auto contents = [&](const char* format, std::vector<sieve::install::InMemory> all, const char* ext) {
                const std::string folder = contents_name(e.path, ext);
                std::vector<uint8_t> joined;
                uint64_t names = 0;
                size_t nfiles = 0;
                for (const auto& in : all)
                {
                    joined.insert(joined.end(), in.bytes.begin(), in.bytes.end());
                    names += folder.size() + in.entry.path.size() + 12;
                    nfiles += !in.entry.dir;
                }
                // (The rope's estimate only for an archive of one file: for more, their bytes joined
                // are not what the rope would be given.)
                if (all.empty() || taken.count(folder) || !worth(joined, names, nfiles == 1)) return false;
                taken.insert(folder);
                m.unpacked.push_back({format, e.path, folder, e.size});
                const std::string prefix = one ? std::string() : folder + "/";
                if (!one) add_dirs(prefix);
                for (auto& in : all)
                {
                    const std::string path = prefix + in.entry.path;
                    add_dirs(path);
                    if (in.entry.dir)
                    {
                        add_dir(path + "/");
                        continue;
                    }
                    ManifestEntry fe;
                    fe.path = path;
                    add_file(fe, std::move(in.bytes), {});
                }
                if (one) m.root = folder; // one archive located alone: its contents' folder is the root
                return true;
            };
            if (is_xz(b))
            {
                auto inner = xz_inner(b);
                const std::string recipe = inner ? xz_recipe(b, *inner) : std::string();
                if (single("xz", std::move(inner), recipe, contents_name(e.path, ".xz"), 40)) continue;
            }
            if (is_gzip(b))
            {
                size_t members = 0;
                auto inner = gzip_inner(b, &members);
                const std::string recipe = inner && gzip_remake && members == 1 ? gzip_recipe(b, *inner) : std::string();
                std::string name = contents_name(e.path, ".gz");
                if (e.path.size() > 4 && e.path.compare(e.path.size() - 4, 4, ".tgz") == 0) name = e.path.substr(0, e.path.size() - 4) + ".tar";
                // v21: else made again by preflate from its skeleton, where that gives it back.
                if (recipe.empty() && v21 && inner)
                    if (auto sk = gzip_skeleton(b))
                    {
                        size_t skeleton = 0;
                        std::vector<uint8_t> whole = sk->inner(skeleton);
                        const std::string r = "sk " + std::to_string(skeleton) + " " + std::to_string(whole.size());
                        if (deflate_skeleton_remake(whole, skeleton) == b && single("gz", std::move(whole), r, name, 40)) continue;
                    }
                if (single("gz", std::move(inner), recipe, name, 40 + 2 * gzip_header_size(b))) continue;
            }
            if (newer_archives && is_bz2(b))
            {
                size_t streams = 0;
                auto inner = bz2_inner(b, &streams);
                const std::string recipe = inner ? bz2_recipe(b, *inner, streams) : std::string();
                std::string name = contents_name(e.path, ".bz2");
                for (const char* t : {".tbz2", ".tbz"})
                    if (e.path.size() > std::strlen(t) && e.path.compare(e.path.size() - std::strlen(t), std::strlen(t), t) == 0)
                        name = e.path.substr(0, e.path.size() - std::strlen(t)) + ".tar";
                if (single("bz2", std::move(inner), recipe, name, 40)) continue;
            }
            if (newer_archives && is_zstd(b))
            {
                size_t frames = 0;
                auto inner = zstd_inner(b, &frames);
                const std::string recipe = inner ? zstd_recipe(b, *inner, frames) : std::string();
                std::string name = contents_name(e.path, ".zst");
                if (e.path.size() > 5 && e.path.compare(e.path.size() - 5, 5, ".tzst") == 0) name = e.path.substr(0, e.path.size() - 5) + ".tar";
                if (single("zst", std::move(inner), recipe, name, 48)) continue;
            }
            // v20's: lz4, brotli (a file named .br: brotli has no mark), lzma and lzip.
            auto ends = [&](const char* t) {
                const size_t n = std::strlen(t);
                return e.path.size() > n && e.path.compare(e.path.size() - n, n, t) == 0;
            };
            auto renamed = [&](const char* ext, const char* tar_ext) {
                if (tar_ext && ends(tar_ext)) return e.path.substr(0, e.path.size() - std::strlen(tar_ext)) + ".tar";
                return contents_name(e.path, ext);
            };
            if (v20 && is_lz4(b))
            {
                size_t frames = 0;
                auto inner = lz4_inner(b, &frames);
                const std::string recipe = inner ? lz4_recipe(b, *inner, frames) : std::string();
                if (single("lz4", std::move(inner), recipe, renamed(".lz4", ".tlz4"), 56)) continue;
            }
            if (v20 && ends(".br"))
            {
                auto inner = brotli_inner(b);
                const std::string recipe = inner ? brotli_recipe(b, *inner) : std::string();
                if (single("br", std::move(inner), recipe, renamed(".br", nullptr), 40)) continue;
            }
            if (v20 && ends(".lzma") && is_lzma_alone(b))
            {
                auto inner = liblzma_inner(b, false);
                const std::string recipe = inner ? lzma_recipe(b, *inner) : std::string();
                if (single("lzma", std::move(inner), recipe, renamed(".lzma", nullptr), 40)) continue;
            }
            if (v20 && is_lzip(b))
                if (single("lz", liblzma_inner(b, true), std::string(), renamed(".lz", ".tlz"), 40)) continue;
            // v21: a 7z made again exactly from its skeleton and the data it holds; else, as before, its contents.
            // (Its contents weighed first, as cheaply as that is done for them: where they would come out
            // no smaller, the search for the levels that make its blocks again is not worth starting.)
            bool sevenz_worth = false;
            if (v21 && sieve::install::is_7z(b))
            {
                try
                {
                    std::vector<uint8_t> joined;
                    for (const auto& in : sieve::install::read_7z(b)) joined.insert(joined.end(), in.bytes.begin(), in.bytes.end());
                    sevenz_worth = !joined.empty() && worth(joined, 40, false);
                }
                catch (const std::exception&)
                {
                    sevenz_worth = false;
                }
            }
            if (sevenz_worth)
                if (auto sk = sevenz_skeleton(b))
                {
                    size_t skeleton = 0;
                    std::vector<uint8_t> whole = sk->inner(skeleton);
                    const std::string r = "sk " + std::to_string(skeleton) + " " + std::to_string(whole.size());
                    if (deflate_skeleton_remake(whole, skeleton) == b && single("7z", std::move(whole), r, contents_name(e.path, ".7z"), 40)) continue;
                }
            if (sieve::install::is_7z(b))
            {
                std::vector<sieve::install::InMemory> all;
                try
                {
                    all = sieve::install::read_7z(b);
                }
                catch (const std::exception&)
                {
                    all.clear();
                }
                if (contents("7z", std::move(all), ".7z")) continue;
            }
            // v21: a zip made again exactly from its skeleton and the data it holds; else, as before, its contents.
            if (v21 && b.size() >= 4 && b[0] == 'P' && b[1] == 'K' && b[2] == 3 && b[3] == 4)
                if (auto sk = zip_skeleton(b))
                {
                    size_t skeleton = 0;
                    std::vector<uint8_t> whole = sk->inner(skeleton);
                    const std::string r = "sk " + std::to_string(skeleton) + " " + std::to_string(whole.size());
                    if (deflate_skeleton_remake(whole, skeleton) == b && single("zip", std::move(whole), r, contents_name(e.path, ".zip"), 40)) continue;
                }
            if (auto all = zip_entries(b))
                if (contents("zip", std::move(*all), ".zip")) continue;
            if (v20)
            {
                std::string format;
                if (auto all = libarchive_entries(b, format))
                {
                    // Its folder: the name without its extension, whatever that is (.rar, .cab, .iso ...).
                    const size_t dot = e.path.find_last_of('.'), slash = e.path.find_last_of('/');
                    const std::string ext = dot != std::string::npos && (slash == std::string::npos || dot > slash) ? e.path.substr(dot) : std::string();
                    if (contents(format.c_str(), std::move(*all), ext.c_str())) continue;
                }
            }
            add_file(e, std::move(b), {});
        }
        if (!m.unpacked.empty() || std::any_of(entries.begin(), entries.end(), [](const ManifestEntry& x) { return !x.remake.empty(); }))
        {
            m.archives = true;
            m.remake_gz = std::any_of(entries.begin(), entries.end(), [](const ManifestEntry& x) { return x.remake.rfind("gz ", 0) == 0; });
            // v19: some bzip2 or zstd remade, or carried as its contents.
            m.more_archives = std::any_of(entries.begin(), entries.end(), [](const ManifestEntry& x) { return x.remake.rfind("bz2 ", 0) == 0 || x.remake.rfind("zst ", 0) == 0; }) ||
                              std::any_of(m.unpacked.begin(), m.unpacked.end(), [](const Manifest::UnpackedArchive& u) { return u.format == "bz2" || u.format == "zst"; });
            // v20: some lz4, brotli or lzma remade, or one of those or an lzip carried as its contents.
            m.v20 =
                std::any_of(entries.begin(), entries.end(),
                            [](const ManifestEntry& x) { return x.remake.rfind("lz4 ", 0) == 0 || x.remake.rfind("br ", 0) == 0 || x.remake.rfind("lzma ", 0) == 0; }) ||
                std::any_of(m.unpacked.begin(), m.unpacked.end(), [](const Manifest::UnpackedArchive& u) { return is_v20_format(u.format); });
            // v21: some gzip or zip made again by preflate.
            m.v21 = std::any_of(entries.begin(), entries.end(), [](const ManifestEntry& x) { return x.remake.rfind("sk ", 0) == 0; });
            if (m.v21) m.v20 = true; // v21 is v20 and more
            if (m.v20) m.more_archives = true; // v20 is v19 and more
            if (m.more_archives) m.remake_gz = true; // v19 is v17 and more
            m.entries.clear();
            bytes.clear();
            original.clear();
            for (size_t k = 0; k < entries.size(); ++k)
            {
                m.entries.push_back(std::move(entries[k]));
                if (!m.entries.back().dir)
                {
                    bytes.push_back(std::move(carried[k]));
                    original.push_back(std::move(kept[k]));
                }
            }
            files.clear();
            for (ManifestEntry& e : m.entries)
                if (!e.dir) files.push_back(&e);
            m.files = files.size();
            m.bytes = 0;
            for (const ManifestEntry* f : files) m.bytes += f->size;
        }
        else
        {
            // Nothing changed: the bytes back where they were (the entries are the walk's still).
            bytes.clear();
            for (size_t k = 0; k < entries.size(); ++k)
                if (!entries[k].dir) bytes.push_back(std::move(carried[k]));
        }
    }

    // Each file's way: an item of one of Sieve's lines, its place there (v5); a program in the x86
    // stream; a text made from another's lines, a mask; the rest as it is.
    std::vector<bool> item(files.size(), false);
    std::vector<size_t> used_lines;
    for (size_t i = 0; i < files.size(); ++i)
    {
        files[i]->way = is_program(bytes[i]) && files[i]->remake.empty() ? "x86" : "raw";
        if (!items || bytes[i].empty() || !files[i]->remake.empty()) continue;
        if (const auto found = (*items)(bytes[i]))
        {
            // v6 carries an item only where its place is shorter than the file: the least of the two.
            if (m.minimal && m.line_widths.at(found->first) >= bytes[i].size()) continue;
            // Lines are numbered in the order the files first use them.
            auto slot = std::find(used_lines.begin(), used_lines.end(), found->first);
            if (slot == used_lines.end()) slot = used_lines.insert(used_lines.end(), found->first);
            files[i]->way = "item " + std::to_string(slot - used_lines.begin()) + " " + found->second;
            item[i] = true;
        }
    }
    // v7: each line's places coded where every one has a code and they come to less than raw, each
    // as it would be carried (as it is, or LZMA2 where that is smaller): near-identical models'
    // places pack to almost nothing raw, while their codes, numbers far apart, do not. A code's
    // length, as a varint, goes before it.
    std::vector<std::string> coding(m.lines.size()); // by the line's place in m.lines
    if (m.compact)
        for (size_t slot = 0; slot < used_lines.size(); ++slot)
        {
            const size_t line = used_lines[slot];
            if (!coder) continue;
            std::vector<uint8_t> raw, coded;
            bool all = true;
            std::vector<std::pair<size_t, std::vector<uint8_t>>> codes;
            for (size_t i = 0; i < files.size() && all; ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    const std::string& w = files[i]->way;
                    const std::string place = w.substr(w.find(' ', 5) + 1);
                    auto c = (*coder)(line, place);
                    if (!c)
                    {
                        all = false;
                        continue;
                    }
                    const std::vector<uint8_t> p = place_bytes_of(place, m.line_widths.at(line));
                    raw.insert(raw.end(), p.begin(), p.end());
                    uint64_t n = c->size();
                    while (n >= 0x80)
                    {
                        coded.push_back(uint8_t(n | 0x80));
                        n >>= 7;
                    }
                    coded.push_back(uint8_t(n));
                    coded.insert(coded.end(), c->begin(), c->end());
                    codes.emplace_back(i, std::move(*c));
                }
            if (!all) continue;
            const uint64_t raw_carried = std::min<uint64_t>(raw.size(), estimate(raw));
            const uint64_t coded_carried = std::min<uint64_t>(coded.size(), estimate(coded));
            if (coded_carried >= raw_carried) continue;
            coding[line] = kCodedPlaces;
            static const char hx[] = "0123456789abcdef";
            for (auto& [i, c] : codes)
            {
                std::string h = "item " + std::to_string(slot) + " h";
                for (const uint8_t b : c) h += {hx[b >> 4], hx[b & 15]};
                files[i]->way = std::move(h);
            }
        }
    // v6: each line's items carried as places only where that is smaller, as it would be carried:
    // their places (raw, or LZMA2 where that is smaller) against their files packed together. Files
    // that are near one another (a sample of a line's places) pack to almost nothing as files and
    // stay files; a model alone is shorter as its digits. Measured at the quick preset the
    // derivations use, both alike.
    if (m.minimal)
        for (size_t slot = 0; slot < used_lines.size(); ++slot)
        {
            std::vector<uint8_t> as_files, as_places;
            for (size_t i = 0; i < files.size(); ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    as_files.insert(as_files.end(), bytes[i].begin(), bytes[i].end());
                    const std::string& w = files[i]->way;
                    const std::string place = w.substr(w.find(' ', 5) + 1);
                    if (!coding[used_lines[slot]].empty())
                    {
                        // v7: a coded place, its length before it, as the items stream carries it.
                        std::vector<uint8_t> c;
                        for (size_t h = 1; h + 1 < place.size(); h += 2)
                            c.push_back(uint8_t(std::stoul(place.substr(h, 2), nullptr, 16)));
                        uint64_t n = c.size();
                        while (n >= 0x80)
                        {
                            as_places.push_back(uint8_t(n | 0x80));
                            n >>= 7;
                        }
                        as_places.push_back(uint8_t(n));
                        as_places.insert(as_places.end(), c.begin(), c.end());
                        continue;
                    }
                    const std::vector<uint8_t> p = place_bytes_of(place, m.line_widths.at(used_lines[slot]));
                    as_places.insert(as_places.end(), p.begin(), p.end());
                }
            const uint64_t places = std::min<uint64_t>(as_places.size(), estimate(as_places));
            if (places < estimate(as_files)) continue;
            for (size_t i = 0; i < files.size(); ++i)
                if (item[i] && item_line_of_way(files[i]->way) == slot)
                {
                    files[i]->way = is_program(bytes[i]) ? "x86" : "raw";
                    item[i] = false;
                }
        }
    // The lines some item still uses, numbered again in the order the files first use them.
    if (m.minimal)
    {
        std::vector<size_t> still;
        for (size_t i = 0; i < files.size(); ++i)
            if (item[i])
            {
                const size_t old = item_line_of_way(files[i]->way);
                auto at = std::find(still.begin(), still.end(), used_lines[old]);
                if (at == still.end()) at = still.insert(still.end(), used_lines[old]);
                const std::string& w = files[i]->way;
                files[i]->way = "item " + std::to_string(at - still.begin()) + w.substr(w.find(' ', 5));
            }
        used_lines = std::move(still);
    }
    std::vector<std::vector<std::string_view>> lines(files.size());
    std::vector<bool> text(files.size(), false);
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way == "raw" && files[i]->remake.empty() && std::find(bytes[i].begin(), bytes[i].end(), uint8_t('\n')) != bytes[i].end())
        {
            text[i] = true;
            lines[i] = lines_of(bytes[i]);
        }
    std::vector<size_t> order(files.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return bytes[a].size() > bytes[b].size(); }); // largest first
    std::vector<bool> base_of_some(files.size(), false), derived(files.size(), false);
    std::vector<std::vector<uint8_t>> masks(files.size());
    std::vector<size_t> base(files.size(), 0);
    std::unordered_map<size_t, std::unordered_set<std::string_view>> line_sets; // a base's lines, for a quick refusal
    for (const size_t t : order)
    {
        if (!text[t] || base_of_some[t]) continue;
        std::optional<std::vector<uint8_t>> best;
        size_t best_base = 0;
        for (const size_t b : order)
        {
            if (b == t || !text[b] || derived[b] || lines[b].size() < lines[t].size()) continue;
            if (best && (lines[b].size() + 7) / 8 >= best->size()) continue; // a smaller mask is had already
            auto [it, made] = line_sets.try_emplace(b);
            if (made) it->second.insert(lines[b].begin(), lines[b].end());
            bool all = true;
            for (const auto& l : lines[t])
                if (!it->second.count(l))
                {
                    all = false;
                    break;
                }
            if (!all) continue;
            if (auto mk = mask_of(lines[t], lines[b]))
            {
                best = std::move(mk);
                best_base = b;
            }
        }
        // Only where the mask packs smaller than the text itself.
        if (best && estimate(*best) < estimate(bytes[t]))
        {
            derived[t] = true;
            base_of_some[best_base] = true;
            masks[t] = std::move(*best);
            base[t] = best_base;
            files[t]->way = "lines " + std::to_string(best_base);
        }
    }

    // v8: a file carried by the tightened rope (rope-wave-v1) where its code, searched for within the
    // time given, is smaller than the file packed alone. Not a file with a copy in the folder: the
    // LZMA2 stream carries a copy for almost nothing, a rope code each time; nor a small one.
    std::vector<std::vector<uint8_t>> rope_codes(files.size());
    std::vector<RopeParams> rope_params(files.size());
    std::vector<std::vector<uint8_t>> piece_z(files.size()); // v13: a file's LZMA2 pieces, joined
    PackReport r;
    m.roped = m.rope_columns = m.rope_lines = m.rope_groups = m.pieces = m.rope_above = false;
    if (m.compact && rope_seconds >= 0)
    {
        std::unordered_map<std::string, size_t> copies;
        for (size_t i = 0; i < files.size(); ++i) ++copies[sha256_hex(bytes[i])];
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (files[i]->way != "raw" || bytes[i].size() < 512 || copies[sha256_hex(bytes[i])] > 1) continue;
            if (stop_requested()) break; // asked to stop: the rest carried as they are, by LZMA2
            progress("file " + std::to_string(i + 1) + " of " + std::to_string(files.size()) + ": " + files[i]->path + ", the rope and pieces");
            // The smallest of LZMA2 alone, the rope, and (v13) the file in pieces (file_ways).
            FileWays fw = file_ways(bytes[i], rope_seconds, rope_newest, pieces);
            r.rope_tried += fw.tried;
            if (fw.forced || force_requested()) break; // force stop: this file and the rest as they are
            std::string way;
            std::vector<uint8_t> code;
            RopeParams params;
            uint64_t best = fw.lzma2;
            if (fw.rope_bytes < best)
            {
                best = fw.rope_bytes;
                way = "rope " + fw.rope.text();
                code = std::move(fw.rope_code);
                params = fw.rope;
            }
            if (fw.has_pieces && fw.pieces_bytes < best)
            {
                best = fw.pieces_bytes;
                way = fw.pieces_way;
                code = std::move(fw.pieces_code);
                params = fw.pieces_rope;
                piece_z[i] = std::move(fw.pieces_z);
            }
            if (way.empty()) continue;
            rope_codes[i] = std::move(code);
            rope_params[i] = params;
            files[i]->way = way;
            m.roped = true;
            if (way.rfind("pieces ", 0) == 0)
            {
                m.pieces = true; // v13
                ++r.pieces;
                r.pieces_lzma2 += piece_z[i].size();
                r.pieces_rope += bytes[i].size() - piece_z[i].size();
            }
            if (params.columns) m.rope_columns = true;    // v9
            if (params.columns == 2) m.rope_lines = true; // v10
            if (params.grouped) m.rope_groups = m.rope_lines = true; // v11 (which reads v10's ways too)
            if (params.above) m.rope_above = true;                   // v15
        }
    }

    // The streams: the programs; then every file carried as it is, and then the masks, in order.
    std::vector<uint8_t> x86, plain;
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way == "x86")
        {
            x86.insert(x86.end(), bytes[i].begin(), bytes[i].end());
            ++r.x86;
            r.x86_bytes += bytes[i].size();
        }
        else if (files[i]->way == "raw")
        {
            plain.insert(plain.end(), bytes[i].begin(), bytes[i].end());
            ++r.raw;
            r.raw_bytes += bytes[i].size();
        }
        else if (files[i]->way.rfind("pieces ", 0) == 0) plain.insert(plain.end(), piece_z[i].begin(), piece_z[i].end()); // v13: its LZMA2 pieces, at its turn
    for (size_t i = 0; i < files.size(); ++i)
        if (derived[i])
        {
            plain.insert(plain.end(), masks[i].begin(), masks[i].end());
            ++r.derived;
            r.derived_bytes += bytes[i].size();
            r.mask_bytes += masks[i].size();
            r.derivations.emplace_back(files[i]->path, files[base[i]]->path);
        }
    // v8: the rope stream, each code's length then the code, in the listing's order.
    m.rope_carried.clear();
    for (size_t i = 0; i < files.size(); ++i)
        if (files[i]->way.rfind("rope ", 0) == 0 || files[i]->way.rfind("pieces ", 0) == 0)
        {
            uint64_t len = rope_codes[i].size();
            while (len >= 0x80)
            {
                m.rope_carried.push_back(uint8_t(len | 0x80));
                len >>= 7;
            }
            m.rope_carried.push_back(uint8_t(len));
            m.rope_carried.insert(m.rope_carried.end(), rope_codes[i].begin(), rope_codes[i].end());
            ++r.roped;
            r.rope_bytes += bytes[i].size();
        }
    r.rope_carried = m.rope_carried.size();
    r.rope_columns = m.rope_columns;
    r.rope_lines = m.rope_lines;
    r.rope_groups = m.rope_groups;
    m.rope_stream = {m.rope_carried.size(), m.rope_carried.size(), 0};
    // (While a sample is timed for the File Locator's estimate, the dictionary of the whole it stands
    // for: LZMA2 takes longer a byte with a larger one. On the 52 MB recording the estimate came to
    // 8 min 01 s with it, 7 min 15 s without, the run taking 11.5 min; IDEAS §19.97.)
    const uint32_t floor = g_sample_floor.load(std::memory_order_relaxed);
    m.x86 = {0, x86.size(), std::max(dictionary_for(x86.size()), x86.empty() ? 0 : floor)};
    m.lzma2 = {0, plain.size(), std::max(dictionary_for(plain.size()), plain.empty() ? 0 : floor)};
    // v17's packing: LZMA2's settings chosen for each stream (older versions are made as they were).
    const bool tune = gzip && binary;
    std::vector<uint8_t> px = pack_stream(x86, m.x86.dictionary, true, 9 | LZMA_PRESET_EXTREME, tune, true),
                         pp = pack_stream(plain, m.lzma2.dictionary, false, 9 | LZMA_PRESET_EXTREME, tune, true);
    // A force stop before a stream's first pass was done: the x86 stream (never carried as it is)
    // packed at LZMA2's quickest preset, which is not broken off; the lzma2 stream carried as it is
    // where the version allows (below), else the same.
    if (px.empty() && !x86.empty())
    {
        progress("force stopped: the programs packed at LZMA2's quickest");
        px = encode_lzma2(x86, m.x86.dictionary, true, 0, -1, 0, false);
    }
    const bool pp_abandoned = pp.empty() && !plain.empty();
    if (pp_abandoned && !(trim && m.compact)) pp = encode_lzma2(plain, m.lzma2.dictionary, false, 0, -1, 0, false);
    m.x86.packed = px.size();
    m.lzma2.packed = pp.size();
    m.streams = px;
    // v12: the files carried as they are where LZMA2 makes them no smaller (a PNG, a JPEG, a 7z).
    bool lzma2_as_is = false;
    if (trim && m.compact && !plain.empty() && (pp_abandoned || pp.size() >= plain.size()))
    {
        lzma2_as_is = true;
        m.lzma2 = {plain.size(), plain.size(), 0};
        m.streams.insert(m.streams.end(), plain.begin(), plain.end());
    }
    else m.streams.insert(m.streams.end(), pp.begin(), pp.end());
    m.contents.clear();
    for (size_t i = 0; i < bytes.size(); ++i) // a remade file's own bytes, not what it holds
    {
        const std::vector<uint8_t>& b = original[i].empty() ? bytes[i] : original[i];
        m.contents.insert(m.contents.end(), b.begin(), b.end());
    }
    m.packed = m.with_contents = true;
    m.with_addresses = false;
    r.packed = m.streams.size();
    // v5: the lines the items use, renumbered, and the tree's SHA-256 over every file.
    // v7 and v8 keep their form with no items too (no lines, an empty items stream): what they
    // leave out and the listing packed help any folder, and the tree's SHA-256 is their check.
    m.items = !used_lines.empty() || m.compact;
    if (m.items)
    {
        std::vector<std::string> renumbered, codings;
        std::vector<uint32_t> widths;
        for (size_t k : used_lines)
        {
            renumbered.push_back(m.lines.at(k));
            if (m.minimal) widths.push_back(m.line_widths.at(k));
            if (m.compact) codings.push_back(coding.at(k));
            if (m.compact && !coding[k].empty()) r.coded_lines.push_back(m.lines[k]);
        }
        m.lines = std::move(renumbered);
        m.line_widths = std::move(widths);
        m.line_codings = std::move(codings);
        r.compact = m.compact;
        if (m.minimal)
        {
            // v6: the places, carried raw or as LZMA2, whichever is smaller.
            const std::vector<uint8_t> places = m.items_stream();
            r.item_places = places.size();
            const uint32_t dict = dictionary_for(places.size());
            std::vector<uint8_t> packed = places.empty() ? std::vector<uint8_t>() : pack_stream(places, dict, false, 9 | LZMA_PRESET_EXTREME, tune);
            if (!places.empty() && packed.size() < places.size())
            {
                m.item_stream = {packed.size(), places.size(), dict};
                m.items_carried = std::move(packed);
            }
            else
            {
                m.item_stream = {places.size(), places.size(), 0};
                m.items_carried = places;
            }
            r.item_carried = m.items_carried.size();
        }
        for (size_t i = 0; i < files.size(); ++i)
            if (item[i])
            {
                if (!item_hashes) files[i]->sha256 = "-";
                ++r.items;
                r.item_bytes += bytes[i].size();
                r.item_text += m.minimal ? files[i]->way.find(' ', 5) : files[i]->way.size();
                if (m.minimal) r.item_raw_places += m.line_widths.at(item_line_of_way(files[i]->way));
            }
        // v7: every file's own SHA-256 left out, as an item's is: the tree's covers them all.
        if (m.compact && !item_hashes)
            for (ManifestEntry* f : files) f->sha256 = "-";
        m.tree = tree_sha256(m);
        // v12, only where it differs from v11: no items line where there are no items, or the
        // lzma2 stream carried as it is.
        if (m.archives) m.pieces = true; // v14 reads v13's ways
        for (const ManifestEntry* f : files) r.remade += !f->remake.empty();
        r.unpacked = m.unpacked.size();
        m.trimmed = trim && m.compact && (lzma2_as_is || m.pieces || (m.item_stream.packed == 0 && m.item_stream.bytes == 0));
        r.pieces_made = m.pieces;
        r.archives_made = m.archives;
        r.remake_gz = m.remake_gz;
        r.more_archives = m.more_archives;
        r.v20 = m.v20;
        r.rope_above = m.rope_above;
        r.trimmed = m.trimmed;
        r.lzma2_as_is = lzma2_as_is;
    }
    else
    {
        m.lines.clear();
        m.line_widths.clear();
        m.line_codings.clear();
        m.minimal = m.compact = m.roped = m.rope_columns = m.rope_lines = m.rope_groups = m.rope_above = false; // no items: v4, as before
    }

    // Where it is no smaller than the files as they are, they are carried so (v3). (Not with items:
    // they are what makes v5 smaller.)
    if (!m.items)
    {
        Manifest plain_m = m;
        plain_m.packed = false;
        plain_m.streams.clear();
        for (ManifestEntry& e : plain_m.entries) e.way.clear();
        if (plain_m.text().size() + plain_m.contents.size() <= m.text().size() + m.streams.size())
        {
            m = std::move(plain_m);
            r.kept_v3 = true;
            if (report) *report = std::move(r);
            return;
        }
    }
    // v7: the listing carried as LZMA2 where that, with the line saying so, is smaller (a listing of
    // many files repeats itself: paths, and the SHA-256 of every copy of a file).
    m.listing_stream = {};
    m.listing_carried.clear();
    m.listing_of.clear();
    // v16: the head in bytes, where everything v15 allows is allowed (never an older version asked for).
    m.binary = binary && m.compact;
    // v20: wherever the head is in bytes at the newest, the body is too (and v20's archives allowed).
    const bool had_v21 = m.v21;
    m.v20 = m.v20 || (m.binary && gzip && v20_on);
    m.v21 = had_v21 && m.binary; // (v21 only in bytes, where a deflate is made again)
    if (m.v20) m.more_archives = m.remake_gz = true;
    r.v20 = m.v20;
    r.v21 = m.v21;
    if (m.binary)
    {
        r.head_text = m.text().size();
        const std::string text = listing_body(m);
        // v20: the body in bytes, or as text where that comes out smaller after all (head flag 128),
        // each packed by LZMA2 where that is smaller; the smallest kept.
        std::string body = text;
        if (m.v20)
        {
            const std::vector<uint8_t> bytes = listing_to_bytes(text);
            auto carried = [&](const std::string& b) {
                if (b.empty()) return size_t(1);
                const std::vector<uint8_t> p = pack_stream(std::vector<uint8_t>(b.begin(), b.end()), dictionary_for(b.size()), false, 9 | LZMA_PRESET_EXTREME, tune);
                return std::min(b.size() + 3, p.size() + 12);
            };
            const std::string as_bytes(bytes.begin(), bytes.end());
            m.body_text = carried(text) < carried(as_bytes);
            if (!m.body_text) body = as_bytes;
        }
        const uint32_t dict = dictionary_for(body.size());
        std::vector<uint8_t> packed = body.empty() ? std::vector<uint8_t>() : pack_stream(std::vector<uint8_t>(body.begin(), body.end()), dict, false, 9 | LZMA_PRESET_EXTREME, tune);
        if (!body.empty() && packed.size() + 12 < body.size())
        {
            m.listing_stream = {packed.size(), body.size(), dict};
            m.listing_carried = std::move(packed);
            m.listing_of = text;
            r.listing = body.size();
            r.listing_carried = m.listing_carried.size();
        }
        r.binary = true;
    }
    else if (m.compact)
    {
        const std::string t = m.text();
        const std::vector<uint8_t> listing(t.begin() + std::ptrdiff_t(t.find('\n') + 1), t.end());
        const uint32_t dict = dictionary_for(listing.size());
        std::vector<uint8_t> packed = pack_stream(listing, dict, false, 9 | LZMA_PRESET_EXTREME, tune);
        const std::string line = "listing " + std::to_string(packed.size()) + " " + std::to_string(listing.size()) + " " + std::to_string(dict) + "\n";
        if (line.size() + packed.size() < listing.size())
        {
            m.listing_stream = {packed.size(), listing.size(), dict};
            m.listing_carried = std::move(packed);
            m.listing_of = t;
            r.listing = listing.size();
            r.listing_carried = line.size() + m.listing_carried.size();
        }
    }
    // Read back before it is used: unpacked, every file as it was.
    const std::vector<uint8_t> whole = m.file();
    if (m.binary)
    {
        const size_t carried = m.streams.size() + m.items_carried.size() + (m.roped ? m.rope_carried.size() : 0);
        r.head_bytes = whole.size() - carried;
    }
    const Manifest back = Manifest::parse(std::string_view(reinterpret_cast<const char*>(whole.data()), whole.size()));
    if (back.contents != m.contents) throw std::runtime_error("packing the installer did not give its files back (a fault in the packer)");
    if (report) *report = std::move(r);
}

std::string pack_report_text(const PackReport& r, uint64_t original)
{
    std::ostringstream o;
    if (r.kept_v3)
    {
        o << "packed    not: packing would not make it smaller, so its files are carried as they are (sieve-manifest-v3)\n";
        return o.str();
    }
    auto pct = [&](uint64_t v) {
        char b[32];
        std::snprintf(b, sizeof b, "%.2f%%", original ? 100.0 * double(v) / double(original) : 100.0);
        return std::string(b);
    };
    if (r.compact)
    {
        const char* version = r.v21 ? kDeflateManifestVersion : r.v20 ? kNewestArchivesManifestVersion : r.more_archives ? kMoreArchivesManifestVersion : r.remake_gz ? kGzipManifestVersion : r.binary ? kBinaryManifestVersion : r.rope_above ? kRopeAboveManifestVersion : r.archives_made ? kArchivesManifestVersion : r.pieces_made ? kPiecesManifestVersion : r.trimmed ? kTrimmedManifestVersion : r.rope_groups ? kRopeGroupsManifestVersion : r.rope_lines ? kRopeLinesManifestVersion : r.rope_columns ? kRopeColumnsManifestVersion
                            : r.roped ? kRopeManifestVersion : kCompactManifestVersion;
        if (r.items)
        {
            o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, as " << r.item_places
              << " bytes of places";
            if (!r.coded_lines.empty())
                o << " (" << r.item_raw_places << " raw; " << kCodedPlaces << " on " << r.coded_lines.size() << " line(s))";
            o << ", carried in " << r.item_carried << "; " << version << ", made again on installing)\n";
        }
        else o << "items     none; " << version << "\n";
        if (r.listing_carried) o << "listing   " << r.listing << " bytes, carried as LZMA2 in " << r.listing_carried << "\n";
        if (r.binary)
            o << "head      " << r.head_bytes << " bytes in bytes, the listing " << r.head_text << " as text (" << (r.v21 ? kDeflateManifestVersion : r.v20 ? kNewestArchivesManifestVersion : kBinaryManifestVersion)
              << (r.v20 ? "; its body in bytes too" : "") << ")\n";
        if (r.remade) o << "remade    " << r.remade << " archive(s) carried as what they hold, remade exactly on installing (" << version << ")\n";
        if (r.unpacked)
            o << "unpacked  " << r.unpacked << " archive(s) carried as their contents, not as the archives (Optimise Archive Formats; " << version
              << ")\n";
        if (r.pieces)
            o << "pieces    " << r.pieces << " file(s) cut into pieces: " << r.pieces_lzma2 << " bytes packed by LZMA2, " << r.pieces_rope
              << " pulled tight by the rope (" << kPiecesManifestVersion << ")\n";
        if (r.lzma2_as_is) o << "as is     the files packed by LZMA2 carried as they are instead: it made them no smaller (" << kTrimmedManifestVersion << ")\n";
        if (r.roped)
            o << "rope      " << r.roped << " file(s) pulled tight by " << (r.rope_above ? "rope-wave-v1 to v5" : r.rope_groups ? "rope-wave-v1 to v4" : r.rope_lines ? "rope-wave-v1 to v3" : r.rope_columns ? "rope-wave-v1 and v2" : "rope-wave-v1") << " (" << r.rope_bytes << " bytes, carried in " << r.rope_carried
              << "; " << version << ")\n";
    }
    else if (r.items && r.item_places)
        o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, as " << r.item_places
          << " bytes of places, carried in " << r.item_carried << ", and " << r.item_text
          << " characters of the listing; sieve-manifest-v6, made again on installing)\n";
    else if (r.items)
        o << "items     " << r.items << " file(s) carried as items of Sieve's lines (" << r.item_bytes << " bytes, named in " << r.item_text
          << " characters of the listing; sieve-manifest-v5, made again on installing)\n";
    o << "packed    " << r.packed << " bytes (" << pct(r.packed) << " of " << original << "): " << r.x86 << " program(s) in the x86 stream ("
      << r.x86_bytes << " bytes), " << r.raw << " file(s) as they are (" << r.raw_bytes << " bytes), " << r.derived
      << " derived from another's lines (" << r.derived_bytes << " bytes, as " << r.mask_bytes << " bytes of masks)\n";
    for (const auto& [d, b] : r.derivations) o << "          " << d << ": lines of " << b << "\n";
    return o.str();
}

// ---- Any archive Sieve reads, extracted (25 October 2026): the right-click menu's Extract on an
// archive, as an archiver's. One layer only, as 7-Zip's: a .tar.gz gives its .tar.

namespace {

bool name_ends(const std::string& name, const char* tail)
{
    const size_t n = std::strlen(tail);
    if (name.size() <= n) return false;
    for (size_t i = 0; i < n; ++i)
        if (std::tolower(static_cast<unsigned char>(name[name.size() - n + i])) != tail[i]) return false;
    return true;
}

// The one file a compressed stream holds: its name without the extension (a .tgz's, .tar), else the
// name with ".out".
std::string stream_inner_name(const std::string& name, std::initializer_list<const char*> exts, std::initializer_list<const char*> tar_exts)
{
    for (const char* t : tar_exts)
        if (name_ends(name, t)) return name.substr(0, name.size() - std::strlen(t)) + ".tar";
    for (const char* t : exts)
        if (name_ends(name, t)) return name.substr(0, name.size() - std::strlen(t));
    return name + ".out";
}

} // namespace

// Every kind extracted, and the names its files go by: the one list, which the right-click menu is
// made from (sieve-install --archive-extensions; context/install_context.bat). A kind added to
// read_any_archive is added here, and the menu takes it up the next time it is set up.
const std::vector<ArchiveKind>& archive_kinds()
{
    static const std::vector<ArchiveKind> kinds = {
        {"7z", {".7z"}},
        {"zip", {".zip", ".zipx"}},
        {"xz", {".xz", ".txz"}},
        {"gz", {".gz", ".tgz"}},
        {"bz2", {".bz2", ".tbz2", ".tbz"}},
        {"zst", {".zst", ".tzst"}},
        {"lz4", {".lz4", ".tlz4"}},
        {"lz", {".lz", ".tlz"}},
        {"lzma", {".lzma"}},
        {"br", {".br"}},
        {"rar", {".rar"}},
        {"cab", {".cab"}},
        {"iso", {".iso"}},
        {"lha", {".lzh", ".lha"}},
        {"cpio", {".cpio"}},
        {"ar", {".ar", ".deb"}},
        {"warc", {".warc"}},
        {"tar", {".tar"}},
    };
    return kinds;
}

std::optional<AnyArchive> read_any_archive(const std::vector<uint8_t>& b, const std::string& file_name)
{
    std::string name = file_name;
    const size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) name = name.substr(slash + 1);
    AnyArchive a;
    auto take = [&](std::vector<sieve::install::InMemory>&& all) {
        for (auto& m : all) a.entries.push_back({std::move(m.entry.path), m.entry.dir, std::move(m.bytes)});
    };
    auto one = [&](const char* format, std::optional<std::vector<uint8_t>>&& inner, std::string inner_name) {
        if (!inner) throw std::runtime_error(std::string("it is a damaged ") + format + " stream, or one this cannot read");
        a.format = format;
        a.entries.push_back({std::move(inner_name), false, std::move(*inner)});
    };
    if (sieve::install::is_7z(b))
    {
        a.format = "7z";
        // (read_7z leaves out a top folder; put back, as an archiver's Extract keeps it.)
        const std::string top = sieve::install::list_7z(b).top;
        auto all = sieve::install::read_7z(b);
        if (!top.empty())
        {
            a.entries.push_back({top, true, {}});
            for (auto& m : all) m.entry.path = top + "/" + m.entry.path;
        }
        take(std::move(all));
    }
    else if (b.size() >= 4 && b[0] == 'P' && b[1] == 'K' && ((b[2] == 3 && b[3] == 4) || (b[2] == 5 && b[3] == 6)))
    {
        a.format = "zip";
        if (auto all = zip_entries(b, false)) take(std::move(*all));
        else
        {
            std::string format;
            auto more = libarchive_entries(b, format, &a.links_left_out); // zip64, other methods
            if (!more) throw std::runtime_error("it is a zip this cannot read (encrypted, damaged, or in a method it does not know)");
            take(std::move(*more));
        }
    }
    else if (is_xz(b)) one("xz", xz_inner(b), stream_inner_name(name, {".xz"}, {".txz"}));
    else if (is_gzip(b)) one("gz", gzip_inner(b), stream_inner_name(name, {".gz", ".z"}, {".tgz"}));
    else if (is_bz2(b)) one("bz2", bz2_inner(b), stream_inner_name(name, {".bz2"}, {".tbz2", ".tbz"}));
    else if (is_zstd(b)) one("zst", zstd_inner(b), stream_inner_name(name, {".zst"}, {".tzst"}));
    else if (is_lz4(b)) one("lz4", lz4_inner(b), stream_inner_name(name, {".lz4"}, {".tlz4"}));
    else if (is_lzip(b)) one("lz", liblzma_inner(b, true), stream_inner_name(name, {".lz"}, {".tlz"}));
    else if (name_ends(name, ".lzma") && is_lzma_alone(b)) one("lzma", liblzma_inner(b, false), stream_inner_name(name, {".lzma"}, {}));
    else if (name_ends(name, ".br")) one("br", brotli_inner(b), stream_inner_name(name, {".br"}, {}));
    else
    {
        std::string format;
        auto all = libarchive_entries(b, format, &a.links_left_out);
        if (!all) return std::nullopt;
        a.format = format;
        take(std::move(*all));
    }
    for (const auto& e : a.entries)
        if (!e.dir)
        {
            ++a.files;
            a.bytes += e.bytes.size();
        }
    return a;
}

void leave_out_top_folder(AnyArchive& a, std::string* top_out)
{
    if (a.entries.empty()) return;
    const std::string top = a.entries[0].path.substr(0, a.entries[0].path.find('/'));
    bool one = true, inside = false;
    for (const auto& e : a.entries)
    {
        if (e.path.substr(0, e.path.find('/')) != top || (e.path == top && !e.dir)) one = false;
        if (e.path.size() > top.size()) inside = true;
    }
    if (!one || !inside) return;
    std::vector<AnyArchive::Entry> kept;
    for (auto& e : a.entries)
        if (e.path != top)
        {
            e.path = e.path.substr(top.size() + 1);
            kept.push_back(std::move(e));
        }
    a.entries = std::move(kept);
    if (top_out) *top_out = top;
}

bool unpack_archive(const AnyArchive& a, const std::filesystem::path& dest, bool force,
                    const std::function<void(size_t, size_t, const std::string&)>& progress, const std::atomic<bool>* cancel)
{
    namespace fs = std::filesystem;
    auto path_of = [&](const std::string& p) {
        // (Every reader has refused a name that could leave its folder; checked again here.)
        if (p.empty() || p[0] == '/' || p.find('\\') != std::string::npos || p.find(':') != std::string::npos)
            throw std::runtime_error("the archive has a name that leaves its folder: " + p);
        for (size_t at = 0; at <= p.size();)
        {
            size_t s = p.find('/', at);
            if (s == std::string::npos) s = p.size();
            if (p.compare(at, s - at, "..") == 0 && s - at == 2) throw std::runtime_error("the archive has a name that leaves its folder: " + p);
            at = s + 1;
        }
        return dest / fs::path(std::u8string(p.begin(), p.end()));
    };
    if (!force)
        for (const auto& e : a.entries)
            if (!e.dir && fs::exists(path_of(e.path))) throw std::runtime_error("already there (tick Replace to write over it): " + e.path);
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
    size_t done = 0;
    try
    {
        make_dirs(dest);
        for (const auto& e : a.entries)
        {
            if (cancel && *cancel)
            {
                undo();
                return false;
            }
            const fs::path to = path_of(e.path);
            if (e.dir)
            {
                make_dirs(to);
                continue;
            }
            make_dirs(to.parent_path());
            vault::check_bytes(e.bytes, e.path); // nothing the vault withholds is written, whatever it arrives in
            std::ofstream f(to, std::ios::binary | std::ios::trunc);
            if (!f) throw std::runtime_error("cannot write " + e.path);
            written.push_back(to);
            if (!e.bytes.empty()) f.write(reinterpret_cast<const char*>(e.bytes.data()), std::streamsize(e.bytes.size()));
            f.close();
            if (!f) throw std::runtime_error("cannot write " + e.path);
            ++done;
            if (progress) progress(done, size_t(a.files), e.path);
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
