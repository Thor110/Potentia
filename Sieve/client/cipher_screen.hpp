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
#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hallway {

class CipherScreen
{
public:
    CipherScreen(SDL_Window* window, SDL_Renderer* renderer);
    enum class Result { Back, Quit };
    Result run();
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
};

} // namespace hallway
