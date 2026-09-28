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
#include "sieve/filekind.hpp"

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


// ---- J: between an item and its file

// The item in hand as the file F saves it (a picture at one pixel a pixel, so the file is the
// picture at its own size), and the name to give it.
std::vector<uint8_t> Hallway::item_file(const Book& bk, std::string& name)
{
    std::string title = bk.parts ? utf8_encode(line().space.text_of(bk.parts->title)) : title_text(bk);
    while (!title.empty() && title.back() == ' ') title.pop_back();
    title = safe_name(title);
    auto named = [&](const std::string& ext) {
        name = (title.empty() ? std::string("sieve-") + media_name(media()) : title) + ext;
    };
    if (bk.model && model_space_)
    {
        named(".obj");
        const std::string obj = model_space_->to_obj(*bk.model);
        return std::vector<uint8_t>(obj.begin(), obj.end());
    }
    if (bk.parts)
    {
        named(".txt");
        std::string text = utf8_encode(line().space.text_of(bk.parts->title));
        while (!text.empty() && text.back() == ' ') text.pop_back();
        text += "\n\n";
        for (size_t i = 0; i < bk.parts->pages.size(); ++i) text += utf8_encode(line().space.text_of(bk.parts->pages[i])) + "\n\n";
        return std::vector<uint8_t>(text.begin(), text.end());
    }
    switch (line().kind)
    {
    case LineKind::Text: named(".txt"); break;
    case LineKind::Audio: named(".mid"); break;
    default: named(".png"); break;
    }
    return cli::unit_file(line(), bk.unit, 1);
}

void Hallway::jump_kind()
{
    if (!in_hand_) return;
    const Book bk = *in_hand_;
    if (withheld(bk))
    {
        message(tr("vault.withheld"));
        return;
    }
    try
    {
        if (on_binary())
        {
            if (bk.is_file) open_as_kind(file_of(bk));
            return;
        }
        std::string name;
        const std::vector<uint8_t> bytes = item_file(bk, name);
        cli::vault::check_bytes(bytes, "the file"); // the vault: refused before it has a place
        const std::string kind = file_kind(bytes, bytes.size());
        const int from = li_;
        // A file longer than the binary line makes the line long enough for it (only the room you
        // stand in keeps its pictures past 64 KB, as the File Locator does past the budget).
        if (bytes.size() > binary_space_->max_bytes())
        {
            if (bytes.size() > 65536) set_thin(true);
            set_binary_length(bytes.size());
        }
        drop_in_hand();
        set_line(kBinaryLine);
        binary_from_ = from; // its door leads back to the line the item came from
        walked_names_[cli::sha256_hex(bytes)] = name;
        // The item's own title, where it has one: every line's titles are the same space.
        go_to_file(bytes, true, bk.title.empty() ? nullptr : &bk.title);
        if (!refused_) message(trf("msg.jump.file", {kind, std::to_string(bytes.size())}));
    }
    catch (const std::exception& e)
    {
        message(trf("msg.jump.failed", {e.what()}));
    }
}

// A file from the binary line opened on the line that holds its kind, fitted to it as T fits
// what is warped in (the report says what changed); a text file's units become a trail (N / B).
void Hallway::open_as_kind(const std::vector<uint8_t>& bytes)
{
    const std::string kind = file_kind(bytes, bytes.size());
    int to = -1;
    if (kind == "TXT") to = 0;
    else if (kind == "PNG" || kind == "JPG" || kind == "GIF" || kind == "BMP") to = 1;
    else if (kind == "MID") to = 2;
    else if (kind == "BOOK") to = kBooksLine;
    if (to < 0)
    {
        message(trf("msg.jump.no_line", {kind}));
        return;
    }
    if (to == kBooksLine)
    {
        const std::string text(bytes.begin(), bytes.end());
        const sieve::cli::Book record = parse_book(text);
        const auto decoded = decode_book(record);
        if (book_id(decoded) != record.id) throw std::runtime_error("the book's content does not match its id");
        const BookSpace::Parts parts = record_parts(decoded, *books_);
        drop_in_hand();
        set_line(kBooksLine);
        go_to_book(parts, true);
        trail_.clear();
        message(trf("msg.jump.opened", {kind, tr(theme().key), "book record"}));
        return;
    }
    std::vector<std::vector<uint32_t>> units;
    std::string report;
    if (to == 1)
    {
        std::vector<RgbaImage> frames = decode_image_frames(bytes.data(), bytes.size(), kind);
        if (kind == "GIF" && frames.size() > 1) to = 3; // an animation: the video line
        const Line& l = lines_[size_t(to)];
        if (to == 1) frames.resize(1);
        ImageCanonReport r;
        units.push_back(canonicalise_image(frames, l.image, &r));
        report = std::string(kImageCanonVersion) + ": " + std::to_string(r.source_width) + "x" + std::to_string(r.source_height) + " -> " +
                 l.image.symbols_id();
        if (cli::unit_withheld(l, units[0])) throw cli::VaultWithheld("withheld by the vault");
    }
    else if (to == 2)
    {
        const Line& l = lines_[2];
        const sieve::NoteSet set = note_set_of(l.space.symbols_id());
        std::vector<std::string> notes;
        const std::string notation = midi_to_notation(bytes, set, &notes);
        const NotesCanonResult c = set.legacy ? canonicalise_notes(notation, l.space.unit_length())
                                              : canonicalise_notes2(notation, set, l.space.unit_length() / set.voices);
        units = c.units;
        report = std::string(set.legacy ? kNotesCanonVersion : kNotes2CanonVersion) + ": " + std::to_string(c.events) + " event(s), " +
                 std::to_string(c.units.size()) + " unit(s)";
        for (const std::string& n : notes) report += "; " + n;
        for (const auto& u : units)
            if (cli::unit_withheld(l, u)) throw cli::VaultWithheld("withheld by the vault");
    }
    else
    {
        Args a;
        a.opts["line"] = "text";
        a.positional = {std::string(bytes.begin(), bytes.end())};
        const WarpInput w = read_warp_input(lines_[0], a);
        units = w.units;
        report = w.report.front();
    }
    if (units.empty()) throw std::invalid_argument(tr("msg.warp.empty"));
    drop_in_hand();
    set_line(to);
    trail_ = units;
    trail_index_ = 0;
    go_to_unit(trail_[0], true);
    message(trf("msg.jump.opened", {kind, tr(theme().key), report}));
}

} // namespace hallway::hall
