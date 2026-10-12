// Sieve — the console of the command prompt a program was started from, on Windows.
#pragma once

namespace hallway {

// The hallway is a Windows program, not a console one, so starting it opens no console window.
// Started from a command prompt, this connects its output to that prompt's console. With its
// output redirected or piped (scripted runs, CI) it does nothing: the output already goes there.
// Elsewhere than Windows it does nothing.
void attach_parent_console();

} // namespace hallway
