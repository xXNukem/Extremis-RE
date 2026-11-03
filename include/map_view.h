#pragma once

#include "game_lab.h"
#include "camera.h"

struct SDL_Renderer;

// Draws geometry and optional player marker; returns false on an SDL drawing error.
bool drawMap(SDL_Renderer* renderer, const game_lab::Level& level,
             bool showPolygons = false, const Camera* camera = nullptr);
