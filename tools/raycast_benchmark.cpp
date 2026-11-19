// Access the production prepared caster and selector without exporting test APIs.
#include "../src/first_person_view.cpp"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

namespace {
using Clock = std::chrono::steady_clock;
volatile double consumed = 0;

bool sameHit(const WallHit& a, const WallHit& b)
{
    return a.distance == b.distance && a.type == b.type &&
           a.segmentPosition == b.segmentPosition && a.textureSpan == b.textureSpan;
}

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

CameraProjection projection(const Camera& camera, int width, int referenceWidth)
{
    const double plane = std::tan(fov * 0.5) * double(width) / referenceWidth;
    return {width, 1080, std::cos(camera.angle), std::sin(camera.angle),
            plane, width / (2 * plane), camera.viewHeight()};
}

void checkRay(StaticRayGrid& grid, const PreparedRayScene& scene, const Camera& camera,
              Point direction, double backtrack)
{
    const auto expected = castPreparedSceneRay(scene, direction);
    const auto actual = grid.cast(scene, camera, direction, backtrack);
    if (!sameHit(expected, actual)) {
        throw std::runtime_error("Prepared hit differs at camera " + std::to_string(camera.x) +
            "," + std::to_string(camera.y) + " direction " + std::to_string(direction.x) +
            "," + std::to_string(direction.y) + " material " + std::to_string(expected.type) +
            " vs " + std::to_string(actual.type));
    }
}

void checkEdges()
{
    game_lab::Level level;
    auto& sector = level.sectors.emplace_back();
    sector.vertices = {{512, 512}, {768, 768}, {512, 1024}, {1024, 512},
                       {1024, 1024}, {1536, 1024}, {512, 0}, {512, 1025}};
    sector.walls = {{0, 1, 4, 0}, {1, 0, 5, 0}, {0, 1, 6, 0}, {0, 2, 7, 0},
                    {0, 3, 8, 0}, {4, 5, 0x2001, 0}, {6, 0, 9, 0}, {0, 7, 0x1000, 0}};
    const std::vector<doors::Door> doors{{0, {
        doors::Face{{1024, 512}, {512, 512}, 9},
        doors::Face{{512, 512}, {1024, 512}, 10}}}};
    level.connectionStates = {0};
    StaticRayGrid grid(level, true);
    require(grid.valid() && !StaticRayGrid(level).valid(), "Small-level fallback");
    const std::array<Camera, 10> cameras{{{0, 0, 0}, {512, 512, 0}, {512, 256, 0},
        {100, 512, 0}, {2048, 1024, 0}, {2048, 2048, 0}, {-1000, 512, 0},
        {512, -1000, 0}, {512 - 5e-8, 511, 0}, {264000, 1024, 0}}};
    const std::array<Point, 16> rays{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1},
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1 - 1e-10}, {1, 1 + 1e-10}, {1, 1 + 4e-12},
        {1, 1 - 4e-12}, {1, 1e-50}, {-1e-50, 1}, {1, -1e-50}, {-1, 1e-50}}};
    for (const auto state : {0, 30, 59, 60, int(doors::dos::locked)}) {
        level.connectionStates[0] = static_cast<std::uint16_t>(state);
        for (const auto& camera : cameras) {
            const auto scene = prepareRayScene(level, camera, &doors);
            for (const auto direction : rays)
                checkRay(grid, scene, camera, direction, grid.backtrack(camera));
        }
    }
    const Camera corner{0, 0, 0};
    const auto scene = prepareRayScene(level, corner, nullptr);
    unsigned cells = 0;
    std::size_t candidates = 0;
    const auto hit = grid.cast(scene, corner, {1, 1}, grid.backtrack(corner), &cells, &candidates);
    require(cells > 1 && candidates > 0 && candidates <= scene.walls.size() &&
            hit.distance == 512, "Corner must exercise indexed endpoint hit");

    // Put coincident surfaces in distinct mask blocks. The second is the first
    // front face in source order, while the third reverses the texture direction.
    sector.walls.assign(130, {4, 4, 0, 0});
    sector.walls[1] = {0, 1, 10, 0};
    sector.walls[65] = {1, 0, 11, 0};
    sector.walls[129] = {1, 0, 12, 0};
    StaticRayGrid replacement(level);
    require(replacement.valid(), "Replacement grid must be active");
    const Camera camera{0, 640, 0};
    const auto overlapping = prepareRayScene(level, camera, nullptr);
    const auto baseline = castPreparedSceneRay(overlapping, {1, 0});
    require(baseline.type == 11 && baseline.segmentPosition == 0.5, "Coincident face fixture");
    checkRay(replacement, overlapping, camera, {1, 0}, replacement.backtrack(camera));

    sector.vertices = {{512, 0}, {512, 1024}, {513, 0}, {511, 1024}};
    sector.walls = {{0, 1, 1, 0}, {3, 2, 2, 0}};
    StaticRayGrid nearTie(level, true);
    const Camera offset{0, 512 - 1e-4, 0};
    const auto tied = prepareRayScene(level, offset, nullptr);
    const auto farther = castPreparedSceneRay(tied, {1, 0});
    require(farther.type == 2 && farther.distance > 512, "Near tie must prefer the front face");
    checkRay(nearTie, tied, offset, {1, 0}, nearTie.backtrack(offset));

    // A far surface can change the winner even when it is outside the
    // nearest hit's tie band. All three faces have the same orientation.
    sector.vertices = {{512, 0}, {512, 1024}, {1024, 0}, {0, 1024},
                       {1024, 256}, {0, 768}};
    sector.walls = {{0, 1, 1, 0}, {2, 3, 2, 0}, {4, 5, 3, 0}};
    StaticRayGrid chain(level, true);
    const Camera chainCamera{0, 512 + 4e-7, 0};
    auto chained = prepareRayScene(level, chainCamera, nullptr);
    require(castPreparedSceneRay(chained, {1, 0}).type == 3, "Chained tie winner");
    checkRay(chain, chained, chainCamera, {1, 0}, chain.backtrack(chainCamera));
    chained.walls.erase(chained.walls.begin());
    require(castPreparedSceneRay(chained, {1, 0}).type == 2, "Dropping far tie changes winner");

    // The long diagonal is encountered by its AABB well before its hit.
    // The nearer wall allows traversal to stop before the distant wall.
    sector.vertices = {{512, 0}, {4096, 1536}, {1024, 0}, {1024, 1500},
                       {16384, 0}, {16384, 1500}};
    sector.walls = {{0, 1, 1, 0}, {2, 3, 2, 0}, {4, 5, 3, 0}};
    StaticRayGrid longWall(level, true);
    const Camera nearCamera{100, 1000, 0};
    const auto longScene = prepareRayScene(level, nearCamera, nullptr);
    cells = 0;
    candidates = 0;
    const auto near = longWall.cast(longScene, nearCamera, {1, 0}, longWall.backtrack(nearCamera),
                                    &cells, &candidates);
    require(near.type == 2 && cells < 8 && candidates == 2, "Long diagonal with early exit");
    checkRay(longWall, longScene, nearCamera, {1, 0}, longWall.backtrack(nearCamera));
    const Camera missCamera{100, 1600, 0};
    const auto missScene = prepareRayScene(level, missCamera, nullptr);
    cells = 0;
    const auto miss = longWall.cast(missScene, missCamera, {1, 0}, longWall.backtrack(missCamera),
                                    &cells);
    require(!std::isfinite(miss.distance) && cells >= 30, "Miss reaches the far edge of the grid");
    checkRay(longWall, missScene, missCamera, {1, 0}, longWall.backtrack(missCamera));
}

void checkFramebuffer()
{
    game_lab::Level level;
    auto& sector = level.sectors.emplace_back();
    sector.vertices = {{512, 0}, {512, 1024}, {2048, 0}, {2048, 1024}};
    sector.walls.assign(64, {2, 3, 1, 0});
    sector.walls.push_back({0, 1, 2, 0});
    const std::vector<doors::Door> doors{{0, {
        doors::Face{{256, 512}, {256, 0}, 1}, doors::Face{{256, 0}, {256, 512}, 2}}}};
    level.connectionStates = {30};
    const wall_textures::Texture wall{2, 2, {{20, 120, 30}, {50, 20, 60},
                                            {200, 100, 30}, {18, 79, 160}}};
    const wall_textures::Texture floor{128, 128,
        std::vector<wall_textures::Rgb>(128 * 128, {40, 60, 90})};
    const wall_textures::PlaneBank planes{floor, floor};
    sprites::Scene sprites;
    sprites.bank.images.push_back({wall, 1, 1, {1, 1, 1, 1}});
    sprites.objects.push_back({700, 580, 128, 90, 0, 0x4000, 0});
    for (const auto size : {SDL_Point{320, 240}, SDL_Point{853, 480}}) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
            SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        require(bool(surface), SDL_GetError());
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
        require(bool(renderer), SDL_GetError());
        FirstPersonRenderer scan(renderer.get()), dda(renderer.get());
        // Replace geometry in place between frames with the same level address
        // and wall count. The new wall must be found through the new cells.
        for (const auto y : {0, 1536, 0}) {
            sector.vertices[0].y = static_cast<std::uint16_t>(y);
            sector.vertices[1].y = static_cast<std::uint16_t>(y + 1024);
            const Camera camera{100, double(y + 540), 0};
            dda.setRaycastLevel(level);
            for (const bool fisheye : {false, true}) {
                portVideoFisheyeEnabled = fisheye;
                const auto draw = [&](FirstPersonRenderer& view) {
                    require(view.draw(level, camera, &wall, nullptr, &doors, &planes, &sprites) &&
                            SDL_RenderPresent(renderer.get()), SDL_GetError());
                };
                draw(scan);
                const auto* pixels = static_cast<const std::uint8_t*>(surface->pixels);
                const std::vector<std::uint8_t> expected(pixels, pixels + surface->pitch * size.y);
                draw(dda);
                require(std::memcmp(expected.data(), surface->pixels, expected.size()) == 0,
                        "Indexed renderer framebuffer differs");
            }
        }
    }
    portVideoFisheyeEnabled = false;
}

std::vector<Camera> viewpoints(const game_lab::Level& level)
{
    std::vector<Camera> result{initialCamera(level)};
    for (const auto index : {std::size_t{0}, level.sectors.size() / 2, level.sectors.size() - 1}) {
        const auto& sector = level.sectors[index];
        if (sector.vertices.empty()) continue;
        double x = 0, y = 0;
        for (const auto& vertex : sector.vertices) { x += vertex.x; y += vertex.y; }
        result.push_back({sector.gx + x / sector.vertices.size(),
                          sector.gy + y / sector.vertices.size(), 0});
    }
    return result;
}

void setPhase(game_lab::Level& level, const std::vector<doors::Door>& doors, unsigned phase)
{
    for (const auto& door : doors)
        level.connectionStates[door.stateIndex] = static_cast<std::uint16_t>(phase);
}

std::size_t checkLevel(game_lab::Level& level)
{
    StaticRayGrid grid(level, true);
    require(grid.valid(), "Expected a valid grid for campaign level");
    const auto doors = doors::closedDoors(level);
    const auto states = level.connectionStates;
    const auto cameras = viewpoints(level);
    std::size_t rays = 0;
    for (const auto state : {0u, 17u, 39u, 59u, 60u, 90u, unsigned(doors::dos::locked)}) {
        setPhase(level, doors, state);
        for (auto camera : cameras) {
            const auto backtrack = grid.backtrack(camera);
            const auto scene = prepareRayScene(level, camera, &doors);
            const double startingAngle = camera.angle;
            for (const auto rotation : {0.0, pi * 0.5, pi, pi * 1.5, 0.31}) {
                camera.angle = startingAngle + rotation;
                for (const auto width : {161, 427}) {
                    const auto view = projection(camera, width, width == 161 ? width : 240);
                    for (int x = 0; x < width; ++x)
                        checkRay(grid, scene, camera, columnRay(view, x), backtrack);
                    rays += width;
                }
            }
        }
    }
    level.connectionStates = states;
    return rays;
}

template <bool indexed>
double frameTime(StaticRayGrid& grid, const game_lab::Level& level, const Camera& camera,
                 const CameraProjection& view, const std::vector<doors::Door>& doors,
                 std::vector<WallHit>& hits)
{
    const auto start = Clock::now();
    const auto scene = prepareRayScene(level, camera, &doors);
    const double backtrack = indexed ? grid.backtrack(camera) : 0;
    for (int x = 0; x < view.width; ++x) {
        const auto direction = columnRay(view, x);
        if constexpr (indexed)
            hits[x] = grid.cast(scene, camera, direction, backtrack);
        else
            hits[x] = castPreparedSceneRay(scene, direction);
    }
    const auto end = Clock::now();
    double checksum = 0;
    for (const auto& hit : hits) checksum += hit.type + hit.segmentPosition + hit.distance;
    consumed = checksum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double median(std::vector<double>& samples)
{
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

void benchmark(game_lab::Level& level, unsigned levelNumber)
{
    constexpr int width = 1920;
    constexpr int frames = 21;
    auto doors = doors::closedDoors(level);
    std::vector<double> construction;
    for (int trial = 0; trial < frames; ++trial) {
        const auto start = Clock::now();
        StaticRayGrid trialGrid(level, true);
        construction.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
        require(trialGrid.valid(), "Benchmark grid is invalid");
    }
    StaticRayGrid grid(level, true);
    const auto cameras = viewpoints(level);
    std::vector<WallHit> hits(width);
    for (const auto phase : {0u, 30u}) {
        const bool alternate = phase != 0;
        auto camera = alternate ? cameras.back() : cameras.front();
        if (alternate) camera.angle += 0.31;
        setPhase(level, doors, phase);
        const auto view = projection(camera, width, alternate ? 1440 : width);
        const auto scene = prepareRayScene(level, camera, &doors);
        const double backtrack = grid.backtrack(camera);
        unsigned cells = 0;
        std::size_t candidates = 0;
        for (int x = 0; x < width; ++x) {
            const auto direction = columnRay(view, x);
            const auto hit = grid.cast(scene, camera, direction, backtrack, &cells, &candidates);
            require(sameHit(hit, castPreparedSceneRay(scene, direction)), "Benchmark hit differs");
        }
        std::vector<double> scan, dda;
        for (int frame = -1; frame < frames; ++frame) {
            double before, after;
            if (frame % 2 == 0) {
                before = frameTime<false>(grid, level, camera, view, doors, hits);
                after = frameTime<true>(grid, level, camera, view, doors, hits);
            } else {
                after = frameTime<true>(grid, level, camera, view, doors, hits);
                before = frameTime<false>(grid, level, camera, view, doors, hits);
            }
            if (frame >= 0) { scan.push_back(before); dda.push_back(after); }
        }
        const auto doorTests = scene.doors.size() * width;
        std::cout << levelNumber << ',' << phase << ',' << width << ',' << median(construction)
                  << ',' << grid.bytes() << ',' << double(cells) / width << ','
                  << double(candidates) / width << ',' << scene.walls.size() * width + doorTests
                  << ',' << candidates + doorTests << ',' << scene.walls.size() * width - candidates
                  << ',' << median(scan) << ',' << median(dda) << '\n';
    }
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        checkEdges();
        if (argc == 1) { checkFramebuffer(); return 0; }
        require(argc == 3, "Usage: raycast_benchmark GAME.LAB check|measure");
        const std::string mode = argv[2];
        if (mode == "check") {
            const auto count = game_lab::loadLevel(argv[1], 0).header.levelCount;
            require(count == 30, "Expected the 30 campaign levels");
            std::size_t rays = 0;
            for (unsigned index = 0; index < count; ++index) {
                auto level = game_lab::loadLevel(argv[1], index).level;
                try {
                    rays += checkLevel(level);
                } catch (const std::exception& error) {
                    throw std::runtime_error("Level " + std::to_string(index + 1) + ": " + error.what());
                }
            }
            std::cout << count << " levels, " << rays << " exactly equivalent column hits\n";
        } else {
            require(mode == "measure", "Expected check or measure");
            std::cout << std::fixed << std::setprecision(4)
                      << "level,phase,width,build_ms,index_bytes,cells_per_ray,"
                         "candidates_per_ray,scan_tests,dda_tests,avoided,scan_ms,dda_ms\n";
            for (const auto number : {1u, 20u, 22u, 23u}) {
                auto level = game_lab::loadLevel(argv[1], number - 1).level;
                benchmark(level, number);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
