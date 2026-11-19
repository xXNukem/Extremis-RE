#pragma once

#include "app/startup/port_menu_types.h"
#include "extremis/render/first_person/presentation/video_render_settings.h"

#include <filesystem>
#include <optional>

struct SDL_Window;
struct SDL_Renderer;

// Retains the exact selector mode across menu openings and owns the published settings.
struct PortMenuVideoState {
    VideoRenderSettings render;
    std::optional<PortVideoMode> selectedMode;
};

// SDL modal: suspends gameplay; VIDEO changes the real output window.
// Other settings remain menu-only previews.
PortMenuChoice showPortMenu(SDL_Window* window, SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory, bool inGame,
                            PortMenuVideoState& video);
