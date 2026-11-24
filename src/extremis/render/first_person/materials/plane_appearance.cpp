#include <cstdint>
#include <filesystem>
#include "extremis/render/first_person/materials/plane_appearance.h"

#include <iostream>

void PlaneAppearance::load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
                           std::uint16_t palBank)
{
    bank_.reset();
    try {
        // Original filename patch at 0x7a08 uses the level's DALLE bank for SOL too.
        bank_ = wall_textures::loadPlanes(dataDirectory, dalleBank, palBank);
        std::cout << "SOL" << dalleBank << ": original floor/ceiling 128x128, repeat 400 units.\n";
    } catch (const std::exception& error) {
        std::cerr << "Floor/ceiling textures unavailable: " << error.what() << ". Using flat colors.\n";
    }
}

const wall_textures::PlaneBank* PlaneAppearance::materials() const
{
    return bank_ ? &*bank_ : nullptr;
}
