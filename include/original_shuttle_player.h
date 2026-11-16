#pragma once
#include "startup_credits.h"
#include <filesystem>
struct SDL_Window;
struct SDL_Renderer;
StartupCreditsResult showOriginalShuttle(SDL_Window*,SDL_Renderer*,const std::filesystem::path&,bool inheritedAnmEscape=false);
