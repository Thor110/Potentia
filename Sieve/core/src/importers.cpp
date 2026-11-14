// Sieve — importers (see importers.hpp).
#include "sieve/importers.hpp"

#include "sieve/json.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

const std::vector<ImportFormat>& import_formats()
{
    static const std::vector<ImportFormat> f = {
        {"obj", "Wavefront OBJ", "obj", "every v and f line: vertices and faces (polygons as fans)",
         "texture coordinates, normals, materials, groups"},
        {"gltf", "glTF 2.0 (JSON)", "gltf",
         "every triangle mesh of the default scene, placed as its nodes place it; buffers embedded or beside it; "
         "the first skin's joints as a skeleton (sieve-character-v3); animations sampled as turns (sieve-animation-v1)",
         "skins after the first; joints' movement and growth in animations; materials, textures; points and lines; Draco and meshopt compression"},
        {"glb", "glTF 2.0 (binary)", "glb", "as glTF: the JSON and its buffer in one file",
         "as glTF"},
    };
    return f;
}

bool import_uri_is_local(std::string_view uri)
{
    if (uri.empty() || uri.find(':') != std::string_view::npos) return false; // a scheme, or a drive
    if (uri.front() == '/' || uri.front() == '\\') return false;
    if (uri.find('\0') != std::string_view::npos) return false;
    size_t i = 0;
    while (i <= uri.size())
    {
        size_t j = uri.find_first_of("/\\", i);
        if (j == std::string_view::npos) j = uri.size();
        if (uri.substr(i, j - i) == "..") return false;
        i = j + 1;
    }
    return true;
}

std::string import_uri_decoded(std::string_view uri)
{
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string out;
    for (size_t i = 0; i < uri.size(); ++i)
    {
        if (uri[i] == '%' && i + 2 < uri.size() && hex(uri[i + 1]) >= 0 && hex(uri[i + 2]) >= 0)
        {
            out.push_back(char(hex(uri[i + 1]) * 16 + hex(uri[i + 2])));
            i += 2;
        }
        else out.push_back(uri[i]);
    }
    return out;
}

namespace {

[[noreturn]] void refuse(const std::string& why) { throw std::invalid_argument("glTF: " + why); }

std::string base64_decode(std::string_view s)
{
    auto val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    while (!s.empty() && s.back() == '=') s.remove_suffix(1);
    std::string out;
    out.reserve(s.size() * 3 / 4);
    uint32_t acc = 0;
    int bits = 0;
    for (const char c : s)
    {
        const int v = val(c);
        if (v < 0) refuse("a data: URI that is not base64");
        acc = acc << 6 | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(char((acc >> bits) & 0xFF));
        }
    }
    return out;
}

uint32_t le32(const char* p)
{
    return uint32_t(uint8_t(p[0])) | uint32_t(uint8_t(p[1])) << 8 | uint32_t(uint8_t(p[2])) << 16 | uint32_t(uint8_t(p[3])) << 24;
}

// A member that is a non-negative integer, or `def` if it is not there.
uint64_t uint_of(const json::Value& o, std::string_view name, uint64_t def)
{
    const json::Value* v = o.find(name);
    if (!v) return def;
    if (!v->is_number()) refuse("\"" + std::string(name) + "\" is not a number");
    try
    {
        return v->u64();
    }
    catch (const std::exception&)
    {
        refuse("\"" + std::string(name) + "\" is not a non-negative integer");
    }
}

const std::vector<json::Value>& array_of(const json::Value& root, std::string_view name)
{
    static const std::vector<json::Value> kNone;
    const json::Value* v = root.find(name);
    if (!v) return kNone;
    if (!v->is_array()) refuse("\"" + std::string(name) + "\" is not an array");
    return v->array();
}

using Mat = std::array<double, 16>; // column-major, as glTF writes it

Mat identity()
{
    Mat m{};
    m[0] = m[5] = m[10] = m[15] = 1;
    return m;
}

Mat mul(const Mat& a, const Mat& b)
{
    Mat r{};
    for (int c = 0; c < 4; ++c)
        for (int rI = 0; rI < 4; ++rI)
        {
            double s = 0;
            for (int k = 0; k < 4; ++k) s += a[size_t(k * 4 + rI)] * b[size_t(c * 4 + k)];
            r[size_t(c * 4 + rI)] = s;
        }
    return r;
}

std::vector<double> numbers(const json::Value& v, size_t n, const char* what)
{
    if (!v.is_array() || v.array().size() != n) refuse(std::string(what) + " is not " + std::to_string(n) + " numbers");
    std::vector<double> out;
    for (const auto& x : v.array())
    {
        if (!x.is_number()) refuse(std::string(what) + " is not " + std::to_string(n) + " numbers");
        out.push_back(x.number());
    }
    return out;
}

Mat trs_matrix(const double t[3], const double q[4], const double s[3]);

Mat local_matrix(const json::Value& node)
{
    if (const json::Value* m = node.find("matrix"))
    {
        const auto v = numbers(*m, 16, "a node's matrix");
        Mat r{};
        std::copy(v.begin(), v.end(), r.begin());
        return r;
    }
    double t[3] = {0, 0, 0}, q[4] = {0, 0, 0, 1}, s[3] = {1, 1, 1};
    if (const json::Value* v = node.find("translation"))
    {
        const auto n = numbers(*v, 3, "a node's translation");
        std::copy(n.begin(), n.end(), t);
    }
    if (const json::Value* v = node.find("rotation"))
    {
        const auto n = numbers(*v, 4, "a node's rotation");
        std::copy(n.begin(), n.end(), q);
    }
    if (const json::Value* v = node.find("scale"))
    {
        const auto n = numbers(*v, 3, "a node's scale");
        std::copy(n.begin(), n.end(), s);
    }
    return trs_matrix(t, q, s);
}

// T * R * S, column-major; the quaternion (x, y, z, w) normalised first.
Mat trs_matrix(const double t[3], const double qin[4], const double s[3])
{
    double n = std::sqrt(qin[0] * qin[0] + qin[1] * qin[1] + qin[2] * qin[2] + qin[3] * qin[3]);
    if (n == 0) n = 1;
    const double q[4] = {qin[0] / n, qin[1] / n, qin[2] / n, (qin[0] == 0 && qin[1] == 0 && qin[2] == 0 && qin[3] == 0) ? 1 : qin[3] / n};
    const double x = q[0], y = q[1], z = q[2], w = q[3];
    Mat r{};
    r[0] = (1 - 2 * (y * y + z * z)) * s[0];
    r[1] = (2 * (x * y + z * w)) * s[0];
    r[2] = (2 * (x * z - y * w)) * s[0];
    r[4] = (2 * (x * y - z * w)) * s[1];
    r[5] = (1 - 2 * (x * x + z * z)) * s[1];
    r[6] = (2 * (y * z + x * w)) * s[1];
    r[8] = (2 * (x * z + y * w)) * s[2];
    r[9] = (2 * (y * z - x * w)) * s[2];
    r[10] = (1 - 2 * (x * x + y * y)) * s[2];
    r[12] = t[0], r[13] = t[1], r[14] = t[2], r[15] = 1;
    return r;
}

constexpr size_t kMaxTriangles = size_t(1) << 26; // 67 million: a roof against a file that asks for more than memory

class Gltf
{
public:
    Gltf(const json::Value& root, std::optional<std::string> bin, const ImportLoader& load, ImportedModel& out)
        : root_(root), out_(out)
    {
        const json::Value* asset = root.find("asset");
        if (!asset || !asset->is_object()) refuse("no \"asset\" (not a glTF file?)");
        const json::Value* version = asset->find("version");
        if (!version || !version->is_string() || version->string().rfind("2.", 0) != 0)
            refuse("not glTF 2.0 (its asset version is " + (version && version->is_string() ? version->string() : std::string("missing")) + ")");
        for (const auto& e : array_of(root, "extensionsRequired"))
        {
            if (!e.is_string()) continue;
            if (e.string() == "KHR_draco_mesh_compression" || e.string() == "EXT_meshopt_compression")
                refuse("compressed with " + e.string() + ", which is not read yet");
        }
        const auto& buffers = array_of(root, "buffers");
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            const json::Value& b = buffers[i];
            const uint64_t length = uint_of(b, "byteLength", 0);
            std::string data;
            if (const json::Value* uri = b.find("uri"))
            {
                if (!uri->is_string()) refuse("buffer " + std::to_string(i) + "'s uri is not a string");
                const std::string& u = uri->string();
                if (u.rfind("data:", 0) == 0)
                {
                    const size_t comma = u.find(',');
                    if (comma == std::string::npos || u.substr(0, comma).find(";base64") == std::string::npos)
                        refuse("buffer " + std::to_string(i) + " is a data: URI that is not base64");
                    data = base64_decode(std::string_view(u).substr(comma + 1));
                }
                else
                {
                    const std::string path = import_uri_decoded(u);
                    if (!import_uri_is_local(path)) refuse("buffer " + std::to_string(i) + " is outside the file's folder (" + u + ")");
                    std::optional<std::string> got = load ? load(path) : std::nullopt;
                    if (!got) refuse("buffer " + std::to_string(i) + " could not be read (" + u + ")");
                    data = std::move(*got);
                }
            }
            else if (i == 0 && bin) data = std::move(*bin); // (not copied again: review 1)
            else refuse("buffer " + std::to_string(i) + " has no data");
            if (data.size() < length) refuse("buffer " + std::to_string(i) + " is shorter than its byteLength");
            buffers_.push_back(std::move(data));
        }
    }

    // An accessor's values, `comps` numbers each, as floats (integers normalised where the
    // accessor says so).
    std::vector<float> floats(uint64_t index, size_t comps)
    {
        const json::Value& a = accessor(index);
        const uint64_t count = uint_of(a, "count", 0);
        if (components(a) != comps) refuse("accessor " + std::to_string(index) + " has " + std::to_string(components(a)) + " numbers per element, not " + std::to_string(comps));
        const uint32_t type = uint32_t(uint_of(a, "componentType", 0));
        const bool normalized = a.find("normalized") && a.at("normalized").is_bool() && a.at("normalized").boolean();
        std::vector<float> out(size_t(count) * comps, 0.0f);
        auto read_into = [&](const char* p, uint32_t t, float* dst, size_t n) {
            for (size_t c = 0; c < n; ++c)
            {
                double v = 0;
                switch (t)
                {
                case 5120: { int8_t x; std::memcpy(&x, p + c, 1); v = normalized ? std::max(x / 127.0, -1.0) : x; break; }
                case 5121: { uint8_t x; std::memcpy(&x, p + c, 1); v = normalized ? x / 255.0 : x; break; }
                case 5122: { int16_t x; std::memcpy(&x, p + 2 * c, 2); v = normalized ? std::max(x / 32767.0, -1.0) : x; break; }
                case 5123: { uint16_t x; std::memcpy(&x, p + 2 * c, 2); v = normalized ? x / 65535.0 : x; break; }
                case 5125: { uint32_t x; std::memcpy(&x, p + 4 * c, 4); v = x; break; }
                case 5126: { float x; std::memcpy(&x, p + 4 * c, 4); v = x; break; }
                default: refuse("accessor " + std::to_string(index) + " has an unknown component type " + std::to_string(t));
                }
                dst[c] = float(v);
            }
        };
        if (a.find("bufferView"))
        {
            const auto [base, stride] = view_of(a, index, count, comps * csize(type));
            for (size_t i = 0; i < count; ++i) read_into(base + i * stride, type, out.data() + i * comps, comps);
        }
        if (const json::Value* sparse = a.find("sparse"))
        {
            const uint64_t n = uint_of(*sparse, "count", 0);
            const json::Value& ix = sparse->at("indices");
            const json::Value& vs = sparse->at("values");
            const uint32_t it = uint32_t(uint_of(ix, "componentType", 0));
            if (it != 5121 && it != 5123 && it != 5125) refuse("accessor " + std::to_string(index) + "'s sparse indices are not unsigned integers");
            const char* ip = span_in(uint_of(ix, "bufferView", UINT64_MAX), uint_of(ix, "byteOffset", 0), n * csize(it));
            const char* vp = span_in(uint_of(vs, "bufferView", UINT64_MAX), uint_of(vs, "byteOffset", 0), n * comps * csize(type));
            for (size_t k = 0; k < n; ++k)
            {
                uint32_t at = 0;
                if (it == 5121) at = uint8_t(ip[k]);
                else if (it == 5123) { uint16_t x; std::memcpy(&x, ip + 2 * k, 2); at = x; }
                else std::memcpy(&at, ip + 4 * k, 4);
                if (at >= count) refuse("accessor " + std::to_string(index) + " has a sparse index past its end");
                read_into(vp + k * comps * csize(type), type, out.data() + size_t(at) * comps, comps);
            }
        }
        return out;
    }

    std::vector<uint32_t> indices(uint64_t index)
    {
        const json::Value& a = accessor(index);
        if (components(a) != 1) refuse("accessor " + std::to_string(index) + " (indices) is not a list of numbers");
        const uint32_t type = uint32_t(uint_of(a, "componentType", 0));
        if (type != 5121 && type != 5123 && type != 5125) refuse("accessor " + std::to_string(index) + " (indices) is not unsigned integers");
        if (a.find("sparse")) refuse("accessor " + std::to_string(index) + " (indices) is sparse, which is not read yet");
        const uint64_t count = uint_of(a, "count", 0);
        std::vector<uint32_t> out(size_t(count), 0);
        if (!a.find("bufferView")) return out;
        const auto [base, stride] = view_of(a, index, count, csize(type));
        for (size_t i = 0; i < count; ++i)
        {
            const char* p = base + i * stride;
            if (type == 5121) out[i] = uint8_t(*p);
            else if (type == 5123) { uint16_t x; std::memcpy(&x, p, 2); out[i] = x; }
            else std::memcpy(&out[i], p, 4);
        }
        return out;
    }

    uint64_t count_of(uint64_t index) { return uint_of(accessor(index), "count", 0); }

    // One instance of a mesh, placed by `m`, added to the output.
    void add_mesh(uint64_t mesh_index, const Mat& m)
    {
        const auto& meshes = array_of(root_, "meshes");
        if (mesh_index >= meshes.size()) refuse("a node names mesh " + std::to_string(mesh_index) + ", which is not there");
        for (const auto& p : array_of(meshes[mesh_index], "primitives"))
        {
            ++out_.primitives;
            const uint64_t mode = uint_of(p, "mode", 4);
            if (mode < 4)
            {
                ++skipped_points_lines_;
                continue;
            }
            if (mode > 6) refuse("a primitive has an unknown mode " + std::to_string(mode));
            const json::Value* attrs = p.find("attributes");
            if (!attrs || !attrs->find("POSITION"))
            {
                ++skipped_no_position_;
                continue;
            }
            const std::vector<float> pos = floats(uint_of(*attrs, "POSITION", 0), 3);
            const size_t n = pos.size() / 3;
            std::vector<uint32_t> idx;
            if (p.find("indices")) idx = indices(uint_of(p, "indices", 0));
            else
            {
                idx.resize(n);
                for (size_t i = 0; i < n; ++i) idx[i] = uint32_t(i);
            }
            const size_t first = out_.mesh.verts.size();
            if (first + n > UINT32_MAX) refuse("more vertices than can be counted");
            for (size_t i = 0; i < n; ++i)
            {
                const double x = pos[i * 3], y = pos[i * 3 + 1], z = pos[i * 3 + 2];
                out_.mesh.verts.push_back({float(m[0] * x + m[4] * y + m[8] * z + m[12]), float(m[1] * x + m[5] * y + m[9] * z + m[13]),
                                           float(m[2] * x + m[6] * y + m[10] * z + m[14])});
            }
            auto tri = [&](uint32_t a, uint32_t b, uint32_t c) {
                if (a >= n || b >= n || c >= n) refuse("a primitive's index is past its vertices");
                if (out_.mesh.faces.size() >= kMaxTriangles) refuse("more than " + std::to_string(kMaxTriangles) + " triangles");
                out_.mesh.faces.push_back({uint32_t(first + a), uint32_t(first + b), uint32_t(first + c)});
            };
            if (mode == 4)
                for (size_t i = 0; i + 2 < idx.size(); i += 3) tri(idx[i], idx[i + 1], idx[i + 2]);
            else if (mode == 5)
                for (size_t i = 0; i + 2 < idx.size(); ++i)
                    i % 2 == 0 ? tri(idx[i], idx[i + 1], idx[i + 2]) : tri(idx[i + 1], idx[i], idx[i + 2]);
            else
                for (size_t i = 1; i + 1 < idx.size(); ++i) tri(idx[0], idx[i], idx[i + 1]);
        }
    }

    void walk(uint64_t node_index, const Mat& parent, size_t depth)
    {
        const auto& nodes = array_of(root_, "nodes");
        if (node_index >= nodes.size()) refuse("a scene names node " + std::to_string(node_index) + ", which is not there");
        if (depth > 1024) refuse("its nodes are nested more than 1,024 deep");
        if (seen_.size() < nodes.size()) seen_.resize(nodes.size(), false);
        if (seen_[node_index]) refuse("node " + std::to_string(node_index) + " is used twice (its nodes are not a tree)");
        seen_[node_index] = true;
        const json::Value& node = nodes[node_index];
        const Mat m = mul(parent, local_matrix(node));
        if (node.find("mesh")) add_mesh(uint_of(node, "mesh", 0), m);
        for (const auto& c : array_of(node, "children"))
        {
            if (!c.is_number()) refuse("a node's child is not a number");
            walk(c.u64(), m, depth + 1);
        }
    }

    size_t skipped_points_lines_ = 0, skipped_no_position_ = 0;

private:
    const json::Value& accessor(uint64_t index)
    {
        const auto& acc = array_of(root_, "accessors");
        if (index >= acc.size()) refuse("accessor " + std::to_string(index) + " is not there");
        return acc[index];
    }
    static size_t components(const json::Value& a)
    {
        const json::Value* t = a.find("type");
        if (!t || !t->is_string()) refuse("an accessor has no type");
        const std::string& s = t->string();
        if (s == "SCALAR") return 1;
        if (s == "VEC2") return 2;
        if (s == "VEC3") return 3;
        if (s == "VEC4" || s == "MAT2") return 4;
        if (s == "MAT3") return 9;
        if (s == "MAT4") return 16;
        refuse("an accessor has an unknown type " + s);
    }
    static size_t csize(uint32_t t)
    {
        switch (t)
        {
        case 5120: case 5121: return 1;
        case 5122: case 5123: return 2;
        case 5125: case 5126: return 4;
        default: refuse("an unknown component type " + std::to_string(t));
        }
    }
    // `bytes` bytes at `offset` into a buffer view, checked to lie inside it and its buffer.
    const char* span_in(uint64_t view_index, uint64_t offset, uint64_t bytes)
    {
        const auto& views = array_of(root_, "bufferViews");
        if (view_index >= views.size()) refuse("buffer view " + std::to_string(view_index) + " is not there");
        const json::Value& v = views[view_index];
        const uint64_t buffer = uint_of(v, "buffer", UINT64_MAX), vo = uint_of(v, "byteOffset", 0), vl = uint_of(v, "byteLength", 0);
        if (buffer >= buffers_.size()) refuse("buffer view " + std::to_string(view_index) + " names a buffer that is not there");
        if (vo > buffers_[buffer].size() || vl > buffers_[buffer].size() - vo) refuse("buffer view " + std::to_string(view_index) + " runs past its buffer");
        if (offset > vl || bytes > vl - offset) refuse("an accessor runs past its buffer view");
        return buffers_[buffer].data() + vo + offset;
    }
    // An accessor's first element and its stride, checked so that every element lies in the view.
    std::pair<const char*, size_t> view_of(const json::Value& a, uint64_t index, uint64_t count, size_t elem)
    {
        const auto& views = array_of(root_, "bufferViews");
        const uint64_t vi = uint_of(a, "bufferView", UINT64_MAX);
        if (vi >= views.size()) refuse("accessor " + std::to_string(index) + " names a buffer view that is not there");
        const uint64_t stride = uint_of(views[vi], "byteStride", 0);
        const uint64_t step = stride ? stride : elem;
        if (stride && stride < elem) refuse("buffer view " + std::to_string(vi) + "'s stride is shorter than an element");
        if (count > (uint64_t(1) << 32)) refuse("accessor " + std::to_string(index) + " has too many elements");
        const uint64_t need = count == 0 ? 0 : step * (count - 1) + elem;
        return {span_in(vi, uint_of(a, "byteOffset", 0), need), size_t(step)};
    }

    const json::Value& root_;
    ImportedModel& out_;
    std::vector<std::string> buffers_;
    std::vector<bool> seen_;
};

// A node's rest: its matrix as given, or its translation, rotation and scale (which animations
// replace one at a time).
struct Rest
{
    bool has_matrix = false;
    Mat m{};
    double t[3] = {0, 0, 0}, r[4] = {0, 0, 0, 1}, s[3] = {1, 1, 1};
};

Rest rest_of(const json::Value& node)
{
    Rest k;
    if (node.find("matrix"))
    {
        k.has_matrix = true;
        k.m = local_matrix(node);
        return k;
    }
    if (const json::Value* v = node.find("translation"))
    {
        const auto n = numbers(*v, 3, "a node's translation");
        std::copy(n.begin(), n.end(), k.t);
    }
    if (const json::Value* v = node.find("rotation"))
    {
        const auto n = numbers(*v, 4, "a node's rotation");
        std::copy(n.begin(), n.end(), k.r);
    }
    if (const json::Value* v = node.find("scale"))
    {
        const auto n = numbers(*v, 3, "a node's scale");
        std::copy(n.begin(), n.end(), k.s);
    }
    return k;
}

// Every node's matrix in the world, from each node's local one and its parent (-1: none).
std::vector<Mat> world_matrices(const std::vector<Mat>& local, const std::vector<int64_t>& parent)
{
    std::vector<Mat> w(local.size());
    std::vector<uint8_t> done(local.size(), 0);
    for (size_t i = 0; i < local.size(); ++i)
    {
        // The chain up to a node already done (or a root), then down it.
        std::vector<size_t> chain;
        size_t at = i;
        while (!done[at])
        {
            chain.push_back(at);
            if (parent[at] < 0) break;
            at = size_t(parent[at]);
            if (chain.size() > local.size()) refuse("its nodes form a loop");
        }
        for (size_t k = chain.size(); k-- > 0;)
        {
            const size_t n = chain[k];
            w[n] = parent[n] < 0 ? local[n] : mul(w[size_t(parent[n])], local[n]);
            done[n] = 1;
        }
    }
    return w;
}

// A world matrix's turn (its scale taken off each column), row-major 3x3.
std::array<double, 9> turn_of(const Mat& m)
{
    std::array<double, 9> r{};
    for (int c = 0; c < 3; ++c)
    {
        double n = std::sqrt(m[size_t(c * 4)] * m[size_t(c * 4)] + m[size_t(c * 4 + 1)] * m[size_t(c * 4 + 1)] + m[size_t(c * 4 + 2)] * m[size_t(c * 4 + 2)]);
        if (n == 0) n = 1;
        for (int row = 0; row < 3; ++row) r[size_t(row * 3 + c)] = m[size_t(c * 4 + row)] / n;
    }
    return r;
}

std::array<double, 4> quaternion_of(const std::array<double, 9>& m)
{
    // Shepperd's method: the largest of the four first, for accuracy.
    const double tr = m[0] + m[4] + m[8];
    double x, y, z, w;
    if (tr > 0)
    {
        const double s = std::sqrt(tr + 1.0) * 2;
        w = 0.25 * s, x = (m[7] - m[5]) / s, y = (m[2] - m[6]) / s, z = (m[3] - m[1]) / s;
    }
    else if (m[0] > m[4] && m[0] > m[8])
    {
        const double s = std::sqrt(1.0 + m[0] - m[4] - m[8]) * 2;
        w = (m[7] - m[5]) / s, x = 0.25 * s, y = (m[1] + m[3]) / s, z = (m[2] + m[6]) / s;
    }
    else if (m[4] > m[8])
    {
        const double s = std::sqrt(1.0 + m[4] - m[0] - m[8]) * 2;
        w = (m[2] - m[6]) / s, x = (m[1] + m[3]) / s, y = 0.25 * s, z = (m[5] + m[7]) / s;
    }
    else
    {
        const double s = std::sqrt(1.0 + m[8] - m[0] - m[4]) * 2;
        w = (m[3] - m[1]) / s, x = (m[2] + m[6]) / s, y = (m[5] + m[7]) / s, z = 0.25 * s;
    }
    return {x, y, z, w};
}

// The first skin's joints as a skeleton (parents first, at most kAnimationMaxBones), and every
// animation sampled into their turns from rest.
void read_skeleton(const json::Value& root, Gltf& g, ImportedModel& out)
{
    const auto& nodes = array_of(root, "nodes");
    std::vector<int64_t> parent(nodes.size(), -1);
    for (size_t i = 0; i < nodes.size(); ++i)
        for (const auto& c : array_of(nodes[i], "children"))
        {
            if (!c.is_number() || c.u64() >= nodes.size()) refuse("a node's child is not there");
            if (parent[c.u64()] >= 0) refuse("node " + std::to_string(c.u64()) + " has two parents");
            parent[c.u64()] = int64_t(i);
        }
    std::vector<Rest> rest;
    std::vector<Mat> local;
    for (const auto& n : nodes)
    {
        rest.push_back(rest_of(n));
        local.push_back(rest.back().has_matrix ? rest.back().m : trs_matrix(rest.back().t, rest.back().r, rest.back().s));
    }
    const std::vector<Mat> world0 = world_matrices(local, parent);

    const auto& skin = array_of(root, "skins")[0];
    std::vector<uint64_t> joints;
    std::vector<int64_t> joint_index(nodes.size(), -1);
    for (const auto& j : array_of(skin, "joints"))
    {
        if (!j.is_number() || j.u64() >= nodes.size()) refuse("a skin names a joint that is not there");
        if (joint_index[j.u64()] >= 0) continue;
        joint_index[j.u64()] = int64_t(joints.size());
        joints.push_back(j.u64());
    }
    if (joints.empty()) return;
    // Each joint's parent: the nearest joint above it.
    std::vector<int64_t> jparent(joints.size(), -1);
    for (size_t k = 0; k < joints.size(); ++k)
    {
        int64_t at = parent[joints[k]];
        size_t steps = 0;
        while (at >= 0 && joint_index[size_t(at)] < 0 && ++steps <= nodes.size()) at = parent[size_t(at)];
        jparent[k] = at >= 0 ? joint_index[size_t(at)] : -1;
    }
    // Parents first: from the roots down, in the skin's order; every root after the first hangs
    // from the first.
    std::vector<size_t> order;
    std::vector<std::vector<size_t>> kids(joints.size());
    std::vector<size_t> roots;
    for (size_t k = 0; k < joints.size(); ++k)
        (jparent[k] < 0 ? roots : kids[size_t(jparent[k])]).push_back(k);
    for (size_t r : roots) order.push_back(r);
    for (size_t at = 0; at < order.size(); ++at)
        for (size_t c : kids[order[at]]) order.push_back(c);
    const size_t kept = std::min<size_t>(order.size(), kAnimationMaxBones);
    std::vector<int64_t> sphere_of(joints.size(), -1);
    for (size_t i = 0; i < kept; ++i) sphere_of[order[i]] = int64_t(i);
    for (size_t i = 0; i < kept; ++i)
    {
        const size_t k = order[i];
        const Mat& w = world0[joints[k]];
        ImportedModel::Joint j;
        if (const json::Value* nm = nodes[joints[k]].find("name"); nm && nm->is_string()) j.name = nm->string();
        j.parent = i == 0 ? -1 : jparent[k] >= 0 ? int32_t(sphere_of[size_t(jparent[k])]) : 0;
        j.x = w[12], j.y = w[13], j.z = w[14];
        out.skeleton.push_back(std::move(j));
    }
    auto plural = [](size_t n, const char* one, const char* many) { return std::to_string(n) + " " + (n == 1 ? one : many); };
    if (kept < order.size())
        out.left_out.push_back(plural(order.size() - kept, "joint", "joints") + " past the roof of " + std::to_string(kAnimationMaxBones) + ", not carried");

    // The animations, each sampled at kAnimationRate a second.
    std::vector<std::array<double, 9>> rest_turn(kept);
    for (size_t i = 0; i < kept; ++i) rest_turn[i] = turn_of(world0[joints[order[i]]]);
    size_t translations = 0, scales = 0, weights = 0, cubic = 0, frames_cut = 0;
    for (const auto& a : array_of(root, "animations"))
    {
        struct Channel
        {
            size_t node;
            int path; // 0 translation, 1 rotation, 2 scale
            std::vector<float> times, values;
            int interp; // 0 linear, 1 step, 2 cubic spline
        };
        std::vector<Channel> chans;
        const auto& samplers = array_of(a, "samplers");
        double end = 0;
        for (const auto& c : array_of(a, "channels"))
        {
            const json::Value* target = c.find("target");
            if (!target || !target->find("node") || !target->find("path")) continue;
            const std::string path = target->at("path").is_string() ? target->at("path").string() : "";
            const uint64_t node = uint_of(*target, "node", 0), si = uint_of(c, "sampler", UINT64_MAX);
            if (node >= nodes.size() || si >= samplers.size()) refuse("an animation's channel names what is not there");
            if (path == "weights")
            {
                ++weights;
                continue;
            }
            const int pi = path == "translation" ? 0 : path == "rotation" ? 1 : path == "scale" ? 2 : -1;
            if (pi < 0) continue;
            if (pi == 0) ++translations;
            if (pi == 2) ++scales;
            if (rest[node].has_matrix) refuse("an animation moves node " + std::to_string(node) + ", which has a matrix (glTF forbids it)");
            const json::Value& smp = samplers[si];
            Channel ch;
            ch.node = size_t(node);
            ch.path = pi;
            const std::string interp = smp.find("interpolation") && smp.at("interpolation").is_string() ? smp.at("interpolation").string() : "LINEAR";
            ch.interp = interp == "STEP" ? 1 : interp == "CUBICSPLINE" ? 2 : 0;
            if (ch.interp == 2) ++cubic;
            ch.times = g.floats(uint_of(smp, "input", UINT64_MAX), 1);
            const size_t comps = pi == 1 ? 4 : 3;
            ch.values = g.floats(uint_of(smp, "output", UINT64_MAX), comps);
            const size_t per = comps * (ch.interp == 2 ? 3 : 1);
            if (ch.times.empty() || ch.values.size() != ch.times.size() * per) refuse("an animation sampler's keys and values do not match");
            end = std::max(end, double(ch.times.back()));
            chans.push_back(std::move(ch));
        }
        ImportedModel::Sampled sm;
        if (const json::Value* nm = a.find("name"); nm && nm->is_string()) sm.name = nm->string();
        for (const Channel& ch : chans) sm.moved = sm.moved || ch.path == 0;
        sm.seconds = end;
        double want = std::floor(end * kAnimationRate + 1e-9) + 1;
        if (want > kAnimationMaxFrames) ++frames_cut;
        sm.frames = uint32_t(std::min<double>(want, kAnimationMaxFrames));
        for (uint32_t f = 0; f < sm.frames; ++f)
        {
            const double t = double(f) / kAnimationRate;
            std::vector<Rest> now = rest;
            for (const Channel& ch : chans)
            {
                const size_t comps = ch.path == 1 ? 4 : 3, stride = comps * (ch.interp == 2 ? 3 : 1), off = ch.interp == 2 ? comps : 0;
                size_t k = size_t(std::upper_bound(ch.times.begin(), ch.times.end(), float(t)) - ch.times.begin());
                double v[4];
                auto value = [&](size_t key, size_t c) { return double(ch.values[key * stride + off + c]); };
                if (k == 0 || k >= ch.times.size() || ch.interp == 1)
                {
                    const size_t key = k == 0 ? 0 : k - 1;
                    for (size_t c = 0; c < comps; ++c) v[c] = value(std::min(key, ch.times.size() - 1), c);
                }
                else
                {
                    const double t0 = ch.times[k - 1], t1 = ch.times[k], u = t1 > t0 ? (t - t0) / (t1 - t0) : 0;
                    if (ch.path == 1)
                    {
                        // Slerp, the shorter way round.
                        double a0[4], a1[4], dot = 0;
                        for (size_t c = 0; c < 4; ++c) a0[c] = value(k - 1, c), a1[c] = value(k, c), dot += a0[c] * a1[c];
                        if (dot < 0)
                        {
                            for (double& x : a1) x = -x;
                            dot = -dot;
                        }
                        double wa = 1 - u, wb = u;
                        if (dot < 0.9995)
                        {
                            const double th = std::acos(std::min(1.0, dot)), st = std::sin(th);
                            wa = std::sin((1 - u) * th) / st, wb = std::sin(u * th) / st;
                        }
                        for (size_t c = 0; c < 4; ++c) v[c] = wa * a0[c] + wb * a1[c];
                    }
                    else
                        for (size_t c = 0; c < comps; ++c) v[c] = (1 - u) * value(k - 1, c) + u * value(k, c);
                }
                Rest& r = now[ch.node];
                if (ch.path == 0) std::copy(v, v + 3, r.t);
                else if (ch.path == 1) std::copy(v, v + 4, r.r);
                else std::copy(v, v + 3, r.s);
            }
            std::vector<Mat> loc(nodes.size());
            for (size_t i = 0; i < nodes.size(); ++i) loc[i] = now[i].has_matrix ? now[i].m : trs_matrix(now[i].t, now[i].r, now[i].s);
            const std::vector<Mat> w = world_matrices(loc, parent);
            for (size_t i = 0; i < kept; ++i)
            {
                // The turn from rest: now times rest's inverse (its transpose).
                const auto r1 = turn_of(w[joints[order[i]]]);
                const auto& r0 = rest_turn[i];
                std::array<double, 9> d{};
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                    {
                        double sum = 0;
                        for (int k2 = 0; k2 < 3; ++k2) sum += r1[size_t(row * 3 + k2)] * r0[size_t(col * 3 + k2)];
                        d[size_t(row * 3 + col)] = sum;
                    }
                sm.turns.push_back(quaternion_of(d));
                const Mat& wj = w[joints[order[i]]];
                sm.places.push_back({wj[12], wj[13], wj[14]});
            }
        }
        out.sampled.push_back(std::move(sm));
    }
    if (scales) out.left_out.push_back(plural(scales, "scale channel", "scale channels") + ", not carried: the animation line holds turns");
    if (weights) out.left_out.push_back(plural(weights, "morph-weight channel", "morph-weight channels") + ", not carried");
    if (cubic) out.left_out.push_back(plural(cubic, "cubic-spline channel", "cubic-spline channels") + " played as straight lines between their keys");
    if (frames_cut) out.left_out.push_back(plural(frames_cut, "animation", "animations") + " cut at the roof of " + std::to_string(kAnimationMaxFrames) + " frames");
}

ImportedModel read_gltf_json(std::string_view json_text, std::optional<std::string> bin, const ImportLoader& load, const char* format)
{
    json::Value root;
    try
    {
        root = json::parse(json_text);
    }
    catch (const std::exception& e)
    {
        refuse(std::string("its JSON could not be read: ") + e.what());
    }
    if (!root.is_object()) refuse("its JSON is not an object");
    ImportedModel out;
    out.format = format;
    Gltf g(root, std::move(bin), load, out);
    const auto& meshes = array_of(root, "meshes");
    const auto& nodes = array_of(root, "nodes");
    const auto& scenes = array_of(root, "scenes");
    out.meshes = meshes.size();
    out.nodes = nodes.size();
    if (!scenes.empty())
    {
        const uint64_t s = uint_of(root, "scene", 0);
        if (s >= scenes.size()) refuse("its default scene is not there");
        for (const auto& n : array_of(scenes[s], "nodes"))
        {
            if (!n.is_number()) refuse("a scene's node is not a number");
            g.walk(n.u64(), identity(), 0);
        }
    }
    else if (!nodes.empty())
    {
        // No scene: every node that is no one's child is a root.
        std::vector<bool> child(nodes.size(), false);
        for (const auto& n : nodes)
            for (const auto& c : array_of(n, "children"))
                if (c.is_number() && c.u64() < nodes.size()) child[c.u64()] = true;
        for (size_t i = 0; i < nodes.size(); ++i)
            if (!child[i]) g.walk(i, identity(), 0);
    }
    else
        for (size_t i = 0; i < meshes.size(); ++i) g.add_mesh(i, identity());

    const auto& skins = array_of(root, "skins");
    out.skins = skins.size();
    for (const auto& s : skins) out.joints += array_of(s, "joints").size();
    const auto& anims = array_of(root, "animations");
    out.animations = anims.size();
    for (const auto& a : anims)
    {
        out.channels += array_of(a, "channels").size();
        for (const auto& s : array_of(a, "samplers"))
            if (s.find("input")) out.keyframes = std::max<size_t>(out.keyframes, size_t(g.count_of(uint_of(s, "input", 0))));
    }
    auto plural = [](size_t n, const char* one, const char* many) { return std::to_string(n) + " " + (n == 1 ? one : many); };
    if (!skins.empty()) read_skeleton(root, g, out);
    if (out.skins > 1) out.left_out.push_back(plural(out.skins - 1, "skin", "skins") + " after the first, not carried: a character has one skeleton");
    if (!anims.empty() && out.skeleton.empty())
        out.left_out.push_back(plural(out.animations, "animation", "animations") + ", not carried: they move a skeleton, and there is none");
    if (g.skipped_points_lines_) out.left_out.push_back(plural(g.skipped_points_lines_, "primitive", "primitives") + " of points or lines, not carried: a held model is triangles");
    if (g.skipped_no_position_) out.left_out.push_back(plural(g.skipped_no_position_, "primitive", "primitives") + " with no positions, skipped");
    const size_t materials = array_of(root, "materials").size(), textures = array_of(root, "textures").size();
    if (materials || textures)
        out.left_out.push_back(plural(materials, "material", "materials") + " and " + plural(textures, "texture", "textures") + ", not carried: a held model has no surface yet");
    if (out.mesh.verts.empty() && out.skeleton.empty()) refuse("no triangles in its default scene, and no skeleton");
    return out;
}

} // namespace

ImportedModel read_gltf(std::string_view bytes, const ImportLoader& load)
{
    if (bytes.size() >= 4 && bytes.substr(0, 4) == "glTF")
    {
        if (bytes.size() < 20) refuse("a .glb too short for its header");
        const uint32_t version = le32(bytes.data() + 4), length = le32(bytes.data() + 8);
        if (version != 2) refuse("a .glb of version " + std::to_string(version) + ", not 2");
        if (length > bytes.size() || length < 20) refuse("a .glb whose length is not its size");
        size_t at = 12;
        std::string_view json_chunk;
        std::optional<std::string> bin;
        while (at + 8 <= length)
        {
            const uint32_t clen = le32(bytes.data() + at), ctype = le32(bytes.data() + at + 4);
            if (clen > length - at - 8) refuse("a .glb chunk runs past the file");
            const std::string_view data = bytes.substr(at + 8, clen);
            if (ctype == 0x4E4F534Au && json_chunk.empty()) json_chunk = data;      // "JSON"
            else if (ctype == 0x004E4942u && !bin) bin = std::string(data);           // "BIN\0"
            at += 8 + size_t(clen);
        }
        if (json_chunk.empty()) refuse("a .glb with no JSON chunk");
        return read_gltf_json(json_chunk, std::move(bin), load, "glb");
    }
    return read_gltf_json(bytes, std::nullopt, load, "gltf");
}

Character skeleton_character(const ImportedModel& m)
{
    if (m.skeleton.empty()) throw std::invalid_argument("no skeleton in it (a glTF's first skin is read as one)");
    const auto& j = m.skeleton;
    const double metre = kCharacterUnitsPerMetre;
    auto dist = [&](size_t a, size_t b) {
        const double dx = j[a].x - j[b].x, dy = j[a].y - j[b].y, dz = j[a].z - j[b].z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    };
    std::vector<double> nearest(j.size(), 0), farthest(j.size(), 0);
    for (size_t i = 1; i < j.size(); ++i)
    {
        const size_t p = size_t(j[i].parent);
        const double d = dist(i, p);
        if (d <= 0) continue;
        for (const size_t k : {i, p})
        {
            nearest[k] = nearest[k] == 0 ? d : std::min(nearest[k], d);
            farthest[k] = std::max(farthest[k], d);
        }
    }
    std::vector<Sphere> sp;
    for (size_t i = 0; i < j.size(); ++i)
    {
        Sphere s;
        s.parent = j[i].parent;
        s.x = int32_t(std::lround((j[i].x - j[0].x) * metre));
        s.y = int32_t(std::lround((j[i].y - j[0].y) * metre));
        s.z = int32_t(std::lround((j[i].z - j[0].z) * metre));
        const double r = i == 0 ? farthest[0] / 3 : nearest[i] / 4;
        s.r = std::max<int32_t>(1, int32_t(std::lround(r * metre)));
        if (s.r == 1 && i == 0) s.r = std::max<int32_t>(1, int32_t(0.05 * metre));
        std::string lower = j[i].name;
        for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
        s.kind = i == 0 ? Sphere::Body : lower.find("head") != std::string::npos ? Sphere::Head : Sphere::Joint;
        sp.push_back(s);
    }
    return given_character(std::move(sp));
}

Animation imported_animation(const ImportedModel& m, size_t which)
{
    if (which >= m.sampled.size()) throw std::invalid_argument("no animation " + std::to_string(which) + " in it (it has " + std::to_string(m.sampled.size()) + ")");
    const ImportedModel::Sampled& s = m.sampled[which];
    Animation a;
    a.bones = uint32_t(m.skeleton.size());
    a.frames = s.frames;
    for (const auto& q : s.turns) a.turns.push_back(turn_of_quaternion(q[0], q[1], q[2], q[3]));
    // Its translation channels (sieve-animation-v2): each bone's move, what is left between where the
    // file puts it and where its parent's turn (as kept, rounded to its steps) puts it from its parent
    // (as played back, moves rounded), so the rounding never adds up down a limb.
    if (s.moved && s.places.size() == s.turns.size())
    {
        const Character c = skeleton_character(m);
        const auto& j = m.skeleton;
        const double metre = kCharacterUnitsPerMetre;
        a.moves.resize(a.turns.size());
        std::vector<std::array<double, 3>> at(a.bones);
        for (uint32_t f = 0; f < a.frames; ++f)
            for (uint32_t b = 0; b < a.bones; ++b)
            {
                const auto& w = s.places[size_t(f) * a.bones + b];
                const double want[3] = {(w[0] - j[0].x) * metre, (w[1] - j[0].y) * metre, (w[2] - j[0].z) * metre};
                const Sphere& sp = c.spheres[b];
                double from[3] = {double(sp.x), double(sp.y), double(sp.z)};
                if (sp.parent >= 0 && uint32_t(sp.parent) < b)
                {
                    const Sphere& q = c.spheres[size_t(sp.parent)];
                    double t[9];
                    turn_matrix(a.at(f, uint32_t(sp.parent)), t);
                    const double o[3] = {double(sp.x - q.x), double(sp.y - q.y), double(sp.z - q.z)};
                    for (int r = 0; r < 3; ++r) from[r] = at[size_t(sp.parent)][size_t(r)] + t[r * 3] * o[0] + t[r * 3 + 1] * o[1] + t[r * 3 + 2] * o[2];
                }
                BoneMove mv;
                int32_t* comp[3] = {&mv.x, &mv.y, &mv.z};
                for (int r = 0; r < 3; ++r)
                {
                    *comp[r] = int32_t(std::clamp(std::llround(want[r] - from[r]), (long long)kAnimationMoveLeast, (long long)kAnimationMoveMost));
                    at[b][size_t(r)] = from[r] + *comp[r];
                }
                a.moves[size_t(f) * a.bones + b] = mv;
            }
        if (!a.moving()) a.moves.clear(); // (nothing moved after all: v1)
    }
    return a;
}

ImportedModel read_model(std::string_view bytes, const ImportLoader& load)
{
    if (bytes.size() >= 4 && bytes.substr(0, 4) == "glTF") return read_gltf(bytes, load);
    size_t i = 0;
    if (bytes.size() >= 3 && bytes.substr(0, 3) == "\xEF\xBB\xBF") i = 3; // a byte-order mark
    while (i < bytes.size() && (bytes[i] == ' ' || bytes[i] == '\t' || bytes[i] == '\r' || bytes[i] == '\n')) ++i;
    if (i < bytes.size() && bytes[i] == '{' && bytes.find("\"asset\"") != std::string_view::npos) return read_gltf(bytes.substr(i), load);
    ImportedModel out;
    out.format = "obj";
    out.mesh = ModelSpace::read_any_obj(bytes);
    out.meshes = 1;
    return out;
}

} // namespace sieve
