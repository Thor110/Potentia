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

#include "cli/image_io.hpp"
#include "cli/lines.hpp"
#include "cli/locate.hpp"
#include "cli/media_decode.hpp"
#include "cli/vault.hpp"
#include "sieve/filekind.hpp"

#include <fstream>

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// A path's extension in lower case (".TRACK" is a track's record too).
std::string lower_ext(const fs::path& p)
{
    const std::u8string e = p.extension().u8string();
    std::string out(e.begin(), e.end());
    for (char& c : out) c = char(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

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

void SDLCALL save_chosen(void* user, const char* const* files, int filter)
{
    auto* h = static_cast<Hallway*>(user);
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    h->item_save_chosen(files[0], filter);
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

void Hallway::item_save_chosen(const std::string& path, int filter)
{
    std::lock_guard<std::mutex> lock(save_mx_);
    save_pending_ = path;
    save_filter_ = filter;
}

// F: asks where to save the item in hand.
void Hallway::save_in_hand()
{
    const std::string name = prepare_save();
    if (name.empty()) return;
    const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    const std::string start = docs ? std::string(docs) + name : name;
    // Every format the item can be saved as, its own first: typing an extension chooses one too.
    save_filter_text_.clear();
    save_filters_.clear();
    for (const cli::ExportFormat& f : save_formats_)
    {
        save_filter_text_.push_back(f.name);
        save_filter_text_.push_back(f.ext.substr(1));
    }
    for (size_t i = 0; i < save_formats_.size(); ++i)
        save_filters_.push_back({save_filter_text_[2 * i].c_str(), save_filter_text_[2 * i + 1].c_str()});
    SDL_ShowSaveFileDialog(save_chosen, this, window_, save_filters_.empty() ? nullptr : save_filters_.data(), int(save_filters_.size()),
                           start.c_str());
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
            case LineKind::Audio: ext = on_sound() ? ".wav" : ".mid"; break;
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
    // An item of the image, video or audio line can be saved in other formats through ffmpeg.
    save_formats_.clear();
    if (!bk.is_file && !bk.model && !bk.parts && line().kind != LineKind::Text) save_formats_ = cli::export_formats(line());
    // A track or movie can also be saved whole, as a record that J opens onto its shelf.
    if (on_composition() && !bk.unit.empty()) save_formats_.push_back({record_ext(), tr("save.record"), ""});
    return name;
}

// A track's or movie's record: ".track" or ".movie".
std::string Hallway::record_ext() const
{
    std::string id = kDimensions[li_].id;
    if (!id.empty() && id.back() == 's') id.pop_back();
    return "." + id;
}

// The item in hand as a record (cli/book.hpp: composition_record), in the ordering in use (guided
// has none here): its cover and title when not blank, and its units.
std::string Hallway::composition_record_of(const Book& bk) const
{
    const Composition& c = comp();
    CompositionSpace::Parts p;
    p.cover = bk.cover;
    p.title = bk.title;
    p.units = split_units(bk.unit, c.strands, c.space->units());
    std::optional<Line> title;
    if (c.space->title_space())
    {
        title = unit_line(LineKind::Text); // the titles are written in the pages line's alphabet
        title->space = *c.space->title_space();
        title->guided.reset();
    }
    const sieve::cli::Book record = sieve::cli::composition_record(unit_line(LineKind::Image), title ? &*title : nullptr,
                                                                   unit_line(*kDimensions[li_].composes), p,
                                                                   mode_ == AddressMode::Positional ? "positional" : "scrambled");
    return sieve::cli::serialise_book(record);
}

// The viewer's F: what the view shows, saved. The thing itself is saved as F on the item page saves
// it; the picture on the item and a cover as a PNG at their own pixels; a title as text.
std::optional<std::pair<std::vector<uint8_t>, std::string>> Hallway::view_file()
{
    if (!in_hand_ || view_kinds_.empty()) return std::nullopt;
    const Book& bk = *in_hand_;
    switch (view_kinds_[view_at_])
    {
    case ViewKind::Raw: return std::nullopt;
    case ViewKind::Title:
    {
        std::string t = bk.parts ? utf8_encode(unit_line(LineKind::Text).space.text_of(bk.parts->title)) : title_text(bk);
        while (!t.empty() && t.back() == ' ') t.pop_back();
        return std::make_pair(std::vector<uint8_t>(t.begin(), t.end()), std::string("-title.txt"));
    }
    case ViewKind::Picture:
    case ViewKind::Cover:
    {
        if (view_job_.valid()) view_.px = view_job_.get(); // the picture on the item, waited for
        view_dirty_ = true;
        if (view_.px.empty()) return std::nullopt;
        // The frame shown, at one pixel a pixel.
        const size_t n = size_t(view_.pw) * view_.ph, base = size_t(view_.frame) * n;
        std::vector<Rgb> rgb(n);
        for (size_t i = 0; i < n && base + i < view_.px.size(); ++i)
        {
            const uint32_t c = view_.px[base + i];
            rgb[i] = Rgb{uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)};
        }
        const std::string png = cli::encode_png(view_.pw, view_.ph, rgb, 1);
        return std::make_pair(std::vector<uint8_t>(png.begin(), png.end()),
                              std::string(view_kinds_[view_at_] == ViewKind::Cover ? "-cover.png" : "-picture.png"));
    }
    }
    return std::nullopt;
}

// F in the viewer: asks where to save what is shown.
void Hallway::save_view()
{
    std::string name = prepare_save(); // the item's own name, and a copy of it
    if (name.empty()) return;
    auto file = view_file();
    if (!file)
    {
        if (view_kinds_.empty() || view_kinds_[view_at_] == ViewKind::Raw) save_in_hand();
        else message(tr("view.drawing"));
        return;
    }
    if (!save_ext_.empty() && name.size() > save_ext_.size()) name.resize(name.size() - save_ext_.size());
    save_blob_ = std::move(file->first);
    save_ext_ = file->second.substr(file->second.rfind('.'));
    name += file->second;
    const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    const std::string start = docs ? std::string(docs) + name : name;
    SDL_ShowSaveFileDialog(save_chosen, this, window_, nullptr, 0, start.c_str());
}

// For testing without a person (--save-view PATH): the same, to PATH, without the dialog.
void Hallway::save_view_to(const std::string& path)
{
    if (prepare_save().empty()) return;
    if (auto file = view_file())
    {
        save_blob_ = std::move(file->first);
        save_ext_ = file->second.substr(file->second.rfind('.'));
    }
    item_save_chosen(path);
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
    int filter = -1;
    {
        std::lock_guard<std::mutex> lock(save_mx_);
        filter = save_filter_;
    }
    // No extension typed: the format of the filter chosen, else the item's own.
    if (from_u8(*to).extension().empty() && filter >= 0 && size_t(filter) < save_formats_.size()) *to += save_formats_[size_t(filter)].ext;
    const Book bk = std::move(*save_item_);
    save_item_.reset();
    std::optional<std::vector<uint8_t>> blob;
    blob.swap(save_blob_);
    fs::path path = from_u8(*to);
    if (!save_ext_.empty() && path.extension().empty()) path += from_u8(save_ext_);
    const std::string shown = [&] {
        const std::u8string s = path.filename().u8string();
        return std::string(s.begin(), s.end());
    }();
    try
    {
        if (withheld(bk)) throw cli::VaultWithheld("withheld by the vault");
        if (blob) // a view's picture or title, made when F was pressed
        {
            write_all(path, blob->data(), blob->size());
            message(trf("hand.saved", {shown}));
            return;
        }
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
        else if (on_composition() && lower_ext(path) == record_ext())
        {
            const std::string record = composition_record_of(bk);
            write_all(path, record.data(), record.size());
        }
        else
        {
            const std::u8string s = path.u8string();
            cli::save_unit(line(), bk.unit, std::string(s.begin(), s.end()), 16, export_fps_);
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
    case LineKind::Audio: named(on_sound() ? ".wav" : ".mid"); break;
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
        // stand in keeps its pictures past a large file, menu.hpp, as the File Locator does past the
        // budget).
        const bool longer = bytes.size() > binary_space_->max_bytes();
        if (longer && bytes.size() > large_file_bytes()) set_thin(true, kBinaryLine);
        clear_faces();
        const Space::Digits title = bk.title; // the item is let go below
        drop_in_hand();
        set_line(kBinaryLine);
        binary_from_ = from; // its door leads back to the line the item came from
        // The rest takes seconds for a file of megabytes: on a worker, while the window says so.
        busy(tr("locating"), [&] {
            if (longer) set_binary_length(bytes.size());
            walked_names_[cli::sha256_hex(bytes)] = name;
            // The item's own title, where it has one: every line's titles are the same space.
            go_to_file(bytes, true, title.empty() ? nullptr : &title);
            warm_room();
        });
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
    const bool on_pcm = sieve::is_pcm_symbols(unit_line(LineKind::Audio).space.symbols_id());
    // `to` is the door of the line the file goes to.
    const int text_line = line_of(LineKind::Text), image_line = line_of(LineKind::Image);
    const int audio_line = line_of(LineKind::Audio), video_line = line_of(LineKind::Video);
    int to = -1;
    if (kind == "TXT") to = text_line;
    else if (kind == "PNG" || kind == "JPG" || kind == "GIF" || kind == "BMP") to = image_line;
    // Sound files on the audio line when it holds sound itself (pcm), MIDI when it holds notes.
    else if (kind == "MID" || kind == "WAV" || kind == "MP3" || kind == "OGG" || kind == "FLAC")
    {
        if (on_pcm == (kind != "MID")) to = audio_line;
        else
        {
            message(trf(on_pcm ? "msg.jump.needs_notes" : "msg.jump.needs_pcm", {kind}));
            return;
        }
    }
    else if (kind == "BOOK") to = kBooksLine;
    // Every other picture or video format is ffmpeg's to read, if there is one: the video and picture
    // kinds the table knows, and files whose kind it does not know (MKV, WebM, TIFF, ...), which
    // fail if ffmpeg cannot read them either.
    // A file the table does not know whose first bytes are an MPEG audio or AAC frame (an MP3
    // without an ID3 tag) is sound first.
    else if ((kind == "MP4" || kind == "AVI" || kind == "WEBP" || kind == "?") && cli::find_ffmpeg())
        to = kind == "?" && on_pcm && cli::looks_like_mpeg_audio(bytes) ? audio_line : image_line;
    if (to < 0)
    {
        message(trf("msg.jump.no_line", {kind}));
        return;
    }
    if (to == kBooksLine)
    {
        // A record (sieve bind, or a track's or movie's saved from here): its sections say which
        // line it belongs on, books, tracks or movies.
        const std::string text(bytes.begin(), bytes.end());
        const sieve::cli::Book record = parse_book(text);
        const auto decoded = decode_book(record);
        if (book_id(decoded) != record.id) throw std::runtime_error("the book's content does not match its id");
        const std::string where = sieve::cli::record_line(decoded);
        const int li = where.empty() ? kBooksLine : line_named(where);
        try
        {
            if (li == kBooksLine)
            {
                const BookSpace::Parts parts = record_parts(decoded, *books_);
                drop_in_hand();
                set_line(kBooksLine);
                go_to_book(parts, true);
            }
            else
            {
                const CompositionSpace::Parts parts = sieve::cli::record_composition(decoded, *comps_[size_t(li)].space);
                drop_in_hand();
                set_line(li);
                go_to_composition(parts, true);
            }
        }
        catch (const std::invalid_argument& e)
        {
            throw std::runtime_error(li == kBooksLine ? trf("msg.book_shape", {e.what()}) : trf("msg.record_shape", {e.what(), tr(theme_of(li).key)}));
        }
        trail_.clear();
        message(trf("msg.jump.opened", {kind, tr(theme().key), li == kBooksLine ? "book record" : "record"}));
        return;
    }
    std::vector<std::vector<uint32_t>> units;
    std::string report;
    // Read frame by frame into both lines' fittings: how many frames there are decides the line.
    // A container with no picture in it (an .m4a, a video's sound alone) is sound, when the audio
    // line holds sound.
    ImageCanoniser still(unit_line(LineKind::Image).image), moving(unit_line(LineKind::Video).image);
    cli::MediaRead m;
    if (to == image_line)
    {
        try
        {
            m = cli::read_media_frames(bytes.data(), bytes.size(), kind, std::max<uint32_t>(2, unit_line(LineKind::Video).image.frames), [&](const RgbaImage& f) {
                still.add(f);
                moving.add(f);
            });
        }
        catch (const std::exception&)
        {
            if (!on_pcm || kind == "PNG" || kind == "JPG" || kind == "GIF" || kind == "BMP") throw;
            to = audio_line;
        }
    }
    if (to == image_line)
    {
        ImageCanonReport r;
        std::vector<uint32_t> s = still.finish(&r);
        if (r.source_frames > 1) // an animation: the video line
        {
            to = video_line;
            s = moving.finish(&r);
        }
        const Line& l = line_at(to);
        units.push_back(std::move(s));
        report = std::string(kImageCanonVersion) + ": " + std::to_string(r.source_width) + "x" + std::to_string(r.source_height) + " -> " +
                 l.image.symbols_id();
        if (m.decoder != "stb_image") report += "; decoded by " + m.decoder;
        if (!m.complaints.empty()) report += "; ffmpeg reported: " + m.complaints;
        if (cli::unit_withheld(l, units[0])) throw cli::VaultWithheld("withheld by the vault");
    }
    else if (to == audio_line && on_pcm)
    {
        const Line& l = unit_line(LineKind::Audio);
        const sieve::PcmFormat f = sieve::pcm_format_of(l.space.symbols_id());
        std::optional<sieve::PcmCanoniser> c;
        const cli::AudioRead m = cli::read_media_audio(
            bytes.data(), bytes.size(), kind,
            [&](uint32_t rate, uint32_t channels) { c.emplace(f, l.space.unit_length() / f.channels, rate, channels); },
            [&](std::span<const int32_t> b) { c->add(b); });
        const sieve::PcmCanonResult r = c->finish();
        units = r.units;
        report = std::string(sieve::kPcmCanonVersion) + ": " + std::to_string(r.source_frames) + " sample(s) at " + std::to_string(r.source_rate) +
                 ", " + std::to_string(r.source_channels) + " channel(s) -> " + f.id() + ", " + std::to_string(units.size()) + " unit(s)";
        if (m.decoder != "sieve-wav") report += "; decoded by " + m.decoder;
        if (!m.complaints.empty()) report += "; ffmpeg reported: " + m.complaints;
        for (const auto& u : units)
            if (cli::unit_withheld(l, u)) throw cli::VaultWithheld("withheld by the vault");
    }
    else if (to == audio_line && sieve::is_notes3_symbols(unit_line(LineKind::Audio).space.symbols_id()))
    {
        // Open-ended notes read a MIDI file as warp does (canon-notes-v3, MIDI read back first).
        Args a;
        a.opts["line"] = "audio";
        a.positional = {std::string(bytes.begin(), bytes.end())};
        const WarpInput w = read_warp_input(unit_line(LineKind::Audio), a);
        units = w.units;
        report = w.report.front();
        for (size_t i = 1; i < w.report.size(); ++i) report += "; " + w.report[i];
    }
    else if (to == audio_line)
    {
        const Line& l = unit_line(LineKind::Audio);
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
        const WarpInput w = read_warp_input(unit_line(LineKind::Text), a);
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
