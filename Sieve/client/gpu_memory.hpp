// Sieve hallway — what the program's textures take of the graphics memory.
//
// SDL cannot ask the card how much memory it has or how much is used, so the setup menu's graphics
// bar works it out (menu.cpp world_graphics_mb). Every texture the program makes goes through
// create() and destroy() here, which keep a running count of their bytes by what they are for:
// the world (the font's atlases, the doors' signs and portals, the close-up pictures, Real
// Graphics' frame) and the items' pictures (the display cache, which the menu sizes on its own).
#pragma once

#include <SDL3/SDL.h>

#include <cstddef>

namespace hallway::gpu {

enum class Use
{
    World,    // everything but the items' pictures
    Pictures, // the items' pictures (item_faces.cpp)
};

// SDL_CreateTexture, counted under `use` (nullptr, as SDL gives, when it cannot be made).
SDL_Texture* create(SDL_Renderer* r, SDL_PixelFormat format, SDL_TextureAccess access, int w, int h, Use use = Use::World);
// SDL_DestroyTexture of one made by create(), uncounted (nullptr is allowed).
void destroy(SDL_Texture* tex);
// The bytes held now by the textures made for `use`.
double bytes(Use use);
// The widest (and tallest) texture `r` can make. A renderer that states no limit gets the
// largest an int can double to, which leaves the memory as the only limit.
int max_texture_px(SDL_Renderer* r);

} // namespace hallway::gpu
