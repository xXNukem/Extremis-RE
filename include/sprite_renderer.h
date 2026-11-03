#pragma once

#include "camera.h"
#include "sprite_scene.h"

namespace sprites {

struct Projection {
    int width, height;
    double focal, eyeHeight;
};

struct Rectangle {
    double left, top, width, height, depth;
};

std::optional<Rectangle> project(const Instance& object, const Image& image,
                               const Camera& camera, const Projection& projection);
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Bank& bank, const std::vector<Instance>& objects,
            const Camera& camera, const Projection& projection);
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Scene& scene, const Camera& camera, const Projection& projection);

} // namespace sprites
