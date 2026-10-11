// Sieve hallway — the AI Training Harness (see harness.hpp).
#include "harness.hpp"

#include "font.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <thread>

namespace hallway {

namespace {

namespace fs = std::filesystem;

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kGood{120, 230, 120, 255},
    kBad{255, 110, 110, 255}, kAccent{200, 140, 255, 255}; // the AI line's lilac

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }

void frame(SDL_Renderer* r, const SDL_FRect& f, SDL_Color c, Uint8 fill = 235)
{
    SDL_SetRenderDrawColor(r, 0, 0, 0, fill);
    SDL_RenderFillRect(r, &f);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
    SDL_RenderRect(r, &f);
}

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

std::string grouped(uint64_t n)
{
    std::string s = std::to_string(n);
    for (int i = int(s.size()) - 3; i > 0; i -= 3) s.insert(size_t(i), ",");
    return s;
}

// What a model says, as the shelves show it: printable ASCII and whole UTF-8 characters, any other
// byte a middle dot, a newline as a space.
std::string printable(const std::string& bytes)
{
    std::string out;
    for (size_t i = 0; i < bytes.size();)
    {
        const uint8_t c = uint8_t(bytes[i]);
        const size_t n = c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 1;
        bool whole = n > 1 && i + n <= bytes.size();
        for (size_t k = 1; whole && k < n; ++k) whole = (uint8_t(bytes[i + k]) & 0xC0) == 0x80;
        if (whole) out += bytes.substr(i, n), i += n;
        else
        {
            out += c == '\n' ? std::string(" ") : (c >= 0x20 && c < 0x7F) ? std::string(1, char(c)) : std::string("\xC2\xB7");
            ++i;
        }
    }
    return out;
}

void SDLCALL on_folder(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    static_cast<Harness*>(user)->picked(files[0]);
}

std::string duration(double secs)
{
    char b[48];
    if (secs < 90) std::snprintf(b, sizeof b, "%.0f s", std::max(1.0, secs));
    else if (secs < 90 * 60) std::snprintf(b, sizeof b, "%.0f min", secs / 60);
    else if (secs < 48 * 3600) std::snprintf(b, sizeof b, "%.1f h", secs / 3600);
    else std::snprintf(b, sizeof b, "%.1f days", secs / 86400);
    return b;
}

std::string megabytes(uint64_t bytes)
{
    char b[48];
    if (bytes < (uint64_t(1) << 30)) std::snprintf(b, sizeof b, "%llu MB", (unsigned long long)((bytes + (1 << 20) - 1) >> 20));
    else std::snprintf(b, sizeof b, "%.1f GB", double(bytes) / double(uint64_t(1) << 30));
    return b;
}

std::string kilobytes(uint64_t bytes)
{
    if (bytes >= (uint64_t(1) << 20)) return megabytes(bytes);
    return grouped((bytes + 1023) / 1024) + " KB";
}

const char* const kRates[] = {"0.00002", "0.00005", "0.0001", "0.0002", "0.0005", "0.001", "0.002", "0.005", "0.01", "0.02", "0.05", "0.1"};
const uint32_t kWindows[] = {9, 17, 33, 65, 129, 257, 513};
constexpr int kSettingRows = 9; // layers, width, heads, bits, learning rate, batch, window, epochs, reading

} // namespace

Harness::Harness(SDL_Window* window, SDL_Renderer* renderer, fs::path runs_dir, sieve::AiShape shape)
    : window_(window), r_(renderer), dir_(std::move(runs_dir)), line_shape_(shape)
{
    std::error_code ec;
    fs::create_directories(dir_, ec);
    scan();
    select(0);
}

Harness::~Harness()
{
    cancel_ = true;
    if (job_.valid()) job_.wait();
}

void Harness::say(std::string text, bool bad)
{
    status_ = std::move(text);
    status_bad_ = bad;
}

void Harness::scan()
{
    entries_.clear();
    std::error_code ec;
    for (const fs::directory_entry& e : fs::directory_iterator(dir_, ec))
    {
        const fs::path p = e.path();
        if (e.is_directory() && fs::exists(p / "training.ini")) entries_.push_back({p, u8(p.filename()), true});
        else if (e.is_regular_file() && p.extension() == ".sieve") entries_.push_back({p, u8(p.filename()), false});
    }
    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) { return a.name < b.name; });
}

void Harness::select(int row)
{
    row_ = std::clamp(row, 0, int(entries_.size()));
    run_.reset();
    load_error_.clear();
    outcome_.reset();
    verdict_.clear();
    says_.clear();
    curve_.clear();
    step_ = steps_ = 0;
    if (row_ >= int(entries_.size())) return;
    try
    {
        run_ = sieve::cli::load_training_run(entries_[size_t(row_)].path);
        steps_ = run_->steps();
    }
    catch (const std::exception& e)
    {
        load_error_ = e.what();
    }
}

std::vector<std::pair<Harness::Action, bool>> Harness::buttons() const
{
    const bool busy = training();
    const bool trained = outcome_ || (run_ && run_->result);
    const bool folder = row_ < int(entries_.size()) && entries_[size_t(row_)].folder;
    if (curriculum_)
        return {{Action::MoveUp, cur_row_ > 0}, {Action::MoveDown, cur_row_ + 1 < int(curriculum_->size())}, {Action::Reading, true},
                {Action::Start, true}, {Action::Cancel, true}};
    return {{Action::Train, !busy && run_.has_value()},
            {Action::GoTo, !busy && trained},
            {Action::Pack, !busy && folder && trained},
            {Action::Record, !busy}};
}

void Harness::act(Action a)
{
    for (const auto& [b, ok] : buttons())
        if (b == a && !ok) return;
    switch (a)
    {
    case Action::Train: start_training(); break;
    case Action::Record:
        // The corpus: a folder, chosen in the system's folder dialog (or, the dialog aside, its path
        // typed, as scripts do).
        prompting_ = true;
        prompt_.clear();
        SDL_StartTextInput(window_);
        if (!scripted_) SDL_ShowOpenFolderDialog(on_folder, this, window_, nullptr, false);
        break;
    case Action::MoveUp:
    case Action::MoveDown:
    {
        const int to = cur_row_ + (a == Action::MoveUp ? -1 : 1);
        std::swap((*curriculum_)[size_t(cur_row_)], (*curriculum_)[size_t(to)]);
        cur_row_ = to;
        break;
    }
    case Action::Reading:
        recipe_.reading = recipe_.reading == sieve::training::Reading::Shuffled           ? sieve::training::Reading::InOrder
                          : recipe_.reading == sieve::training::Reading::InOrder          ? sieve::training::Reading::ShuffledWithinFiles
                                                                                          : sieve::training::Reading::Shuffled;
        break;
    case Action::Cancel: curriculum_.reset(); break;
    case Action::Start:
    {
        // The new run, read in the order the list now has, in a new folder beside the others.
        std::string name = new_name_;
        for (int n = 2; fs::exists(dir_ / from_u8(name)) || fs::exists(dir_ / from_u8(name + ".sieve")); ++n) name = new_name_ + "-" + std::to_string(n);
        try
        {
            sieve::cli::RecordOptions o;
            o.recipe = recipe_;
            sieve::cli::record_training_run(*curriculum_, dir_ / from_u8(name), o);
            curriculum_.reset();
            scan();
            for (size_t i = 0; i < entries_.size(); ++i)
                if (entries_[i].name == name) select(int(i));
            say(trf("harness.recorded", {name}));
            start_training();
        }
        catch (const std::exception& ex)
        {
            say(trf("harness.failed", {ex.what()}), true);
        }
        break;
    }
    case Action::GoTo:
    {
        const sieve::AiSpace space(run_->recipe.shape, run_->recipe.key);
        go_shape = run_->recipe.shape;
        if (outcome_) go_digits = outcome_->digits;
        else
        {
            const sieve::training::Result res = sieve::training::Result::parse(*run_->result);
            go_digits = space.digits_at(space.parse(res.final_address), sieve::AddressMode::Positional);
        }
        result_ = Result::GoTo;
        done_ = true;
        break;
    }
    case Action::Pack:
    {
        const Entry& e = entries_[size_t(row_)];
        const fs::path out = dir_ / from_u8(e.name + ".sieve");
        if (fs::exists(out))
        {
            say(trf("harness.pack.exists", {u8(out.filename())}));
            break;
        }
        try
        {
            sieve::cli::pack_training_run(e.path, out);
            say(trf("harness.packed", {u8(out.filename()), grouped(fs::file_size(out))}));
            const std::string name = e.name;
            scan();
            for (size_t i = 0; i < entries_.size(); ++i)
                if (entries_[i].name == name) row_ = int(i);
        }
        catch (const std::exception& ex)
        {
            say(trf("harness.failed", {ex.what()}), true);
        }
        break;
    }
    }
}

void Harness::start_training()
{
    if (!run_ || training()) return;
    outcome_.reset();
    verdict_.clear();
    says_.clear();
    curve_.clear();
    step_ = 0;
    steps_ = run_->steps();
    cancel_ = false;
    started_ = std::chrono::steady_clock::now();
    say(tr("harness.training"));
    const sieve::cli::TrainingRun run = *run_;
    job_ = std::async(std::launch::async, [this, run] {
        // The curve: about 200 points over the run, each the bits a byte since the point before
        // (`recent` counts from the last checkpoint, so what it had at the last point is taken off).
        double bits_then = 0;
        uint64_t predictions_then = 0;
        return sieve::cli::train_run(run, 0, [&, this](uint64_t k, uint64_t n, const sieve::training::Score& recent) {
            std::lock_guard<std::mutex> lock(mu_);
            step_ = k;
            steps_ = n;
            const uint64_t stride = std::max<uint64_t>(1, n / 200);
            if (k % stride == 0 || k == n)
            {
                if (recent.predictions < predictions_then) bits_then = 0, predictions_then = 0; // a checkpoint came between
                if (recent.predictions > predictions_then)
                    curve_.push_back(float((recent.bits - bits_then) / double(recent.predictions - predictions_then)));
                bits_then = recent.bits, predictions_then = recent.predictions;
            }
            if (k % run.recipe.checkpoint_every == 0) bits_then = 0, predictions_then = 0; // `recent` starts again after this
        }, &cancel_);
    });
}

void Harness::picked(const std::string& folder)
{
    std::lock_guard<std::mutex> lock(mu_);
    picked_ = folder;
}

void Harness::poll()
{
    std::optional<std::string> folder;
    {
        std::lock_guard<std::mutex> lock(mu_);
        folder.swap(picked_);
    }
    if (folder && prompting_)
    {
        prompting_ = false;
        SDL_StopTextInput(window_);
        open_curriculum(*folder);
    }
    if (trial_)
    {
        std::lock_guard<std::mutex> lock(trial_->m);
        if (trial_->done)
        {
            step_secs_ = trial_->secs;
            measured_for_ = trial_for_;
        }
    }
    if (trial_ && measured_for_ == trial_for_) trial_.reset();
    if (curriculum_) start_trial();
    if (!job_.valid() || job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    took_ = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count();
    try
    {
        sieve::cli::TrainingOutcome out = job_.get();
        if (out.cancelled)
        {
            say(tr("harness.stopped"));
            return;
        }
        const Entry& e = entries_[size_t(row_)];
        if (run_->result)
        {
            if (const auto diff = sieve::cli::result_difference(*run_->result, out.text))
            {
                verdict_ = trf("harness.verdict.differs", {*diff});
                verdict_colour_ = kBad;
            }
            else
            {
                verdict_ = tr("harness.verdict.same");
                verdict_colour_ = kGood;
            }
        }
        else if (e.folder)
        {
            std::FILE* f = nullptr;
#ifdef _WIN32
            _wfopen_s(&f, (e.path / "result.txt").wstring().c_str(), L"wb");
#else
            f = std::fopen((e.path / "result.txt").string().c_str(), "wb");
#endif
            if (f)
            {
                std::fwrite(out.text.data(), 1, out.text.size(), f);
                std::fclose(f);
            }
            run_->result = out.text;
            verdict_ = tr("harness.verdict.written");
            verdict_colour_ = kGood;
        }
        else
        {
            verdict_ = tr("harness.verdict.unchecked");
            verdict_colour_ = kGrey;
        }
        // What it became, in its own words: the model continues a newline.
        const sieve::AiSpace space(run_->recipe.shape, run_->recipe.key);
        sieve::llm::Model model(space.config(), space.tensors_of(out.digits), 1);
        const sieve::llm::Tokenizer tok(sieve::json::parse(sieve::AiSpace::tokenizer_json()));
        sieve::llm::Sampling sm;
        sm.temperature = 0.5f;
        sm.top_p = 1.0f;
        sm.seed = 1;
        sieve::llm::Sampler sampler(sm);
        says_ = printable(sieve::llm::continue_text(model, tok, "\n", sampler, 80));
        char t[32];
        std::snprintf(t, sizeof t, "%.1f", took_);
        say(trf("harness.trained", {grouped(out.result.steps), t}));
        outcome_ = std::move(out);
    }
    catch (const std::exception& ex)
    {
        say(trf("harness.failed", {ex.what()}), true);
    }
}

void Harness::settle()
{
    for (int i = 0; i < 4; ++i) // a training, and the trials a change of settings starts
    {
        if (job_.valid()) job_.wait();
        while (trial_)
        {
            poll();
            if (trial_) SDL_Delay(5);
        }
        poll();
    }
}

Harness::Result Harness::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    music_mode(MusicMode::Menus);
    music_colours_default();
    while (!done_)
    {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 16))
        {
            handle(e);
            while (SDL_PollEvent(&e)) handle(e);
        }
        poll();
        render();
        present(r_);
    }
    if (prompting_) SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void Harness::press(SDL_Keycode k, SDL_Keymod mod)
{
    key(k, mod);
    poll();
}

void Harness::type(const std::string& utf8)
{
    if (prompting_) prompt_ += utf8;
}

void Harness::handle(const SDL_Event& event)
{
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT)
    {
        cancel_ = true;
        result_ = Result::Quit;
        done_ = true;
        return;
    }
    if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        type(e.text.text);
        return;
    }
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        key(e.key.key, e.key.mod);
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT && !prompting_)
    {
        for (const auto& [rect, i] : cur_rects_)
            if (inside(rect, e.button.x, e.button.y))
            {
                cur_row_ = i;
                focus_settings_ = false;
                return;
            }
        for (const auto& [rect, i] : set_rects_)
            if (inside(rect, e.button.x, e.button.y))
            {
                // A setting: chosen, and stepped by clicking its right half up, its left half down.
                if (focus_settings_ && set_row_ == i) change_setting(i, e.button.x > rect.x + rect.w / 2 ? 1 : -1, false);
                focus_settings_ = true;
                set_row_ = i;
                return;
            }
        for (const auto& [rect, i] : row_rects_)
            if (inside(rect, e.button.x, e.button.y) && !training())
            {
                if (i == int(entries_.size())) act(Action::Record);
                else select(i);
                return;
            }
        for (const auto& [rect, a] : button_rects_)
            if (inside(rect, e.button.x, e.button.y))
            {
                act(a);
                return;
            }
    }
}

void Harness::open_curriculum(const std::string& folder)
{
    // The corpus's files, to put in the order they are to be read (the curriculum), and the new
    // run's settings, starting from the AI line's shape with the learning rate suggested for it.
    const fs::path corpus = from_u8(folder);
    try
    {
        curriculum_ = sieve::cli::corpus_files(corpus);
        cur_row_ = 0;
        focus_settings_ = false;
        set_row_ = 0;
        recipe_ = {};
        recipe_.shape = line_shape_;
        recipe_.learning_rate = sieve::cli::suggested_learning_rate(recipe_);
        lr_suggested_ = true;
        new_name_ = u8(fs::absolute(corpus).lexically_normal().filename());
        if (new_name_.empty()) new_name_ = u8(fs::absolute(corpus).lexically_normal().parent_path().filename());
        if (new_name_.empty()) new_name_ = "run";
        say(tr("harness.curriculum.status"));
    }
    catch (const std::exception& ex)
    {
        say(trf("harness.failed", {ex.what()}), true);
    }
}

void Harness::change_setting(int row, int dir, bool big)
{
    sieve::training::Recipe& rc = recipe_;
    sieve::AiShape& sh = rc.shape;
    auto even_heads = [&](uint32_t h) { return h >= 1 && sh.width % h == 0 && (sh.width / h) % 2 == 0; };
    switch (row)
    {
    case 0: sh.layers = uint32_t(std::clamp(int(sh.layers) + dir * (big ? 4 : 1), 1, 64)); break;
    case 1:
    {
        // The width in steps of two heads' worth, so the heads still divide it into even halves
        // (Shift: doubled or halved).
        const uint32_t step = 2 * sh.heads;
        uint32_t w = big ? (dir > 0 ? sh.width * 2 : sh.width / 2) : uint32_t(int(sh.width) + dir * int(step));
        w = std::max(step, w / step * step);
        sh.width = std::min(w, 4096 / step * step);
        break;
    }
    case 2:
    {
        // The heads: the next number either way that divides the width into heads of an even size.
        for (int h = int(sh.heads) + dir; h >= 1 && h <= int(sh.width); h += dir)
            if (even_heads(uint32_t(h)))
            {
                sh.heads = uint32_t(h);
                break;
            }
        break;
    }
    case 3: sh.bits = uint32_t(std::clamp(int(sh.bits) + dir, 1, 8)); break;
    case 4:
    {
        int i = 0;
        for (int k = 0; k < int(std::size(kRates)); ++k)
            if (std::stod(kRates[k]) <= std::stod(rc.learning_rate) * (1 + 1e-9)) i = k;
        i = std::clamp(i + dir, 0, int(std::size(kRates)) - 1);
        rc.learning_rate = kRates[i];
        lr_suggested_ = rc.learning_rate == sieve::cli::suggested_learning_rate(rc);
        return;
    }
    case 5: rc.batch = uint32_t(std::clamp(int(rc.batch) + dir * (big ? 4 : 1), 1, 32)); break;
    case 6:
    {
        int i = 0;
        for (int k = 0; k < int(std::size(kWindows)); ++k)
            if (kWindows[k] <= rc.window) i = k;
        rc.window = kWindows[std::clamp(i + dir, 0, int(std::size(kWindows)) - 1)];
        break;
    }
    case 7: rc.epochs = uint32_t(std::clamp(int(rc.epochs) + dir * (big ? 10 : 1), 1, 10000)); break;
    case 8: act(Action::Reading); return;
    default: return;
    }
    // A change of shape, batch or window moves the suggested rate, and the rate with it while it
    // is still the one suggested.
    if (lr_suggested_) rc.learning_rate = sieve::cli::suggested_learning_rate(rc);
}

void Harness::start_trial()
{
    if (!curriculum_ || trial_) return;
    const sieve::training::Recipe& rc = recipe_;
    const std::string key = std::to_string(rc.shape.layers) + "/" + std::to_string(rc.shape.width) + "/" + std::to_string(rc.shape.heads) + "/" +
                            std::to_string(rc.shape.bits) + "/" + std::to_string(rc.batch) + "/" + std::to_string(rc.window);
    if (key == measured_for_) return;
    // Nothing is measured that would not fit: a shape past half this computer's memory says so instead.
    const sieve::cli::TrainingEstimate est = sieve::cli::estimate_training(rc, *curriculum_);
    if (est.bytes > uint64_t(SDL_GetSystemRAM()) * (uint64_t(1) << 20) / 2)
    {
        measured_for_ = key;
        step_secs_ = -1;
        return;
    }
    trial_for_ = key;
    const sieve::training::Recipe copy = rc;
    const std::vector<sieve::cli::TrainingFile> files = *curriculum_;
    trial_ = std::make_shared<Trial>();
    std::thread([t = trial_, copy, files] {
        double secs = -1;
        try
        {
            secs = sieve::cli::measure_training_step(copy, files);
        }
        catch (const std::exception&)
        {
        }
        std::lock_guard<std::mutex> lock(t->m);
        t->secs = secs;
        t->done = true;
    }).detach();
}

void Harness::key(SDL_Keycode k, SDL_Keymod mod)
{
    if (prompting_)
    {
        if (k == SDLK_ESCAPE)
        {
            prompting_ = false;
            SDL_StopTextInput(window_);
        }
        else if (k == SDLK_BACKSPACE && !prompt_.empty())
        {
            prompt_.pop_back();
            while (!prompt_.empty() && (uint8_t(prompt_.back()) & 0xC0) == 0x80) prompt_.pop_back();
        }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER)
        {
            prompting_ = false;
            SDL_StopTextInput(window_);
            std::string path = prompt_;
            if (path.size() >= 2 && path.front() == '"' && path.back() == '"') path = path.substr(1, path.size() - 2);
            open_curriculum(path);
        }
        return;
    }
    if (training())
    {
        if (k == SDLK_ESCAPE) cancel_ = true;
        return;
    }
    if (curriculum_)
    {
        const bool shift = (mod & SDL_KMOD_SHIFT) != 0;
        const int n = int(curriculum_->size());
        switch (k)
        {
        case SDLK_ESCAPE: act(Action::Cancel); break;
        case SDLK_TAB: focus_settings_ = !focus_settings_; break;
        case SDLK_UP:
            if (focus_settings_) set_row_ = std::max(0, set_row_ - 1);
            else if (shift) act(Action::MoveUp);
            else cur_row_ = std::max(0, cur_row_ - 1);
            break;
        case SDLK_DOWN:
            if (focus_settings_) set_row_ = std::min(kSettingRows - 1, set_row_ + 1);
            else if (shift) act(Action::MoveDown);
            else cur_row_ = std::min(n - 1, cur_row_ + 1);
            break;
        case SDLK_LEFT:
        case SDLK_RIGHT:
            if (focus_settings_) change_setting(set_row_, k == SDLK_LEFT ? -1 : 1, shift);
            break;
        case SDLK_M: act(Action::Reading); break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: act(Action::Start); break;
        default: break;
        }
        return;
    }
    switch (k)
    {
    case SDLK_ESCAPE:
        result_ = Result::Back;
        done_ = true;
        break;
    case SDLK_UP: select(row_ - 1); break;
    case SDLK_DOWN: select(row_ + 1); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: act(row_ == int(entries_.size()) ? Action::Record : Action::Train); break;
    case SDLK_G: act(Action::GoTo); break;
    case SDLK_P: act(Action::Pack); break;
    case SDLK_N: act(Action::Record); break;
    default: break;
    }
}

void Harness::render()
{
    constexpr int kMinW = 1024, kMinH = 640;
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    txt(r_, 16, 12, tr("harness.title"), kAccent, 2.5f);
    txt(r_, 16, 38, trf("harness.folder", {u8(dir_)}), kGrey);

    // The runs, on the left.
    const float top = 60, listW = 340, rowH = 18;
    frame(r_, {8, top, listW, H - top - 60}, kDim);
    txt(r_, 16, top + 8, tr("harness.runs"), kWhite);
    row_rects_.clear();
    float y = top + 28;
    for (int i = 0; i <= int(entries_.size()); ++i)
    {
        const bool sel = i == row_;
        const SDL_FRect rr{12, y - 3, listW - 8, rowH};
        if (sel)
        {
            SDL_SetRenderDrawColor(r_, 48, 0, 80, 255);
            SDL_RenderFillRect(r_, &rr);
        }
        row_rects_.push_back({rr, i});
        if (i < int(entries_.size()))
        {
            const Entry& e = entries_[size_t(i)];
            txt(r_, 20, y, (e.name + (e.folder ? "/" : "")).substr(0, 38), sel ? kWhite : kGrey);
        }
        else txt(r_, 20, y, tr("harness.record"), sel ? kAccent : kGrey);
        y += rowH;
    }
    if (entries_.empty()) txt(r_, 20, y + 8, tr("harness.none"), kDim);

    // The run, on the right.
    const float x = listW + 28, colW = W - x - 16;
    float ry = top + 8;
    auto line = [&](const std::string& label, const std::string& value, SDL_Color c = kWhite) {
        txt(r_, x, ry, label, kGrey);
        txt(r_, x + 150, ry, value.substr(0, size_t(std::max(0.0f, (colW - 150) / 8))), c);
        ry += 16;
    };
    if (prompting_)
    {
        txt(r_, x, ry, tr("harness.prompt.title"), kWhite, 1.5f);
        ry += 28;
        txt(r_, x, ry, tr("harness.prompt.help"), kGrey);
        ry += 22;
        frame(r_, {x, ry - 4, colW, 20}, kAccent);
        txt(r_, x + 6, ry, prompt_ + "_", kWhite);
    }
    else if (curriculum_)
    {
        txt(r_, x, ry, trf("harness.curriculum.title", {new_name_}), kWhite, 1.5f);
        ry += 26;
        // The settings, a row each; the one chosen highlighted while the settings have the keys.
        const sieve::training::Recipe& rc = recipe_;
        const sieve::AiShape& sh = rc.shape;
        const std::string suggested = sieve::cli::suggested_learning_rate(rc);
        const std::string values[kSettingRows] = {
            std::to_string(sh.layers),
            trf("harness.set.width.value", {std::to_string(sh.width), std::to_string(sh.ffn())}),
            trf("harness.set.heads.value", {std::to_string(sh.heads), std::to_string(sh.width / sh.heads)}),
            trf("harness.set.bits.value", {std::to_string(sh.bits), std::to_string(1u << sh.bits)}),
            rc.learning_rate + "  " + (rc.learning_rate == suggested ? tr("harness.set.suggested") : trf("harness.set.suggest", {suggested})),
            std::to_string(rc.batch),
            trf("harness.set.window.value", {std::to_string(rc.window), std::to_string(rc.window - 1)}),
            std::to_string(rc.epochs),
            tr(std::string("harness.reading.") + sieve::training::to_string(rc.reading))};
        static const char* const kLabels[kSettingRows] = {"harness.set.layers", "harness.set.width", "harness.set.heads", "harness.set.bits",
                                                         "harness.set.rate", "harness.set.batch", "harness.set.window", "harness.set.epochs",
                                                         "harness.reading"};
        set_rects_.clear();
        for (int i = 0; i < kSettingRows; ++i)
        {
            const SDL_FRect rr{x - 4, ry - 3, std::min(colW, 620.0f), 16};
            if (focus_settings_ && i == set_row_)
            {
                SDL_SetRenderDrawColor(r_, 48, 0, 80, 255);
                SDL_RenderFillRect(r_, &rr);
            }
            set_rects_.push_back({rr, i});
            txt(r_, x, ry, tr(kLabels[i]), kGrey);
            const bool chosen = focus_settings_ && i == set_row_;
            if (chosen) txt(r_, x + 136, ry, "<", kAccent);
            txt(r_, x + 150, ry, values[i], chosen ? kWhite : kGrey);
            if (chosen) txt(r_, x + 158 + text_width(values[i], 1), ry, ">", kAccent);
            ry += 16;
        }
        // What it will take: worked out from the settings, and a step measured on this computer.
        const sieve::cli::TrainingEstimate est = sieve::cli::estimate_training(rc, *curriculum_);
        const uint64_t ram = uint64_t(SDL_GetSystemRAM()) * (uint64_t(1) << 20);
        const std::string key = std::to_string(sh.layers) + "/" + std::to_string(sh.width) + "/" + std::to_string(sh.heads) + "/" +
                                std::to_string(sh.bits) + "/" + std::to_string(rc.batch) + "/" + std::to_string(rc.window);
        ry += 6;
        line(tr("harness.est.size"), trf("harness.est.size.value", {grouped(est.weights), grouped(est.steps), std::to_string(est.threads)}));
        const bool too_big = est.bytes > ram / 2;
        line(tr("harness.est.memory"), too_big ? trf("harness.est.memory.over", {megabytes(est.bytes), megabytes(ram)}) : trf("harness.est.memory.value", {megabytes(est.bytes)}),
             too_big ? kBad : kWhite);
        std::string time;
        if (too_big) time = tr("harness.est.time.none");
        else if (measured_for_ != key) time = tr("harness.est.time.measuring");
        else if (step_secs_ < 0) time = tr("harness.est.time.none");
        else
        {
            char ms[32];
            std::snprintf(ms, sizeof ms, step_secs_ < 1 ? "%.1f ms" : "%.2f s", step_secs_ < 1 ? step_secs_ * 1000 : step_secs_);
            time = trf("harness.est.time.value", {duration(step_secs_ * double(est.steps)), ms});
        }
        line(tr("harness.est.time"), time);
        line(tr("harness.est.result"), trf("harness.est.result.value", {kilobytes(est.result_bytes)}));
        if (est.learning_rate_at_floor)
        {
            txt(r_, x, ry, tr("harness.est.floor"), kAccent);
            ry += 16;
        }
        ry += 6;
        txt(r_, x, ry, tr("harness.curriculum.list"), kGrey);
        ry += 18;
        cur_rects_.clear();
        const int fit = std::max(1, int((H - 120 - ry) / 16));
        const int first = std::clamp(cur_row_ - fit / 2, 0, std::max(0, int(curriculum_->size()) - fit));
        for (int i = first; i < std::min(int(curriculum_->size()), first + fit); ++i)
        {
            const auto& f = (*curriculum_)[size_t(i)];
            const SDL_FRect rr{x - 4, ry - 3, colW, 16};
            if (i == cur_row_ && !focus_settings_)
            {
                SDL_SetRenderDrawColor(r_, 48, 0, 80, 255);
                SDL_RenderFillRect(r_, &rr);
            }
            cur_rects_.push_back({rr, i});
            txt(r_, x, ry, std::to_string(i + 1) + ".", kGrey);
            txt(r_, x + 40, ry, f.path.substr(7, size_t(std::max(0.0f, (colW - 200) / 8))), i == cur_row_ && !focus_settings_ ? kWhite : kGrey);
            txt(r_, x + colW - 120, ry, grouped(f.size) + " B", kDim);
            ry += 16;
        }
    }
    else if (row_ < int(entries_.size()) && !load_error_.empty())
    {
        txt(r_, x, ry, entries_[size_t(row_)].name, kWhite, 1.5f);
        ry += 28;
        txt(r_, x, ry, trf("harness.not_a_run", {load_error_.substr(0, size_t(colW / 8) - 20)}), kBad);
    }
    else if (run_)
    {
        const sieve::training::Recipe& rc = run_->recipe;
        const sieve::AiShape& s = rc.shape;
        txt(r_, x, ry, run_->name, kWhite, 1.5f);
        ry += 28;
        const uint64_t weights = 256ull * s.width + 16ull * s.layers * s.width * s.width;
        line(tr("harness.model"), trf(s.layers == 1 ? "harness.shape.one" : "harness.shape",
                                      {std::to_string(s.layers), std::to_string(s.width), std::to_string(s.heads), std::to_string(s.bits), grouped(weights)}));
        line(tr("harness.start"), std::string(sieve::to_string(rc.start_mode)) + " " + (rc.start.size() > 24 ? rc.start.substr(0, 24) + "..." : rc.start));
        line(tr("harness.learns"), trf("harness.recipe", {rc.learning_rate, rc.momentum, std::to_string(rc.batch)}));
        uint64_t bytes = 0;
        size_t files = 0;
        for (const auto& [p, b] : run_->files)
            if (p.rfind("corpus/", 0) == 0) bytes += b.size(), ++files;
        line(tr("harness.corpus"), trf(files == 1 ? "harness.corpus.value.one" : "harness.corpus.value", {grouped(files), grouped(bytes)}));
        line(tr("harness.order"), trf("harness.order.value", {grouped(run_->order.size()), grouped(run_->steps())}));
        // How it reads, and its files in the order it first reads them.
        std::vector<std::string> seen;
        for (const sieve::training::Window& w : run_->order)
            if (std::find(seen.begin(), seen.end(), w.path) == seen.end()) seen.push_back(w.path);
        std::string files_line;
        for (size_t i = 0; i < seen.size(); ++i) files_line += (i ? "  " : "") + std::to_string(i + 1) + ". " + seen[i].substr(seen[i].rfind("corpus/", 0) == 0 ? 7 : 0);
        if (rc.order == "order-v1")
            line(tr("harness.reading"), trf("harness.reads", {tr(std::string("harness.reading.") + sieve::training::to_string(rc.reading)),
                                                               std::to_string(rc.epochs), std::to_string(rc.window)}));
        else line(tr("harness.reading"), tr("harness.reads.written"));
        line(tr("harness.files"), files_line);
        line(tr("harness.result"), run_->result ? tr("harness.result.yes") : tr("harness.result.no"), run_->result ? kWhite : kGrey);
        ry += 10;

        // Progress, and how much it has learnt: bits a byte at each checkpoint.
        uint64_t step, steps;
        std::vector<float> curve;
        {
            std::lock_guard<std::mutex> lock(mu_);
            step = step_, steps = steps_, curve = curve_;
        }
        if (training() || outcome_ || !curve.empty())
        {
            const float bw = colW;
            frame(r_, {x, ry, bw, 14}, kDim);
            const float f = steps ? float(double(step) / double(steps)) : 0.0f;
            SDL_SetRenderDrawColor(r_, kAccent.r, kAccent.g, kAccent.b, 255);
            const SDL_FRect bar{x + 1, ry + 1, (bw - 2) * f, 12};
            SDL_RenderFillRect(r_, &bar);
            ry += 20;
            const double secs = training() ? std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count() : took_;
            char t[32];
            std::snprintf(t, sizeof t, "%.1f", secs);
            char b[32];
            std::snprintf(b, sizeof b, "%.3f", curve.empty() ? 0.0 : double(curve.back()));
            txt(r_, x, ry, trf("harness.progress", {grouped(step), grouped(steps), t, curve.empty() ? std::string("-") : std::string(b)}), kWhite);
            ry += 20;
            // The curve: 8 bits a byte (guessing among 256) at the top, 0 at the bottom.
            const SDL_FRect plot{x, ry, bw, std::min(160.0f, H - ry - 170)};
            if (plot.h > 40)
            {
                frame(r_, plot, kDim);
                txt(r_, plot.x + 4, plot.y + 4, tr("harness.curve"), kDim);
                if (curve.size() >= 2)
                {
                    SDL_SetRenderDrawColor(r_, kAccent.r, kAccent.g, kAccent.b, 255);
                    for (size_t i = 1; i < curve.size(); ++i)
                    {
                        auto px = [&](size_t j) { return plot.x + plot.w * float(j) / float(curve.size() - 1); };
                        auto py = [&](float v) { return plot.y + plot.h * (1.0f - std::clamp(v, 0.0f, 8.0f) / 8.0f); };
                        SDL_RenderLine(r_, px(i - 1), py(curve[i - 1]), px(i), py(curve[i]));
                    }
                }
                ry += plot.h + 10;
            }
        }
        if (!verdict_.empty())
        {
            txt(r_, x, ry, verdict_.substr(0, size_t(colW / 8)), verdict_colour_);
            ry += 18;
        }
        if (!says_.empty())
        {
            txt(r_, x, ry, tr("harness.says"), kGrey);
            ry += 16;
            txt(r_, x, ry, says_, kWhite);
            ry += 18;
        }
    }
    else txt(r_, x, ry, tr("harness.record.help"), kGrey);

    // The buttons, along the bottom of the right side.
    button_rects_.clear();
    float bx = x;
    const float by = H - 88;
    for (const auto& [a, ok] : buttons())
    {
        const char* id = a == Action::Train    ? (run_ && run_->result ? "harness.button.check" : "harness.button.train")
                         : a == Action::GoTo   ? "harness.button.goto"
                         : a == Action::Pack   ? "harness.button.pack"
                         : a == Action::MoveUp ? "harness.button.up"
                         : a == Action::MoveDown ? "harness.button.down"
                         : a == Action::Reading  ? "harness.button.reading"
                         : a == Action::Start    ? "harness.button.start"
                         : a == Action::Cancel   ? "harness.button.cancel"
                                                 : "harness.button.record";
        const std::string label = tr(id);
        const float bw = text_width(label, 1) + 20;
        const SDL_FRect br{bx, by, bw, 22};
        frame(r_, br, ok ? kAccent : kDim);
        txt(r_, bx + 10, by + 7, label, ok ? kWhite : kDim);
        button_rects_.push_back({br, a});
        bx += bw + 10;
    }
    txt(r_, 16, H - 46, status_.substr(0, size_t((W - 32) / 8)), status_bad_ ? kBad : kWhite);
    txt(r_, 16, H - 26, tr(training() ? "harness.footer.training" : prompting_ ? "harness.footer.prompt" : curriculum_ ? "harness.footer.curriculum" : "harness.footer"), kGrey);
}

} // namespace hallway
