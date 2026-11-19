#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

// Original wall material index, verified against EXTREMIS.EXE. Portals and
// unsupported families/indices have no opaque material in this prototype.
std::optional<std::size_t> wallTextureIndex(std::uint16_t type, std::size_t textureCount);

// Active horizontal columns: original 0x2000 frames use 30 of the 60 DAL columns.
// Returns at least one column for a positive texture width; preserves other families.
int wallTextureColumnCount(std::uint16_t type, int textureWidth);
