// Sieve hallway -- F on the item page: the item in hand saved as a file, wherever you choose.
//
// Each line saves what its item is, in the plainest format that holds it: a page as text, a picture
// as a PNG, a video's frames side by side in one PNG, notes as a MIDI file (what `sieve read
// --out` writes for those lines), a model as its canonical .obj, a book as text (its title, then
// its pages; the cover is a picture, and a book has only one file), and a file on the binary line
// as exactly its bytes. The name offered is the item's title where it has one (on the binary line,
// the file's own name if you walked to it from one, as V names an anchor), else the line and the
// start of its address. The save dialog runs on the system's side; what it chose is taken up at
// the next frame, with a copy of the item made when F was pressed, so putting the item back first
// changes nothing.

#include "hallway.hpp"

#include "cli/lines.hpp"
#include "cli/locate.hpp"
#include "cli/vault.hpp"

#include <fstream>

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// A name as every system takes one: no separators, no characters Windows refuses.
std::string safe_name(std::string s)
{
    for (char& c : s)
        if (std::string("/\\:*?\"<>|\t\r\n").find(c) != std::string::npos) c = '_';
    while (!s.empty() && (s.back() == ' ' || s.back() == '.')) s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

void write_all(const fs::path& to, const void* data, size_t n)
{
    std::ofstream out(to, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write the file");
    out.write(static_cast<const char*>(data), std::streamsize(n));
    if (!out) throw std::runtime_error("cannot write the file");
}

void SDLCALL save_chosen(void* user, const char* const* files, int)
{
    auto* h = static_cast<Hallway*>(user);
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    h->item_save_chosen(files[0]);
}

} // namespace

// The binary line's name for a file: its own, if it was walked to from one; else its title, else
// the start of its SHA-256; with the kind its first bytes say it is (V's anchors are named so too).
std::string Hallway::binary_file_name(const Book& bk, const std::string& sha)
{
    if (auto it = walked_names_.find(sha); it != walked_names_.end()) return safe_name(it->second);
    std::string t = title_text(bk);
    std::string kind = file_type(bk.head, bk.file_size);
    for (char& c : kind) c = char(std::tolower(uint8_t(c)));
    return safe_name((t.empty() ? "anchor-" + sha.substr(0, 12) : t) + (kind == "?" || kind == "empty" ? "" : "." + kind));
}

void Hallway::item_save_chosen(const std::string& path)
{
    std::lock_guard<std::mutex> lock(save_mx_);
    save_pending_ = path;
}

// F: asks where to save the item in hand.
void Hallway::save_in_hand()
{
    const std::string name = prepare_save();
    if (name.empty()) return;
    const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    const std::string start = docs ? std::string(docs) + name : name;
    SDL_ShowSaveFileDialog(save_chosen, this, window_, nullptr, 0, start.c_str());
}

// For testing without a person (--save-item PATH): the same, to PATH, without the dialog.
void Hallway::save_in_hand_to(const std::string& path)
{
    if (!prepare_save().empty()) item_save_chosen(path);
}

// Keeps a copy of the item in hand to save, and returns the name to offer ("" with nothing in hand).
std::string Hallway::prepare_save()
{
    if (!in_hand_) return {};
    const Book& bk = *in_hand_;
    if (withheld(bk)) return {}; // the vault (it cannot be in hand, but never saved either way)
    std::string name, ext;
    if (on_binary() && bk.is_file) name = binary_file_name(bk, cli::sha256_hex(file_of(bk)));
    else
    {
        if (bk.model) ext = ".obj";
        else if (bk.parts) ext = ".txt";
        else
            switch (line().kind)
            {
            case LineKind::Text: ext = ".txt"; break;
            case LineKind::Image:
            case LineKind::Video: ext = ".png"; break;
            case LineKind::Audio: ext = ".mid"; break;
            default: ext = ".bin"; break;
            }
        std::string title = bk.parts ? utf8_encode(line().space.text_of(bk.parts->title)) : title_text(bk);
        title = safe_name(title);
        if (title.size() > 64) title.resize(64);
        const std::string where = bk.hex.substr(0, 12);
        name = (title.empty() ? std::string("sieve-") + media_name(media()) + (where.empty() ? "" : "-" + where) : title) + ext;
    }
    save_item_ = bk; // what F was pressed on, whatever is in hand when the dialog answers
    save_ext_ = ext;
    return name;
}

// Called every frame: writes the item once the dialog has said where.
void Hallway::item_save_poll()
{
    std::optional<std::string> to;
    {
        std::lock_guard<std::mutex> lock(save_mx_);
        to.swap(save_pending_);
    }
    if (!to || !save_item_) return;
    const Book bk = std::move(*save_item_);
    save_item_.reset();
    fs::path path = from_u8(*to);
    if (!save_ext_.empty() && path.extension().empty()) path += from_u8(save_ext_);
    const std::string shown = [&] {
        const std::u8string s = path.filename().u8string();
        return std::string(s.begin(), s.end());
    }();
    try
    {
        if (withheld(bk)) throw cli::VaultWithheld("withheld by the vault");
        if (bk.is_file)
        {
            const auto& bytes = file_of(bk);
            write_all(path, bytes.data(), bytes.size());
        }
        else if (bk.model && model_space_)
        {
            const std::string obj = model_space_->to_obj(*bk.model);
            write_all(path, obj.data(), obj.size());
        }
        else if (bk.parts)
        {
            // Its title, then each page, a blank line between.
            std::string text = utf8_encode(line().space.text_of(bk.parts->title));
            while (!text.empty() && text.back() == ' ') text.pop_back();
            text += "\n\n";
            for (size_t i = 0; i < bk.parts->pages.size(); ++i)
                text += utf8_encode(line().space.text_of(bk.parts->pages[i])) + "\n\n";
            write_all(path, text.data(), text.size());
        }
        else
        {
            const std::u8string s = path.u8string();
            cli::save_unit(line(), bk.unit, std::string(s.begin(), s.end()), 16);
        }
        message(trf("hand.saved", {shown}));
    }
    catch (const std::exception& ex)
    {
        message(trf("hand.save_failed", {shown, ex.what()}));
    }
}

} // namespace hallway::hall
