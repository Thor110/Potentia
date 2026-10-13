// Sieve hallway — CHAIN: models one after another (IDEAS §19.46, SPECIFICATIONS §12.0h).
//
// A chain is a list of stages, each a model, and `/chain TEXT` in CHAT runs it: the first stage is
// given TEXT, and each stage after is given what the one before it made. A text model (the model
// in hand, a trained text run, or the model given as a file) continues what it is given, as CHAT's
// models do. A picture or melody model takes no input, so what it is given chooses its noise: drawn
// from a generator seeded by the SHA-256 of what it was given, so the same chain on the same text
// makes the same things on every machine. What it makes goes on as its rows or its notation, as CHAT gives
// pictures and melodies to a model. Every stage's result is a card or a reply in the conversation,
// with the buttons any card has.
//
// The CHAIN tab (beside CHAT, before it) shows the chain as a row of nodes and edits it: Up and Down
// choose a stage, Left and Right change its model, N adds one after it, Delete takes it away,
// Ctrl+Up and Ctrl+Down move it, Enter goes to CHAT with "/chain " typed, Ctrl+S saves it as a
// sieve-chain-v1 file and Ctrl+O opens one.
#include "hallway.hpp"

#include "dialogs.hpp"
#include "cli/dictionaries.hpp"
#include "cli/train.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

template <class Entry>
Entry entry_of(typename Entry::Kind kind, std::string text)
{
    Entry e;
    e.kind = kind;
    e.text = std::move(text);
    return e;
}

constexpr size_t kChainReply = 80; // bytes a text model of the AI line writes at a stage, at most
constexpr const char* kChainHeader = "sieve-chain-v1";

// The trained text runs in these folders, by name (a folder with a training.ini, or a .sieve file);
// a name in an earlier folder hides the same name in a later one.
std::vector<std::string> text_runs_in(const std::vector<fs::path>& dirs)
{
    std::vector<std::string> names;
    for (const fs::path& d : dirs)
    {
        std::error_code ec;
        std::vector<std::string> here;
        for (const fs::directory_entry& e : fs::directory_iterator(d, ec))
        {
            const fs::path p = e.path();
            if (e.is_directory() && fs::exists(p / "training.ini")) here.push_back(u8(p.filename()));
            else if (e.is_regular_file() && p.extension() == ".sieve") here.push_back(u8(p.stem()));
        }
        std::sort(here.begin(), here.end());
        for (const std::string& n : here)
            if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n);
    }
    return names;
}

// A picture model by its name in the training folders, or by its path.
fs::path picture_model_path(const std::string& name)
{
    if (fs::exists(from_u8(name))) return from_u8(name);
    for (const fs::path& p : cli::picture_models_in(cli::training_run_dirs()))
        if (u8(p.filename()) == name) return p;
    throw std::runtime_error(trf("chain.no_model", {name}));
}

// The first eight bytes of the SHA-256 of `text`, as a number.
uint64_t seed_of(const std::string& text)
{
    const auto d = sieve::Sha256::hash(text);
    uint64_t s = 0;
    for (int i = 0; i < 8; ++i) s = (s << 8) | d[size_t(i)];
    return s;
}

void SDLCALL chain_opened(void* self, const char* const* files, int)
{
    if (files && files[0]) static_cast<Hallway*>(self)->chain_chosen(files[0], false);
}

void SDLCALL chain_saved(void* self, const char* const* files, int)
{
    if (files && files[0]) static_cast<Hallway*>(self)->chain_chosen(files[0], true);
}

} // namespace

void Hallway::chain_chosen(const std::string& path, bool save)
{
    std::lock_guard<std::mutex> lock(chat_mx_);
    (save ? chain_save_to_ : chain_load_from_) = path;
}

std::vector<Hallway::ChainStage> Hallway::chain_choices()
{
    std::vector<ChainStage> all;
    all.push_back({ChainStage::Kind::Hand, ""});
    if (!text_model_path_.empty()) all.push_back({ChainStage::Kind::File, ""});
    for (const std::string& n : text_runs_in(cli::training_run_dirs())) all.push_back({ChainStage::Kind::Run, n});
    for (const fs::path& p : cli::picture_models_in(cli::training_run_dirs())) all.push_back({ChainStage::Kind::Draw, u8(p.filename())});
    return all;
}

std::string Hallway::chain_stage_name(const ChainStage& s)
{
    switch (s.kind)
    {
    case ChainStage::Kind::Hand: return tr("chain.hand");
    case ChainStage::Kind::File:
        return text_model_path_.empty() ? tr("chain.file.none") : trf("chain.file", {u8(from_u8(text_model_path_).filename())});
    case ChainStage::Kind::Run: return trf("chain.run", {s.name});
    case ChainStage::Kind::Draw: return trf("chain.draw", {s.name});
    }
    return std::string();
}

void Hallway::chain_defaults()
{
    // The trained models there are, one of each kind in turn: the first text run (else the model in
    // hand), then every picture and melody model.
    chain_.clear();
    const std::vector<ChainStage> all = chain_choices();
    const auto run = std::find_if(all.begin(), all.end(), [](const ChainStage& s) { return s.kind == ChainStage::Kind::Run; });
    chain_.push_back(run != all.end() ? *run : all.front());
    for (const ChainStage& s : all)
        if (s.kind == ChainStage::Kind::Draw) chain_.push_back(s);
    chain_sel_ = 0;
    chain_ready_ = true;
}

std::string Hallway::chain_text() const
{
    const auto word = [](ChainStage::Kind k) {
        return k == ChainStage::Kind::File ? "file" : k == ChainStage::Kind::Run ? "run" : k == ChainStage::Kind::Draw ? "draw" : "hand";
    };
    std::string t = std::string(kChainHeader) + "\n";
    for (const ChainStage& s : chain_) t += std::string(word(s.kind)) + (s.name.empty() ? "" : " " + s.name) + "\n";
    return t;
}

void Hallway::chain_parse(const std::string& text)
{
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || (line != kChainHeader && line != std::string(kChainHeader) + "\r"))
        throw std::runtime_error(trf("chain.not_a_chain", {kChainHeader}));
    std::vector<ChainStage> stages;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t sp = line.find(' ');
        const std::string word = line.substr(0, sp), name = sp == std::string::npos ? "" : line.substr(sp + 1);
        ChainStage s;
        if (word == "hand") s.kind = ChainStage::Kind::Hand;
        else if (word == "file") s.kind = ChainStage::Kind::File;
        else if (word == "run" && !name.empty()) s.kind = ChainStage::Kind::Run;
        else if (word == "draw" && !name.empty()) s.kind = ChainStage::Kind::Draw;
        else throw std::runtime_error(trf("chain.bad_line", {line}));
        s.name = name;
        stages.push_back(std::move(s));
    }
    if (stages.empty()) throw std::runtime_error(tr("chain.empty"));
    chain_ = std::move(stages);
    chain_sel_ = 0;
    chain_ready_ = true;
}

void Hallway::chain_run(const std::string& input)
{
    if (!in_hand_ || !in_hand_->ai) return;
    if (!chain_ready_) chain_defaults();
    if (chain_.empty())
    {
        chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, tr("chain.empty")));
        return;
    }
    talk_ready();
    std::string carried = input; // what the next stage is given
    const size_t n = chain_.size();
    for (size_t i = 0; i < n; ++i)
    {
        const ChainStage& st = chain_[i];
        const std::string tag = trf("chain.stage", {std::to_string(i + 1), std::to_string(n), chain_stage_name(st)});
        // Each stage's choices are seeded by what it is given and where it stands in the chain.
        const uint64_t seed = seed_of(std::to_string(i) + "\n" + carried);
        try
        {
            if (st.kind == ChainStage::Kind::Draw)
            {
                ChatEntry e;
                e.kind = ChatEntry::Kind::Picture;
                busy(tr("chat.drawing"), [&] {
                    std::shared_ptr<cli::PictureModel>& m = chain_picture_models_[st.name];
                    if (!m) m = std::make_shared<cli::PictureModel>(cli::load_picture_model(picture_model_path(st.name)));
                    e.model = m->name;
                    // Its noise from what it was given: a picture's worth of cells from splitmix64
                    // seeded with the seed (each next() mod B, cell after cell), as a run's k-th
                    // noise is drawn from seed 1; and made with that seed.
                    const sieve::diffusion::Setup& su = m->recipe.setup;
                    sieve::diffusion::SplitMix g(seed);
                    std::vector<uint32_t> noise(su.cells());
                    for (auto& c : noise) c = uint32_t(g.next() % su.colours);
                    e.cells = sieve::diffusion::make_picture(*m->space, m->digits, su, noise, seed);
                    e.width = m->recipe.setup.width, e.height = m->recipe.setup.height, e.palette = m->recipe.palette;
                    if (m->recipe.audio()) e.symbols = m->recipe.symbols;
                    e.address = cli::unit_space_of(m->recipe).address_of(e.cells, AddressMode::Positional);
                });
                e.title = tag;
                const std::string rows = unit_rows(e);
                if (!chat_show_unit(std::move(e))) return; // the vault withheld it: the chain stops there
                carried = rows;
                continue;
            }
            std::string reply;
            if (st.kind == ChainStage::Kind::File)
            {
                if (text_model_path_.empty()) throw std::runtime_error(tr("chain.file.none"));
                if (!load_file_model()) return;
                const std::optional<std::string> r = file_reply(carried, std::to_string(seed));
                if (!r) return;
                reply = *r;
            }
            else
            {
                // A model of the AI line (the one in hand, or a run's), fresh for each stage so the
                // conversation in CHAT is left as it was; given the end of what came before, as much
                // as fits beside its reply.
                std::shared_ptr<sieve::llm::Model> model;
                busy(tr("chat.thinking"), [&] {
                    if (st.kind == ChainStage::Kind::Hand)
                        model = std::make_shared<sieve::llm::Model>(ai_space_->config(), ai_space_->tensors_of(*in_hand_->ai), 1);
                    else
                    {
                        std::shared_ptr<sieve::llm::Model>& m = chain_text_models_[st.name];
                        if (!m)
                        {
                            const sieve::ModelRun run = cli::model_run_of(st.name, cli::training_run_dirs());
                            if (run.final_address.empty()) throw std::runtime_error(trf("chain.untrained", {st.name}));
                            const sieve::AiSpace space(sieve::AiShape{run.layers, run.width, run.heads, run.bits}, "sieve");
                            const auto digits = space.digits_at(space.parse(run.final_address), AddressMode::Positional);
                            m = std::make_shared<sieve::llm::Model>(space.config(), space.tensors_of(digits), 1);
                        }
                        m->reset();
                        model = m;
                    }
                    const size_t room = model->config().max_positions - kChainReply - 2;
                    std::string given = carried.size() > room ? carried.substr(carried.size() - room) : carried;
                    sieve::llm::Sampling s;
                    s.temperature = 0.8f;
                    s.top_p = 1.0f;
                    s.seed = seed;
                    sieve::llm::Sampler sampler(s);
                    reply = sieve::llm::continue_text(*model, *ai_tokenizer_, given + "\n", sampler, kChainReply, {}, uint32_t('\n'));
                });
            }
            talk_ += tag + ": " + reply + "\n";
            ChatEntry it = entry_of<ChatEntry>(ChatEntry::Kind::It, reply);
            it.model = tag;
            chat_.push_back(std::move(it));
            chat_scroll_ = 0;
            carried = reply;
        }
        catch (const std::exception& ex)
        {
            chat_.push_back(entry_of<ChatEntry>(ChatEntry::Kind::Note, trf("chain.failed", {tag, ex.what()})));
            return;
        }
    }
}

bool Hallway::chain_key(const SDL_KeyboardEvent& k)
{
    if (!chain_ready_) chain_defaults();
    const bool ctrl = (k.mod & SDL_KMOD_CTRL) != 0;
    const int n = int(chain_.size());
    chain_sel_ = n ? std::clamp(chain_sel_, 0, n - 1) : 0;
    switch (k.key)
    {
    case SDLK_UP:
        if (ctrl && chain_sel_ > 0) std::swap(chain_[size_t(chain_sel_)], chain_[size_t(chain_sel_ - 1)]);
        if (chain_sel_ > 0) --chain_sel_;
        return true;
    case SDLK_DOWN:
        if (ctrl && chain_sel_ + 1 < n) std::swap(chain_[size_t(chain_sel_)], chain_[size_t(chain_sel_ + 1)]);
        if (chain_sel_ + 1 < n) ++chain_sel_;
        return true;
    case SDLK_LEFT:
    case SDLK_RIGHT:
    {
        if (!n) return true;
        const std::vector<ChainStage> all = chain_choices();
        ChainStage& s = chain_[size_t(chain_sel_)];
        int at = 0;
        for (int i = 0; i < int(all.size()); ++i)
            if (all[size_t(i)].kind == s.kind && all[size_t(i)].name == s.name) at = i;
        const int m = int(all.size());
        s = all[size_t(((at + (k.key == SDLK_RIGHT ? 1 : -1)) % m + m) % m)];
        return true;
    }
    case SDLK_N:
    case SDLK_INSERT:
        chain_.insert(chain_.begin() + (n ? chain_sel_ + 1 : 0), n ? chain_[size_t(chain_sel_)] : chain_choices().front());
        chain_sel_ = n ? chain_sel_ + 1 : 0;
        return true;
    case SDLK_DELETE:
    case SDLK_BACKSPACE:
        if (n) chain_.erase(chain_.begin() + chain_sel_);
        chain_sel_ = std::max(0, std::min(chain_sel_, int(chain_.size()) - 1));
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        hand_tab_ = chat_tab();
        open_chat();
        chat_text_ = "/chain ";
        return true;
    case SDLK_S:
        if (!ctrl) return false;
        {
            const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
            const std::string start = std::string(docs ? docs : "") + "models.chain";
            static const SDL_DialogFileFilter kChain[] = {{"Sieve chains", "chain"}};
            save_file_dialog(chain_saved, this, window_, kChain, 1, start.c_str());
        }
        return true;
    case SDLK_O:
        if (!ctrl) return false;
        {
            static const SDL_DialogFileFilter kChain[] = {{"Sieve chains", "chain"}};
            open_file_dialog(chain_opened, this, window_, kChain, 1, nullptr, false);
        }
        return true;
    default: return false;
    }
}

void Hallway::chain_poll()
{
    std::optional<std::string> load, save;
    {
        std::lock_guard<std::mutex> lock(chat_mx_);
        load.swap(chain_load_from_);
        save.swap(chain_save_to_);
    }
    if (save) chain_save(*save);
    if (load) chain_load(*load);
}

void Hallway::open_chain()
{
    if (!in_hand_ || !in_hand_->ai) return;
    if (chat_open_) close_chat();
    if (!chain_ready_) chain_defaults();
    hand_tab_ = chain_tab();
}

void Hallway::chain_save(const std::string& path)
{
    if (!chain_ready_) chain_defaults();
    fs::path p = from_u8(path);
    if (p.extension().empty()) p += ".chain";
    std::ofstream out(p, std::ios::binary);
    out << chain_text();
    message(out ? trf("chain.saved", {u8(p)}) : trf("chain.save_failed", {u8(p)}));
}

void Hallway::chain_load(const std::string& path)
{
    try
    {
        std::ifstream in(from_u8(path), std::ios::binary);
        if (!in) throw std::runtime_error(trf("chain.unreadable", {path}));
        std::ostringstream all;
        all << in.rdbuf();
        chain_parse(all.str());
        message(trf("chain.opened", {path, std::to_string(chain_.size())}));
    }
    catch (const std::exception& ex)
    {
        message(trf("chain.open_failed", {ex.what()}));
    }
}

void Hallway::draw_chain(float x, float top, float w, float bottom)
{
    if (!chain_ready_) chain_defaults();
    const SDL_Color ink = menu_ink(theme()), white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    float y = top;
    for (const std::string& l : wrap_words(tr("chain.about"), size_t(std::max(8.0f, w / 8))))
    {
        text(x, y, l, 1, grey);
        y += 14;
    }
    y += 10;
    // The chain as a row of nodes, YOU first, wrapping onto more rows as it grows; every node the
    // same height, the arrows between them on its middle line.
    const float node_h = 36, gap = 28;
    std::vector<std::string> labels = {tr("chain.you")};
    for (const ChainStage& s : chain_) labels.push_back(chain_stage_name(s));
    float nx = x, ny = y;
    for (size_t i = 0; i < labels.size(); ++i)
    {
        const std::string label = fit(labels[i], std::min(w, 260.0f) - 16, 1);
        const float nw = text_width(label, 1) + 24;
        if (nx > x && nx + nw > x + w)
        {
            nx = x;
            ny += node_h + 16;
        }
        const bool sel = i > 0 && int(i) - 1 == chain_sel_;
        const SDL_FRect box{nx, ny, nw, node_h};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, sel ? 110 : 40);
        SDL_RenderFillRect(r_, &box);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &box);
        if (sel)
        {
            const SDL_FRect inner{nx + 2, ny + 2, nw - 4, node_h - 4};
            SDL_RenderRect(r_, &inner);
        }
        text(nx + 12, ny + (node_h - 8) / 2, label, 1, i == 0 ? grey : white);
        nx += nw;
        if (i + 1 < labels.size())
        {
            // An arrow to the next node: a line and its head, drawn to the pixel.
            const float ay = ny + node_h / 2;
            SDL_RenderLine(r_, nx + 4, ay, nx + gap - 6, ay);
            SDL_RenderLine(r_, nx + gap - 6, ay, nx + gap - 11, ay - 4);
            SDL_RenderLine(r_, nx + gap - 6, ay, nx + gap - 11, ay + 4);
            nx += gap;
        }
    }
    y = ny + node_h + 18;
    if (!chain_.empty())
    {
        const ChainStage& s = chain_[size_t(std::clamp(chain_sel_, 0, int(chain_.size()) - 1))];
        const char* what = s.kind == ChainStage::Kind::Draw ? "chain.what.draw" : s.kind == ChainStage::Kind::File ? "chain.what.file" : "chain.what.text";
        for (const std::string& l : wrap_words(trf("chain.selected", {std::to_string(chain_sel_ + 1), chain_stage_name(s)}) + " " + tr(what),
                                               size_t(std::max(8.0f, w / 8))))
        {
            text(x, y, l, 1, ink);
            y += 14;
        }
    }
    else text(x, y, tr("chain.empty"), 1, ink);
    const std::vector<std::string> keys = wrap_words(tr("chain.keys"), size_t(std::max(8.0f, w / 8)));
    float ky = bottom - 8 - 14 * float(keys.size());
    for (const std::string& l : keys)
    {
        text(x, ky, l, 1, grey);
        ky += 14;
    }
}

} // namespace hallway::hall
