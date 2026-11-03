#pragma once

#include "texture_bank.h"

#include <optional>

union SDL_Event;

// Loads demonstrated SOL materials and owns the F display switch; no projection logic.
class PlaneAppearance {
public:
    void load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
              std::uint16_t palBank);
    bool handleEvent(const SDL_Event& event, std::uint32_t windowId);
    bool enabled() const;
    const wall_textures::PlaneBank* materials() const;
private:
    bool enabled_ = true; // Provisional default when original configuration is unavailable.
    std::optional<wall_textures::PlaneBank> bank_;
};
