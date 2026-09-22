#include "strings.hpp"

#include "font.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

namespace hallway {

namespace fs = std::filesystem;

namespace {

using Table = std::map<std::string, std::string, std::less<>>;

Table g_english, g_current;
std::string g_code = "en";

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

Table load_file(const fs::path& path)
{
    Table t;
    std::ifstream in(path, std::ios::binary);
    std::string line;
    bool first = true;
    while (std::getline(in, line))
    {
        if (first && line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3); // UTF-8 byte order mark
        first = false;
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        // "Quoted" keeps spaces at either end (and anything after the closing quote is ignored).
        if (value.size() >= 2 && value.front() == '"' && value.find('"', 1) != std::string::npos)
            value = value.substr(1, value.rfind('"') - 1);
        // A comment after an unquoted value needs a space before its ';'.
        else for (size_t i = 1; i < value.size(); ++i)
            if (value[i] == ';' && (value[i - 1] == ' ' || value[i - 1] == '\t'))
            {
                value = trim(value.substr(0, i));
                break;
            }
        std::string out;
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] == '\\' && i + 1 < value.size() && value[i + 1] == 'n') { out += '\n'; ++i; }
            else out += value[i];
        }
        t[key] = out;
    }
    return t;
}

// The language's font (its "font =" line), else English's, else SDL's built-in ASCII font.
void apply_font()
{
    for (const Table* t : {&g_current, &g_english})
        if (auto it = t->find("font"); it != t->end())
        {
            if (set_font(it->second)) return;
            std::cerr << "note: font '" << it->second << "' not found in the fonts folder\n";
        }
    set_font("debug");
}

} // namespace

fs::path language_folder()
{
    std::vector<fs::path> folders = {fs::path("data") / "lang"};
    if (const fs::path exe = sieve::cli::executable_dir(); !exe.empty())
    {
        folders.push_back(exe / "lang");
        folders.push_back(exe / ".." / "data" / "lang");
    }
    for (const auto& f : folders)
    {
        std::error_code ec;
        if (fs::is_directory(f, ec)) return f;
    }
    return folders.front();
}

std::vector<LanguageInfo> available_languages()
{
    std::vector<LanguageInfo> out;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(language_folder(), ec))
    {
        if (e.path().extension() != ".txt") continue;
        LanguageInfo li{e.path().stem().string(), "", e.path()};
        const Table t = load_file(e.path());
        const auto it = t.find("language.name");
        li.name = it == t.end() ? li.code : it->second;
        out.push_back(li);
    }
    std::sort(out.begin(), out.end(), [](const LanguageInfo& a, const LanguageInfo& b) {
        if ((a.code == "en") != (b.code == "en")) return a.code == "en";
        return a.name < b.name;
    });
    return out;
}

bool set_language(const std::string& code)
{
    const fs::path folder = language_folder();
    if (g_english.empty()) g_english = load_file(folder / "en.txt");
    std::error_code ec;
    const fs::path file = folder / (code + ".txt");
    if (code.empty() || code.find_first_of("/\\.") != std::string::npos || !fs::exists(file, ec))
    {
        g_code = "en";
        g_current = g_english;
        apply_font();
        return code == "en";
    }
    g_code = code;
    g_current = code == "en" ? g_english : load_file(file);
    apply_font();
    return true;
}

const std::string& language_code() { return g_code; }

const std::string& tr(std::string_view key)
{
    if (auto it = g_current.find(key); it != g_current.end()) return it->second;
    if (auto it = g_english.find(key); it != g_english.end()) return it->second;
    // Unknown everywhere: show the key, so the gap is visible and easy to fill.
    static std::map<std::string, std::string, std::less<>> missing;
    auto [it, _] = missing.emplace(std::string(key), std::string(key));
    return it->second;
}

std::string tr_or(std::string_view key, const std::string& fallback)
{
    if (auto it = g_current.find(key); it != g_current.end()) return it->second;
    if (auto it = g_english.find(key); it != g_english.end()) return it->second;
    return fallback;
}

std::string trf(std::string_view key, const std::vector<std::string>& args)
{
    std::string s = tr(key);
    for (size_t i = 0; i < args.size(); ++i)
    {
        const std::string mark = "{" + std::to_string(i) + "}";
        for (size_t p = s.find(mark); p != std::string::npos; p = s.find(mark, p + args[i].size())) s.replace(p, mark.size(), args[i]);
    }
    return s;
}

} // namespace hallway
