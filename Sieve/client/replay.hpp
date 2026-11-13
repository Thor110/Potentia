// Sieve hallway — recording and replay (IDEAS 19.184, its first step): what you did in the hallway kept
// as where you began and every input in order with its time, and played back by running the same steps
// again, so a walk costs a few bytes a frame and plays back exactly (the hallway is deterministic given
// its settings, its start and its inputs).
//
// "sieve-replay-v1", text, every line ending in a line feed:
//     sieve-replay-v1
//     start line=NAME tile=N x=X y=Y z=Z yaw=YAW pitch=PITCH     (N the corridor tile in full, '-' first when
//                                                                     below 0; the camera exactly: %.9g)
//     f DT HELD TOKEN...                                              (one line a frame)
// DT is the frame's time in microseconds (1 to 100,000; the hallway's own frame time is rounded to it
// as it is recorded, so the recording and the replay take the same steps). HELD is the keys held down,
// their SDL scancodes in increasing order separated by commas, or "-". Each TOKEN is one input event
// of that frame, in order:
//     kd:SC:KEY:MOD:REPEAT   a key pressed (scancode, keycode, modifiers, 1 if a repeat)
//     ku:SC:KEY:MOD:0        a key let go
//     tx:HEX                 text typed (its UTF-8 bytes, lower-case hex)
//     mm:X:Y:DX:DY           the mouse moved (where to, and by how much)
//     bd:B:CLICKS:X:Y        a mouse button pressed;  bu:B:CLICKS:X:Y  let go
//     mw:DX:DY               the wheel turned
// Nothing else is kept (window events, quitting). The replay must start where the recording did: the
// same settings and options, so the same line and tile, which the start line names and the replay
// checks; the camera is then put exactly where it was.
#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hallway {

inline constexpr const char* kReplayVersion = "sieve-replay-v1";
inline constexpr uint32_t kReplayMaxDtUs = 100000; // (the hallway's own longest step: 0.1 s)

struct ReplayFrame
{
    uint32_t dt_us = 0;
    std::vector<uint16_t> held;      // scancodes, increasing
    std::vector<SDL_Event> events;   // as they came
};

struct Replay
{
    std::string line, tile;
    float x = 0, y = 0, z = 0, yaw = 0, pitch = 0;
    std::vector<ReplayFrame> frames;
    std::string text() const;
    double seconds() const;
};
// One frame's line, as text() writes it (its length is what a recording grows by).
std::string replay_frame_line(const ReplayFrame& f);

// One event as a token, or empty for one that is not kept.
std::string replay_token(const SDL_Event& e);
// A recording read back, checked: its version, its start, every frame's line. Throws
// std::invalid_argument saying which line is wrong and why.
Replay read_replay(const std::string& text);

// The keys held, from a keyboard state (SDL_GetKeyboardState), as a frame keeps them.
std::vector<uint16_t> held_keys(const bool* keys);
// And back: a keyboard state of SDL_SCANCODE_COUNT entries with those keys down.
std::array<bool, SDL_SCANCODE_COUNT> keyboard_of(const std::vector<uint16_t>& held);

} // namespace hallway
