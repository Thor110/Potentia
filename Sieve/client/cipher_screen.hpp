// Sieve hallway — the Encryption Engine, stage 2's first screen (IDEAS §19.52), opened from the
// main menu and the pause menu: bytes, a recipe of layers (cipher-v1, SPECIFICATIONS §12.0i), and
// what the recipe makes of them.
//
// Rows, chosen with Up/Down: the input (typed text, or bytes in hex), whether it is text or hex,
// the recipe ("vigenere lemon; feistel my key"), and what to do: encrypt, decrypt, or try to break
// it (the weak ciphers' guesses, ranked). Typing edits the row chosen; Left/Right change a choice;
// Enter runs it; Tab puts the output in as the input (in hex), so a result can be decrypted, broken
// or encrypted again. Ctrl+V pastes into the row chosen, Ctrl+C copies the output's hex. Esc goes
// back. The output is worked out again whenever it is run, never kept apart from what it shows.
//
// Files and items (stage 2, IDEAS §19.79 and §19.82): the input can be a file's bytes instead of
// what was typed, from the item in hand (I on the item page opens the engine with it; Ctrl+I takes
// it again), or from a file (Ctrl+O). Then INPUT names it and shows its first bytes, and typing in
// the row goes back to typed input. The output can be saved as a file (Ctrl+S), or gone to as an
// item: GO TO IT (Ctrl+G) leaves the engine on the binary line, in front of the output's bytes. A
// row of buttons under the rows does the same four things with the mouse.
#pragma once

#include "dialogs.hpp"
#include <SDL3/SDL.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

class CipherScreen
{
public:
    CipherScreen(SDL_Window* window, SDL_Renderer* renderer);
    enum class Result { Back, Quit, GoTo };
    Result run();
    // The item in hand, as F would save it: offered to Ctrl+I and its button, and taken as the
    // input at once.
    void give_item(std::vector<uint8_t> bytes, std::string name);
    // A file's bytes as the input (Ctrl+O's, or --cipher-in in scripted runs).
    void set_input_file(std::vector<uint8_t> bytes, std::string name);
    // Whether GO TO IT is offered (not in a scripted picture, which has nowhere to go).
    void allow_go(bool on) { can_go_ = on; }
    // After Result::GoTo: the bytes to walk to on the binary line, and the name they go by.
    const std::vector<uint8_t>& go_bytes() const { return out_; }
    std::string go_name() const;
    // The output's bytes written to a file (Ctrl+S's, or --cipher-out); false, with the status
    // saying why, if there is no output or it cannot be written.
    bool save_output(const std::string& path);
    // From the file windows' callbacks, on any thread: applied on the main thread (poll_chosen).
    void file_chosen(const std::string& path, bool for_input);
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void type(const std::string& utf8);          // scripting: text typed into the row chosen
    void render();
    const std::string& status() const { return status_; }
    // The output's bytes in hex (empty when the last run failed or was a break).
    std::string output_hex() const;
    // The lines shown under OUTPUT, as they were last worked out.
    const std::vector<std::string>& output_lines() const { return lines_; }

private:
    enum Row { kInput, kInputKind, kRecipe, kAction, kRows };
    enum class Action { Encrypt, Decrypt, Break };
    void key(SDL_Keycode k, SDL_Keymod mod);
    void handle(const SDL_Event& e);
    void run_recipe(); // works the output out again
    std::string* text_row(); // the row being typed into, if the row chosen takes text
    void typed_input();      // typing into INPUT: back to typed input, from a file's
    void poll_chosen();
    enum Button { kItemButton, kFileButton, kSaveButton, kGoButton, kButtons };
    void button(int b);

    SDL_Window* window_;
    SDL_Renderer* r_;
    bool done_ = false;
    Result result_ = Result::Back;
    int row_ = kInput;
    std::string input_ = "attack at dawn", recipe_ = "vigenere lemon; feistel my key";
    bool input_hex_ = false;
    Action action_ = Action::Encrypt;
    std::vector<uint8_t> out_;
    bool have_out_ = false;
    std::vector<std::string> lines_;
    std::string status_;
    std::vector<SDL_FRect> row_rects_;
    // The input as a file's bytes (the item in hand, or a file), when it is not what was typed.
    std::optional<std::vector<uint8_t>> in_bytes_;
    std::string in_name_;
    std::optional<std::vector<uint8_t>> item_; // the item in hand, as it was given
    std::string item_name_;
    bool can_go_ = true;
    std::vector<SDL_FRect> button_rects_;
    std::mutex chosen_mx_;
    std::optional<std::pair<std::string, bool>> chosen_; // a path, and whether it is the input (else the output's file)
    // Last, so it is the first let go: its dialogs' answers after it has gone are dropped (dialogs.hpp).
    hallway::DialogOwner dialog_owner_{this};
};

} // namespace hallway
