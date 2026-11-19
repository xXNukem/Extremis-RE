#pragma once

#include "vortex/render/first_person/camera_pose.h"
#include "vortex/render/first_person/sprites/sprite_types.h"

#include <optional>

namespace vortex::sprites {

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
                               const CameraPose& camera, const Projection& projection);
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Bank& bank, const std::vector<Instance>& objects,
            const CameraPose& camera, const Projection& projection,
            DepthInput depthInput = DepthInput::MayContainNaN);

} // namespace vortex::sprites
