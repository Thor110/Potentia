// Sieve CLI -- files weighed (weigh.hpp).
#include "cli/weigh.hpp"

#include "cli/locate.hpp"
#include "sieve/corridor.hpp"
#include "sieve/filekind.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// A way's group: what a reader must be told for it, paid once however many files use it. The base
// (the binary line positional) is group -1, which every reader has.
struct Group
{
    int line;          // index into the lines weighed (0 is binary)
    std::string name;  // "binary, compact", "pages", "pages, compact", "pages, tailored"
    double cost = 0;   // its filters written down (0 for a line's positional way)
};

struct Candidate
{
    int group;
    double bits;
    std::string written;
};

std::string written_route(const BigUint& number, const BigUint& count)
{
    if (count.bit_length() > (1u << 16)) return number.is_zero() ? "0" : number.to_hex();
    return shortest_path(number, count, 20).written;
}

} // namespace

Weighing weigh_files(const fs::path& root, const std::vector<std::string>& files, const WeighLines& given, bool tailor, TailorProgress* progress)
{
    Weighing out;
    // The lines, binary first: their names, shapes and stacks.
    struct Weighed
    {
        std::string name;
        const Line* line = nullptr;
        LineFilters filters;
        std::optional<FilterStack> stack;
        FilterLine fl;
        std::string shape; // what a reader must be told to read a place on it: its symbols and length
    };
    std::vector<Weighed> lines(1);
    lines[0].name = "binary";
    for (const auto& [name, l] : std::vector<std::pair<std::string, const Line*>>{{"pages", given.pages}, {"image", given.image}, {"audio", given.audio}, {"video", given.video}})
    {
        if (!l) continue;
        Weighed w;
        w.name = name;
        w.line = l;
        w.filters = given.filters.of(l->kind);
        w.fl = filter_line(*l);
        w.shape = l->space.symbols_id() + "/L" + std::to_string(l->space.unit_length());
        try
        {
            if (!w.filters.enabled.empty()) w.stack = build_stack(*l, w.filters);
        }
        catch (const std::exception&)
        {
        }
        lines.push_back(std::move(w));
    }
    // The binary line's survivors, where its filters rank: as long as the longest file (or as set).
    uint64_t longest = 0;
    for (const auto& f : files)
    {
        std::error_code ec;
        const auto n = fs::file_size(root.empty() ? from_u8(f) : root / from_u8(f), ec);
        if (!ec) longest = std::max<uint64_t>(longest, n);
    }
    const uint64_t binary_bytes = std::max<uint64_t>(1, given.binary_bytes ? given.binary_bytes : longest);
    std::optional<BinarySpace> bspace;
    std::optional<BinarySieve> bsieve;
    if (!given.filters.binary.enabled.empty())
    {
        try
        {
            bspace.emplace(binary_bytes, "sieve");
            const BinaryItems items = binary_items(given.pages, given.image, given.video, given.audio, nullptr);
            bsieve.emplace(build_binary_sieve(*bspace, given.filters.binary, &items));
            if (!bsieve->can_rank()) bsieve.reset();
        }
        catch (const std::exception&)
        {
            bsieve.reset();
        }
    }
    lines[0].shape = bspace ? bspace->shape() : std::string("binary");

    std::vector<Group> groups;
    auto group_of = [&](int line, const std::string& name, double cost) {
        for (size_t i = 0; i < groups.size(); ++i)
            if (groups[i].name == name) return int(i);
        groups.push_back({line, name, cost});
        return int(groups.size() - 1);
    };
    std::vector<std::vector<Candidate>> candidates;
    if (progress) progress->total = int(files.size());

    // Each file once: its own address, and every other way it can be named.
    for (const auto& f : files)
    {
        if (progress)
        {
            if (progress->cancel) break;
            std::lock_guard<std::mutex> lock(progress->mx);
            progress->current = f;
        }
        FileWeight fw;
        fw.path = f;
        std::vector<Candidate> cand;
        try
        {
            const std::vector<uint8_t> bytes = read_file_bytes(root.empty() ? from_u8(f) : root / from_u8(f));
            fw.bytes = bytes.size();
            const BigUint own = binary_address(bytes);
            fw.base_bits = own.is_zero() ? 0.0 : 4.0 * double(own.to_hex().size());
            cand.push_back({-1, fw.base_bits, ""});
            if (bsieve && bytes.size() <= binary_bytes && bsieve->first_failure_of(bytes).empty())
            {
                const BigUint k = bsieve->index_of(bytes, AddressMode::Positional);
                cand.push_back({group_of(0, "binary, compact", description_bits(binary_filter_line(binary_bytes), given.filters.binary)),
                                route_bits(k, bsieve->count()), written_route(k, bsieve->count())});
            }
            for (size_t li = 1; li < lines.size(); ++li)
            {
                const Weighed& w = lines[li];
                const auto unit = item_of(*w.line, bytes);
                if (!unit) continue;
                fw.line = int(w.line->kind);
                fw.unit = *unit;
                const BigUint size = w.line->space.size(), index = BigUint::from_digits(*unit, w.line->space.base());
                cand.push_back({group_of(int(li), w.name, 0), route_bits(index, size), written_route(index, size)});
                if (w.stack && w.stack->passes(*unit))
                    if (const Ranker* r = w.stack->ranker())
                    {
                        const BigUint k = r->rank(*unit);
                        cand.push_back({group_of(int(li), w.name + ", compact", description_bits(w.fl, w.filters)), route_bits(k, r->count()),
                                        written_route(k, r->count())});
                    }
                break; // a file is an item of one line
            }
        }
        catch (const std::exception& e)
        {
            fw.error = e.what();
            cand.clear();
        }
        out.files.push_back(std::move(fw));
        candidates.push_back(std::move(cand));
        if (progress) ++progress->done;
    }

    // With tailoring: each line's items are its anchors, its filters tailored to them, paid for.
    std::map<int, TailorResult> tailored;
    if (tailor && !(progress && progress->cancel))
        for (size_t li = 1; li < lines.size(); ++li)
        {
            const Weighed& w = lines[li];
            std::vector<std::vector<uint32_t>> anchors;
            std::vector<size_t> who;
            for (size_t i = 0; i < out.files.size(); ++i)
                if (out.files[i].line == int(w.line->kind))
                {
                    anchors.push_back(out.files[i].unit);
                    who.push_back(i);
                }
            if (anchors.empty()) continue;
            TailorOptions opt;
            opt.count_description = true;
            TailorResult r = tailor_filters(w.fl, anchors, w.filters, nullptr, opt);
            if (!r.finished || r.filters.enabled.empty()) continue;
            try
            {
                const FilterStack st = build_stack(*w.line, r.filters);
                const Ranker* rk = st.ranker();
                if (!rk) continue;
                const int g = group_of(int(li), w.name + ", tailored", r.description_bits);
                for (size_t a = 0; a < anchors.size(); ++a)
                {
                    const BigUint k = rk->rank(anchors[a]);
                    candidates[who[a]].push_back({g, route_bits(k, rk->count()), written_route(k, rk->count())});
                }
                tailored.emplace(int(li), std::move(r));
            }
            catch (const std::exception&)
            {
            }
        }

    // Which ways to allow: every group at first, then, while it lowers the total, the group whose
    // loss lowers it most taken away (a line used by too few files to pay for its shape, filters too
    // costly to write down for what they save).
    std::vector<bool> on(groups.size(), true);
    auto choose = [&](const std::vector<bool>& allowed, std::vector<size_t>& pick) {
        double total = 0;
        std::vector<bool> used_group(groups.size(), false), used_line(lines.size(), false);
        pick.assign(candidates.size(), 0);
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            const auto& c = candidates[i];
            if (c.empty()) continue;
            size_t best = 0;
            for (size_t j = 1; j < c.size(); ++j)
                if (allowed[size_t(c[j].group)] && c[j].bits < c[best].bits) best = j;
            pick[i] = best;
            total += c[best].bits;
            if (c[best].group >= 0)
            {
                used_group[size_t(c[best].group)] = true;
                used_line[size_t(groups[size_t(c[best].group)].line)] = true;
            }
        }
        for (size_t g = 0; g < groups.size(); ++g)
            if (used_group[g]) total += groups[g].cost;
        for (size_t l = 0; l < lines.size(); ++l)
            if (used_line[l]) total += 8.0 * double(lines[l].shape.size());
        return total;
    };
    std::vector<size_t> pick;
    double total = choose(on, pick);
    for (;;)
    {
        double best_total = total;
        int best_off = -1;
        for (size_t g = 0; g < groups.size(); ++g)
        {
            if (!on[g]) continue;
            std::vector<bool> trial = on;
            trial[g] = false;
            std::vector<size_t> p;
            const double t = choose(trial, p);
            if (t < best_total - 1e-9)
            {
                best_total = t;
                best_off = int(g);
            }
        }
        if (best_off < 0) break;
        on[size_t(best_off)] = false;
        total = choose(on, pick);
    }

    // The result: each file's ways (the base first) and its best, and what the lines share.
    std::vector<LineWeight> lw(lines.size());
    for (size_t l = 0; l < lines.size(); ++l)
    {
        lw[l].name = lines[l].name;
        lw[l].shape = lines[l].shape;
        if (auto it = tailored.find(int(l)); it != tailored.end()) lw[l].tailored = it->second;
    }
    std::vector<bool> group_used(groups.size(), false);
    for (size_t i = 0; i < out.files.size(); ++i)
    {
        FileWeight& fw = out.files[i];
        for (const Candidate& c : candidates[i]) fw.ways.push_back({c.group < 0 ? "binary" : groups[size_t(c.group)].name, c.bits, c.written});
        if (candidates[i].empty()) continue;
        fw.best = pick[i];
        out.base_bits += fw.base_bits;
        out.best_bits += fw.ways[fw.best].bits;
        const int g = candidates[i][fw.best].group;
        if (g >= 0)
        {
            ++lw[size_t(groups[size_t(g)].line)].files;
            group_used[size_t(g)] = true;
        }
    }
    for (size_t g = 0; g < groups.size(); ++g)
        if (group_used[g]) lw[size_t(groups[g].line)].description_bits += groups[g].cost;
    for (size_t l = 0; l < lw.size(); ++l)
    {
        if (lw[l].files == 0) continue;
        lw[l].shape_bits = 8.0 * double(lw[l].shape.size());
        out.shared_bits += lw[l].shape_bits + lw[l].description_bits;
    }
    for (auto& l : lw)
        if (l.files > 0 || l.tailored) out.lines.push_back(std::move(l));
    return out;
}

std::string weighing_table(const Weighing& w, size_t max_rows)
{
    std::ostringstream o;
    auto bits = [](double b) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(0) << b;
        return s.str();
    };
    auto pct = [](double part, double whole) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(1) << (whole > 0 ? part / whole * 100.0 : 100.0) << "%";
        return s.str();
    };
    o << "weighing (bits)              own address   best way   how\n";
    size_t shown = 0;
    for (const FileWeight& f : w.files)
    {
        if (max_rows && shown++ >= max_rows)
        {
            o << "  ... and " << (w.files.size() - max_rows) << " more\n";
            break;
        }
        std::string p = f.path.size() > 28 ? "..." + f.path.substr(f.path.size() - 25) : f.path;
        o << "  " << p << std::string(p.size() < 28 ? 28 - p.size() : 1, ' ');
        if (!f.error.empty())
        {
            o << "unreadable: " << f.error << "\n";
            continue;
        }
        const Way& b = f.ways[f.best];
        const std::string own = bits(f.base_bits), best = bits(b.bits);
        o << std::string(own.size() < 12 ? 12 - own.size() : 1, ' ') << own << std::string(best.size() < 11 ? 11 - best.size() : 1, ' ') << best << "   "
          << (f.best == 0 ? std::string("its own address") : b.how + (b.written.empty() ? "" : " " + (b.written.size() > 24 ? b.written.substr(0, 21) + "..." : b.written)))
          << "\n";
    }
    for (const LineWeight& l : w.lines)
    {
        if (l.files)
            o << "  shared: " << l.name << ", " << l.files << " file(s): shape " << bits(l.shape_bits) << " bits"
              << (l.description_bits > 0 ? ", filters " + bits(l.description_bits) + " bits" : std::string()) << "\n";
        if (l.tailored)
            o << "  tailored " << l.name << ": " << l.tailored->filters.enabled.size() << " filter(s), routes " << bits(l.tailored->route_bits)
              << " bits + description " << bits(l.tailored->description_bits) << ", against " << bits(l.tailored->line_bits) << " on the line\n";
    }
    o << "  total: own addresses " << bits(w.base_bits) << " bits; best ways " << bits(w.best_bits) << " + shared " << bits(w.shared_bits) << " = "
      << bits(w.total_bits()) << " bits (" << pct(w.total_bits(), w.base_bits) << " of the own addresses)\n";
    return o.str();
}

} // namespace sieve::cli
