// Sieve CLI -- files carried as items of Sieve's lines (see items.hpp).
#include "cli/items.hpp"

#include "cli/filter_config.hpp"
#include "cli/locate.hpp"
#include "sieve/digit_histogram.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace sieve::cli {

namespace {

std::string options_text(const Args& b)
{
    std::string t;
    for (const auto& [k, v] : b.opts)
    {
        if (v.find(' ') != std::string::npos || v.find('=') != std::string::npos)
            throw std::invalid_argument("a line's option holds a space or '=', which a listing cannot write: " + k);
        t += (t.empty() ? "" : " ") + k + "=" + v;
    }
    return t;
}

ItemLine item_line(Args b)
{
    b.opts["model"] = "none"; // the guided model is no part of a positional address
    ItemLine l;
    l.line = make_line(b);
    l.spec = l.line->space.id() + " " + options_text(b);
    l.width = place_width(l.line->space.size());
    return l;
}

// The AI line's options, as a listing writes them and as ai_of_options reads them back.
std::shared_ptr<const AiSpace> ai_of_options(const Args& b)
{
    AiShape shape;
    shape.layers = b.get_u32("ai-layers", shape.layers);
    shape.width = b.get_u32("ai-width", shape.width);
    shape.heads = b.get_u32("ai-heads", shape.heads);
    shape.bits = b.get_u32("ai-bits", shape.bits);
    shape.check();
    return std::make_shared<const AiSpace>(shape, b.get("key", "sieve"));
}

ItemLine ai_item_line(const Args& a)
{
    Args b;
    b.opts["line"] = "ai";
    for (const char* k : {"ai-layers", "ai-width", "ai-heads", "ai-bits"})
        if (a.has(k)) b.opts[k] = a.get(k);
    b.opts["key"] = a.get("key", "sieve");
    ItemLine l;
    l.ai = ai_of_options(b);
    // Every option written, so that a change to the defaults can never quietly make another model.
    b.opts["ai-layers"] = std::to_string(l.ai->shape().layers);
    b.opts["ai-width"] = std::to_string(l.ai->shape().width);
    b.opts["ai-heads"] = std::to_string(l.ai->shape().heads);
    b.opts["ai-bits"] = std::to_string(l.ai->shape().bits);
    l.spec = l.ai->id() + " " + options_text(b);
    l.width = place_width(l.ai->size());
    return l;
}

Args args_of_options(const std::string& options)
{
    Args b;
    std::istringstream in(options);
    for (std::string kv; in >> kv;)
    {
        const size_t eq = kv.find('=');
        if (eq == std::string::npos || eq == 0) throw std::invalid_argument("a line's options are key=value: " + kv);
        b.opts[kv.substr(0, eq)] = kv.substr(eq + 1);
    }
    return b;
}

// v7: a line's options with those its id already gives left out: each option the id spells out
// (the AI line's shape and key, the pages' alphabet and length) whose absence makes the same space
// id, and the guided model, which the maker sets to none itself. Reading it back makes the same line
// or refuses it (its id is checked), so nothing is lost. Any other option is kept, even where the id
// would not change without it: it may change the file the line writes without changing its places.
std::string trimmed_spec(const ItemLine& l)
{
    const size_t sp = l.spec.find(' ');
    const std::string id = l.spec.substr(0, sp);
    Args all = args_of_options(l.spec.substr(sp + 1));
    auto id_of = [&](Args b) {
        if (b.get("line") == "ai") return ai_of_options(b)->id();
        if (!b.has("model")) b.opts["model"] = "none";
        return make_line(b).space.id();
    };
    Args kept = all;
    for (const auto& [k, v] : all.opts)
    {
        static const char* const spelt_out[] = {"ai-layers", "ai-width", "ai-heads", "ai-bits", "key", "alphabet", "length", "model"};
        if (std::find(std::begin(spelt_out), std::end(spelt_out), k) == std::end(spelt_out)) continue;
        Args without = kept;
        without.opts.erase(k);
        if (k == "model" && v == "none") kept = without;
        else
        {
            try
            {
                if (id_of(without) == id) kept = without;
            }
            catch (const std::exception&)
            {
            }
        }
    }
    if (id_of(kept) != id) throw std::logic_error("a line's options trimmed to another line");
    return id + " " + options_text(kept);
}

std::vector<uint64_t> ai_segments(const AiSpace& ai)
{
    std::vector<uint64_t> seg;
    for (const AiSpace::Tensor& t : ai.tensors()) seg.push_back(uint64_t(t.rows) * t.cols);
    return seg;
}

} // namespace

std::vector<uint8_t> ai_place_code(const AiSpace& ai, const std::string& place)
{
    const AiSpace::Digits d = ai.digits_at(ai.parse(place), AddressMode::Positional);
    const BigUint z = digit_histogram_number(d, ai_segments(ai), 1u << ai.shape().bits);
    return z.to_bytes((z.bit_length() + 7) / 8);
}

std::vector<uint8_t> ai_digits_of_code(const AiSpace& ai, std::span<const uint8_t> code)
{
    if (!code.empty() && code[0] == 0) throw std::runtime_error("manifest: a model's code begins with a zero byte (not the fewest bytes)");
    return digit_histogram_digits(BigUint::from_bytes(code), ai_segments(ai), 1u << ai.shape().bits);
}

uint32_t place_width(const BigUint& size)
{
    if (size.is_zero()) return 0;
    BigUint largest = size;
    largest -= BigUint(1);
    return uint32_t((largest.bit_length() + 7) / 8);
}

std::vector<uint8_t> place_bytes(const std::string& hex, uint32_t width)
{
    const std::string h = BigUint::from_hex(hex).to_hex(size_t(width) * 2);
    if (h.size() != size_t(width) * 2) throw std::runtime_error("a place is wider than its line's places");
    std::vector<uint8_t> out(width);
    auto digit = [](char c) { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; };
    for (uint32_t i = 0; i < width; ++i) out[i] = uint8_t(digit(h[2 * i]) * 16 + digit(h[2 * i + 1]));
    return out;
}

std::string place_hex(std::span<const uint8_t> bytes)
{
    static const char k[] = "0123456789abcdef";
    std::string h;
    for (const uint8_t b : bytes) h += {k[b >> 4], k[b & 15]};
    return h.empty() ? std::string("0") : h;
}

std::vector<ItemLine> item_lines(const Args& a, bool with_ai)
{
    std::vector<ItemLine> out;
    {
        Args b;
        b.opts["line"] = "text";
        b.opts["alphabet"] = a.get("alphabet", "lower27");
        b.opts["length"] = a.get("page-length", "32");
        out.push_back(item_line(b));
    }
    for (const char* kind : {"image", "video", "audio"})
    {
        Args b;
        b.opts["line"] = kind;
        out.push_back(item_line(b));
    }
    if (with_ai) out.push_back(ai_item_line(a));
    return out;
}

Line line_of_options(const std::string& options) { return make_line(args_of_options(options)); }

ItemFinder item_finder(const std::vector<ItemLine>& lines)
{
    return [&lines](const std::vector<uint8_t>& bytes) -> std::optional<std::pair<size_t, std::string>> {
        for (size_t k = 0; k < lines.size(); ++k)
        {
            const ItemLine& l = lines[k];
            if (l.ai)
            {
                // A model of the line: its file read back to digits, and taken only when the line
                // writes exactly that file for them (the same bytes, header and all).
                const std::string_view file(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                const auto digits = l.ai->digits_of_safetensors(file);
                if (digits && l.ai->safetensors_of(*digits) == file)
                    return std::make_pair(k, l.ai->hex_of(l.ai->index_of(*digits, AddressMode::Positional)));
                continue;
            }
            if (const auto unit = item_of(*l.line, bytes)) return std::make_pair(k, l.line->space.address_of(*unit, AddressMode::Positional));
        }
        return std::nullopt;
    };
}

void add_packed_items(Manifest& m, const std::filesystem::path& root, const Args& a, PackReport* report, int version, bool item_hashes,
                      double rope_seconds, uint32_t rope_newest)
{
    if (version < 5 || version > 8) throw std::invalid_argument("Sieve instructions with items are v5, v6, v7 or v8");
    const std::vector<ItemLine> lines = item_lines(a, version >= 6);
    m.minimal = version >= 6;
    m.compact = version >= 7;
    m.lines.clear();
    m.line_widths.clear();
    m.line_codings.clear();
    for (const ItemLine& l : lines)
    {
        m.lines.push_back(m.compact ? trimmed_spec(l) : l.spec);
        m.line_widths.push_back(l.width);
    }
    const ItemFinder finder = item_finder(lines);
    const PlaceCoder coder = [&lines](size_t k, const std::string& place) -> std::optional<std::vector<uint8_t>> {
        if (!lines.at(k).ai) return std::nullopt;
        return ai_place_code(*lines[k].ai, place);
    };
    add_packed(m, root, report, &finder, item_hashes, m.compact ? &coder : nullptr, version >= 8 ? rope_seconds : -1, rope_newest);
}

void use_item_maker()
{
    // A line made from a listing's options: one of the CLI's lines, or the AI line.
    struct MadeLine
    {
        std::shared_ptr<const Line> line;
        std::shared_ptr<const AiSpace> ai;
    };
    auto made = std::make_shared<std::map<std::string, std::shared_ptr<const MadeLine>>>();
    auto mu = std::make_shared<std::mutex>();
    set_item_maker([made, mu](const std::string& spec, const std::string& address) {
        std::shared_ptr<const MadeLine> line;
        {
            std::lock_guard<std::mutex> lock(*mu);
            auto& slot = (*made)[spec];
            if (!slot)
            {
                const size_t sp = spec.find(' ');
                if (sp == std::string::npos) throw std::runtime_error("manifest: a line with no options: " + spec);
                Args b = args_of_options(spec.substr(sp + 1));
                auto made_line = std::make_shared<MadeLine>();
                if (b.get("line") == "ai") made_line->ai = ai_of_options(b);
                else
                {
                    if (!b.has("model")) b.opts["model"] = "none"; // v7 leaves it out: no part of a positional place
                    made_line->line = std::make_shared<const Line>(make_line(b));
                }
                const std::string id = made_line->ai ? made_line->ai->id() : made_line->line->space.id();
                if (id != spec.substr(0, sp))
                    throw std::runtime_error("manifest: its line " + spec.substr(0, sp) + " is made here as " + id +
                                             " (this Sieve makes that line otherwise): its items are refused");
                slot = std::move(made_line);
            }
            line = slot;
        }
        if (!address.empty() && address[0] == 'h')
        {
            // v7: a coded place (digit-histogram-v1), its bytes in hex after the h.
            if (!line->ai) throw std::runtime_error("manifest: a coded place on a line that has no coding: " + spec);
            const std::string h = address.substr(1);
            std::vector<uint8_t> code;
            for (size_t i = 0; i + 1 < h.size(); i += 2) code.push_back(uint8_t(std::stoul(h.substr(i, 2), nullptr, 16)));
            const std::string file = line->ai->safetensors_of(ai_digits_of_code(*line->ai, code));
            return std::vector<uint8_t>(file.begin(), file.end());
        }
        if (line->ai)
        {
            const std::string file = line->ai->safetensors_of(line->ai->digits_at(line->ai->parse(address), AddressMode::Positional));
            return std::vector<uint8_t>(file.begin(), file.end());
        }
        return unit_file(*line->line, line->line->space.unit_at(address, AddressMode::Positional), 1);
    });
}

} // namespace sieve::cli
