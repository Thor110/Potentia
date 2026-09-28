// Sieve — content-defined chunks (chunks.hpp), cdc-v1: the gear table, made once from SHA-256, and
// the one pass over the bytes that finds the cuts (the Chunker; chunks() feeds it all at once).

#include "sieve/chunks.hpp"

namespace sieve::cdc {

const std::array<uint64_t, 256>& gear()
{
    static const std::array<uint64_t, 256> table = [] {
        std::array<uint64_t, 256> t{};
        for (size_t i = 0; i < 256; ++i)
        {
            Sha256 h;
            h.update(std::string_view("cdc-v1 gear"));
            const uint8_t b = uint8_t(i);
            h.update(std::span<const uint8_t>(&b, 1));
            const Sha256::Digest d = h.finish();
            uint64_t v = 0;
            for (size_t k = 0; k < 8; ++k) v = v << 8 | d[k];
            t[i] = v;
        }
        return t;
    }();
    return table;
}

void Chunker::close()
{
    Chunk c;
    c.offset = start_;
    c.length = len_;
    c.sha256 = sha_.finish();
    c.uniform = uniform_;
    ready_.push_back(c);
    start_ += len_;
    len_ = 0;
    h_ = 0;
    sha_ = Sha256();
    uniform_ = true;
}

void Chunker::update(std::span<const uint8_t> data)
{
    const auto& g = gear();
    size_t from = 0; // the first byte of data not yet given to the chunk's SHA-256
    for (size_t i = 0; i < data.size(); ++i)
    {
        const uint8_t b = data[i];
        if (len_ == 0) first_ = b;
        else if (b != first_) uniform_ = false;
        h_ = (h_ << 1) + g[b];
        ++len_;
        if ((len_ >= kMin && (h_ >> 56) == 0) || len_ >= kMax)
        {
            sha_.update(data.subspan(from, i + 1 - from));
            from = i + 1;
            close();
        }
    }
    if (from < data.size()) sha_.update(data.subspan(from));
}

void Chunker::finish()
{
    if (len_ > 0) close();
}

std::vector<Chunk> chunks(std::span<const uint8_t> bytes)
{
    Chunker c;
    c.update(bytes);
    c.finish();
    return std::move(c.ready());
}

} // namespace sieve::cdc
