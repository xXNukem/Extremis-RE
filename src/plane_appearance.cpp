#include "plane_appearance.h"

#include <SDL3/SDL.h>
#include <fstream>
#include <iostream>

namespace {
bool initialEnabled(const std::filesystem::path& config)
{
    std::ifstream file(config, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() != 13) return true;
    file.seekg(12);
    char value = 0;
    return !file.get(value) || value != 0;
}
} // namespace

void PlaneAppearance::load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
                           std::uint16_t palBank)
{
    bank_.reset();
    enabled_ = initialEnabled(dataDirectory.parent_path() / "EXTREMIS.CFG");
    try {
        // Original filename patch at 0x7a08 uses the level's DALLE bank for SOL too.
        bank_ = wall_textures::loadPlanes(dataDirectory, dalleBank, palBank);
        std::cout << "SOL" << dalleBank << ": original floor/ceiling 128x128, repeat 400 units. F: "
                  << (enabled_ ? "textured" : "flat (original CFG)") << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Floor/ceiling textures unavailable: " << error.what() << ". Using flat colors.\n";
    }
}

bool PlaneAppearance::handleEvent(const SDL_Event& event, std::uint32_t windowId)
{
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.windowID != windowId ||
        event.key.scancode != SDL_SCANCODE_F || event.key.repeat) return false;
    enabled_ = !enabled_;
    SDL_Log("Floor/ceiling: %s", enabled_ ? (bank_ ? "textured" : "requested, SOL unavailable; flat fallback")
                                       : "flat colors");
    return true;
}

bool PlaneAppearance::enabled() const { return enabled_; }
const wall_textures::PlaneBank* PlaneAppearance::materials() const
{
    return enabled_ && bank_ ? &*bank_ : nullptr;
}
