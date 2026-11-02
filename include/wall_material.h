#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

// Original wall material index, verified against EXTREMIS.EXE. Portals and
// unsupported families/indices have no opaque material in this prototype.
std::optional<std::size_t> wallTextureIndex(std::uint16_t type, std::size_t textureCount);
