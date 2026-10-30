// Sieve CLI -- a JPEG made again exactly from its brunsli form (jpeg_glue.hpp).

#include "cli/jpeg_glue.hpp"

#include <brunsli/brunsli_decode.h>
#include <brunsli/brunsli_encode.h>
#include <brunsli/jpeg_data.h>
#include <brunsli/jpeg_data_reader.h>
#include <brunsli/jpeg_data_writer.h>

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
