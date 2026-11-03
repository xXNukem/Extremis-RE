#include "player_debug.h"

#include <SDL3/SDL.h>

bool drawPlayerDebug(SDL_Renderer* renderer, const Camera& camera)
{
    return SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) &&
           SDL_RenderDebugTextFormat(renderer, 8, 8, "X %.1f  Y %.1f  angle %.3f rad",
                                     camera.x, camera.y, camera.angle);
}
