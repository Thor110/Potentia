// Sieve CLI — `sieve chat` (see chat.hpp).
#include "cli/chat.hpp"

#include "sieve/json.hpp"
#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

namespace {

std::filesystem::path path_of(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

std::string read_text(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::invalid_argument("cannot open " + file.string());
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

} // namespace

int cmd_chat(const Args& a)
{
    if (!a.has("model")) throw std::invalid_argument("give the model's folder: sieve chat --model FOLDER (config.json, model.safetensors, tokenizer.json)");
    const std::filesystem::path dir = path_of(a.get("model"));
    const llm::Tokenizer tok(json::parse(read_text(dir / "tokenizer.json")));
    if (a.has("encode"))
    {
        // One line a token: its id and its bytes, as JSON writes a string.
        const std::vector<uint32_t> ids = tok.encode(a.get("encode"), true);
        for (const uint32_t id : ids) std::cout << id << "\t" << json::write(json::Value::make_string(tok.bytes_of(id))) << "\n";
        return 0;
    }
    if (a.has("encode-lines"))
    {
        // A file of texts, one a line, each written as a JSON string: their tokens, one line each.
        std::ifstream in(path_of(a.get("encode-lines")), std::ios::binary);
        if (!in) throw std::invalid_argument("cannot open " + a.get("encode-lines"));
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<uint32_t> ids = tok.encode(json::parse(line).string(), true);
            for (size_t i = 0; i < ids.size(); ++i) std::cout << (i ? " " : "") << ids[i];
            std::cout << "\n";
        }
        return 0;
    }
    const json::Value config = json::parse(read_text(dir / "config.json"));
    const unsigned threads = a.has("threads") ? a.get_u32("threads", 0) : 0;
    const auto loaded_at = std::chrono::steady_clock::now();
    llm::Model model(config, dir / "model.safetensors", threads);
    const double load_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - loaded_at).count();
    if (a.has("logits"))
    {
        // The model's view after a text (no chat around it): the likeliest next tokens, and the
        // logits' sum and sum of squares, to compare with another implementation.
        const std::vector<uint32_t> ids = tok.encode(a.get("logits"), true);
        if (ids.empty()) throw std::invalid_argument("--logits needs some text");
        std::vector<float> logits;
        for (const uint32_t id : ids) logits = model.step(id);
        std::vector<uint32_t> order(logits.size());
        for (uint32_t i = 0; i < order.size(); ++i) order[i] = i;
        std::partial_sort(order.begin(), order.begin() + 5, order.end(), [&](uint32_t x, uint32_t y) { return logits[x] > logits[y]; });
        double sum = 0, sum2 = 0;
        for (const float v : logits)
        {
            sum += v;
            sum2 += double(v) * v;
        }
        std::cout << "tokens       " << ids.size() << "\n";
        for (int i = 0; i < 5; ++i)
        {
            char b[64];
            std::snprintf(b, sizeof b, "%.4f", logits[order[size_t(i)]]);
            std::cout << "next         " << order[size_t(i)] << "\t" << b << "\t" << json::write(json::Value::make_string(tok.bytes_of(order[size_t(i)]))) << "\n";
        }
        char b[96];
        std::snprintf(b, sizeof b, "%.6e %.6e", sum, sum2);
        std::cout << "sums         " << b << "\n";
        if (a.has("logits-out"))
        {
            // All of them, as 32-bit floats, little-endian.
            std::ofstream out(path_of(a.get("logits-out")), std::ios::binary);
            for (const float v : logits)
            {
                uint32_t u;
                std::memcpy(&u, &v, 4);
                const char bytes[4] = {char(u & 0xFF), char((u >> 8) & 0xFF), char((u >> 16) & 0xFF), char(u >> 24)};
                out.write(bytes, 4);
            }
            if (!out) throw std::invalid_argument("cannot write " + a.get("logits-out"));
        }
        return 0;
    }
    llm::Sampling sampling;
    if (a.has("temperature")) sampling.temperature = float(std::stod(a.get("temperature")));
    if (a.has("top-p")) sampling.top_p = float(std::stod(a.get("top-p")));
    if (a.has("top-k")) sampling.top_k = a.get_u32("top-k", 0);
    if (a.has("repeat-penalty")) sampling.repeat_penalty = float(std::stod(a.get("repeat-penalty")));
    if (a.has("seed")) sampling.seed = a.get_u32("seed", 1);
    llm::Sampler sampler(sampling);
    const size_t max_tokens = a.has("max-tokens") ? a.get_positive("max-tokens", 512) : 512;
    if (a.has("raw"))
    {
        // No chat around it: the model continues the text it is given (a model with no chat format,
        // such as one off the AI line's shelves).
        const std::string text = a.has("prompt") ? a.get("prompt") : std::string("\n");
        llm::continue_text(model, tok, text, sampler, max_tokens, [&](const std::string& piece) {
            std::cout << piece << std::flush;
            return true;
        });
        std::cout << "\n";
        return 0;
    }
    llm::Chat chat(model, tok, a.has("system") ? a.get("system") : std::string(llm::Chat::kDefaultSystem));
    std::cerr << "model        " << a.get("model") << ": " << model.config().layers << " layers, " << (model.weight_bytes() >> 20) << " MB of weights, loaded in "
              << std::lround(load_s * 10) / 10.0 << " s\n";
    auto say = [&](const std::string& message) {
        const size_t before = chat.tokens();
        const auto t0 = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point first{};
        chat.reply(message, sampler, max_tokens, [&](const std::string& piece) {
            if (first == std::chrono::steady_clock::time_point{}) first = std::chrono::steady_clock::now();
            std::cout << piece << std::flush;
            return true;
        });
        const auto t1 = std::chrono::steady_clock::now();
        if (first == std::chrono::steady_clock::time_point{}) first = t1;
        std::cout << "\n";
        const size_t reply = chat.last_reply_tokens(), prompt = chat.tokens() - before - reply;
        const double read_s = std::chrono::duration<double>(first - t0).count(), write_s = std::chrono::duration<double>(t1 - first).count();
        char b[160];
        std::snprintf(b, sizeof b, "(read %zu tokens in %.1f s; wrote %zu in %.1f s, %.1f a second)", prompt, read_s, reply, write_s,
                      write_s > 0 ? double(reply) / write_s : 0.0);
        std::cerr << b << "\n";
    };
    if (a.has("prompt"))
    {
        say(a.get("prompt"));
        return 0;
    }
    // A conversation: each line typed is a message; an empty line, or the end of the input, ends it.
    std::cerr << "(type a message and press Enter; an empty line ends the conversation)\n";
    std::string line;
    while (std::cout << "> " << std::flush, std::getline(std::cin, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        say(line);
    }
    return 0;
}

} // namespace sieve::cli
