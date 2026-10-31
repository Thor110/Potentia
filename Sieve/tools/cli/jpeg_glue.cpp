// Sieve CLI -- a JPEG made again exactly from its brunsli form (jpeg_glue.hpp).

#include "cli/jpeg_glue.hpp"

#include <brunsli/brunsli_decode.h>
#include <brunsli/brunsli_encode.h>
#include <brunsli/jpeg_data.h>
#include <brunsli/jpeg_data_reader.h>
#include <brunsli/jpeg_data_writer.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace sieve::cli {

namespace {

size_t append(void* to, const uint8_t* buf, size_t len)
{
    auto* out = static_cast<std::vector<uint8_t>*>(to);
    out->insert(out->end(), buf, buf + len);
    return len;
}

// Annex K.2's code lengths for these counts (257: the 256 symbols and one reserved, so that no code
// is all ones), limited to 16 bits, in brunsli's form: the counts by length and the symbols by
// length then value, the reserved one (256) last at the longest length, as brunsli's reader keeps a
// table it has read.
brunsli::JPEGHuffmanCode optimal_code(const std::array<long, 257>& counts, int slot_id, bool is_last)
{
    std::array<long, 257> freq = counts;
    freq[256] = 1;
    std::array<int, 257> size{}, others;
    others.fill(-1);
    for (;;)
    {
        // The two least frequent left (the later of equals first, as libjpeg takes them).
        int c1 = -1, c2 = -1;
        long v = 0;
        for (int i = 0; i <= 256; ++i)
            if (freq[i] && (c1 < 0 || freq[i] <= v)) v = freq[i], c1 = i;
        for (int i = 0; i <= 256; ++i)
            if (freq[i] && i != c1 && (c2 < 0 || freq[i] <= v)) v = freq[i], c2 = i;
        if (c2 < 0) break;
        freq[c1] += freq[c2];
        freq[c2] = 0;
        ++size[c1];
        while (others[c1] >= 0) ++size[c1 = others[c1]];
        others[c1] = c2;
        ++size[c2];
        while (others[c2] >= 0) ++size[c2 = others[c2]];
    }
    std::array<int, 33> bits{};
    for (int i = 0; i <= 256; ++i)
        if (size[i])
        {
            if (size[i] > 32) throw std::runtime_error("a Huffman code longer than Annex K allows"); // (refused by the caller)
            ++bits[size_t(size[i])];
        }
    // Every code to 16 bits at most: a pair at the longest length made one shorter, a shorter code
    // made one longer to make room for it (Annex K, Figure K.3).
    for (int i = 32; i > 16; --i)
        while (bits[i] > 0)
        {
            int j = i - 2;
            while (bits[j] == 0) --j;
            bits[i] -= 2;
            ++bits[i - 1];
            bits[j + 1] += 2;
            --bits[j];
        }
    brunsli::JPEGHuffmanCode h;
    h.slot_id = slot_id;
    h.is_last = is_last;
    size_t n = 0;
    for (int len = 1; len <= 32; ++len)
        for (int s = 0; s < 256; ++s)
            if (size[s] == len) h.values[n++] = s;
    h.values[n] = brunsli::kJpegHuffmanAlphabetSize;
    for (int len = 1; len <= 16; ++len) h.counts[len] = bits[len];
    // (The reserved symbol is the least frequent, so it is at the longest length, where brunsli's
    // form counts it: nothing to move.)
    return h;
}

} // namespace

bool is_jpeg(const std::vector<uint8_t>& b) { return b.size() >= 4 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF; }

std::vector<uint8_t> brunsli_to_jpeg(const std::vector<uint8_t>& form)
{
    brunsli::JPEGData jpg;
    if (brunsli::BrunsliDecodeJpeg(form.data(), form.size(), &jpg) != brunsli::BRUNSLI_OK)
        throw std::runtime_error("a JPEG's brunsli form that does not read");
    std::vector<uint8_t> out;
    if (!brunsli::WriteJpeg(jpg, brunsli::JPEGOutput(append, &out))) throw std::runtime_error("a JPEG that brunsli could not write again");
    return out;
}

std::optional<std::vector<uint8_t>> jpeg_optimised(const std::vector<uint8_t>& jpeg)
{
    if (!is_jpeg(jpeg)) return std::nullopt;
    try
    {
        brunsli::JPEGData jpg;
        if (!brunsli::ReadJpeg(jpeg.data(), jpeg.size(), brunsli::JPEG_READ_ALL, &jpg)) return std::nullopt;
        // Sequential only (SOF0 or SOF1), every table defined once and before the first scan, so each
        // scan codes with the tables made here.
        bool sequential = false, scanned = false;
        for (const uint8_t marker : jpg.marker_order)
        {
            if (marker == 0xC0 || marker == 0xC1) sequential = true;
            if (marker == 0xC2 || marker == 0xC3) return std::nullopt;
            if (marker == 0xDA) scanned = true;
            if (marker == 0xC4 && scanned) return std::nullopt;
        }
        if (!sequential || jpg.scan_info.empty()) return std::nullopt;
        std::array<bool, 0x20> defined{};
        for (const auto& h : jpg.huffman_code)
        {
            if (h.slot_id < 0 || h.slot_id >= 0x20 || defined[size_t(h.slot_id)]) return std::nullopt;
            defined[size_t(h.slot_id)] = true;
        }
        // How often each table codes each symbol, block by block as brunsli's writer codes them:
        // each DC as its difference from the last of its component (from 0 after a restart), each
        // AC as runs of zeros and a value, 16 zeros at a time (F0), then the end of the block (00).
        std::vector<std::array<long, 257>> freq(0x20);
        for (auto& f : freq) f.fill(0);
        size_t scan_number = 0;
        bool dri = false;
        for (const uint8_t marker : jpg.marker_order)
        {
            if (marker == 0xDD) dri = true;
            if (marker != 0xDA) continue;
            if (scan_number >= jpg.scan_info.size()) return std::nullopt;
            const brunsli::JPEGScanInfo& scan = jpg.scan_info[scan_number++];
            const int restart = dri ? jpg.restart_interval : 0;
            const bool interleaved = scan.num_components > 1;
            const brunsli::JPEGComponent& base = jpg.components.at(scan.components[0].comp_idx);
            const int hg = interleaved ? 1 : base.h_samp_factor, vg = interleaved ? 1 : base.v_samp_factor;
            const int per_row = (jpg.width * hg + 8 * jpg.max_h_samp_factor - 1) / (8 * jpg.max_h_samp_factor);
            const int rows = (jpg.height * vg + 8 * jpg.max_v_samp_factor - 1) / (8 * jpg.max_v_samp_factor);
            std::array<int, 4> last{};
            int to_go = restart;
            for (int my = 0; my < rows; ++my)
                for (int mx = 0; mx < per_row; ++mx)
                {
                    if (restart > 0 && to_go == 0)
                    {
                        last.fill(0);
                        to_go = restart;
                    }
                    for (size_t k = 0; k < scan.num_components; ++k)
                    {
                        const brunsli::JPEGComponentScanInfo& si = scan.components[k];
                        const brunsli::JPEGComponent& c = jpg.components.at(si.comp_idx);
                        auto& dc = freq.at(size_t(si.dc_tbl_idx));
                        auto& ac = freq.at(size_t(si.ac_tbl_idx) + 0x10);
                        const int ny = interleaved ? c.v_samp_factor : 1, nx = interleaved ? c.h_samp_factor : 1;
                        for (int iy = 0; iy < ny; ++iy)
                            for (int ix = 0; ix < nx; ++ix)
                            {
                                const size_t block = size_t(my * ny + iy) * c.width_in_blocks + size_t(mx * nx + ix);
                                if ((block + 1) * 64 > c.coeffs.size()) return std::nullopt;
                                const brunsli::coeff_t* q = &c.coeffs[block * 64];
                                int d = int(q[0]) - last.at(si.comp_idx);
                                last.at(si.comp_idx) = q[0];
                                int bits = 0;
                                for (unsigned a = unsigned(d < 0 ? -d : d); a; a >>= 1) ++bits;
                                ++dc[size_t(bits)];
                                int run = 0;
                                for (int z = 1; z < 64; ++z)
                                {
                                    const int v = q[brunsli::kJPEGNaturalOrder[z]];
                                    if (v == 0)
                                    {
                                        ++run;
                                        continue;
                                    }
                                    for (; run > 15; run -= 16) ++ac[0xF0];
                                    int nb = 0;
                                    for (unsigned a = unsigned(v < 0 ? -v : v); a; a >>= 1) ++nb;
                                    ++ac[size_t((run << 4) + nb)];
                                    run = 0;
                                }
                                if (run > 0) ++ac[0x00];
                            }
                    }
                    --to_go;
                }
        }
        if (scan_number != jpg.scan_info.size()) return std::nullopt;
        for (auto& h : jpg.huffman_code)
        {
            const auto& f = freq[size_t(h.slot_id)];
            bool used = false;
            for (int s = 0; s < 256; ++s) used = used || f[size_t(s)] > 0;
            if (used) h = optimal_code(f, h.slot_id, h.is_last);
        }
        // Written afresh: the padding bits standard (ones), and no extra runs of zeros, which some
        // encoders put before a block's end and which change nothing.
        jpg.has_zero_padding_bit = false;
        jpg.padding_bits.clear();
        for (auto& scan : jpg.scan_info) scan.extra_zero_runs.clear();
        std::vector<uint8_t> out;
        if (!brunsli::WriteJpeg(jpg, brunsli::JPEGOutput(append, &out)) || out.size() >= jpeg.size()) return std::nullopt;
        // The same coefficients and tables, read back from what was written.
        brunsli::JPEGData again, first;
        if (!brunsli::ReadJpeg(out.data(), out.size(), brunsli::JPEG_READ_ALL, &again) ||
            !brunsli::ReadJpeg(jpeg.data(), jpeg.size(), brunsli::JPEG_READ_ALL, &first))
            return std::nullopt;
        if (again.width != first.width || again.height != first.height || again.components.size() != first.components.size() ||
            again.quant.size() != first.quant.size())
            return std::nullopt;
        for (size_t k = 0; k < first.components.size(); ++k)
            if (again.components[k].coeffs != first.components[k].coeffs || again.components[k].quant_idx != first.components[k].quant_idx)
                return std::nullopt;
        for (size_t k = 0; k < first.quant.size(); ++k)
            if (again.quant[k].values != first.quant[k].values) return std::nullopt;
        return out;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<std::vector<uint8_t>> jpeg_to_brunsli(const std::vector<uint8_t>& jpeg)
{
    if (!is_jpeg(jpeg)) return std::nullopt;
    try
    {
        brunsli::JPEGData jpg;
        if (!brunsli::ReadJpeg(jpeg.data(), jpeg.size(), brunsli::JPEG_READ_ALL, &jpg)) return std::nullopt;
        size_t len = brunsli::GetMaximumBrunsliEncodedSize(jpg);
        std::vector<uint8_t> form(len);
        if (!brunsli::BrunsliEncodeJpeg(jpg, form.data(), &len)) return std::nullopt;
        form.resize(len);
        // Given back byte for byte, or not taken: brunsli refuses what it cannot keep, and this
        // checks it did.
        if (brunsli_to_jpeg(form) != jpeg) return std::nullopt;
        return form;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

} // namespace sieve::cli
