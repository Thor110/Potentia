// Sieve hallway — favourites (see favourites.hpp).
#include "favourites.hpp"

#include "cli/sieve_cache.hpp"
#include "sieve/paths.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace hallway {

namespace fs = std::filesystem;

namespace {

std::string digits_text(const std::vector<uint32_t>& d)
{
    std::string t;
    for (size_t i = 0; i < d.size(); ++i) t += (i ? "," : "") + std::to_string(d[i]);
    return t;
}

std::string encode(const std::string& s)
{
    std::string o;
    for (const char c : s)
        if (c == '%') o += "%25";
        else if (c == '\n') o += "%0A";
        else if (c == '\r') o += "%0D";
        else o += c;
    return o;
}

std::string decode(const std::string& s)
{
    std::string o;
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i] == '%' && i + 2 < s.size())
        {
            const std::string h = s.substr(i + 1, 2);
            o += h == "25" ? '%' : h == "0A" ? '\n' : h == "0D" ? '\r' : '?';
            i += 2;
        }
        else o += s[i];
    return o;
}

std::vector<std::string> lines_of(const fs::path& p)
{
    std::vector<std::string> out;
    std::ifstream in(p, std::ios::binary);
    std::string l;
    while (std::getline(in, l))
    {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        out.push_back(l);
    }
    return out;
}

} // namespace

std::string favourite_sha(const std::string& space, const std::vector<uint32_t>& digits)
{
    return sieve::Sha256::hex(sieve::Sha256::hash(space + "\n" + digits_text(digits)));
}

fs::path favourites_dir(const fs::path& base) { return base / "favourites"; }

bool save_favourite(const fs::path& base, const Favourite& f, std::string& why)
{
    std::error_code ec;
    fs::create_directories(favourites_dir(base), ec);
    std::ofstream out(favourites_dir(base) / (f.sha + ".item"), std::ios::binary);
    out << "sieve-favourite-v1\nline " << f.line << "\nspace " << f.space << "\ntitle " << encode(f.title) << "\ndigits " << digits_text(f.digits) << "\n";
    if (!out)
    {
        why = sieve::u8(favourites_dir(base));
        return false;
    }
    return true;
}

std::optional<Favourite> read_favourite(const fs::path& base, const std::string& sha)
{
    const auto ls = lines_of(favourites_dir(base) / (sha + ".item"));
    if (ls.empty() || ls[0] != "sieve-favourite-v1") return std::nullopt;
    Favourite f;
    f.sha = sha;
    bool digits = false;
    for (size_t i = 1; i < ls.size(); ++i)
    {
        const size_t sp = ls[i].find(' ');
        const std::string k = ls[i].substr(0, sp), v = sp == std::string::npos ? std::string() : ls[i].substr(sp + 1);
        if (k == "line") f.line = v;
        else if (k == "space") f.space = v;
        else if (k == "title") f.title = decode(v);
        else if (k == "digits")
        {
            digits = true;
            std::istringstream ss(v);
            std::string d;
            try
            {
                while (std::getline(ss, d, ',')) f.digits.push_back(uint32_t(std::stoul(d)));
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }
    // (checked against its own name, so a file changed by hand is not taken for the favourite it was)
    if (!digits || f.line.empty() || favourite_sha(f.space, f.digits) != sha) return std::nullopt;
    return f;
}

std::vector<std::string> dimension_favourites(const fs::path& list_file)
{
    const auto ls = lines_of(list_file);
    std::vector<std::string> out;
    if (ls.empty() || ls[0] != "sieve-favourites-v1") return out;
    for (size_t i = 1; i < ls.size(); ++i)
        if (ls[i].size() == 64 && ls[i].find_first_not_of("0123456789abcdef") == std::string::npos) out.push_back(ls[i]);
    return out;
}

namespace {
bool write_list(const fs::path& list_file, const std::vector<std::string>& shas, std::string& why)
{
    std::error_code ec;
    fs::create_directories(list_file.parent_path(), ec);
    std::ofstream out(list_file, std::ios::binary);
    out << "sieve-favourites-v1\n";
    for (const auto& s : shas) out << s << "\n";
    if (!out) why = sieve::u8(list_file);
    return bool(out);
}
} // namespace

bool add_to_dimension(const fs::path& list_file, const std::string& sha, std::string& why)
{
    auto shas = dimension_favourites(list_file);
    if (std::find(shas.begin(), shas.end(), sha) != shas.end()) return false;
    shas.push_back(sha);
    return write_list(list_file, shas, why);
}

bool remove_from_dimension(const fs::path& list_file, const std::string& sha)
{
    auto shas = dimension_favourites(list_file);
    const auto it = std::find(shas.begin(), shas.end(), sha);
    if (it == shas.end()) return false;
    shas.erase(it);
    std::string why;
    return write_list(list_file, shas, why);
}

std::vector<FavouriteRow> all_favourites(const fs::path& base)
{
    std::vector<FavouriteRow> out;
    std::vector<fs::path> lists;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(base / "worlds", ec))
        if (e.path().extension() == ".favourites") lists.push_back(e.path());
    std::sort(lists.begin(), lists.end());
    for (const fs::path& l : lists)
        for (const std::string& sha : dimension_favourites(l))
        {
            const auto f = read_favourite(base, sha);
            out.push_back({sieve::u8(l.stem()), f ? f->line : std::string("?"), f ? f->title : std::string(), sha});
        }
    // The Media Player's melodies: their names, from the index in its .sieve (read in memory, checked).
    try
    {
        const fs::path music = base / "sieve-favourites.sieve";
        if (fs::exists(music, ec))
            for (const auto& [name, bytes] : sieve::cli::files_in_sieve(music, uint64_t(64) << 20))
                if (fs::path(sieve::from_u8(name)).filename() == "favourites.tsv")
                {
                    std::istringstream in(std::string(bytes.begin(), bytes.end()));
                    std::string l;
                    while (std::getline(in, l))
                        if (!l.empty()) out.push_back({"media", "audio", l.substr(0, l.find('\t')), {}});
                }
    }
    catch (const std::exception&)
    {
        // (a Media Player file that does not read is the Media Player's to say; it is left out here)
    }
    return out;
}

} // namespace hallway
