#pragma once

#include <vector>
#include "extremis/gameplay/player/camera.h"
#include "extremis/world/door.h"
#include "vortex/render/first_person/raycast/raycast_geometry.h"

using WallHit = vortex::RayHit;

// All functions consume validated loader data without copying or modifying geometry.
// Finds the nearest opaque segment; portal surfaces are omitted. Direction is
// not normalized: distance is the ray parameter, not necessarily Euclidean length.
// A miss returns an infinite distance.
WallHit castWallRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy);

WallHit castSceneRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy, const std::vector<doors::Door>* doors);
