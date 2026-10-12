// Sieve CLI -- files carried as items of Sieve's lines (see items.hpp).
#include "cli/items.hpp"

#include "cli/filter_config.hpp"
#include "cli/locate.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>

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
    ItemLine l{"", make_line(b)};
    l.spec = l.line.space.id() + " " + options_text(b);
    return l;
}

} // namespace

std::vector<ItemLine> item_lines(const Args& a)
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
    return out;
}

Line line_of_options(const std::string& options)
{
    Args b;
    std::istringstream in(options);
    for (std::string kv; in >> kv;)
    {
        const size_t eq = kv.find('=');
        if (eq == std::string::npos || eq == 0) throw std::invalid_argument("a line's options are key=value: " + kv);
        b.opts[kv.substr(0, eq)] = kv.substr(eq + 1);
    }
    return make_line(b);
}

ItemFinder item_finder(const std::vector<ItemLine>& lines)
{
    return [&lines](const std::vector<uint8_t>& bytes) -> std::optional<std::pair<size_t, std::string>> {
        for (size_t k = 0; k < lines.size(); ++k)
            if (const auto unit = item_of(lines[k].line, bytes))
                return std::make_pair(k, lines[k].line.space.address_of(*unit, AddressMode::Positional));
        return std::nullopt;
    };
}

void use_item_maker()
{
    auto made = std::make_shared<std::map<std::string, std::shared_ptr<const Line>>>();
    auto mu = std::make_shared<std::mutex>();
    set_item_maker([made, mu](const std::string& spec, const std::string& address) {
        std::shared_ptr<const Line> line;
        {
            std::lock_guard<std::mutex> lock(*mu);
            auto& slot = (*made)[spec];
            if (!slot)
            {
                const size_t sp = spec.find(' ');
                if (sp == std::string::npos) throw std::runtime_error("manifest: a line with no options: " + spec);
                auto l = std::make_shared<const Line>(line_of_options(spec.substr(sp + 1)));
                if (l->space.id() != spec.substr(0, sp))
                    throw std::runtime_error("manifest: its line " + spec.substr(0, sp) + " is made here as " + l->space.id() +
                                             " (this Sieve makes that line otherwise): its items are refused");
                slot = std::move(l);
            }
            line = slot;
        }
        return unit_file(*line, line->space.unit_at(address, AddressMode::Positional), 1);
    });
}

} // namespace sieve::cli
