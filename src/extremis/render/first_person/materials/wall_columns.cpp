#include "wall_columns.h"

#include "extremis/render/first_person/materials/wall_material.h"

#include <algorithm>
#include <cmath>

namespace extremis::render_detail {
wall_textures::Rgb wallBaseColor(std::uint16_t type)
{
    wall_textures::Rgb color{170, 170, 170};
    switch (type & 0xf000) {
    case 0x2000: color = {255, 220, 60}; break;
    case 0x3000: color = {255, 100, 190}; break;
    }
    return color;
}

void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height)
{
    vortex::first_person::fillBackground(pixels, width, height,
        vortex::packOpaqueRgb(20, 23, 32), vortex::packOpaqueRgb(45, 39, 34));
}

namespace {
vortex::first_person::WallSample wallSample(const vortex::RayHit& hit,
                                           const wall_textures::Texture* texture)
{
    // Frames span the first half of DAL's columns; other walls use the full image.
    const int columns = texture ? wallTextureColumnCount(hit.type, texture->width) : 0;
    const double textureExtent = columns * hit.textureSpan;
    const int lastColumn = texture ? std::max(0, int(std::ceil(textureExtent)) - 1) : 0;
    const int textureX = texture ? std::min(lastColumn,
        int(hit.segmentPosition * textureExtent)) : 0;
    return {texture, textureX, wallBaseColor(hit.type)};
}
} // namespace

std::pair<int, int> rasterizeWallColumn(
    std::vector<std::uint32_t>& pixels, int width, int height, int column,
    const vortex::RayHit& hit, const vortex::ProjectedColumn& projected,
    const wall_textures::Texture* texture)
{
    const auto sample = wallSample(hit, texture);
    return vortex::rasterizeWallColumn(pixels, width, height, column, hit.distance,
                                       projected, sample.texture, sample.textureX, sample.color);
}

const wall_textures::Texture* selectWallTexture(const vortex::RayHit& hit,
                                               const wall_textures::Bank* materials,
                                               const wall_textures::Texture* overrideTexture)
{
    if (overrideTexture) {
        return overrideTexture;
    }
    if (!materials) return nullptr;
    const auto index = wallTextureIndex(hit.type, materials->textures.size());
    if (!index || !vortex::validTexture(materials->textures[*index])) {
        return nullptr; // Preserve a visible flat-color wall for unsupported materials.
    }
    return &materials->textures[*index];
}

vortex::first_person::WallSample resolveWallSample(const void* context, const vortex::RayHit& hit)
{
    const auto& materials = *static_cast<const WallMaterials*>(context);
    return wallSample(hit, selectWallTexture(hit, materials.bank, materials.overrideTexture));
}

} // namespace extremis::render_detail
