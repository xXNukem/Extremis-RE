#pragma once

#include "game_lab.h"
#include "texture_bank.h"

#include <memory>

struct SDL_Renderer;
struct SDL_Texture;

struct Camera {
    double x = 0, y = 0;
    double angle = 0; // Radians: zero = +X, positive rotation towards +Y.
};

struct WallHit {
    double distance; // Ray parameter; perpendicular depth for camera-plane rays.
    std::uint16_t type;
    double segmentPosition = 0; // Fraction from vertex A to B, clamped to [0, 1].
};

// All functions consume validated loader data without copying or modifying geometry.
// Selects an interior sample with the greatest clearance, then faces the longest
// finite sightline. Throws if no polygon interior clear of walls can be found.
Camera initialCamera(const game_lab::Level& level);

// Finds the nearest opaque segment; portal surfaces are omitted. Direction is
// not normalized: distance is the ray parameter, not necessarily Euclidean length.
// A miss returns an infinite distance.
WallHit castWallRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy);

// Draws background and projected wall columns at the current output size.
// Borrows the renderer; the caller owns clearing, presenting and SDL resources.
// Returns false on an SDL error.
bool drawFirstPerson(SDL_Renderer* renderer, const game_lab::Level& level,
                     const Camera& camera);

// Owns the CPU framebuffer and its SDL upload texture; borrows the renderer.
// Destroy this object before destroying its SDL_Renderer.
class FirstPersonRenderer {
public:
    explicit FirstPersonRenderer(SDL_Renderer* renderer);
    ~FirstPersonRenderer();
    bool draw(const game_lab::Level& level, const Camera& camera,
              const wall_textures::Texture* provisionalTexture = nullptr,
              const wall_textures::Bank* materials = nullptr);

private:
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const;
    };

    bool resizeFrame(int width, int height);
    bool presentFrame();

    SDL_Renderer* renderer_;
    std::unique_ptr<SDL_Texture, TextureDeleter> uploadTexture_;
    std::vector<std::uint32_t> pixels_;
    int width_ = 0, height_ = 0;
};
