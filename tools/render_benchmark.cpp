#include "render_benchmark.h"
#include "player_debug.h"
#include "plane_material.h"

// Diagnostic TU only: call the real internal raster functions without exporting them
// from the application or copying their algorithms. Do not link first_person_view.cpp again.
#include "../src/first_person_view.cpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>

namespace render_benchmark {
namespace {
using Clock = std::chrono::steady_clock;
volatile double consumed = 0;
std::atomic<std::uint64_t> allocations{0};
SDL_malloc_func originalMalloc;
SDL_calloc_func originalCalloc;
SDL_realloc_func originalRealloc;
SDL_free_func originalFree;

void* SDLCALL countedMalloc(std::size_t size) { ++allocations; return originalMalloc(size); }
void* SDLCALL countedCalloc(std::size_t n, std::size_t size) { ++allocations; return originalCalloc(n, size); }
void* SDLCALL countedRealloc(void* pointer, std::size_t size) { ++allocations; return originalRealloc(pointer, size); }
void SDLCALL countedFree(void* pointer) { originalFree(pointer); }

void require(bool ok)
{
    if (!ok) throw std::runtime_error(SDL_GetError());
}

double since(Clock::time_point& start)
{
    const auto now = Clock::now();
    const double ms = std::chrono::duration<double, std::milli>(now - start).count();
    start = now;
    return ms;
}

Timings stagedFrame(Target& target, const game_lab::Level& level, const Camera& camera,
                    const wall_textures::Bank& bank, const std::vector<doors::Door>& doors,
                    const wall_textures::PlaneBank* planes, std::vector<std::uint32_t>& pixels,
                    std::vector<WallHit>& hits)
{
    Timings result{};
    auto start = Clock::now();
    require(SDL_SetRenderDrawColor(target.renderer.get(), 0, 0, 0, 255) && SDL_RenderClear(target.renderer.get()));
    result[Clear] = since(start);
    fillBackground(pixels, target.width, target.height);
    result[Background] = since(start);
    const double plane = std::tan(fov * 0.5), focal = target.width / (2 * plane);
    const CameraProjection projection{target.width, target.height, std::cos(camera.angle),
                                      std::sin(camera.angle), plane, focal};
    if (planes) planes::render(pixels, camera, {target.width, target.height, focal, eyeHeight, wallHeight}, *planes);
    result[Planes] = since(start);
    for (int x = 0; x < target.width; ++x) {
        const auto direction = columnRay(projection, x);
        hits[x] = castSceneRay(level, camera, direction.x, direction.y, &doors);
    }
    result[Rays] = since(start);
    for (int x = 0; x < target.width; ++x) {
        if (!std::isfinite(hits[x].distance)) continue;
        const auto* texture = selectWallTexture(hits[x], &bank, nullptr);
        rasterizeWallColumn(pixels, target.width, target.height, x, hits[x],
                            projectWall(projection, hits[x].distance), texture);
    }
    result[Walls] = since(start);
    require(SDL_UpdateTexture(target.texture.get(), nullptr, pixels.data(), target.width * 4));
    result[Upload] = since(start);
    require(SDL_RenderTexture(target.renderer.get(), target.texture.get(), nullptr, nullptr));
    result[Queue] = since(start);
    require(drawPlayerDebug(target.renderer.get(), camera));
    result[Overlay] = since(start);
    require(SDL_RenderPresent(target.renderer.get()));
    result[Present] = since(start);
    result[Total] = std::accumulate(result.begin(), result.begin() + Total, 0.0);
    return result;
}

void verifyOutput(Target& target, const game_lab::Level& level, const Camera& camera,
                  const wall_textures::Bank& bank, const std::vector<doors::Door>& doors,
                  const wall_textures::PlaneBank* planes, std::vector<std::uint32_t>& pixels,
                  std::vector<WallHit>& hits)
{
    if (!target.surface) return; // Full framebuffer comparison on the deterministic software target.
    stagedFrame(target, level, camera, bank, doors, planes, pixels, hits);
    std::vector<std::uint8_t> baseline(target.surface->pitch * target.height);
    std::memcpy(baseline.data(), target.surface->pixels, baseline.size());
    FirstPersonRenderer production(target.renderer.get());
    require(SDL_SetRenderDrawColor(target.renderer.get(), 0, 0, 0, 255) && SDL_RenderClear(target.renderer.get()));
    require(production.draw(level, camera, nullptr, &bank, &doors, planes));
    require(drawPlayerDebug(target.renderer.get(), camera) && SDL_RenderPresent(target.renderer.get()));
    if (std::memcmp(baseline.data(), target.surface->pixels, baseline.size()))
        throw std::runtime_error("Staged benchmark differs from production renderer");
}

void pumpEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            throw std::runtime_error("Benchmark interrupted by window close");
    }
}

std::vector<double> productionSamples(Target& target, const game_lab::Level& level,
    const Camera& camera, const wall_textures::Bank& bank, const std::vector<doors::Door>& doors,
    const wall_textures::PlaneBank* planes, int frames, bool paced, std::uint64_t& allocationCount)
{
    std::vector<double> samples;
    samples.reserve(frames);
    FirstPersonRenderer production(target.renderer.get());
    auto before = allocations.load();
    for (int i = -8; i < frames; ++i) {
        pumpEvents();
        if (i == 0) before = allocations.load();
        auto start = Clock::now();
        require(SDL_SetRenderDrawColor(target.renderer.get(), 0, 0, 0, 255) && SDL_RenderClear(target.renderer.get()));
        require(production.draw(level, camera, nullptr, &bank, &doors, planes));
        require(drawPlayerDebug(target.renderer.get(), camera) && SDL_RenderPresent(target.renderer.get()));
        if (paced) SDL_Delay(16);
        if (i >= 0) samples.push_back(since(start));
    }
    allocationCount += allocations.load() - before;
    return samples;
}

Samples measure(Target& target, const game_lab::Level& level, const Camera& camera,
                const wall_textures::Bank& bank, const std::vector<doors::Door>& doors,
                const wall_textures::PlaneBank* planes, int frames, bool paced,
                std::uint64_t& allocationCount)
{
    std::vector<std::uint32_t> pixels(std::size_t(target.width) * target.height);
    std::vector<WallHit> hits(target.width);
    verifyOutput(target, level, camera, bank, doors, planes, pixels, hits);
    Samples samples;
    samples.stages.reserve(frames);
    auto before = allocations.load();
    for (int i = -8; i < frames; ++i) {
        pumpEvents();
        if (i == 0) before = allocations.load();
        const auto frame = stagedFrame(target, level, camera, bank, doors, planes, pixels, hits);
        if (i >= 0) samples.stages.push_back(frame);
    }
    allocationCount = allocations.load() - before;
    samples.production = productionSamples(target, level, camera, bank, doors, planes,
                                            frames, paced, allocationCount);
    return samples;
}

void printSamples(const Target& target, bool textured, bool open, bool paced, const Samples& samples,
                  std::uint64_t allocationCount)
{
    std::cout << SDL_GetRendererName(target.renderer.get()) << ',' << target.width << ',' << target.height
              << ',' << textured << ',' << open << ',' << paced << ',' << samples.stages.size();
    for (int stage = 0; stage < Count; ++stage) {
        double total = 0;
        for (const auto& frame : samples.stages) total += frame[stage];
        std::cout << ',' << total / samples.stages.size();
    }
    const double ms = mean(samples.production);
    std::cout << ',' << ms << ',' << percentile(samples.production, 0.5) << ','
              << percentile(samples.production, 0.95) << ',' << 1000 / ms << ','
              << allocationCount << '\n' << std::flush;
}

void surfaceMicrobench(int width, int height, const Camera& camera, const game_lab::Level& level,
                       const wall_textures::Bank& bank)
{
    const auto doors = doors::closedDoors(level);
    const CameraProjection p{width, height, std::cos(camera.angle), std::sin(camera.angle),
                             std::tan(fov * 0.5), width / (2 * std::tan(fov * 0.5))};
    std::array<std::vector<int>, 2> columns;
    std::vector<WallHit> hits(width);
    for (int x = 0; x < width; ++x) {
        const auto ray = columnRay(p, x);
        const auto wall = castWallRay(level, camera, ray.x, ray.y);
        hits[x] = castSceneRay(level, camera, ray.x, ray.y, &doors);
        columns[hits[x].distance < wall.distance ? 1 : 0].push_back(x);
    }
    std::vector<std::uint32_t> pixels(std::size_t(width) * height);
    for (int kind = 0; kind < 2; ++kind) {
        auto start = Clock::now();
        for (int frame = 0; frame < 100; ++frame) for (const int x : columns[kind]) {
            if (!std::isfinite(hits[x].distance)) continue;
            rasterizeWallColumn(pixels, width, height, x, hits[x], projectWall(p, hits[x].distance),
                                 selectWallTexture(hits[x], &bank, nullptr));
        }
        std::cerr << "surface," << width << ',' << height << ",door," << kind << ",columns,"
                  << columns[kind].size() << ",raster_ms," << since(start) / 100 << '\n';
    }
    auto start = Clock::now();
    const game_lab::Level noWalls{};
    double checksum = 0;
    for (int frame = 0; frame < 100; ++frame) for (int x = 0; x < width; ++x) {
        const auto ray = columnRay(p, x);
        const auto hit = castSceneRay(noWalls, camera, ray.x, ray.y, &doors);
        if (std::isfinite(hit.distance)) checksum += hit.distance;
    }
    std::cerr << "door_geometry," << width << ',' << height << ",ms," << since(start) / 100 << '\n';
    consumed = checksum + pixels[width * height / 2];
}

void microbench(int width, int height, const Camera& camera, const game_lab::Level& level)
{
    const planes::Projection projection{width, height, width / (2 * std::tan(fov * 0.5)), eyeHeight, wallHeight};
    double checksum = 0;
    auto start = Clock::now();
    for (int i = 0; i < 100; ++i) for (int y = 0; y < height; ++y) {
        const auto row = planes::projectRow(camera, projection, y);
        if (row) checksum += row->x + row->y;
    }
    const double rows = since(start) / 100;
    for (int i = 0; i < 20; ++i) for (int y = 0; y < height; ++y) {
        const auto row = planes::projectRow(camera, projection, y);
        if (!row) continue;
        for (int x = 0; x < width; ++x) {
            const auto uv = plane_material::coordinates(row->x + x * row->stepX, row->y + x * row->stepY, 128, 128);
            checksum += uv.x + uv.y;
        }
    }
    const double coordinates = since(start) / 20;
    const CameraProjection p{width, height, std::cos(camera.angle), std::sin(camera.angle),
                              std::tan(fov * 0.5), projection.focal};
    for (int i = 0; i < 100; ++i) for (int x = 0; x < width; ++x) {
        const auto ray = columnRay(p, x);
        checksum += castWallRay(level, camera, ray.x, ray.y).distance;
    }
    const double staticRays = since(start) / 100;
    consumed = checksum;
    std::cerr << "micro," << width << ',' << height << ",projection_rows_ms," << rows
              << ",coordinates_only_ms," << coordinates << ",static_rays_ms," << staticRays << '\n';
}
} // namespace

Target createTarget(int width, int height, bool windowed)
{
    Target target;
    target.width = width;
    target.height = height;
    if (windowed) {
        target.window.reset(SDL_CreateWindow("In Extremis render benchmark (no input)", width, height, 0));
        if (!target.window) throw std::runtime_error(SDL_GetError());
        target.renderer.reset(SDL_CreateRenderer(target.window.get(), nullptr));
    } else {
        target.surface.reset(SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888));
        if (!target.surface) throw std::runtime_error(SDL_GetError());
        target.renderer.reset(SDL_CreateSoftwareRenderer(target.surface.get()));
    }
    require(bool(target.renderer));
    int vsync = -999;
    const bool queried = SDL_GetRenderVSync(target.renderer.get(), &vsync);
    int actualWidth = 0, actualHeight = 0;
    require(SDL_GetRenderOutputSize(target.renderer.get(), &actualWidth, &actualHeight));
    if (actualWidth != width || actualHeight != height) throw std::runtime_error("Unexpected drawable pixel size");
    std::cerr << "target," << SDL_GetRendererName(target.renderer.get()) << ',' << width << ',' << height
              << ",default_vsync," << (queried ? vsync : -999) << '\n';
    if (windowed) {
        const auto* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(target.window.get()));
        std::cerr << "display_refresh_hz," << (mode ? mode->refresh_rate : 0)
                  << ",video_driver," << SDL_GetCurrentVideoDriver() << '\n';
    }
    // Same default as main; report rather than silently forcing a different present policy.
    target.texture.reset(SDL_CreateTexture(target.renderer.get(), SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING, width, height));
    require(bool(target.texture));
    require(SDL_SetTextureBlendMode(target.texture.get(), SDL_BLENDMODE_NONE));
    require(SDL_SetTextureScaleMode(target.texture.get(), SDL_SCALEMODE_NEAREST));
    return target;
}

double mean(const std::vector<double>& samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

double percentile(std::vector<double> samples, double fraction)
{
    std::sort(samples.begin(), samples.end());
    return samples[std::min(samples.size() - 1, std::size_t(std::ceil(fraction * samples.size()) - 1))];
}
} // namespace render_benchmark

int main(int argc, char* argv[])
{
    using namespace render_benchmark;
    if (argc < 2 || argc > 5) {
        std::cerr << "Usage: render_benchmark GAME.LAB [software|window] [frames=60] [paced]\n";
        return 1;
    }
    try {
        SDL_GetMemoryFunctions(&originalMalloc, &originalCalloc, &originalRealloc, &originalFree);
        require(SDL_SetMemoryFunctions(countedMalloc, countedCalloc, countedRealloc, countedFree));
        const bool windowed = argc > 2 && std::string(argv[2]) == "window";
        const bool paced = argc > 4 && std::string(argv[4]) == "paced";
        const int frames = argc > 3 ? std::stoi(argv[3]) : 60;
        if (frames <= 0) throw std::runtime_error("Frames must be positive");
        require(SDL_Init(windowed ? SDL_INIT_VIDEO : SDL_INIT_EVENTS));
        const auto data = game_lab::loadLevel1(argv[1]);
        const auto resources = std::filesystem::path(argv[1]).parent_path() / "DATA";
        const auto bank = wall_textures::load(resources, data.level.dalleBank, data.level.palBank);
        const auto planes = wall_textures::loadPlanes(resources, data.level.dalleBank, data.level.palBank);
        const auto camera = initialCamera(data.level);
        std::cerr << "SDL," << SDL_GetVersion() << ",cpu_count," << SDL_GetNumLogicalCPUCores()
                  << ",ram_mb," << SDL_GetSystemRAM() << ",camera," << camera.x << ',' << camera.y << ',' << camera.angle << '\n';
        std::cout << std::fixed << std::setprecision(6)
                  << "backend,width,height,F,doors_open,delay16,frames,clear_ms,background_ms,planes_ms,rays_ms,walls_ms,upload_ms,queue_ms,overlay_ms,present_ms,staged_ms,production_ms,p50_ms,p95_ms,measured_fps,sdl_alloc_calls\n";
        for (const auto size : {SDL_Point{640,400}, SDL_Point{960,600}, SDL_Point{1280,800}, SDL_Point{1920,1080}}) {
            auto target = createTarget(size.x, size.y, windowed);
            if (!windowed) {
                microbench(size.x, size.y, camera, data.level);
                surfaceMicrobench(size.x, size.y, camera, data.level, bank);
            }
            for (const bool open : {false, true}) for (const bool textured : {false, true}) {
                auto level=data.level;
                auto doors = doors::closedDoors(level);
                if (open) for (const auto& door:doors) level.connectionStates[door.stateIndex]|=0x0140;
                std::uint64_t allocationCount = 0;
                const auto samples = measure(target, level, camera, bank, doors,
                                              textured ? &planes : nullptr, frames, paced, allocationCount);
                printSamples(target, textured, open, paced, samples, allocationCount);
            }
        }
        SDL_Quit();
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        SDL_Quit();
        return 1;
    }
}
