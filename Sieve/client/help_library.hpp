// Sieve hallway — the Documentation (IDEAS 19.201; list V2): Sieve's help library, opened from the main
// menu, laid out as the old help libraries of Windows programs were: on the left the library's
// CONTENTS (its categories, each with its pages), its INDEX (every term the pages are found under, in
// order) or a SEARCH (the pages holding every word typed, each with the line it was found on); on the
// right the page chosen, its links to other pages lit, followed in place, and BACK through the pages
// read.
//
// The library is data, not code (IDEAS 19.196): help/contents.txt lists the categories and their
// pages ("category ID Title", "page ID Title"), and each page is help/ID.txt: an "index:" line of its
// terms, then paragraphs, "## " subheadings, "- " list items and links written [[id]] or
// [[id|words]] ("1. " steps too). A key a page names is written {key1:SECTION/ID|shown}, so it reads as the key is set.
// The pages are written for the person using Sieve; the developers' documents stay in docs/ beside
// them.
//
// Keys (Settings > Controls > Keys, "The menus" and "The Documentation"; Tab is the menus' next): Up and Down choose a row of
// the list, or scroll the page; Right or Enter opens the row's page, Left goes back to the list; Tab
// (Shift+Tab back) moves through CONTENTS, INDEX and SEARCH on the list, and through the page's links
// on the page, where Enter follows the one chosen; Ctrl+F searches (typing is the search, Backspace
// takes a letter back); Backspace (or Alt+Left) goes back to the page read before; PgUp and PgDn
// scroll the page by a screen; Esc goes back to the main menu.
#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace hallway {

class HelpLibrary
{
public:
    // The library in `folder` (help/contents.txt and its pages), opened on its first page, or on
    // `page` where that names one.
    HelpLibrary(SDL_Window* window, SDL_Renderer* renderer, const std::filesystem::path& folder, const std::string& page = {});
    enum class Result { Back, Quit };
    Result run();
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void type(const std::string& utf8);          // scripting: typed into the search
    void render();
    // For scripted runs: the page shown (its id), the list's tab and row, and what the library holds.
    std::string page_id() const;
    std::string summary() const;
    std::string list_text() const;  // the list's rows as shown, one a line
    std::string page_text() const;  // the page's lines as laid out, its links in [brackets]
    const std::string& status() const { return status_; }

    // The library's folder: help beside the program (or data/help in the repository).
    static std::filesystem::path folder();

private:
    struct Page
    {
        std::string id, title, category;
        std::vector<std::string> terms; // its index line
        std::vector<std::string> lines; // as written, its index line taken off
    };
    struct Category
    {
        std::string id, title;
        std::vector<size_t> pages;
    };
    // A row of the list: a heading (a category, or an index term's letter) or a page.
    struct Row
    {
        std::string text;
        int page = -1; // -1: a heading
        int indent = 0;
    };
    // A line of the page as laid out: its text, and the links on it (their columns and targets).
    struct Link
    {
        size_t from = 0, to = 0; // characters of the line
        int number = 0;          // the page's link it is part of (a link wrapped over two lines is one)
    };
    struct Laid
    {
        std::string text;
        std::vector<Link> links;
        bool heading = false;
    };
    enum class Tab { Contents, Index, Search };

    void load(const std::filesystem::path& folder);
    void build_rows();
    void open(int page, bool remember);
    void lay_out(size_t cells);
    void key(SDL_Keycode k, SDL_Keymod mod);
    void handle(const SDL_Event& e);
    void choose_row(int dir);
    void step_link(int dir);
    int find_page(const std::string& id) const;
    std::string with_keys(const std::string& line) const;

    SDL_Window* window_;
    SDL_Renderer* r_;
    bool done_ = false;
    Result result_ = Result::Back;
    std::vector<Page> pages_;
    std::vector<Category> categories_;
    Tab tab_ = Tab::Contents;
    std::vector<Row> rows_;
    int row_ = 0;
    size_t list_top_ = 0;
    bool on_page_ = false;  // the keys move on the page, not the list
    int page_ = -1;
    std::vector<int> history_;
    std::string query_;
    std::vector<Laid> laid_;
    size_t laid_cells_ = 0;
    int laid_page_ = -1;
    size_t scroll_ = 0;     // the page's first line shown
    size_t page_rows_ = 1;  // the page's lines that fit
    std::vector<int> link_targets_; // each of the page's links, the page it goes to
    int link_ = -1;         // the link chosen, counted over the page's links in order
    std::string status_;
    // Where things were drawn, for the mouse.
    std::vector<std::pair<SDL_FRect, int>> row_boxes_, tab_boxes_, link_boxes_;
};

} // namespace hallway
