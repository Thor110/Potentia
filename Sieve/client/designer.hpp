// Sieve hallway — the filter designer (docs/FILTER-PLUGINS.md section 12), opened from the main
// menu: make a filter plugin as nodes, test it as you go, and save it as a version.
//
// Three panels. On the left, the filters that exist for the line (the setup menu's list, with its
// BUILT-IN and CUSTOM tabs), for reference: what each does, and a way to pick one as a
// prerequisite or to open a custom one here. In the middle, the filter as nodes: the entry node
// (its prerequisites, each with a + for the settings it pins, and a + to add one), the header,
// the parameters, and the rule: for the token form a node per word set, joined by `follow`
// arrows; for the table form its lines in an editor. On the right, the test: survivors and what
// is excluded at a length, survivors by rank, text typed in and judged (and where it fails), the
// relations to every other custom filter (duplicates are named), and Save.
//
// The model (designer_model.hpp) holds the filter; this is only its screen. Compiling and the
// relation check run on a worker, so a large dictionary never freezes the window.
#pragma once

#include "designer_model.hpp"

#include <SDL3/SDL.h>

#include <functional>
#include <set>
#include <future>
#include <string>
#include <vector>

namespace hallway {

class Designer
{
public:
    Designer(SDL_Window* window, SDL_Renderer* renderer, const std::string& open_file = {});
    ~Designer();
    enum class Result { Back, Quit };
    Result run();
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void type(const std::string& utf8);          // scripting: text typed
    void settle();                               // scripting: wait for the worker, then take its result
    void render();
    const design::Doc& doc() const { return doc_; }
    const std::string& status() const { return status_; }

private:
    enum class Area { List, Canvas, Test };
    enum class FKind { Text, Choice, Toggle, Button, Info };
    struct Field
    {
        std::string node;       // the node it is on
        FKind kind = FKind::Info;
        std::string label, value;
        std::function<void(int)> change;                 // Choice / Toggle / Button: -1 or +1
        std::function<void(const std::string&)> set;     // Text: the edited value
        std::function<void()> remove;                    // Delete, where an entry can go
        SDL_FRect rect{};
    };
    struct Node
    {
        std::string key, title;
        std::vector<int> fields; // indices into fields_
        SDL_FRect rect{};
    };

    void build();                      // the nodes and fields, from the model
    void add(const std::string& node, FKind kind, const std::string& label, const std::string& value,
             std::function<void(int)> change = {}, std::function<void(const std::string&)> set = {}, std::function<void()> remove = {});
    void changed();                    // the model changed: test it again (on the worker)
    void poll();                       // take the worker's result when it is ready
    void handle(const SDL_Event& e);
    void key(SDL_Keycode k, SDL_Keymod mod);
    void activate(int dir);            // the selected field: change, toggle, press or start editing
    void start_edit();
    void finish_edit(bool keep);
    void open_file(const std::string& path);
    void list_key(SDL_Keycode k);
    std::vector<const sieve::FilterSpec*> list_rows() const;
    void open_editor(const std::string& target);
    void editor_key(SDL_Keycode k, SDL_Keymod mod);
    void close_editor();
    void do_save();
    void start_relations();
    std::vector<std::string> source_choices() const;

    void draw_list(float x, float y, float w, float h);
    void draw_canvas(float x, float y, float w, float h);
    void draw_test(float x, float y, float w, float h);
    void draw_editor(float W, float H);

    SDL_Window* window_;
    SDL_Renderer* r_;
    design::Doc doc_;
    std::string path_;      // the file it came from, if any
    bool dirty_ = false;
    std::string status_;
    Area area_ = Area::Canvas;
    // The canvas.
    std::vector<Field> fields_;
    std::vector<Node> nodes_;
    int sel_ = 0;
    bool editing_ = false;
    bool judge_edit_ = false; // the text being edited is the test's text to judge
    std::string edit_;
    std::vector<std::string> open_pins_; // prerequisites whose settings node is open
    std::string link_from_;              // a set waiting for the other end of a follow arrow
    float pan_x_ = 0, pan_y_ = 0;
    std::set<std::string> placed_;       // nodes put where they are (dragged, or read from the file)
    std::string dragging_;               // the node being dragged by its title
    float drag_dx_ = 0, drag_dy_ = 0;
    bool panning_ = false;
    float pan_mx_ = 0, pan_my_ = 0;
    SDL_FRect canvas_{};
    // The list.
    int tab_ = 0, list_row_ = 0, list_top_ = 0;
    bool picking_ = false;               // choosing a prerequisite
    std::vector<std::pair<SDL_FRect, int>> list_rects_;
    // The test.
    uint32_t length_ = 32;
    design::TestResult result_;
    std::string tested_; // the text last tested, with its length
    std::future<design::TestResult> job_;
    bool job_running_ = false, retest_ = false;
    std::string judge_text_ = "the cat sat on the mat", verdict_;
    std::vector<std::string> relations_;
    std::future<std::vector<std::string>> rel_job_;
    bool rel_running_ = false;
    uint32_t save_as_ = 0; // Save found this version taken: the next press saves as this one
    int test_sel_ = 0;
    std::vector<std::pair<SDL_FRect, int>> test_rects_;
    // The multi-line editor (the table's lines, or a word list's words).
    bool editor_open_ = false;
    std::string editor_target_;
    std::vector<std::string> editor_lines_;
    int ed_row_ = 0, ed_col_ = 0, ed_top_ = 0;
    bool done_ = false, leave_warned_ = false;
    Result result_code_ = Result::Back;
};

} // namespace hallway
