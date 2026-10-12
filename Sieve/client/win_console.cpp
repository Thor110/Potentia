#include "win_console.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#endif

namespace hallway {

void attach_parent_console()
{
#ifdef _WIN32
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE) return; // redirected: the handles are already there
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return;         // started from Explorer: no console wanted
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
#endif
}

} // namespace hallway
