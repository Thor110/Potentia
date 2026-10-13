// sieve cipher: the Encryption Engine's first stage from the command line (cipher-v1,
// SPECIFICATIONS §12.0i). Encrypt or decrypt bytes with a recipe of layers, try to break a weak
// cipher, or count the recipes there are.
#include "cli/args.hpp"
#include "cli/locate.hpp"
#include "sieve/biguint.hpp"
#include "sieve/cipher.hpp"
#include "sieve/sha256.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;

std::string hex(const std::vector<uint8_t>& b)
{
    static const char k[] = "0123456789abcdef";
    std::string s;
    for (const uint8_t c : b) s += {k[c >> 4], k[c & 15]};
    return s;
}

std::vector<uint8_t> from_hex(const std::string& h)
{
    if (h.size() % 2) throw std::invalid_argument("--in-hex needs whole bytes");
    std::vector<uint8_t> b;
    for (size_t i = 0; i < h.size(); i += 2) b.push_back(uint8_t(std::stoi(h.substr(i, 2), nullptr, 16)));
    return b;
}

// What to work on: --in FILE, --text TEXT (its UTF-8 bytes) or --in-hex HEX.
std::vector<uint8_t> input(const Args& a)
{
    if (a.has("in")) return read_file_bytes(a.get("in"));
    if (a.has("text"))
    {
        const std::string t = a.get("text");
        return std::vector<uint8_t>(t.begin(), t.end());
    }
    if (a.has("in-hex")) return from_hex(a.get("in-hex"));
    throw std::invalid_argument("give the bytes with --in FILE, --text TEXT or --in-hex HEX");
}

// Bytes shown: as text when every byte is printable ASCII (or a newline or tab), else in hex.
std::string shown(const std::vector<uint8_t>& b, size_t at_most)
{
    bool text = true;
    for (const uint8_t c : b)
        if (!((c >= 0x20 && c < 0x7F) || c == '\n' || c == '\t')) text = false;
    const std::vector<uint8_t> head(b.begin(), b.begin() + std::ptrdiff_t(std::min(b.size(), at_most)));
    std::string s;
    if (text)
    {
        for (const uint8_t c : head) s += c == '\n' ? std::string("\\n") : c == '\t' ? std::string("\\t") : std::string(1, char(c));
        s = "\"" + s + "\"";
    }
    else s = hex(head);
    return s + (b.size() > at_most ? " ..." : "");
}

cipher::Recipe recipe_of(const Args& a)
{
    if (a.has("recipe-file"))
    {
        std::ifstream in(a.get("recipe-file"), std::ios::binary);
        if (!in) throw std::invalid_argument("cannot read " + a.get("recipe-file"));
        std::ostringstream all;
        all << in.rdbuf();
        return cipher::Recipe::parse(all.str());
    }
    if (a.has("recipe")) return cipher::Recipe::parse(a.get("recipe"));
    throw std::invalid_argument("give a recipe with --recipe \"LAYER; LAYER\" or --recipe-file PATH");
}

int catalogue(const Args& a)
{
    // Each layer with a key of at most K bytes: reverse (1), caesar (256), and xor, vigenere and
    // feistel (keys of 1 to K bytes: 256 + 256^2 + ... + 256^K each).
    const uint32_t k = a.get_positive("key-bytes", 16), depth = a.get_positive("depth", 4);
    BigUint keys(0);
    for (uint32_t i = 1; i <= k; ++i) keys += BigUint::pow(256, i);
    BigUint layer = keys;
    layer.mul_small(3);
    layer.add_small(257);
    std::cout << "cipher       " << cipher::kCipherVersion << ": 5 kinds of layer, keys of at most " << k << " bytes\n"
              << "layers       " << layer.to_decimal() << " (" << layer.bit_length() << " bits)\n";
    BigUint total(0);
    for (uint32_t d = 1; d <= depth; ++d)
    {
        const BigUint r = BigUint::pow(layer, d);
        total += r;
        const std::string dec = r.to_decimal();
        std::cout << "depth " << d << "      " << (dec.size() <= 60 ? dec : dec.substr(0, 12) + "... (" + std::to_string(dec.size()) + " digits)")
                  << " recipes, " << r.bit_length() << " bits to name one\n";
    }
    std::cout << "up to " << depth << "      " << total.bit_length() << " bits name any of them. Every one is a bijection of the files\n"
              << "             of each length, so together they reach every file of that length: the key is what\n"
              << "             makes one path the message (IDEAS §19.52).\n";
    return 0;
}

int breaking(const Args& a)
{
    const std::vector<uint8_t> c = input(a);
    const size_t top = a.get_positive("top", 5), max_key = a.get_positive("max-key", 16);
    const auto guesses = cipher::break_weak(c, max_key, top);
    std::cout << "bytes        " << c.size() << ", English score as they are: " << cipher::english_score(c) << "\n";
    for (size_t i = 0; i < guesses.size(); ++i)
    {
        std::string line = guesses[i].recipe.layers.front().text();
        std::cout << "guess " << (i + 1) << "      " << line << std::string(line.size() < 24 ? 24 - line.size() : 1, ' ') << "rank "
                  << guesses[i].rank << " (score " << guesses[i].score << ")  " << shown(guesses[i].plain, 48) << "\n";
    }
    if (!guesses.empty() && guesses.front().rank <= 0)
        std::cout << "none of the weak ciphers gives English: a strong cipher's output, or not English, looks like this\n";
    return 0;
}

} // namespace

int cmd_cipher(const Args& a)
{
    if (a.has("catalogue")) return catalogue(a);
    if (a.has("break")) return breaking(a);
    const bool enc = a.has("encrypt"), dec = a.has("decrypt");
    if (enc == dec) throw std::invalid_argument("say --encrypt or --decrypt (or --break, or --catalogue)");
    const cipher::Recipe r = recipe_of(a);
    const std::vector<uint8_t> in = input(a);
    const std::vector<uint8_t> out = enc ? cipher::encrypt(r, in) : cipher::decrypt(r, in);
    std::string layers;
    for (const auto& l : r.layers) layers += (layers.empty() ? "" : "; ") + l.text();
    std::cout << "recipe       " << cipher::kCipherVersion << ": " << layers << "\n"
              << (enc ? "encrypted    " : "decrypted    ") << in.size() << " bytes, SHA-256 " << Sha256::hex(Sha256::hash(std::string(out.begin(), out.end())))
              << "\n";
    if (a.has("out"))
    {
        std::ofstream f(a.get("out"), std::ios::binary);
        if (!f || !f.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size())))
            throw std::runtime_error("cannot write " + a.get("out"));
        std::cout << "written      " << a.get("out") << "\n";
    }
    else
    {
        std::cout << "hex          " << hex(out) << "\n";
        if (shown(out, out.size() + 1).front() == '"') std::cout << "as text      " << shown(out, 400) << "\n";
    }
    return 0;
}

} // namespace sieve::cli
