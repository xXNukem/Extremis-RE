#pragma once

#include <cstdint>
#include <vector>

namespace hud { class Renderer; }

namespace extremis {
struct ComposedFrame {
    const std::uint32_t* pixels;
    float displayAspect; // Zero fills the SDL drawable.
    const char* error;
};

// Copies the center of the rendered world into the DOS HUD viewport.
// The source holds width * height pixels and composed holds a complete DOS frame.
// Use the validated HUD layout and render scale from the first-person view.
void copyHudView(const std::vector<std::uint32_t>& pixels, std::vector<std::uint32_t>& composed,
                 int width, int height, bool nativeMax, int renderScale);

// Views one of the existing framebuffers. Widescreen modes write into wideComposed.
ComposedFrame composeFrame(const std::vector<std::uint32_t>& pixels,
    const std::vector<std::uint32_t>& composed, std::vector<std::uint32_t>& wideComposed,
    int width, int height, bool withHud, bool nativeComposed, const hud::Renderer* hudRenderer);
}
