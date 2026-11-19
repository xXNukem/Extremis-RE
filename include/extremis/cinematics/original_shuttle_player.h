#pragma once
#include "extremis/cinematics/playback_result.h"
#include <filesystem>
struct SDL_Window;
struct SDL_Renderer;
StartupCreditsResult showOriginalShuttle(SDL_Window*,SDL_Renderer*,const std::filesystem::path&,bool inheritedAnmEscape=false);
