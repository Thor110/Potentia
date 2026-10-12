// The CHAT tab (IDEAS §19.13): the last tab of the hand panel with a model of the AI line in hand,
// beside ITEM, COST and SORT; Enter (or C round to it) opens it. The conversation with the model
// scrolls above, the box to type in below; files given to it and pictures a picture model makes
// are cards in the conversation, each with its place on the lines and buttons to go there.
//
// - What you say is fed to the model as bytes, a newline after it, and the model writes back until
//   it writes a newline itself (Hallway::talk), as the hand panel's conversation always was.
// - The plus button (or Ctrl+O) gives it files: each is read and weighed as the File Locator weighs
//   it (its address on the binary line, and on its own line where it is exactly an item of one),
//   and its first bytes, up to half the model's positions, are given to the model to read. The
//   card's buttons: Go to it (the file on the binary line), J (opened on the line of its kind, as J
//   opens a file in hand) and Save.
// - DRAW (or Ctrl+D) draws a figure with the picture model chosen (a trained diffusion run, from
//   the AI Training Harness's folders; Ctrl+P another): its k-th picture, the same one the harness
//   and `sieve diffuse --make` make (diffusion_run.hpp nth_picture). Its card shows it, with its
//   address on the image line, Go to it and Save (a PNG); and the model is given its rows, a
//   character a cell, so it reads the picture as it reads a file.
//
// Keys: Enter sends, Shift+Enter starts a new line, Ctrl+O attaches, Ctrl+D draws, Ctrl+P changes
// the picture model, Ctrl+G, Ctrl+J and Ctrl+S act on the newest card, Page Up and Page Down (or
// the wheel) scroll, Esc goes back to ITEM (where C reaches the other tabs). While it shows, the
// keys are its box's. The conversation is kept while the model is held.

#include "hallway.hpp"

#include "gpu_memory.hpp"

#include "cli/locate.hpp"
#include "cli/media_decode.hpp"
#include "cli/train.hpp"
#include "cli/vault.hpp"
#include "sieve/filekind.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

// The most a reply may be, in bytes.
constexpr size_t kTalkReply = 80;
// Room kept for what you say next after a file or picture is given (the reply's room besides).
constexpr size_t kTalkRoom = 64;
// Text in the conversation: messages at scale 2, the rest at scale 1.
constexpr float kBig = 2, kSmall = 1, kBigLine = 20, kSmallLine = 14, kPad = 10, kButtonH = 22;
// A file card's thumbnail: its longer side, in pixels (and the most kept of its picture), and the
// lines of a text shown.
constexpr uint32_t kThumb = 96;
constexpr size_t kPreviewLines = 4;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

void write_bytes(const fs::path& p, const std::vector<uint8_t>& b)
{
    std::ofstream out(p, std::ios::binary);
    if (!out || !out.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()))) throw std::runtime_error("cannot write " + u8(p));
}

// Lines apart at newlines, empty ones kept (split_lines drops them).
std::vector<std::string> paragraphs(const std::string& s)
{
    std::vector<std::string> out{std::string()};
    for (const char c : s)
        if (c == '\n') out.emplace_back();
        else out.back().push_back(c);
    return out;
}

void SDLCALL chat_files_chosen(void* user, const char* const* files, int)
{
    if (!files) return; // the dialog failed
    std::vector<std::string> paths;
    for (const char* const* f = files; *f; ++f) paths.push_back(*f);
    if (!paths.empty()) static_cast<Hallway*>(user)->chat_picked(paths);
}

void SDLCALL chat_save_chosen(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return; // cancelled
    static_cast<Hallway*>(user)->chat_saved(files[0]);
}

// An entry of the conversation that is something said (or a note).
template <class Entry>
Entry entry_of(typename Entry::Kind kind, std::string text)
{
    Entry e;
    e.kind = kind;
    e.text = std::move(text);
    return e;
}

// A picture made small: its longer side at most `side`, each pixel the average of those it
// covers. A picture already that small is kept as it is.
RgbaImage small_copy(const RgbaImage& f, uint32_t side)
{
    if (f.width <= side && f.height <= side) return f;
    const double k = double(std::max(f.width, f.height)) / side;
    RgbaImage out;
    out.width = std::max<uint32_t>(1, uint32_t(f.width / k)), out.height = std::max<uint32_t>(1, uint32_t(f.height / k));
    out.rgba.resize(size_t(out.width) * out.height * 4);
    for (uint32_t y = 0; y < out.height; ++y)
        for (uint32_t x = 0; x < out.width; ++x)
        {
            const uint32_t x0 = uint32_t(x * k), x1 = std::max(x0 + 1, std::min(f.width, uint32_t((x + 1) * k)));
            const uint32_t y0 = uint32_t(y * k), y1 = std::max(y0 + 1, std::min(f.height, uint32_t((y + 1) * k)));
            uint64_t sum[4] = {0, 0, 0, 0};
            for (uint32_t v = y0; v < y1; ++v)
                for (uint32_t u = x0; u < x1; ++u)
                    for (int c = 0; c < 4; ++c) sum[c] += f.rgba[(size_t(v) * f.width + u) * 4 + size_t(c)];
            const uint64_t n = uint64_t(x1 - x0) * (y1 - y0);
            for (int c = 0; c < 4; ++c) out.rgba[(size_t(y) * out.width + x) * 4 + size_t(c)] = uint8_t(sum[c] / n);
        }
    return out;
}

// A line of a text file as the font can draw it: its own characters (the font falls back per
// glyph), control characters a middle dot; a line that is not UTF-8 (or is cut mid-character) is
// drawn byte by byte, as the model's words are (Hallway::printable).
std::string text_line(const std::string& bytes)
{
    try
    {
        std::u32string out;
        for (const char32_t c : sieve::utf8_decode(bytes)) out.push_back(c < 0x20 || (c >= 0x7F && c < 0xA0) ? U'\u00B7' : c);
        return sieve::utf8_encode(out);
    }
    catch (const std::invalid_argument&)
    {
        return Hallway::printable(bytes);
    }
}

// A number with its thousands apart: 12,345.
std::string grouped(uint64_t n)
{
    std::string s = std::to_string(n);
    for (int i = int(s.size()) - 3; i > 0; i -= 3) s.insert(size_t(i), ",");
    return s;
}

} // namespace

// ---- the conversation with the model

void Hallway::talk_ready()
{
    if (!in_hand_ || !in_hand_->ai) return;
    if (talk_model_ && talk_hex_ == in_hand_->hex) return;
    talk_model_ = std::make_unique<sieve::llm::Model>(ai_space_->config(), ai_space_->tensors_of(*in_hand_->ai), 1);
    talk_.clear();
    chat_.clear();
    chat_clear_thumbs();
    chat_scroll_ = 0;
    talk_hex_ = in_hand_->hex;
}

// The AI line: what you say is fed to the model in hand as bytes, a newline after it, and the model
// writes back until it writes a newline itself (at most kTalkReply bytes). It is a model like any
// other on the line, mostly noise: what comes back is what its weights make of what it was given.
void Hallway::talk(const std::string& said)
{
    if (!in_hand_ || !in_hand_->ai) return;
    talk_ready();
    const std::string text = said + "\n";
    if (talk_model_->position() + text.size() + kTalkReply >= talk_model_->config().max_positions)
    {
        talk_model_->reset();
        talk_ += tr("talk.full") + "\n";
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("talk.full")));
    }
    sieve::llm::Sampling s;
    s.temperature = 0.8f;
    s.top_p = 1.0f;
    // Its choices seeded by the conversation so far (the first eight bytes of its SHA-256), so the same
    // words to the same model at the same point get the same answer on every machine.
    const auto digest = sieve::Sha256::hash(talk_ + said);
    s.seed = 0;
    for (int i = 0; i < 8; ++i) s.seed = (s.seed << 8) | digest[size_t(i)];
    sieve::llm::Sampler sampler(s);
    const std::string reply = sieve::llm::continue_text(*talk_model_, *ai_tokenizer_, text, sampler, kTalkReply, {}, uint32_t('\n'));
    talk_ += trf("talk.you", {said}) + "\n" + trf("talk.it", {reply}) + "\n";
    chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::You, said));
    chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::It, reply));
    chat_scroll_ = 0;
}

// Given to the model with no reply: its first bytes, up to half its positions, then a newline. When
// that and room for what comes next do not fit after what it has read, it starts again (and says so).
size_t Hallway::talk_give(const std::vector<uint8_t>& bytes)
{
    talk_ready();
    if (!talk_model_) return 0;
    const size_t max = talk_model_->config().max_positions;
    const size_t n = std::min(bytes.size(), max / 2);
    if (talk_model_->position() + n + 1 + kTalkRoom + kTalkReply >= max)
    {
        talk_model_->reset();
        talk_ += tr("talk.full") + "\n";
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("talk.full")));
    }
    for (size_t i = 0; i < n; ++i) talk_model_->step(bytes[i]);
    talk_model_->step(uint32_t('\n'));
    return n;
}

// ---- the window

void Hallway::open_chat()
{
    if (!in_hand_ || !in_hand_->ai) return;
    talk_ready();
    hand_tab_ = chat_tab();
    if (chat_open_) return;
    chat_open_ = true;
    chat_text_.clear();
    chat_scroll_ = 0;
    // The picture models there are now (one trained since the last look is found).
    const std::string was = chat_models_.empty() ? std::string() : u8(chat_models_[size_t(chat_model_)].filename());
    chat_models_ = cli::picture_models_in(cli::training_run_dirs());
    chat_model_ = 0;
    for (size_t i = 0; i < chat_models_.size(); ++i)
        if (u8(chat_models_[i].filename()) == was) chat_model_ = int(i);
    chat_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
}

void Hallway::close_chat()
{
    if (!chat_open_) return;
    chat_open_ = false;
    if (in_hand_) hand_tab_ = 0; // back to ITEM, where C goes round the tabs again
    SDL_StopTextInput(window_);
    SDL_SetWindowRelativeMouseMode(window_, chat_had_mouse_);
}

void Hallway::chat_pick_model(int dir)
{
    if (chat_models_.empty()) return;
    const int n = int(chat_models_.size());
    chat_model_ = ((chat_model_ + dir) % n + n) % n;
}

void Hallway::chat_send()
{
    const std::string said = chat_text_;
    chat_text_.clear();
    talk(said); // an empty line too: the model goes on by itself
}

void Hallway::chat_picked(const std::vector<std::string>& paths)
{
    std::lock_guard<std::mutex> lock(chat_mx_);
    chat_pending_.insert(chat_pending_.end(), paths.begin(), paths.end());
}

void Hallway::chat_saved(const std::string& path)
{
    std::lock_guard<std::mutex> lock(chat_mx_);
    chat_save_to_ = path;
}

// A file given to the model: read, weighed and shown as a card.
void Hallway::chat_attach(const std::string& path)
{
    if (!in_hand_ || !in_hand_->ai) return;
    talk_ready();
    ChatEntry e;
    e.kind = ChatEntry::Kind::File;
    const fs::path p = from_u8(path);
    e.name = u8(p.filename());
    try
    {
        bool withheld = false;
        busy(tr("chat.reading"), [&] {
            e.bytes = cli::read_file_bytes(p);
            if (cli::vault::withheld_bytes(e.bytes))
            {
                withheld = true;
                return;
            }
            e.file_kind = sieve::file_kind(e.bytes, e.bytes.size());
            // What it looks like: a picture or video's first frame, small (the vault looking at
            // it as at any other picture); a text's first lines; anything else its first bytes.
            const bool moving = e.file_kind == "MP4" || e.file_kind == "AVI" || e.file_kind == "WEBP";
            if (e.file_kind == "PNG" || e.file_kind == "JPG" || e.file_kind == "GIF" || e.file_kind == "BMP" || (moving && cli::find_ffmpeg()))
            {
                try
                {
                    bool first = true;
                    cli::read_media_frames(e.bytes.data(), e.bytes.size(), e.name, 1, [&](const RgbaImage& f) {
                        if (!first) return;
                        first = false;
                        if (cli::vault::withheld_picture(f.rgba.data(), f.width, f.height)) withheld = true;
                        else e.thumb = small_copy(f, kThumb);
                    });
                }
                catch (const std::exception&)
                {
                    // Not readable as a picture after all: it has its first bytes instead, below.
                }
                if (withheld) return;
            }
            if (e.thumb.width == 0 && e.file_kind == "TXT")
            {
                const std::string head(e.bytes.begin(), e.bytes.begin() + std::ptrdiff_t(std::min<size_t>(e.bytes.size(), 600)));
                for (std::string l : split_lines(head))
                {
                    if (!l.empty() && l.back() == '\r') l.pop_back();
                    if (e.preview.size() < kPreviewLines && !l.empty()) e.preview.push_back(text_line(l));
                }
            }
            else if (e.thumb.width == 0)
            {
                static const char kHex[] = "0123456789abcdef";
                std::string hex;
                for (size_t k = 0; k < std::min<size_t>(e.bytes.size(), 24); ++k)
                    hex += std::string(k ? " " : "") + kHex[e.bytes[k] >> 4] + kHex[e.bytes[k] & 15];
                if (!hex.empty()) e.preview.push_back(hex + (e.bytes.size() > 24 ? " ..." : ""));
            }
            // Weighed as the File Locator weighs it, under this hallway's lines and filters.
            const cli::Weighing w = cli::weigh_files({}, {path}, locator_host().lines(), false);
            const cli::FileWeight& f = w.files.at(0);
            if (!f.error.empty()) throw std::runtime_error(f.error);
            e.item_line = f.line;
            std::string ways;
            for (size_t i = 0; i < f.ways.size(); ++i)
                ways += (i ? "; " : "") + trf(i == f.best ? "chat.way.best" : "chat.way", {f.ways[i].how, grouped(uint64_t(f.ways[i].bits + 0.5))});
            e.weighing = trf("chat.file.ways", {ways});
            if (f.line >= 0) e.weighing += "\n" + trf("chat.file.item", {tr(theme_of(line_of(LineKind(f.line))).key)});
        });
        if (withheld)
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.withheld", {e.name})));
            return;
        }
    }
    catch (const std::exception& ex)
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.unread", {e.name, ex.what()})));
        return;
    }
    e.given = talk_give(e.bytes);
    talk_ += trf("talk.file", {e.name, grouped(e.bytes.size())}) + "\n";
    chat_.push_back(std::move(e));
    chat_scroll_ = 0;
}

// A figure drawn with the picture model chosen: its next picture.
void Hallway::chat_draw()
{
    if (!in_hand_ || !in_hand_->ai) return;
    talk_ready();
    if (chat_models_.empty()) chat_models_ = cli::picture_models_in(cli::training_run_dirs());
    if (chat_models_.empty())
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("chat.figures.none")));
        return;
    }
    chat_model_ = std::clamp(chat_model_, 0, int(chat_models_.size()) - 1);
    const fs::path dir = chat_models_[size_t(chat_model_)];
    ChatEntry e;
    e.kind = ChatEntry::Kind::Picture;
    try
    {
        busy(tr("chat.drawing"), [&] {
            if (!chat_pm_ || chat_pm_->path != dir) chat_pm_ = cli::load_picture_model(dir);
            const cli::PictureModel& m = *chat_pm_;
            e.model = m.name;
            e.number = chat_drawn_[m.name] + 1;
            e.cells = m.picture(e.number);
            e.width = m.recipe.setup.width, e.height = m.recipe.setup.height, e.palette = m.recipe.palette;
            e.address = cli::image_line_of(m.recipe).address_of(e.cells, AddressMode::Positional);
        });
    }
    catch (const std::exception& ex)
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.draw_failed", {ex.what()})));
        return;
    }
    chat_drawn_[e.model] = e.number;
    // The vault looks at pictures as at any other.
    const sieve::Palette& pal = sieve::palette_by_id(e.palette);
    std::vector<uint8_t> rgba;
    for (const uint32_t c : e.cells)
    {
        const sieve::Rgb x = pal.colour(c);
        rgba.insert(rgba.end(), {x.r, x.g, x.b, 255});
    }
    if (cli::vault::withheld_picture(rgba.data(), e.width, e.height))
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.withheld", {trf("chat.figure", {std::to_string(e.number), e.model})})));
        return;
    }
    // The model reads it as its rows: a character a cell (the background a dot), a newline a row.
    const uint32_t background = pal.nearest(sieve::Rgb{255, 255, 255});
    const char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::vector<uint8_t> rows;
    for (uint32_t y = 0; y < e.height; ++y)
    {
        for (uint32_t x = 0; x < e.width; ++x)
        {
            const uint32_t c = e.cells[size_t(y) * e.width + x];
            rows.push_back(uint8_t(c == background ? '.' : pal.size() <= 2 ? '#' : kDigits[c % 36]));
        }
        if (y + 1 < e.height) rows.push_back('\n');
    }
    e.given = talk_give(rows);
    talk_ += trf("talk.figure", {std::to_string(e.number), e.model, e.address}) + "\n";
    chat_.push_back(std::move(e));
    chat_scroll_ = 0;
}

std::optional<size_t> Hallway::chat_last_card() const
{
    for (size_t i = chat_.size(); i-- > 0;)
        if (chat_[i].kind == ChatEntry::Kind::File || chat_[i].kind == ChatEntry::Kind::Picture) return i;
    return std::nullopt;
}

void Hallway::chat_act(size_t i, ChatAct act)
{
    if (i >= chat_.size()) return;
    const ChatEntry e = chat_[i]; // the window may close, and the conversation with it, below
    const bool file = e.kind == ChatEntry::Kind::File;
    if (act == ChatAct::Save)
    {
        {
            std::lock_guard<std::mutex> lock(chat_mx_);
            chat_saving_ = i;
        }
        const std::string name = file ? e.name : "figure-" + e.model + "-" + std::to_string(e.number) + ".png";
        const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
        const std::string start = docs ? std::string(docs) + name : name;
        SDL_ShowSaveFileDialog(chat_save_chosen, this, window_, nullptr, 0, start.c_str());
        return;
    }
    if (file && act == ChatAct::Go)
    {
        close_chat();
        walk_to_file(e.bytes, e.name, e.bytes.size() > binary_space_->max_bytes());
        return;
    }
    if (file && act == ChatAct::Item)
    {
        close_chat();
        try
        {
            open_as_kind(e.bytes);
        }
        catch (const std::exception& ex)
        {
            message(trf("msg.jump.failed", {ex.what()}));
        }
        return;
    }
    if (!file && act == ChatAct::Go)
    {
        // On this hallway's image line if it is the picture's size and palette; else a hallway
        // built with one that is (Request::GoToPicture).
        const Line& image = unit_line(LineKind::Image);
        if (image.image.width == e.width && image.image.height == e.height && image.image.palette && image.image.palette->id() == e.palette)
            go_to_picture(e.cells);
        else
        {
            picture_to_go_ = PictureToGo{e.width, e.height, e.palette, e.cells};
            close_chat();
            request_ = Request::GoToPicture;
        }
    }
}

void Hallway::go_to_picture(const std::vector<uint32_t>& cells)
{
    if (chat_open_) close_chat();
    if (pause_open_) close_pause();
    drop_in_hand();
    trail_.clear();
    door_back_.clear();
    set_line(line_of(LineKind::Image));
    go_to_unit(cells, true);
    message(tr("chat.went.picture"));
}

void Hallway::chat_event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        chat_text_ += e.text.text;
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        chat_scroll_ = std::max(0.0f, chat_scroll_ + e.wheel.y * 40);
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        // In the renderer's coordinates, as the buttons were drawn (the window's may differ: a
        // scaled display, or a window of another size than what is drawn).
        SDL_FPoint at{e.button.x, e.button.y};
        SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &at.x, &at.y);
        for (const auto& [r, what] : chat_buttons_)
            if (SDL_PointInRectFloat(&at, &r))
            {
                chat_act(what.first, what.second);
                return;
            }
        for (const auto& [r, which] : chat_bar_)
            if (SDL_PointInRectFloat(&at, &r))
            {
                if (which == 0) SDL_ShowOpenFileDialog(chat_files_chosen, this, window_, nullptr, 0, nullptr, true);
                else if (which == 1) chat_draw();
                else if (which == 2) chat_send();
                else chat_pick_model(1);
                return;
            }
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    const bool ctrl = e.key.mod & SDL_KMOD_CTRL, shift = e.key.mod & SDL_KMOD_SHIFT;
    switch (e.key.key)
    {
    case SDLK_ESCAPE: close_chat(); break;
    case SDLK_BACKSPACE:
        // One UTF-8 character.
        while (!chat_text_.empty() && (static_cast<unsigned char>(chat_text_.back()) & 0xC0) == 0x80) chat_text_.pop_back();
        if (!chat_text_.empty()) chat_text_.pop_back();
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (shift) chat_text_ += "\n";
        else chat_send();
        break;
    case SDLK_PAGEUP: chat_scroll_ += 300; break;
    case SDLK_PAGEDOWN: chat_scroll_ = std::max(0.0f, chat_scroll_ - 300); break;
    default:
        if (!ctrl) break;
        if (e.key.key == SDLK_V)
        {
            if (char* clip = SDL_GetClipboardText())
            {
                chat_text_ += clip;
                SDL_free(clip);
            }
        }
        else if (e.key.key == SDLK_O) SDL_ShowOpenFileDialog(chat_files_chosen, this, window_, nullptr, 0, nullptr, true);
        else if (e.key.key == SDLK_D) chat_draw();
        else if (e.key.key == SDLK_P) chat_pick_model(shift ? -1 : 1);
        else if (const auto card = chat_last_card())
        {
            if (e.key.key == SDLK_G) chat_act(*card, ChatAct::Go);
            else if (e.key.key == SDLK_J && chat_[*card].kind == ChatEntry::Kind::File) chat_act(*card, ChatAct::Item);
            else if (e.key.key == SDLK_S) chat_act(*card, ChatAct::Save);
        }
        break;
    }
}

float Hallway::chat_entry(size_t i, float x, float y, float w, bool draw)
{
    const ChatEntry& e = chat_[i];
    const SDL_Color ink = menu_ink(theme()), white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    const size_t big_cols = size_t(std::max(8.0f, w / (8 * kBig))), small_cols = size_t(std::max(8.0f, (w - 2 * kPad) / (8 * kSmall)));
    using K = ChatEntry::Kind;
    if (e.kind == K::You || e.kind == K::It || e.kind == K::Note)
    {
        float h = 0;
        if (e.kind != K::Note)
        {
            if (draw) text(x, y, tr(e.kind == K::You ? "chat.you" : "chat.it"), kSmall, e.kind == K::You ? grey : ink);
            h += kSmallLine;
        }
        const bool big = e.kind != K::Note;
        // What the model wrote is bytes: drawn as the font can (printable); what you wrote, as typed.
        for (const std::string& para : paragraphs(e.kind == K::It ? printable(e.text) : e.text))
            for (const std::string& l : wrap_words(para, big ? big_cols : small_cols))
            {
                if (draw) text(x, y + h, l, big ? kBig : kSmall, e.kind == K::Note ? grey : white);
                h += big ? kBigLine : kSmallLine;
            }
        return h + kPad;
    }
    // A card: a frame, its lines, and a row of buttons; a picture at the left (a figure, or a
    // picture or video file's thumbnail).
    struct Line
    {
        std::string text;
        float scale;
        SDL_Color colour;
    };
    std::vector<Line> lines;
    float pic_w = 0, pic_h = 0;
    std::vector<std::pair<std::string, ChatAct>> buttons{{tr("chat.go"), ChatAct::Go}};
    if (e.kind == K::File)
    {
        if (e.thumb.width > 0)
        {
            const float k = kThumb / float(std::max(e.thumb.width, e.thumb.height));
            pic_w = float(e.thumb.width) * k, pic_h = float(e.thumb.height) * k;
        }
        const float tw = w - 2 * kPad - (pic_w > 0 ? pic_w + kPad : 0);
        const size_t cols = size_t(std::max(8.0f, tw / (8 * kSmall)));
        lines.push_back({fit(e.name, tw, kBig), kBig, white});
        lines.push_back({trf("chat.file.what", {e.file_kind, grouped(e.bytes.size())}), kSmall, ink});
        for (const std::string& para : paragraphs(e.weighing))
            for (const std::string& l : wrap_words(para, cols)) lines.push_back({l, kSmall, ink});
        lines.push_back({trf("chat.given", {grouped(e.given)}), kSmall, ink});
        for (const std::string& l : e.preview) lines.push_back({fit(l, tw, kSmall), kSmall, grey});
        if (e.item_line >= 0 || e.file_kind != "?") buttons.push_back({tr("chat.item"), ChatAct::Item});
    }
    else
    {
        pic_w = pic_h = std::min(120.0f, std::max(40.0f, float(std::max(e.width, e.height)) * 8));
        const size_t cols = size_t(std::max(8.0f, (w - 3 * kPad - pic_w) / (8 * kSmall)));
        lines.push_back({trf("chat.figure", {std::to_string(e.number), e.model}), kBig, white});
        lines.push_back({trf("chat.figure.what", {std::to_string(e.width), std::to_string(e.height), e.palette}), kSmall, ink});
        for (const std::string& l : wrap(trf("chat.figure.address", {e.address}), cols)) lines.push_back({l, kSmall, ink});
        lines.push_back({trf("chat.given.rows", {grouped(e.given)}), kSmall, ink});
    }
    buttons.push_back({tr("chat.save"), ChatAct::Save});
    float text_h = 0;
    for (const Line& l : lines) text_h += l.scale == kBig ? kBigLine : kSmallLine;
    const float body = std::max(text_h, pic_h);
    const float h = kPad + body + 6 + kButtonH + kPad;
    if (!draw) return h + kPad;
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    const SDL_FRect frame{x, y, w, h};
    SDL_RenderRect(r_, &frame);
    float ty = y + kPad;
    const float tx = x + kPad + (pic_w > 0 ? pic_w + kPad : 0);
    if (e.kind == K::Picture)
    {
        ImageFormat f;
        f.width = e.width, f.height = e.height, f.palette = &sieve::palette_by_id(e.palette);
        draw_pixels(e.cells, f, x + kPad, y + kPad, pic_w, 0);
    }
    else if (pic_w > 0)
    {
        SDL_Texture*& tex = chat_thumbs_[i];
        if (!tex)
        {
            tex = gpu::create(r_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, int(e.thumb.width), int(e.thumb.height));
            if (tex)
            {
                SDL_UpdateTexture(tex, nullptr, e.thumb.rgba.data(), int(e.thumb.width * 4));
                // A picture smaller than the thumbnail is drawn in sharp cells, as the shelves draw it.
                SDL_SetTextureScaleMode(tex, e.thumb.width < kThumb && e.thumb.height < kThumb ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
            }
        }
        const SDL_FRect at{x + kPad, y + kPad, pic_w, pic_h};
        if (tex) SDL_RenderTexture(r_, tex, nullptr, &at);
        SDL_SetRenderDrawColor(r_, grey.r, grey.g, grey.b, 255);
        const SDL_FRect border{at.x - 1, at.y - 1, at.w + 2, at.h + 2};
        SDL_RenderRect(r_, &border);
    }
    for (const Line& l : lines)
    {
        text(tx, ty, l.text, l.scale, l.colour);
        ty += l.scale == kBig ? kBigLine : kSmallLine;
    }
    float bx = tx;
    const float by = y + kPad + body + 6;
    for (const auto& [label, act] : buttons)
    {
        const float bw = text_width(label, kSmall) + 16;
        const SDL_FRect b{bx, by, bw, kButtonH};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
        SDL_RenderFillRect(r_, &b);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &b);
        text(bx + 8, by + 7, label, kSmall, white);
        chat_buttons_.push_back({b, {i, act}});
        bx += bw + 8;
    }
    return h + kPad;
}

void Hallway::chat_clear_thumbs()
{
    for (auto& [i, tex] : chat_thumbs_) gpu::destroy(tex);
    chat_thumbs_.clear();
}

void Hallway::chat_poll()
{
    // The CHAT tab keeps the keys only while it shows, with a model in hand.
    if (chat_open_ && !(in_hand_ && in_hand_->ai && hand_tab_ == chat_tab()))
    {
        chat_open_ = false;
        SDL_StopTextInput(window_);
        SDL_SetWindowRelativeMouseMode(window_, chat_had_mouse_);
    }
    // What the dialogs chose, here on the main thread: files to attach, and where to save a card.
    std::vector<std::string> pending;
    std::optional<std::string> save_to;
    std::optional<size_t> saving;
    {
        std::lock_guard<std::mutex> lock(chat_mx_);
        pending.swap(chat_pending_);
        save_to.swap(chat_save_to_);
        saving = chat_saving_;
    }
    for (const std::string& p : pending) chat_attach(p);
    if (save_to && saving && *saving < chat_.size())
    {
        const ChatEntry& e = chat_[*saving];
        try
        {
            if (e.kind == ChatEntry::Kind::File) write_bytes(from_u8(*save_to), e.bytes);
            else
            {
                fs::path path = from_u8(*save_to);
                if (path.extension().empty()) path += ".png";
                const sieve::Palette& pal = sieve::palette_by_id(e.palette);
                std::vector<sieve::Rgb> px;
                for (const uint32_t c : e.cells) px.push_back(pal.colour(c));
                const std::string png = cli::encode_png(e.width, e.height, px, 1);
                write_bytes(path, std::vector<uint8_t>(png.begin(), png.end()));
            }
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.saved", {*save_to})));
        }
        catch (const std::exception& ex)
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.save_failed", {ex.what()})));
        }
    }
}

void Hallway::draw_chat(float x, float top, float w, float bottom)
{
    const SDL_Color ink = menu_ink(theme()), white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    const float px = x, pw = w;
    chat_buttons_.clear();
    chat_bar_.clear();

    // At the top: the model in hand, and the picture model chosen.
    const AiShape& sh = ai_space_->shape();
    text(x, top,
         fit(trf("chat.with", {short_address(talk_hex_),
                               trf(sh.layers == 1 ? "ai.summary.one" : "ai.summary", {std::to_string(sh.layers), std::to_string(sh.width), std::to_string(sh.heads),
                                                                                      std::to_string(ai_space_->weights()), std::to_string(sh.bits)})}),
             w, 1),
         1, grey);
    const std::string figures = chat_models_.empty() ? tr("chat.figures.none")
                                                     : trf(chat_models_.size() > 1 ? "chat.figures.more" : "chat.figures",
                                                           {u8(chat_models_[size_t(std::clamp(chat_model_, 0, int(chat_models_.size()) - 1))].filename())});
    text(x, top + 16, fit(figures, w, 1), 1, grey);
    if (chat_models_.size() > 1) chat_bar_.push_back({SDL_FRect{x, top + 12, std::min(w, text_width(figures, 1)), 16}, 3});

    // The box to type in, its lines wrapped, the last few shown; the buttons round it.
    const std::vector<std::string> keys = wrap_words(tr("chat.keys"), size_t(std::max(8.0f, w / 8)));
    const float button = 36, keys_y = bottom - 8 - kSmallLine * float(keys.size());
    const size_t cols = size_t(std::max(8.0f, (w - 2 * button - 160) / (8 * kBig)));
    std::vector<std::string> typed;
    for (const std::string& para : paragraphs(chat_text_ + ((SDL_GetTicks() / 400) % 2 ? "_" : " ")))
        for (const std::string& l : wrap(para, cols)) typed.push_back(l);
    constexpr size_t kBoxLines = 6;
    if (typed.size() > kBoxLines) typed.erase(typed.begin(), typed.end() - kBoxLines);
    const float box_h = 12 + kBigLine * float(std::max<size_t>(1, typed.size()));
    const float box_y = keys_y - 10 - box_h;
    auto bar_button = [&](float bx, float by, float bw, float bh, const std::string& label, float scale, int which) {
        const SDL_FRect b{bx, by, bw, bh};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
        SDL_RenderFillRect(r_, &b);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &b);
        text(bx + (bw - text_width(label, scale)) / 2, by + (bh - 8 * scale) / 2, label, scale, white);
        chat_bar_.push_back({b, which});
    };
    bar_button(x, box_y + box_h - button, button, button, "+", 3, 0);
    const float send_w = 72, draw_w = 72;
    bar_button(x + w - send_w, box_y + box_h - button, send_w, button, tr("chat.send"), 1, 2);
    bar_button(x + w - send_w - 8 - draw_w, box_y + box_h - button, draw_w, button, tr("chat.draw"), 1, 1);
    const float tx = x + button + 8, tw = w - button - 8 - send_w - draw_w - 16;
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    const SDL_FRect box{tx, box_y, tw, box_h};
    SDL_RenderRect(r_, &box);
    for (size_t k = 0; k < typed.size(); ++k) text(tx + 8, box_y + 8 + kBigLine * float(k), typed[k], kBig, white);
    for (size_t k = 0; k < keys.size(); ++k) text(x, keys_y + kSmallLine * float(k), keys[k], kSmall, grey);

    // The conversation, newest at the foot, scrolled up by chat_scroll_.
    const float area_top = top + 46, area_bottom = box_y - 12;
    std::vector<float> hs(chat_.size());
    float total = 0;
    for (size_t i = 0; i < chat_.size(); ++i) total += hs[i] = chat_entry(i, x, 0, w, false);
    chat_scroll_ = std::clamp(chat_scroll_, 0.0f, std::max(0.0f, total - (area_bottom - area_top)));
    if (chat_.empty()) text(x, area_top, fit(tr("chat.empty"), w, 1), 1, grey);
    const SDL_Rect clip{int(px), int(area_top), int(pw), int(area_bottom - area_top)};
    SDL_SetRenderClipRect(r_, &clip);
    float y = area_bottom + chat_scroll_;
    for (size_t i = chat_.size(); i-- > 0;)
    {
        y -= hs[i];
        if (y > area_bottom) continue;
        if (y + hs[i] < area_top) break;
        chat_entry(i, x, y, w, true);
    }
    SDL_SetRenderClipRect(r_, nullptr);
    // Buttons scrolled out of the conversation are not there to press.
    std::erase_if(chat_buttons_, [&](const auto& b) { return b.first.y < area_top || b.first.y + b.first.h > area_bottom; });
    // Where there is more than shows, a word at that edge.
    if (chat_scroll_ > 0) text(x + w - text_width(tr("chat.more"), 1), area_bottom - 12, tr("chat.more"), 1, ink);
    if (total - chat_scroll_ > area_bottom - area_top) text(x + w - text_width(tr("chat.more.above"), 1), area_top - 14, tr("chat.more.above"), 1, ink);
}

} // namespace hallway::hall
