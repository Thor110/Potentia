// Sieve hallway — counting what the program's textures take. See gpu_memory.hpp.
#include "gpu_memory.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace hallway::gpu {

namespace {

std::mutex g_mx;
std::unordered_map<SDL_Texture*, std::pair<Use, double>> g_made; // (g_mx)
std::array<double, 2> g_bytes{};                                // by Use (g_mx)

} // namespace

SDL_Texture* create(SDL_Renderer* r, SDL_PixelFormat format, SDL_TextureAccess access, int w, int h, Use use)
{
    SDL_Texture* tex = SDL_CreateTexture(r, format, access, w, h);
    if (!tex) return nullptr;
    const double b = double(w) * double(h) * double(SDL_BYTESPERPIXEL(format));
    std::lock_guard<std::mutex> lock(g_mx);
    g_made[tex] = {use, b};
    g_bytes[size_t(use)] += b;
    return tex;
}

void destroy(SDL_Texture* tex)
{
    if (!tex) return;
    {
        std::lock_guard<std::mutex> lock(g_mx);
        if (const auto it = g_made.find(tex); it != g_made.end())
        {
            g_bytes[size_t(it->second.first)] -= it->second.second;
            g_made.erase(it);
        }
    }
    SDL_DestroyTexture(tex);
}

double bytes(Use use)
{
    std::lock_guard<std::mutex> lock(g_mx);
    return g_bytes[size_t(use)];
}

int max_texture_px(SDL_Renderer* r)
{
    const Sint64 n = r ? SDL_GetNumberProperty(SDL_GetRendererProperties(r), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0) : 0;
    return n > 0 ? int(std::min<Sint64>(n, INT_MAX / 2)) : INT_MAX / 2;
}

} // namespace hallway::gpu
