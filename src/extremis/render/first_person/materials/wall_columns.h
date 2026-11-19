#pragma once

#include "extremis/resources/texture_bank.h"
#include "vortex/render/first_person/raycast/raycast_geometry.h"
#include "vortex/render/first_person/scene_renderer.h"

namespace extremis::render_detail {

wall_textures::Rgb wallBaseColor(std::uint16_t type);
void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height);

struct WallMaterials {
    const wall_textures::Bank* bank;
    const wall_textures::Texture* overrideTexture;
};

vortex::first_person::WallSample resolveWallSample(const void* context, const vortex::RayHit& hit);

std::pair<int, int> rasterizeWallColumn(
    std::vector<std::uint32_t>& pixels, int width, int height, int column,
    const vortex::RayHit& hit, const vortex::ProjectedColumn& projected,
    const wall_textures::Texture* texture);

const wall_textures::Texture* selectWallTexture(const vortex::RayHit& hit,
                                                const wall_textures::Bank* materials,
                                                const wall_textures::Texture* overrideTexture);

} // namespace extremis::render_detail
