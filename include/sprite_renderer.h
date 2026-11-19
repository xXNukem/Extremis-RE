#pragma once

#include "camera.h"
#include "sprite_scene.h"

namespace sprites {

struct Projection {
    int width, height;
    double focal, eyeHeight;
    // Optional immutable upper bound on depth for every row of each column.
    // The array has width entries and is used only with DepthInput::NoNaN.
    const double* columnDepthLimit = nullptr;
};

struct Rectangle {
    double left, top, width, height, depth;
};

// NoNaN is valid for buffers initialized with finite depths or infinities.
enum class DepthInput { MayContainNaN, NoNaN };

std::optional<Rectangle> project(const Instance& object, const Image& image,
                               const Camera& camera, const Projection& projection);
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Bank& bank, const std::vector<Instance>& objects,
            const Camera& camera, const Projection& projection,
            DepthInput depthInput = DepthInput::MayContainNaN);
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Scene& scene, const Camera& camera, const Projection& projection,
            DepthInput depthInput = DepthInput::MayContainNaN);

} // namespace sprites
