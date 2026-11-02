#pragma once

#include "game_lab.h"

struct SDL_Renderer;

// Draws loaded geometry only; returns false on an SDL drawing error.
bool drawMap(SDL_Renderer* renderer, const game_lab::Level& level,
             bool showPolygons = false);
