#pragma once

#include "extremis/resources/texture_bank.h"

#include <optional>

// Loads the original SOL materials for a level.
class PlaneAppearance {
public:
    void load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
              std::uint16_t palBank);
    const wall_textures::PlaneBank* materials() const;
private:
    std::optional<wall_textures::PlaneBank> bank_;
};
