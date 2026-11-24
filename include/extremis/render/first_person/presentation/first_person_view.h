#pragma once
#include "extremis/ui/ambient_effects.h"

#include "extremis/resources/game_lab.h"
#include "extremis/resources/texture_bank.h"
#include "extremis/world/door.h"
#include "extremis/render/first_person/camera/camera_start.h"
#include "extremis/render/first_person/sprites/sprite_scene.h"
#include "extremis/ui/original_hud.h"
#include "extremis/ui/original_interactions.h"
#include "extremis/ui/original_map.h"
#include "vortex/render/first_person/scene_renderer.h"
#include "extremis/render/first_person/scene/ray_queries.h"
#include "extremis/render/first_person/presentation/video_render_settings.h"
#include "platform/sdl/video/frame_texture.h"

#include <memory>
#include <utility>

struct SDL_Renderer;
struct SDL_Texture;
namespace extremis::render_detail { class StaticRayGrid; }
namespace platform::sdl::video_profiling { class FrameProfiler; }
namespace vortex::first_person { class RenderWorkers; }

// Draws background and projected wall columns at the current output size.
// Borrows the renderer; the caller owns clearing, presenting and SDL resources.
// Returns false on an SDL error.
bool drawFirstPerson(SDL_Renderer* renderer, const game_lab::Level& level,
                     const Camera& camera, const std::vector<doors::Door>* doors = nullptr);

// Owns the CPU framebuffer and its SDL upload texture; borrows the renderer.
// Destroy this object before destroying its SDL_Renderer.
class FirstPersonRenderer {
public:
    // Optional video settings and frame profiler are borrowed for the lifetime of this view.
    explicit FirstPersonRenderer(SDL_Renderer* renderer,
                                 const VideoRenderSettings* video = nullptr,
                                 platform::sdl::video_profiling::FrameProfiler* profiler = nullptr);
    ~FirstPersonRenderer();
    void enableRenderWorkers();
    // Call when replacing the active level or its static geometry.
    void setRaycastLevel(const game_lab::Level& level);
    const std::vector<std::uint32_t>& frame() const;
    bool drawFrame(const std::vector<std::uint32_t>& pixels);
    bool draw(const game_lab::Level& level, const Camera& camera,
              const wall_textures::Texture* provisionalTexture = nullptr,
              const wall_textures::Bank* materials = nullptr,
              const std::vector<doors::Door>* doors = nullptr,
              const wall_textures::PlaneBank* planes = nullptr,
              const sprites::Scene* sprites = nullptr,
              const hud::Renderer* hud = nullptr, const hud::State& hudState = {},
              const interaction::Renderer* interaction = nullptr,
              const interaction::State* interactionState = nullptr,
              const sprites::Scene* projectiles = nullptr,
              const sprites::Scene* enemies = nullptr,
              const std::vector<sprites::Instance>* enemyEffects = nullptr,
              const original_map::Renderer* map = nullptr, const original_map::State* mapState = nullptr,
              extremis::PaletteTint tint = {}, const sprites::Scene* bomb = nullptr);

private:
    bool resizeFrame(int width, int height, bool withHud);
    bool presentFrame(bool withHud, int outputWidth, int outputHeight,
                      const hud::Renderer* hudRenderer = nullptr, extremis::PaletteTint tint = {});

    SDL_Renderer* renderer_;
    const VideoRenderSettings& video_;
    [[maybe_unused]] platform::sdl::video_profiling::FrameProfiler* profiler_;
    std::unique_ptr<extremis::render_detail::StaticRayGrid> rayGrid_;
    platform::sdl::FrameTexture uploadTexture_;
    platform::sdl::FrameTexture pickupTexture_;
    std::vector<std::uint32_t> pixels_;
    std::vector<std::uint32_t> composed_;
    std::vector<std::uint32_t> tinted_;
    // Widescreen composition: extended world with the original 320x200 HUD
    // overlaid in the middle; cinematics still use their native 4:3 frame.
    std::vector<std::uint32_t> wideComposed_;
    vortex::first_person::RenderBuffers worldBuffers_;
    vortex::FisheyeEffect fisheye_{{5.0 / 6.0, 0.20}};
    std::vector<std::uint32_t> mapBackground_;
    bool showingMap_ = false, frameReady_ = false;
    int width_ = 0, height_ = 0;
    bool withHud_ = false;
    bool nativeComposed_ = false; // Native device output vs integer DOS scaling.
    std::unique_ptr<vortex::first_person::RenderWorkers> renderWorkers_;
};
