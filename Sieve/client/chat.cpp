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

#include "dialogs.hpp"
#include "gpu_memory.hpp"

#include "cli/dictionaries.hpp"
#include "cli/locate.hpp"
#include "cli/media_decode.hpp"
#include "cli/train.hpp"
#include "cli/vault.hpp"
#include "sieve/filekind.hpp"
#include "sieve/audio.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

// The most a reply may be, in bytes.
constexpr size_t kTalkReply = 80;
// Room kept for what you say next after a file or picture is given (the reply's room besides).
constexpr size_t kTalkRoom = 64;
// The model file: the most a reply may be, in tokens; the most of a file or figure given to it, in
// bytes (it reads them with the next message).
constexpr size_t kFileReply = 200, kFileGive = 1500;
// What a ChatML model is told: the system prompt sieve-system-v1 (docs/SYSTEM-PROMPT.md), a file
// that is never edited, by its SHA-256; and what it is told if that file is missing.
constexpr const char* kSystemPromptFile = "sieve-system-v1.txt";
constexpr const char* kSystemPromptSha256 = "04bd46c89c16f86fbccffac8055636b03df1d1b7de2270a343bd20ddd74239a9";
constexpr const char* kFileSystem = "You are a helpful assistant.";
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
    if (!said.empty() && said[0] == '/') // a command, not something said to the model
    {
        chat_command(said);
        return;
    }
    if (talking_to_file())
    {
        talk_file(said);
        return;
    }
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

// ---- the model file (the setup menu's AI rows)

void Hallway::set_model_files(const std::string& text_model, const std::string& picture_model)
{
    if (text_model != text_model_path_)
    {
        // Another model: loaded afresh when next spoken to, with nothing waiting for it.
        file_chat_.reset();
        file_model_.reset();
        file_context_.clear();
        talk_to_file_ = true;
    }
    text_model_path_ = text_model;
    picture_model_path_ = picture_model;
}

// The system prompt a ChatML model file is given (docs/SYSTEM-PROMPT.md): sieve-system-v1's text,
// then "# The place, now" and where the conversation is taking place. Missing, the plain one, and
// a note says so; changed from the published file, it is used, and a note says that too.
std::string Hallway::system_prompt()
{
    std::vector<fs::path> folders = {fs::path("data") / "prompts"};
    if (const fs::path exe = cli::executable_dir(); !exe.empty())
    {
        folders.push_back(exe / "prompts");
        folders.push_back(exe / ".." / "data" / "prompts");
    }
    std::string text;
    for (const fs::path& f : folders)
    {
        std::ifstream in(f / kSystemPromptFile, std::ios::binary);
        if (!in) continue;
        std::ostringstream all;
        all << in.rdbuf();
        text = all.str();
        break;
    }
    if (text.empty())
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.prompt.missing", {kSystemPromptFile})));
        return kFileSystem;
    }
    if (cli::sha256_hex(std::vector<uint8_t>(text.begin(), text.end())) != kSystemPromptSha256)
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.prompt.changed", {kSystemPromptFile})));
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    const std::string folder = u8(from_u8(text_model_path_).filename());
    text += "\n\n# The place, now\n\n";
    std::string filters = filter_status();
    if (!filters.empty()) filters[0] = char(std::toupper(static_cast<unsigned char>(filters[0])));
    text += "- Line: the " + std::string(theme().name) + " line (" + space_id() + ")\n";
    text += "- Ordering: " + ordering_name() + "\n";
    text += "- " + filters + "\n";
    text += "- In hand: a model of the AI line, address " + short_address(talk_hex_) + "\n";
    text += folder.empty() ? std::string("- You: a model given as a file\n") : "- You: the model in the folder " + folder + ", given as a file\n";
    return text;
}

// What the model file says back. A ChatML model answers in a conversation (llm::Chat, with
// kFileSystem); any other continues the text, as the line's models do, until a newline. Its choices
// are seeded by the conversation, as talk()'s are. When the conversation and the reply would not
// fit in its positions, it starts again and says so.
void Hallway::talk_file(const std::string& said)
{
    if (!file_model_)
    {
        try
        {
            busy(tr("chat.loading"), [&] { file_model_ = cli::load_model_folder(from_u8(text_model_path_)); });
        }
        catch (const std::exception& ex)
        {
            // Not loaded: said so, and the model in hand answers instead (Ctrl+M tries the file again).
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.model_failed", {ex.what()})));
            talk_to_file_ = false;
            talk(said);
            return;
        }
    }
    cli::ModelFolder& m = *file_model_;
    const std::string message = file_context_ + said;
    file_context_.clear();
    sieve::llm::Sampling s;
    s.temperature = 0.7f;
    s.top_p = 0.9f;
    const auto digest = sieve::Sha256::hash(talk_ + said);
    s.seed = 0;
    for (int i = 0; i < 8; ++i) s.seed = (s.seed << 8) | digest[size_t(i)];
    sieve::llm::Sampler sampler(s);
    const size_t max = m.model->config().max_positions, need = m.tokenizer->encode(message).size() + kFileReply + 16;
    if (need >= max)
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("chat.too_long")));
        return;
    }
    bool restarted = false;
    std::string reply;
    try
    {
        // A conversation started (or started again) is told what and where it is; a model with too
        // few positions for that and the message together is told the short version, and a note
        // says so.
        const bool fresh = !file_chat_ || m.model->position() + need >= max;
        std::string prompt = m.chatml && fresh ? system_prompt() : std::string();
        if (!prompt.empty() && m.tokenizer->encode(prompt).size() + need + 16 >= max)
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.prompt.too_long", {std::to_string(max)})));
            prompt = kFileSystem;
        }
        busy(tr("chat.thinking"), [&] {
            if (m.model->position() + need >= max) // its memory is full: it starts again
            {
                restarted = m.model->position() > 0;
                m.model->reset();
                file_chat_.reset();
            }
            if (m.chatml)
            {
                if (!file_chat_) file_chat_ = std::make_unique<sieve::llm::Chat>(*m.model, *m.tokenizer, prompt);
                reply = file_chat_->reply(message, sampler, kFileReply);
            }
            else
            {
                const std::vector<uint32_t> nl = m.tokenizer->encode("\n", false);
                reply = sieve::llm::continue_text(*m.model, *m.tokenizer, message + "\n", sampler, kFileReply, {},
                                                  nl.size() == 1 ? std::optional<uint32_t>(nl[0]) : std::nullopt);
            }
        });
    }
    catch (const std::exception& ex)
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.model_failed", {ex.what()})));
        return;
    }
    if (restarted)
    {
        talk_ += tr("talk.full") + "\n";
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("talk.full")));
    }
    talk_ += trf("talk.you", {said}) + "\n" + trf("talk.it", {reply}) + "\n";
    chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::You, said));
    ChatEntry it = entry_of<ChatEntry>(ChatEntry::Kind::It, reply);
    it.model = m.name;
    chat_.push_back(std::move(it));
    chat_scroll_ = 0;
}

size_t Hallway::give_file_model(const std::string& what, const std::string& text)
{
    size_t n = std::min(text.size(), kFileGive);
    while (n > 0 && n < text.size() && (static_cast<unsigned char>(text[n]) & 0xC0) == 0x80) --n; // whole characters
    file_context_ += what + "\n" + text.substr(0, n) + (n ? "\n" : "");
    return n;
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
    // The picture models there are now (one trained since the last look is found); the one chosen
    // here before, else the setup menu's.
    const std::string was = chat_models_.empty() ? std::string() : u8(chat_models_[size_t(chat_model_)]);
    chat_find_models();
    for (size_t i = 0; i < chat_models_.size(); ++i)
        if (u8(chat_models_[i]) == was) chat_model_ = int(i);
    chat_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
}

// The trained picture models in the training folders, with the setup menu's (which may lie
// elsewhere) first among them and chosen.
void Hallway::chat_find_models()
{
    chat_models_ = cli::picture_models_in(cli::training_run_dirs());
    chat_model_ = 0;
    if (picture_model_path_.empty()) return;
    const fs::path chosen = from_u8(picture_model_path_);
    std::error_code ec;
    if (!cli::is_diffusion_run(chosen) || !fs::is_regular_file(chosen / "result.txt", ec)) return; // not a trained run: the first, as without one
    for (size_t i = 0; i < chat_models_.size(); ++i)
        if (fs::equivalent(chat_models_[i], chosen, ec))
        {
            chat_model_ = int(i);
            return;
        }
    chat_models_.insert(chat_models_.begin(), chosen);
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
    if (talking_to_file())
    {
        // The model file reads a text's first bytes, and of anything else what it is.
        e.with_next = true;
        e.given = e.file_kind == "TXT" ? give_file_model(trf("chat.file.tag", {e.name}), std::string(e.bytes.begin(), e.bytes.end()))
                                       : give_file_model(trf("chat.file.tag.kind", {e.name, e.file_kind, std::to_string(e.bytes.size())}), "");
    }
    else e.given = talk_give(e.bytes);
    talk_ += trf("talk.file", {e.name, grouped(e.bytes.size())}) + "\n";
    chat_.push_back(std::move(e));
    chat_scroll_ = 0;
}

// A figure drawn with the picture model chosen: its next picture.
void Hallway::chat_draw()
{
    if (!in_hand_ || !in_hand_->ai) return;
    talk_ready();
    if (chat_models_.empty()) chat_find_models();
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
            if (m.recipe.audio()) e.symbols = m.recipe.symbols; // a melody model's
            e.address = cli::unit_space_of(m.recipe).address_of(e.cells, AddressMode::Positional);
        });
    }
    catch (const std::exception& ex)
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.draw_failed", {ex.what()})));
        return;
    }
    chat_drawn_[e.model] = e.number;
    chat_show_unit(std::move(e));
}

std::string Hallway::ChatEntry::heading() const
{
    if (!title.empty()) return title;
    return trf(melody() ? "chat.melody" : "chat.figure", {std::to_string(number), model});
}

void Hallway::chat_show_unit(ChatEntry e)
{
    std::string rows; // what the model is given: a picture's rows, a melody's notation
    if (e.melody()) rows = sieve::notes_to_notation(sieve::note_set_of(e.symbols), e.cells);
    else
    {
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
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.withheld", {e.heading()})));
            return;
        }
        // Its rows: a character a cell (the background a dot), a newline a row.
        const uint32_t background = pal.nearest(sieve::Rgb{255, 255, 255});
        const char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        for (uint32_t y = 0; y < e.height; ++y)
        {
            for (uint32_t x = 0; x < e.width; ++x)
            {
                const uint32_t c = e.cells[size_t(y) * e.width + x];
                rows.push_back(c == background ? '.' : pal.size() <= 2 ? '#' : kDigits[c % 36]);
            }
            if (y + 1 < e.height) rows.push_back('\n');
        }
    }
    if (talking_to_file())
    {
        e.with_next = true;
        e.given = give_file_model(e.melody() ? trf("chat.melody.tag", {e.heading()})
                                             : trf("chat.figure.tag", {e.heading(), std::to_string(e.width), std::to_string(e.height)}),
                                  rows);
    }
    else e.given = talk_give(std::vector<uint8_t>(rows.begin(), rows.end()));
    talk_ += e.heading() + ": " + e.address + "\n";
    chat_.push_back(std::move(e));
    chat_scroll_ = 0;
}

namespace {

// A number below n, evenly (to within 2^-64): 64 bits more than n has, at random, taken mod n.
BigUint random_below(const BigUint& n, sieve::diffusion::SplitMix& g)
{
    static const char kHex[] = "0123456789abcdef";
    std::string hex;
    for (size_t bits = 0; bits < n.bit_length() + 64; bits += 64)
    {
        uint64_t v = g.next();
        for (int k = 0; k < 16; ++k, v >>= 4) hex.push_back(kHex[v & 15]);
    }
    BigUint q, r;
    BigUint::divmod(BigUint::from_hex(hex), n, q, r);
    return r;
}

} // namespace

void Hallway::chat_command(const std::string& line)
{
    std::istringstream in(line.substr(1));
    std::string verb, what;
    in >> verb >> what;
    chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::You, line));
    talk_ += trf("talk.you", {line}) + "\n";
    chat_scroll_ = 0;
    if (verb == "random" && (what == "picture" || what == "image" || what == "melody"))
    {
        // A unit of the image or audio line at random: a survivor of its filters where they count,
        // else any unit (each cell evenly).
        const bool melody = what == "melody";
        const LineKind kind = melody ? LineKind::Audio : LineKind::Image;
        const Line& l = unit_line(kind);
        if (melody && !sieve::is_note_symbols(l.space.symbols_id()))
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.random.not_notes", {l.space.symbols_id()})));
            return;
        }
        static sieve::diffusion::SplitMix g(uint64_t(SDL_GetPerformanceCounter()) ^ 0x5eed);
        const FilterStack& st = stacks_[size_t(line_of(kind))];
        ChatEntry e;
        e.kind = ChatEntry::Kind::Picture;
        if (!st.empty() && st.ranker() && !st.ranker()->count().is_zero())
        {
            e.cells = st.ranker()->unrank(random_below(st.ranker()->count(), g));
            e.title = tr(melody ? "chat.random.melody.kept" : "chat.random.picture.kept");
        }
        else
        {
            e.cells.resize(l.space.unit_length());
            for (auto& c : e.cells) c = uint32_t(g.next() % l.space.base());
            e.title = tr(melody ? "chat.random.melody" : "chat.random.picture");
        }
        if (melody) e.symbols = l.space.symbols_id(), e.width = l.space.unit_length(), e.height = 1;
        else e.width = l.image.width, e.height = l.image.height, e.palette = l.image.palette->id();
        e.address = l.space.address_of(e.cells, AddressMode::Positional);
        chat_show_unit(std::move(e));
        return;
    }
    if (verb == "prompt") // what a model given as a file is told, word for word
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, system_prompt()));
        return;
    }
    chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr(verb == "help" ? "chat.commands" : "chat.command.unknown")));
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
        const std::string name = file      ? e.name
                                 : e.number ? (e.melody() ? "melody-" : "figure-") + e.model + "-" + std::to_string(e.number) + (e.melody() ? ".mid" : ".png")
                                            : std::string(e.melody() ? "melody.mid" : "picture.png");
        const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
        const std::string start = docs ? std::string(docs) + name : name;
        save_file_dialog(chat_save_chosen, this, window_, nullptr, 0, start.c_str());
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
    if (file && act == ChatAct::Full)
    {
        // The picture at its own size, every pixel its own colour (rgb24): of all the ways the
        // image line can hold it, the one that keeps every pixel. A hallway built with that image
        // line goes to it, as GO TO IT on a figure of another size does.
        try
        {
            RgbaImage first;
            cli::read_media_frames(e.bytes.data(), e.bytes.size(), e.name, 1, [&](const RgbaImage& f) {
                if (!first.width) first = f;
            });
            if (!first.width) throw std::runtime_error(tr("chat.full.none"));
            if (uint64_t(first.width) * first.height > 0xFFFFFFFFull) throw std::runtime_error(tr("chat.full.big"));
            sieve::ImageFormat own{first.width, first.height, 1, &sieve::palette_by_id("rgb24")};
            picture_to_go_ = PictureToGo{own.width, own.height, "rgb24", sieve::canonicalise_frame(first, own)};
        }
        catch (const std::exception& ex)
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chat.full.failed", {ex.what()})));
            return;
        }
        const Line& image = unit_line(LineKind::Image);
        if (image.image.width == picture_to_go_->width && image.image.height == picture_to_go_->height && image.image.palette &&
            image.image.palette->id() == "rgb24")
        {
            const std::vector<uint32_t> cells = std::move(picture_to_go_->cells);
            picture_to_go_.reset();
            go_to_picture(cells);
        }
        else
        {
            close_chat();
            request_ = Request::GoToPicture;
        }
        return;
    }
    if (!file && e.melody() && (act == ChatAct::Go || act == ChatAct::Play))
    {
        // GO TO IT: on the audio line, in hand (a hallway built with the line it needs, where this
        // one's is another length or note set: Request::GoToMelody); PLAY: the music player plays it.
        MusicMelody m;
        m.set = sieve::note_set_of(e.symbols);
        m.notes = e.cells;
        if (act == ChatAct::Play)
        {
            if (MusicPlayer* p = music()) p->play(m);
            else chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("media.no_player")));
            return;
        }
        close_chat();
        go_to_melody(m);
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
                if (which == 0) open_file_dialog(chat_files_chosen, this, window_, nullptr, 0, nullptr, true);
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
        else if (e.key.key == SDLK_O) open_file_dialog(chat_files_chosen, this, window_, nullptr, 0, nullptr, true);
        else if (e.key.key == SDLK_D) chat_draw();
        else if (e.key.key == SDLK_P) chat_pick_model(shift ? -1 : 1);
        else if (e.key.key == SDLK_M && !text_model_path_.empty())
        {
            // The model file, or the model in hand.
            talk_to_file_ = !talk_to_file_;
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, talk_to_file_ ? trf("chat.speaking.file", {u8(from_u8(text_model_path_).filename())})
                                                                                      : tr("chat.speaking.hand")));
            chat_scroll_ = 0;
        }
        else if (const auto card = chat_last_card())
        {
            if (e.key.key == SDLK_G) chat_act(*card, ChatAct::Go);
            else if (e.key.key == SDLK_J && chat_[*card].kind == ChatEntry::Kind::File) chat_act(*card, ChatAct::Item);
            else if (e.key.key == SDLK_S) chat_act(*card, ChatAct::Save);
            else if (e.key.key == SDLK_F && chat_[*card].kind == ChatEntry::Kind::File) chat_act(*card, ChatAct::Full);
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
            if (draw)
                text(x, y, e.kind == K::You ? tr("chat.you") : e.model.empty() ? tr("chat.it") : trf("chat.it.file", {e.model}), kSmall,
                     e.kind == K::You ? grey : ink);
            h += kSmallLine;
        }
        const bool big = e.kind != K::Note;
        // What a line's model wrote is bytes, drawn as the font can (printable); a model file's words
        // are text, and what you wrote is as typed.
        for (const std::string& para : paragraphs(e.kind == K::It ? (e.model.empty() ? printable(e.text) : text_line(e.text)) : e.text))
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
        lines.push_back({e.with_next ? (e.given ? trf("chat.given.next", {grouped(e.given)}) : tr("chat.given.next.what"))
                                     : trf("chat.given", {grouped(e.given)}),
                         kSmall, ink});
        for (const std::string& l : e.preview) lines.push_back({fit(l, tw, kSmall), kSmall, grey});
        if (e.item_line >= 0 || e.file_kind != "?") buttons.push_back({tr("chat.item"), ChatAct::Item});
        if (e.thumb.width && (e.file_kind == "PNG" || e.file_kind == "JPG" || e.file_kind == "GIF" || e.file_kind == "BMP"))
            buttons.push_back({tr("chat.full"), ChatAct::Full});
    }
    else
    {
        if (e.melody()) pic_w = 160, pic_h = 64; // a piano roll
        else pic_w = pic_h = std::min(120.0f, std::max(40.0f, float(std::max(e.width, e.height)) * 8));
        const size_t cols = size_t(std::max(8.0f, (w - 3 * kPad - pic_w) / (8 * kSmall)));
        lines.push_back({e.heading(), kBig, white});
        if (e.melody())
        {
            lines.push_back({trf("chat.melody.what", {std::to_string(e.cells.size()), e.symbols}), kSmall, ink});
            for (const std::string& l : wrap_words(notes_to_notation(sieve::note_set_of(e.symbols), e.cells), cols)) lines.push_back({l, kSmall, white});
            for (const std::string& l : wrap(trf("chat.melody.address", {e.address}), cols)) lines.push_back({l, kSmall, ink});
            lines.push_back({trf(e.with_next ? "chat.given.next" : "chat.given.notation", {grouped(e.given)}), kSmall, ink});
            buttons.push_back({tr("chat.play"), ChatAct::Play});
        }
        else
        {
            lines.push_back({trf("chat.figure.what", {std::to_string(e.width), std::to_string(e.height), e.palette}), kSmall, ink});
            for (const std::string& l : wrap(trf("chat.figure.address", {e.address}), cols)) lines.push_back({l, kSmall, ink});
            lines.push_back({trf(e.with_next ? "chat.given.next" : "chat.given.rows", {grouped(e.given)}), kSmall, ink});
        }
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
    if (e.kind == K::Picture && e.melody())
    {
        // A piano roll: time along, pitch up, each note a bar as long as it lasts; rests are gaps.
        const sieve::NoteSet set = sieve::note_set_of(e.symbols);
        const SDL_FRect box{x + kPad, y + kPad, pic_w, pic_h};
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
        SDL_RenderFillRect(r_, &box);
        SDL_SetRenderDrawColor(r_, grey.r, grey.g, grey.b, 255);
        SDL_RenderRect(r_, &box);
        const uint32_t per = std::max<uint32_t>(1, uint32_t(e.cells.size()) / std::max<uint32_t>(1, set.voices));
        for (uint32_t v = 0; v < set.voices; ++v)
        {
            uint32_t total = 0;
            for (uint32_t k = 0; k < per && v * per + k < e.cells.size(); ++k) total += set.sixteenths(e.cells[v * per + k]);
            float at = 0;
            for (uint32_t k = 0; k < per && v * per + k < e.cells.size(); ++k)
            {
                const uint32_t d = e.cells[v * per + k], len = set.sixteenths(d), midi = set.midi(d);
                const float w16 = (pic_w - 4) / float(std::max<uint32_t>(1, total));
                if (midi)
                {
                    const float fy = 1.0f - float(midi - set.low) / float(std::max<uint32_t>(1, set.high - set.low));
                    const SDL_FRect bar{box.x + 2 + at * w16, box.y + 2 + fy * (pic_h - 8), std::max(1.0f, float(len) * w16 - 1), 4};
                    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
                    SDL_RenderFillRect(r_, &bar);
                }
                at += float(len);
            }
        }
    }
    else if (e.kind == K::Picture)
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
            else if (e.melody())
            {
                fs::path path = from_u8(*save_to);
                if (path.extension().empty()) path += ".mid";
                const sieve::NoteSet set = sieve::note_set_of(e.symbols);
                const std::string midi = set.legacy ? sieve::notes_to_midi(e.cells) : sieve::notes_to_midi(set, e.cells);
                write_bytes(path, std::vector<uint8_t>(midi.begin(), midi.end()));
            }
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
    const std::string file_name = text_model_path_.empty() ? std::string() : u8(from_u8(text_model_path_).filename());
    const std::string with = talking_to_file()
                                 ? trf("chat.with.file", {file_name})
                                 : trf("chat.with", {short_address(talk_hex_),
                                                     trf(sh.layers == 1 ? "ai.summary.one" : "ai.summary",
                                                         {std::to_string(sh.layers), std::to_string(sh.width), std::to_string(sh.heads),
                                                          std::to_string(ai_space_->weights()), std::to_string(sh.bits)})}) +
                                       (file_name.empty() ? std::string() : trf("chat.with.switch", {file_name}));
    text(x, top, fit(with, w, 1), 1, grey);
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
    // One line of the box is exactly a button's height, so the row is even: the box, +, DRAW and
    // SEND share their top and bottom edges, and a box of more lines grows upwards from there.
    const float lines_h = kBigLine * float(std::max<size_t>(1, typed.size()));
    const float box_h = std::max(button, lines_h + (button - kBigLine));
    const float box_y = keys_y - 10 - box_h;
    const float row_y = box_y + box_h - button;
    auto bar_button = [&](float bx, float bw, const std::string& label, int which) {
        const SDL_FRect b{bx, row_y, bw, button};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
        SDL_RenderFillRect(r_, &b);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &b);
        if (!label.empty()) text(bx + (bw - text_width(label, 1)) / 2, row_y + (button - 8) / 2, label, 1, white);
        chat_bar_.push_back({b, which});
    };
    // The +: two bars drawn to the pixel, so it sits exactly in the middle of its square.
    bar_button(x, button, std::string(), 0);
    {
        const float arm = std::round(button / 2), bar = 4, cx = x + button / 2, cy = row_y + button / 2;
        SDL_SetRenderDrawColor(r_, white.r, white.g, white.b, 255);
        const SDL_FRect across{std::round(cx - arm / 2), std::round(cy - bar / 2), arm, bar}, down{std::round(cx - bar / 2), std::round(cy - arm / 2), bar, arm};
        SDL_RenderFillRect(r_, &across);
        SDL_RenderFillRect(r_, &down);
    }
    const float send_w = 72, draw_w = 72;
    bar_button(x + w - send_w, send_w, tr("chat.send"), 2);
    bar_button(x + w - send_w - 8 - draw_w, draw_w, tr("chat.draw"), 1);
    const float tx = x + button + 8, tw = w - button - 8 - send_w - draw_w - 16;
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    const SDL_FRect box{tx, box_y, tw, box_h};
    SDL_RenderRect(r_, &box);
    const float text_y = box_y + (box_h - lines_h) / 2 + (kBigLine - 8 * kBig) / 2;
    for (size_t k = 0; k < typed.size(); ++k) text(tx + 8, text_y + kBigLine * float(k), typed[k], kBig, white);
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
