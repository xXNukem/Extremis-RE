#pragma once

#include "extremis/gameplay/player/camera.h"

struct SDL_Renderer;
bool drawPlayerDebug(SDL_Renderer* renderer, const Camera& camera);
