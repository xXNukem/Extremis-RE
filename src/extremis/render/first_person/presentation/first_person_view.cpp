#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "extremis/render/first_person/presentation/first_person_view.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "platform/sdl/video/frame_profiler.h" // VORTEX_DEBUG_PROFILER_V1
#endif
#include "extremis/ui/frame_composition.h"
#include "extremis/diagnostics/shot_trace.h"
#include "view_layout.h"
#include "../scene/ray_scene.h"
#include "../materials/wall_columns.h"
#include "vortex/render/first_person/render_workers.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <new>
#include <string_view>
#include <system_error>
#include <thread>

namespace {
namespace cpu = vortex::first_person;
namespace profiling = platform::sdl::video_profiling;
using extremis::render_detail::CameraProjection;
using extremis::render_detail::StaticRayGrid;
using extremis::render_detail::fov;
using extremis::render_detail::prepareRayScene;
using extremis::render_detail::wallBaseColor;
using extremis::render_detail::wallHeight;
using vortex::ProjectedColumn;
using vortex::distanceShade;

constexpr VideoRenderSettings defaultVideoSettings{};

bool drawBackground(SDL_Renderer* renderer, int width, int height)
{
    const float split = float((height + 1) / 2);
    const SDL_FRect ceiling{0, 0, float(width), split};
    const SDL_FRect floor{0, split, float(width), float(height) - split};
    return SDL_SetRenderDrawColor(renderer, 20, 23, 32, 255) &&
           SDL_RenderFillRect(renderer, &ceiling) &&
           SDL_SetRenderDrawColor(renderer, 45, 39, 34, 255) &&
           SDL_RenderFillRect(renderer, &floor);
}

bool drawWallColumn(SDL_Renderer* renderer, int column, const ProjectedColumn& projected,
                    std::uint16_t type)
{
    const auto color = wallBaseColor(type);
    const double shade = distanceShade(projected.depth);
    return SDL_SetRenderDrawColor(renderer, Uint8(color.r * shade),
               Uint8(color.g * shade), Uint8(color.b * shade), 255) &&
           SDL_RenderLine(renderer, float(column), projected.top, float(column), projected.bottom);
}

CameraProjection columnProjection(const Camera& camera, int width, int height,
                                  int referenceWidth)
{
    // Extra width adds rays on both sides while preserving vertical FOV.
    const double reference = referenceWidth > 0 ? referenceWidth : width;
    const double plane = std::tan(fov * 0.5) * double(width) / reference;
    return {width, height, std::cos(camera.angle), std::sin(camera.angle),
            plane, width / (2 * plane), camera.viewHeight()};
}

cpu::Timing renderTiming([[maybe_unused]] profiling::FrameProfiler* profiler)
{
#ifdef EXTREMIS_DEBUG_MENU
    if (profiler) return {profiler, SDL_GetTicksNS,
        [](void* context, cpu::Stage stage, std::uint64_t duration) {
            constexpr profiling::Stage stages[] = {
                profiling::Rays, profiling::Walls, profiling::Planes, profiling::Sprites
            };
            profiling::FrameProfiler::addSample(static_cast<profiling::FrameProfiler*>(context),
                                                stages[static_cast<unsigned>(stage)], duration);
        }};
#endif
    return {};
}

vortex::PreparedRayScene prepareProfiledScene(const game_lab::Level& level, const Camera& camera,
                                              const std::vector<doors::Door>* doors,
                                              cpu::Timing timing)
{
    const auto start = timing.now ? timing.now() : 0;
    auto scene = prepareRayScene(level, camera, doors);
#ifdef EXTREMIS_DEBUG_MENU
    if (timing.now) profiling::FrameProfiler::addSample(
        static_cast<profiling::FrameProfiler*>(timing.context), profiling::Geometry, timing.now() - start);
#else
    (void)start;
#endif
    return scene;
}

struct ShotCapture {
    std::size_t layer;
    std::vector<std::uint32_t> pixels;
    std::size_t changed = 0;
};

bool drawFlatView(SDL_Renderer* renderer, const game_lab::Level& level, const Camera& camera,
                  const std::vector<doors::Door>* doors, StaticRayGrid* grid,
                  profiling::FrameProfiler* profiler)
{
    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(renderer, &width, &height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return true;
    }
    if (!drawBackground(renderer, width, height)) {
        return false;
    }
    const auto timing = renderTiming(profiler);
    const auto scene = prepareProfiledScene(level, camera, doors, timing);
    const auto projection = columnProjection(camera, width, height, 0);
    return cpu::visitWallColumns({camera.renderPose(), projection, wallHeight, projection.focal,
                                  scene, grid ? &grid->index : nullptr},
        [](void* context, int column, const WallHit& hit, const ProjectedColumn& projected) {
            return drawWallColumn(static_cast<SDL_Renderer*>(context), column, projected, hit.type);
        }, renderer, timing);
}

} // namespace

bool drawFirstPerson(SDL_Renderer* renderer, const game_lab::Level& level,
                     const Camera& camera, const std::vector<doors::Door>* doors)
{
    return drawFlatView(renderer, level, camera, doors, nullptr, nullptr);
}

FirstPersonRenderer::FirstPersonRenderer(SDL_Renderer* renderer,
                                         const VideoRenderSettings* video,
                                         profiling::FrameProfiler* profiler)
    : renderer_(renderer), video_(video ? *video : defaultVideoSettings), profiler_(profiler) {}
FirstPersonRenderer::~FirstPersonRenderer() = default;

void FirstPersonRenderer::enableRenderWorkers()
{
    const char* setting = SDL_getenv("EXTREMIS_RENDER_WORKERS");
    if (!setting || std::string_view(setting) != "1") {
        try {
            renderWorkers_ = std::make_unique<cpu::RenderWorkers>(0);
        } catch (const std::system_error& error) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "VORTEX RenderWorkers: %s", error.what());
        } catch (const std::bad_alloc&) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "VORTEX RenderWorkers: allocation failed");
        }
    }
    if (renderWorkers_) {
        // The public API does not expose a count. Report its documented auto policy.
        const auto threads = std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
        SDL_Log("VORTEX RenderWorkers: auto, %u render threads including the caller", threads);
    } else {
        SDL_Log("VORTEX CPU: serial, 1 render thread");
    }
}

void FirstPersonRenderer::setRaycastLevel(const game_lab::Level& level)
{
    rayGrid_.reset();
    try {
        auto grid = std::make_unique<StaticRayGrid>(level);
        if (grid->valid()) rayGrid_ = std::move(grid);
    } catch (const std::bad_alloc&) {
        // The renderer can still use the prepared scan without an index.
    }
}

bool FirstPersonRenderer::draw(const game_lab::Level& level, const Camera& camera,
                               const wall_textures::Texture* provisionalTexture,
                               const wall_textures::Bank* materials,
                               const std::vector<doors::Door>* doors,
                               const wall_textures::PlaneBank* planes,
                               const sprites::Scene* spriteScene,
                               const hud::Renderer* hudRenderer, const hud::State& hudState,
                               const interaction::Renderer* interactionRenderer,
                               const interaction::State* interactionState, const sprites::Scene* projectileScene, const sprites::Scene* enemyScene, const std::vector<sprites::Instance>* enemyEffects,
                               const original_map::Renderer* map, const original_map::State* mapState,
                               extremis::PaletteTint tint, const sprites::Scene* bomb)
{
    auto* grid = rayGrid_ && rayGrid_->matches(level) ? rayGrid_.get() : nullptr;
    if (!provisionalTexture && !materials && !planes && !spriteScene && !projectileScene && !hudRenderer) {
        return drawFlatView(renderer_, level, camera, doors, grid, profiler_);
    }
    if (provisionalTexture && !vortex::validTexture(*provisionalTexture)) {
        return SDL_SetError("Invalid wall texture dimensions or pixel count");
    }
    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(renderer_, &width, &height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return true;
    }
    const int outputWidth = width, outputHeight = height;
    if (hudRenderer && interactionRenderer && interactionState &&
        interaction::isPickupMovie(interactionState->phase)) {
        // A_ARME replaces the entire DOS canvas, including the HUD rows.
        if (!pickupTexture_)
            pickupTexture_ = platform::sdl::createFrameTexture(renderer_, hud::width, hud::height);
        if (!pickupTexture_) return false;
        composed_.resize(hud::width * hud::height);
        if (!interactionRenderer->draw(composed_, *interactionState))
            return SDL_SetError("Invalid original interaction framebuffer");
        frameReady_ = false;
        showingMap_ = false;
        return platform::sdl::queueFrameTexture(renderer_, pickupTexture_.get(), composed_.data(),
                                                 hud::width, outputWidth, outputHeight, 4.f/3.f);
    }
    const auto layout = extremis::render_detail::viewLayout(outputWidth, outputHeight,
                                                            hudRenderer != nullptr, video_);
    extremis::shot_trace::event("layout", "output=%dx%d world=%dx%d scale=%d native=%d fisheye=%d",
        outputWidth,outputHeight,layout.width,layout.height,layout.renderScale,layout.nativeMax,
        video_.preferences.fisheyeEnabled);
    width = layout.width;
    height = layout.height;
    const int projectionReferenceWidth = layout.projectionReferenceWidth;
    if (!resizeFrame(width, height, hudRenderer != nullptr)) {
        return false;
    }
    if (hudRenderer && map && mapState && frameReady_) {
        if (!showingMap_) mapBackground_=composed_;
        showingMap_=true;
        composed_=mapBackground_;
        if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        return presentFrame(true,outputWidth,outputHeight,hudRenderer);
    }
    showingMap_=false;
    const auto timing = renderTiming(profiler_);
    const auto scene = prepareProfiledScene(level, camera, doors, timing);
    const auto projection = columnProjection(camera, width, height,
                                              hudRenderer ? projectionReferenceWidth : 0);
    const double focal = projectionReferenceWidth / (2 * std::tan(fov * 0.5));
    const cpu::View view{camera.renderPose(), projection, wallHeight, focal,
                         scene, grid ? &grid->index : nullptr};

    const extremis::render_detail::WallMaterials walls{materials, provisionalTexture};
    cpu::SceneStyle style;
    style.wallMaterial = extremis::render_detail::resolveWallSample;
    style.wallContext = &walls;
    style.ceilingColor = vortex::packOpaqueRgb(20, 23, 32);
    style.floorColor = vortex::packOpaqueRgb(45, 39, 34);
    style.planes = video_.preferences.floorAndCeiling ? planes : nullptr;
    style.planeRepeat = wall_textures::solRepeatPeriod;

    std::array<cpu::SpriteLayer, 5> layers{{
        {spriteScene ? &spriteScene->bank : nullptr, spriteScene ? &spriteScene->objects : nullptr},
        {enemyScene ? &enemyScene->bank : nullptr, enemyScene ? &enemyScene->objects : nullptr},
        {enemyScene ? &enemyScene->bank : nullptr, enemyEffects},
        {bomb ? &bomb->bank : nullptr, bomb ? &bomb->objects : nullptr},
        {projectileScene ? &projectileScene->bank : nullptr,
         projectileScene ? &projectileScene->objects : nullptr},
    }};
    style.layers = layers.data();
    style.layerCount = spriteScene || enemyScene || projectileScene || bomb ? layers.size() : 0;

    ShotCapture capture{layers.size() - 1, {}};
    const bool traceShots = extremis::shot_trace::pixels && projectileScene &&
                            !projectileScene->objects.empty() && !mapState;
    if (traceShots) {
        style.observeLayer = [](void* context, std::size_t layer, bool after, const auto& pixels) {
            auto& capture = *static_cast<ShotCapture*>(context);
            if (layer != capture.layer) return;
            if (!after) {
                capture.pixels = pixels;
                return;
            }
            for (std::size_t i = 0; i < pixels.size(); ++i)
                capture.changed += capture.pixels[i] != pixels[i];
        };
        style.layerContext = &capture;
    }
    if (video_.preferences.fisheyeEnabled) style.fisheye = &fisheye_;
    bool rendered;
    if (renderWorkers_) {
#ifdef EXTREMIS_DEBUG_MENU
        profiling::FrameProfiler::Scope worldTimer(profiler_, profiling::WorkerWorld);
#endif
        rendered = renderWorkers_->render(view, style, pixels_, worldBuffers_);
    } else {
        rendered = cpu::render(view, style, pixels_, worldBuffers_, timing);
    }
    if (!rendered)
        return SDL_SetError("Invalid fisheye world framebuffer");

    auto& withoutShots = capture.pixels;
#ifdef EXTREMIS_DEBUG_MENU
    const auto hudBegin=SDL_GetTicksNS();
#endif
    if (hudRenderer) {
        extremis::copyHudView(pixels_, composed_, width, height, layout.nativeMax, layout.renderScale);
        if (!hudRenderer->draw(composed_,hudState)) return SDL_SetError("Invalid HUD framebuffer");
        if (interactionRenderer && interactionState && !interactionRenderer->draw(composed_,*interactionState))
            return SDL_SetError("Invalid original interaction framebuffer");
        if (map && mapState) {
            mapBackground_=composed_; showingMap_=true;
            if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        }
    }
#ifdef EXTREMIS_DEBUG_MENU
    profiling::FrameProfiler::addSample(profiler_,profiling::Hud,SDL_GetTicksNS()-hudBegin);
#endif
    if (!withoutShots.empty()) {
        // Apply the same downstream transformations with the projectile layer absent.
        // The actual frame, texture upload and SDL presentation remain on their normal path.
        bool ok=!video_.preferences.fisheyeEnabled || fisheye_.apply(withoutShots,width,height);
        auto baselineHud=composed_;
        if (ok && hudRenderer) {
            extremis::copyHudView(withoutShots,baselineHud,width,height,layout.nativeMax,layout.renderScale);
            ok=hudRenderer->draw(baselineHud,hudState) &&
                (!interactionRenderer || !interactionState ||
                 interactionRenderer->draw(baselineHud,*interactionState));
        }
        if (ok) {
            std::vector<std::uint32_t> baselineWide(wideComposed_.size());
            const auto baseline=extremis::composeFrame(withoutShots,baselineHud,baselineWide,
                width,height,hudRenderer!=nullptr,layout.nativeMax,hudRenderer);
            const auto actual=extremis::composeFrame(pixels_,composed_,wideComposed_,
                width,height,hudRenderer!=nullptr,layout.nativeMax,hudRenderer);
            if (!baseline.error && !actual.error) {
                const auto count=!wideComposed_.empty() ? wideComposed_.size()
                    : hudRenderer ? composed_.size() : pixels_.size();
                std::size_t composedChanged=0;
                for (std::size_t i=0;i<count;++i) composedChanged+=baseline.pixels[i]!=actual.pixels[i];
                extremis::shot_trace::event("coverage", "world=%zu composed=%zu",capture.changed,composedChanged);
            }
        }
    }
    frameReady_=true;
    return presentFrame(hudRenderer != nullptr,outputWidth,outputHeight,hudRenderer,tint);
}

const std::vector<std::uint32_t>& FirstPersonRenderer::frame() const
{
    return composed_;
}
