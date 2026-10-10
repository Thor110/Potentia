// Sieve CLI — `sieve tensors` (see tensors.hpp).
#include "cli/tensors.hpp"

#include "sieve/json.hpp"
#include "sieve/safetensors.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

namespace {

namespace st = sieve::safetensors;

std::filesystem::path path_of(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

std::string read_text(const std::string& file)
{
    std::ifstream in(path_of(file), std::ios::binary);
    if (!in) throw std::invalid_argument("cannot open " + file);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

void write_bytes(const std::string& file, const std::string& bytes)
{
    std::ofstream out(path_of(file), std::ios::binary);
    if (!out || !out.write(bytes.data(), std::streamsize(bytes.size()))) throw std::invalid_argument("cannot write " + file);
}

std::string shape_text(const std::vector<uint64_t>& shape)
{
    std::string s;
    for (size_t i = 0; i < shape.size(); ++i) s += (i ? "x" : "") + std::to_string(shape[i]);
    return s.empty() ? "scalar" : s;
}

// A tensor's kind: its name with the layer number taken out ("model.layers.*.mlp.up_proj.weight").
std::string kind_of(const std::string& name)
{
    const std::string p = "model.layers.";
    if (name.rfind(p, 0) != 0) return name;
    size_t i = p.size();
    while (i < name.size() && name[i] >= '0' && name[i] <= '9') ++i;
    return i > p.size() && i < name.size() && name[i] == '.' ? p + "*" + name.substr(i) : name;
}

std::string fixed(double v, int places)
{
    char b[64];
    std::snprintf(b, sizeof b, "%.*f", places, v);
    return b;
}

std::string mb(uint64_t bytes) { return fixed(double(bytes) / 1e6, 1) + " MB"; }

// The first way two tensor lists differ, by name ("" when they hold the same tensors).
std::string difference(std::vector<st::Tensor> a, std::vector<st::Tensor> b)
{
    auto by_name = [](const st::Tensor& x, const st::Tensor& y) { return x.name < y.name; };
    std::sort(a.begin(), a.end(), by_name);
    std::sort(b.begin(), b.end(), by_name);
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size())
    {
        if (a[i].name < b[j].name) return "the file has " + a[i].name + ", which config.json does not";
        if (b[j].name < a[i].name) return "config.json has " + b[j].name + ", which the file does not";
        if (a[i].dtype != b[j].dtype) return a[i].name + " is " + a[i].dtype + " in the file, " + b[j].dtype + " by config.json";
        if (a[i].shape != b[j].shape) return a[i].name + " is " + shape_text(a[i].shape) + " in the file, " + shape_text(b[j].shape) + " by config.json";
        ++i, ++j;
    }
    if (i < a.size()) return "the file has " + a[i].name + ", which config.json does not";
    if (j < b.size()) return "config.json has " + b[j].name + ", which the file does not";
    return "";
}

} // namespace

int cmd_tensors(const Args& a)
{
    if (a.positional.size() > 1) throw std::invalid_argument("give one model file: sieve tensors FILE.safetensors");
    std::optional<json::Value> config;
    if (a.has("config")) config = json::parse(read_text(a.get("config")));

    // No file: the start rebuilt from config.json alone.
    if (a.positional.empty())
    {
        if (!config) throw std::invalid_argument("give a model file, or --config config.json (with --start-out FILE) to rebuild a start");
        std::vector<st::Tensor> laid;
        const std::string start = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, st::llama_tensors(*config), &laid);
        uint64_t data = 0;
        for (const st::Tensor& t : laid) data = std::max(data, t.end);
        std::cout << "config       " << st::llama_summary(*config) << "\n";
        std::cout << "start        " << start.size() << " bytes (" << laid.size() << " tensors, " << st::kLayoutVersion << ", metadata format=pt)\n";
        std::cout << "weights      " << data << " bytes to follow it\n";
        std::cout << "sha256       " << Sha256::hex(Sha256::hash(start)) << " (of the start)\n";
        if (a.has("start-out"))
        {
            write_bytes(a.get("start-out"), start);
            std::cout << "wrote        " << a.get("start-out") << "\n";
        }
        return 0;
    }

    const std::string file = a.positional[0];
    std::ifstream in(path_of(file), std::ios::binary);
    if (!in) throw std::invalid_argument("cannot open " + file);
    const st::Header h = st::read_header(in);
    in.seekg(0, std::ios::end);
    const uint64_t size = uint64_t(in.tellg());
    if (size != h.start_bytes() + h.data_bytes())
        throw std::invalid_argument(file + " is " + std::to_string(size) + " bytes, but its header says " + std::to_string(h.start_bytes() + h.data_bytes()));

    // One pass through the file, in order: its hash, and with --stats each tensor's statistics.
    const bool stats = a.has("stats") || a.has("tsv");
    std::vector<const st::Tensor*> order;
    for (const st::Tensor& t : h.tensors) order.push_back(&t);
    std::sort(order.begin(), order.end(), [](const st::Tensor* x, const st::Tensor* y) { return x->begin < y->begin; });
    std::map<std::string, st::TensorStats> got;
    Sha256 sha;
    {
        in.clear();
        in.seekg(0);
        std::vector<char> buf(size_t(4) << 20);
        uint64_t left = h.start_bytes();
        while (left)
        {
            const size_t n = size_t(std::min<uint64_t>(left, buf.size()));
            in.read(buf.data(), std::streamsize(n));
            sha.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf.data()), n));
            left -= n;
        }
        for (const st::Tensor* t : order)
        {
            std::optional<st::StatsBuilder> b;
            if (stats && st::has_stats(t->dtype)) b.emplace(t->dtype);
            left = t->bytes();
            while (left)
            {
                const size_t n = size_t(std::min<uint64_t>(left, buf.size()));
                if (!in.read(buf.data(), std::streamsize(n))) throw std::invalid_argument(file + " ends early, in " + t->name);
                const std::span<const uint8_t> part(reinterpret_cast<const uint8_t*>(buf.data()), n);
                sha.update(part);
                if (b) b->add(part);
                left -= n;
            }
            if (b) got[t->name] = b->finish();
        }
    }

    std::map<std::string, size_t> types;
    uint64_t values = 0;
    for (const st::Tensor& t : h.tensors)
    {
        ++types[t.dtype];
        values += t.elements();
    }
    std::string type_list;
    for (const auto& [t, n] : types) type_list += (type_list.empty() ? "" : ", ") + t + " " + std::to_string(n);
    std::string meta;
    if (h.metadata)
        for (const auto& [k, v] : *h.metadata) meta += (meta.empty() ? "" : ", ") + k + "=" + v;

    if (a.has("tsv"))
    {
        // One line a tensor, in the file's order: what reference/sieve_ref.py tensors --tsv prints.
        std::cout << "tensor\tdtype\tshape\tvalues\tdistinct\th_value\th_high\th_low\tmean\tsd\n";
        for (const st::Tensor* t : order)
        {
            std::cout << t->name << "\t" << t->dtype << "\t" << shape_text(t->shape) << "\t" << t->elements();
            if (const auto it = got.find(t->name); it != got.end())
            {
                const st::TensorStats& s = it->second;
                char b[256];
                std::snprintf(b, sizeof b, "\t%u\t%.6f\t%.6f\t%.6f\t%.9e\t%.9e", s.distinct, s.h_value, s.h_high, s.h_low, s.mean, s.sd);
                std::cout << b;
            }
            else std::cout << "\t-\t-\t-\t-\t-\t-";
            std::cout << "\n";
        }
        return 0;
    }

    std::cout << "file         " << file << "\n";
    std::cout << "bytes        " << size << "\n";
    std::cout << "sha256       " << Sha256::hex(sha.finish()) << "\n";
    std::cout << "start        " << h.start_bytes() << " bytes: the 8-byte length and " << h.json_bytes << " of JSON (" << h.tensors.size() << " tensors"
              << (h.metadata ? ", metadata " + (meta.empty() ? std::string("empty") : meta) : std::string(", no metadata")) << ")\n";
    std::cout << "weights      " << h.data_bytes() << " bytes: " << values << " values (" << type_list << ")\n";
    const bool canonical = st::is_canonical(h);
    std::cout << "layout       " << (canonical ? std::string(st::kLayoutVersion) + ": the start is rebuilt from the tensors' names, types and shapes"
                                               : std::string("not ") + st::kLayoutVersion + ": the start is kept as it is")
              << "\n";
    if (config)
    {
        std::cout << "config       " << st::llama_summary(*config) << "\n";
        const std::string diff = difference(h.tensors, st::llama_tensors(*config));
        if (!diff.empty()) std::cout << "             not this file's tensors: " << diff << "\n";
        else if (canonical) std::cout << "             the file's tensors, name for name, type and shape: the start is rebuilt from config.json\n";
        else std::cout << "             the file's tensors, name for name, type and shape (but the start is not the layout's)\n";
    }
    if (a.has("start-out"))
    {
        const std::string start = config ? st::layout_start(h.metadata, st::llama_tensors(*config)) : st::layout_start(h.metadata, h.tensors);
        write_bytes(a.get("start-out"), start);
        std::cout << "wrote        " << a.get("start-out") << " (the start, rebuilt from " << (config ? "config.json" : "the tensors") << ")\n";
    }
    if (!stats) return 0;

    // By kind: each layer's tensor of one name together.
    struct Kind
    {
        size_t tensors = 0;
        uint64_t bytes = 0, values = 0;
        double bits = 0, high = 0, low = 0;
        bool measured = true;
    };
    std::vector<std::string> kinds;
    std::map<std::string, Kind> by;
    double bits = 0;
    uint64_t measured_bytes = 0;
    for (const st::Tensor* t : order)
    {
        const std::string k = kind_of(t->name);
        if (!by.count(k)) kinds.push_back(k);
        Kind& g = by[k];
        ++g.tensors;
        g.bytes += t->bytes();
        g.values += t->elements();
        if (const auto it = got.find(t->name); it != got.end())
        {
            const double n = double(it->second.values);
            g.bits += it->second.h_value * n;
            g.high += it->second.h_high * n;
            g.low += it->second.h_low * n;
            bits += it->second.h_value * n;
            measured_bytes += t->bytes();
        }
        else g.measured = false;
    }
    std::stable_sort(kinds.begin(), kinds.end(), [&](const std::string& x, const std::string& y) { return by[x].bytes > by[y].bytes; });
    size_t w = 4;
    for (const std::string& k : kinds) w = std::max(w, k.size());
    std::cout << "\n" << std::string("kind") + std::string(w - 4 + 2, ' ') << "tensors        bytes  bits/value  high byte  low byte\n";
    for (const std::string& k : kinds)
    {
        const Kind& g = by[k];
        char b[160];
        if (g.measured && g.values)
            std::snprintf(b, sizeof b, "%7zu %12llu  %10.3f  %9.3f  %8.3f", g.tensors, (unsigned long long)g.bytes, g.bits / double(g.values),
                          g.high / double(g.values), g.low / double(g.values));
        else std::snprintf(b, sizeof b, "%7zu %12llu  %10s  %9s  %8s", g.tensors, (unsigned long long)g.bytes, "-", "-", "-");
        std::cout << k << std::string(w - k.size() + 2, ' ') << b << "\n";
    }
    const uint64_t coded = uint64_t(std::ceil(bits / 8));
    std::cout << "\ncoded by frequency  " << coded << " bytes (" << mb(coded) << ", " << fixed(100.0 * double(coded) / double(std::max<uint64_t>(measured_bytes, 1)), 1)
              << "% of the " << mb(measured_bytes) << " measured): each tensor's values coded by how often they occur, its table of counts not counted\n";
    if (measured_bytes != h.data_bytes()) std::cout << "                    (statistics are for BF16 and F16 tensors; the others are left out)\n";
    return 0;
}

} // namespace sieve::cli
