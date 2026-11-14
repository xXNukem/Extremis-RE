#include "first_person_view.h"
#include "wall_material.h"
#include "plane_renderer.h"
#include "sprite_renderer.h"
#include "player_collision.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double wallHeight = 256.0;
constexpr double fov = Camera::horizontalFov;

struct Point {
    double x, y;
};

struct PolygonBounds {
    Point low{1e30, 1e30};
    Point high{-1e30, -1e30};
    double area2 = 0;
};

struct CameraProjection {
    int width, height;
    double forwardX, forwardY;
    double plane, focal, eyeHeight;
};

struct ProjectedColumn {
    float top, bottom;
    double depth;
    double unclippedTop, unclippedBottom;
};

struct SegmentHit {
    double distance, position;
};

Point globalVertex(const game_lab::Sector& sector, std::uint16_t index)
{
    const auto& vertex = sector.vertices[index];
    return {double(sector.gx) + vertex.x, double(sector.gy) + vertex.y};
}

double cross(Point a, Point b)
{
    return a.x * b.y - a.y * b.x;
}

double segmentDistance(Point point, Point a, Point b)
{
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    const double t = length2 == 0 ? 0 : std::clamp(
        ((point.x - a.x) * dx + (point.y - a.y) * dy) / length2, 0.0, 1.0);
    return std::hypot(point.x - a.x - t * dx, point.y - a.y - t * dy);
}

PolygonBounds polygonBounds(const game_lab::Sector& sector,
                            const game_lab::Polygon& polygon)
{
    PolygonBounds bounds;
    for (std::size_t i = 0; i < polygon.vertices.size(); ++i) {
        const auto a = globalVertex(sector, polygon.vertices[i]);
        const auto b = globalVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()]);
        bounds.area2 += cross(a, b);
        bounds.low.x = std::min(bounds.low.x, a.x);
        bounds.low.y = std::min(bounds.low.y, a.y);
        bounds.high.x = std::max(bounds.high.x, a.x);
        bounds.high.y = std::max(bounds.high.y, a.y);
    }
    return bounds;
}

double cameraClearance(Point point, const game_lab::Level& level,
                       const game_lab::Sector& sector, const game_lab::Polygon& polygon)
{
    double clearance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < polygon.vertices.size(); ++i) {
        clearance = std::min(clearance, segmentDistance(point,
            globalVertex(sector, polygon.vertices[i]),
            globalVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()])));
    }
    // Include portals here: the initial camera must not lie on any wall segment.
    for (const auto& other : level.sectors) {
        for (const auto& wall : other.walls) {
            clearance = std::min(clearance, segmentDistance(point,
                globalVertex(other, wall.vertexA), globalVertex(other, wall.vertexB)));
        }
    }
    return clearance;
}

void sampleCameraPositions(const game_lab::Level& level, const game_lab::Sector& sector,
                           const game_lab::Polygon& polygon, const PolygonBounds& bounds,
                           double& bestClearance, Camera& best)
{
    // Preserve sample order and strict comparison so ties select the same position.
    // A concave polygon's centroid is not necessarily inside it.
    for (int y = 1; y < 20; ++y) {
        for (int x = 1; x < 20; ++x) {
            const Point point{bounds.low.x + (bounds.high.x - bounds.low.x) * x / 20.0,
                              bounds.low.y + (bounds.high.y - bounds.low.y) * y / 20.0};
            if (!collision::insidePolygon({point.x,point.y}, sector, polygon)) {
                continue;
            }
            const double clearance = cameraClearance(point, level, sector, polygon);
            if (clearance > bestClearance) {
                bestClearance = clearance;
                best = {point.x, point.y, 0};
            }
        }
    }
}

void faceLongestSightline(const game_lab::Level& level, Camera& camera)
{
    double longest = 0;
    for (int i = 0; i < 36; ++i) {
        const double angle = i * 2 * pi / 36;
        const auto hit = castWallRay(level, camera, std::cos(angle), std::sin(angle));
        if (std::isfinite(hit.distance) && hit.distance > longest) {
            longest = hit.distance;
            camera.angle = angle;
        }
    }
}

std::optional<SegmentHit> intersectSegment(const Camera& camera, Point direction,
                                       Point a, Point b)
{
    const Point edge{b.x - a.x, b.y - a.y};
    const Point delta{a.x - camera.x, a.y - camera.y};
    const double denominator = cross(direction, edge);
    if (std::abs(denominator) < 1e-9) {
        return std::nullopt; // Parallel ray or zero-length segment.
    }
    const double t = cross(delta, edge) / denominator;
    const double u = cross(delta, direction) / denominator;
    if (t > 1e-6 && u >= -1e-9 && u <= 1 + 1e-9) {
        return SegmentHit{t, std::clamp(u, 0.0, 1.0)};
    }
    return std::nullopt;
}

Point columnRay(const CameraProjection& projection, int column)
{
    const double offset = (2 * (column + 0.5) / projection.width - 1) * projection.plane;
    // Right = (sin(angle), -cos(angle)), matching the inverted map Y axis.
    // Do not normalize: forward dot ray = 1, so t is perpendicular depth.
    return {projection.forwardX + projection.forwardY * offset,
            projection.forwardY - projection.forwardX * offset};
}

ProjectedColumn projectWall(const CameraProjection& projection, double distance)
{
    const double depth = std::max(0.01, distance);
    const double unclippedTop = projection.height * 0.5 -
        (wallHeight - projection.eyeHeight) * projection.focal / depth;
    const double unclippedBottom = projection.height * 0.5 +
        projection.eyeHeight * projection.focal / depth;
    const float top = float(std::clamp(unclippedTop, 0.0, double(projection.height)));
    const float bottom = float(std::clamp(unclippedBottom, 0.0, double(projection.height)));
    return {top, bottom, depth, unclippedTop, unclippedBottom};
}

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

double distanceShade(double depth)
{
    return 0.2 + 0.8 / (1.0 + depth / 700.0);
}

SDL_Color wallBaseColor(std::uint16_t type)
{
    SDL_Color color{170, 170, 170, 255};
    switch (type & 0xf000) {
    case 0x2000: color = {255, 220, 60, 255}; break;
    case 0x3000: color = {255, 100, 190, 255}; break;
    }
    return color;
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

std::uint32_t rgbaPixel(wall_textures::Rgb color, double shade = 1.0)
{
    return (std::uint32_t(color.r * shade) << 24) |
           (std::uint32_t(color.g * shade) << 16) |
           (std::uint32_t(color.b * shade) << 8) | 255;
}

void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        const auto color = y < height * 0.5 ? rgbaPixel({20, 23, 32}) : rgbaPixel({45, 39, 34});
        auto row = pixels.begin() + std::size_t(y) * width;
        std::fill(row, row + width, color);
    }
}

void rasterizeWallColumn(std::vector<std::uint32_t>& pixels, int width, int height,
                            int column, const WallHit& hit, const ProjectedColumn& projected,
                            const wall_textures::Texture* texture,
                            std::vector<double>* depth = nullptr)
{
    // Frames span the first half of DAL's columns; other walls use the full image.
    const int columns = texture ? wallTextureColumnCount(hit.type, texture->width) : 0;
    const double textureExtent = columns * hit.textureSpan;
    const int lastColumn = texture ? std::max(0, int(std::ceil(textureExtent)) - 1) : 0;
    const int textureX = texture ? std::min(lastColumn,
        int(hit.segmentPosition * textureExtent)) : 0;
    const double projectedHeight = projected.unclippedBottom - projected.unclippedTop;
    const double shade = distanceShade(projected.depth);
    const int begin = int(std::ceil(std::clamp(projected.unclippedTop, 0.0,
                                              double(height)) - 0.5));
    const int end = int(std::ceil(std::clamp(projected.unclippedBottom, 0.0,
                                            double(height)) - 0.5));
    const auto base = wallBaseColor(hit.type);
    const auto flatPixel = rgbaPixel({base.r, base.g, base.b}, shade);
    // This level has sealed floor/ceiling planes and full-height walls/doors.
    // Their nearest hit also hides billboard overhang above/below the wall's
    // screen rectangle (floor stains and ceiling fixtures use interior anchors).
    if (depth) {
        for (int y = 0; y < height; ++y)
            (*depth)[std::size_t(y) * width + column] = hit.distance;
    }
    for (int y = begin; y < end; ++y) {
        auto pixel = flatPixel;
        if (texture) {
            // Use the original projected extent: clipping crops rather than stretching.
            const double v = (y + 0.5 - projected.unclippedTop) / projectedHeight;
            const int textureY = std::clamp(int(v * texture->height), 0, texture->height - 1);
            const auto color = texture->pixels[std::size_t(textureY) * texture->width + textureX];
            pixel = rgbaPixel(color, shade);
        }
        pixels[std::size_t(y) * width + column] = pixel;
    }
}

bool validTexture(const wall_textures::Texture& texture)
{
    return texture.width > 0 && texture.height > 0 &&
           texture.pixels.size() == std::size_t(texture.width) * texture.height;
}

const wall_textures::Texture* selectWallTexture(const WallHit& hit,
                                               const wall_textures::Bank* materials,
                                               const wall_textures::Texture* overrideTexture)
{
    if (overrideTexture) {
        return overrideTexture;
    }
    if (!materials) return nullptr;
    const auto index = wallTextureIndex(hit.type, materials->textures.size());
    if (!index || !validTexture(materials->textures[*index])) {
        return nullptr; // Preserve a visible flat-color wall for unsupported materials.
    }
    return &materials->textures[*index];
}

template <typename DrawColumn>
bool renderVisibleColumns(const game_lab::Level& level, const Camera& camera,
                          int width, int height, const std::vector<doors::Door>* doors,
                          DrawColumn drawColumn)
{
    const double plane = std::tan(fov * 0.5);
    const CameraProjection projection{width, height, std::cos(camera.angle),
                                      std::sin(camera.angle), plane, width / (2 * plane), camera.viewHeight()};
    for (int column = 0; column < width; ++column) {
        const auto direction = columnRay(projection, column);
        const auto hit = castSceneRay(level, camera, direction.x, direction.y, doors);
        if (!std::isfinite(hit.distance)) {
            continue;
        }
        if (!drawColumn(column, hit, projectWall(projection, hit.distance))) {
            return false;
        }
    }
    return true;
}

} // namespace

Camera initialCamera(const game_lab::Level& level)
{
    if (level.playerStart) {
        const auto& start=*level.playerStart;
        if (start.sector>=level.sectors.size())
            throw std::runtime_error("Invalid original player start sector");
        const auto& sector=level.sectors[start.sector];
        Camera camera{double(sector.gx)+start.x,double(sector.gy)+start.y,
                start.angle*(2*3.14159265358979323846/720)};
        camera.sector=start.sector;
        return camera;
    }
    double bestClearance = 1.0;
    Camera best;
    for (const auto& sector : level.sectors) {
        for (const auto& polygon : sector.polygons) {
            if (polygon.vertices.size() < 3) {
                continue;
            }
            const auto bounds = polygonBounds(sector, polygon);
            if (std::abs(bounds.area2) < 1e-6) {
                continue;
            }
            sampleCameraPositions(level, sector, polygon, bounds, bestClearance, best);
        }
    }
    if (bestClearance > 1.0) {
        faceLongestSightline(level, best);
        return best;
    }
    throw std::runtime_error("No polygon interior safely clear of wall segments found");
}

WallHit castWallRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy)
{
    WallHit hit{std::numeric_limits<double>::infinity(), 0};
    bool hitFacesCamera = false;
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            if ((wall.type & 0xf000) == 0x1000) {
                continue; // Portals are not visible surfaces in this prototype.
            }
            const auto a = globalVertex(sector, wall.vertexA);
            const auto b = globalVertex(sector, wall.vertexB);
            const auto intersection = intersectSegment(camera, {dx, dy}, a, b);
            if (!intersection) continue;
            const bool facesCamera = cross({b.x - a.x, b.y - a.y},
                                           {camera.x - a.x, camera.y - a.y}) < 0;
            // Shared boundaries can have opposite faces with different segment
            // cuts. Roundoff must not choose alternating materials/U directions.
            // Use the original A-left/B-right convention only to resolve ties;
            // isolated walls retain their existing two-sided rendering.
            const bool coincident = std::isfinite(hit.distance) &&
                std::abs(intersection->distance - hit.distance) <=
                    1e-9 * std::max(1.0, std::abs(hit.distance));
            if (coincident ? (facesCamera && !hitFacesCamera)
                           : intersection->distance < hit.distance) {
                hit = {intersection->distance, wall.type, intersection->position};
                hitFacesCamera = facesCamera;
            }
        }
    }
    return hit;
}

WallHit castSceneRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy, const std::vector<doors::Door>* doors)
{
    auto hit = castWallRay(level, camera, dx, dy);
    if (!doors) return hit;
    for (const auto& door : *doors) {
        for (const auto& closedFace : door.faces) {
            const auto face = doors::visibleFace(level, door, closedFace);
            if (!face) continue;
            const Point a{face->a.x, face->a.y}, b{face->b.x, face->b.y};
            // Original projected walls retain A-left/B-right faces only.
            if (cross({b.x - a.x, b.y - a.y}, {camera.x - a.x, camera.y - a.y}) >= 0)
                continue;
            const auto intersection = intersectSegment(camera, {dx, dy}, a, b);
            if (intersection && intersection->distance < hit.distance) {
                hit = {intersection->distance, face->material, intersection->position,
                       face->textureSpan};
            }
        }
    }
    return hit;
}

bool drawFirstPerson(SDL_Renderer* renderer, const game_lab::Level& level,
                     const Camera& camera, const std::vector<doors::Door>* doors)
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
    return renderVisibleColumns(level, camera, width, height, doors,
        [&](int column, const WallHit& hit, const ProjectedColumn& projected) {
            return drawWallColumn(renderer, column, projected, hit.type);
        });
}

FirstPersonRenderer::FirstPersonRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}
FirstPersonRenderer::~FirstPersonRenderer() = default;

void FirstPersonRenderer::TextureDeleter::operator()(SDL_Texture* texture) const
{
    SDL_DestroyTexture(texture);
}

bool FirstPersonRenderer::resizeFrame(int width, int height, bool withHud)
{
    if (width == width_ && height == height_ && withHud == withHud_) {
        return true;
    }
    // RGBA8888 specifies channel bit positions, independent of host byte order.
    std::unique_ptr<SDL_Texture, TextureDeleter> texture(SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        width, withHud ? hud::height : height));
    if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE) ||
        !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST)) {
        return false;
    }
    frameReady_=false; showingMap_=false;
    pixels_.resize(std::size_t(width) * height);
    composed_.resize(withHud ? hud::width * hud::height : 0);
    uploadTexture_ = std::move(texture);
    width_ = width;
    height_ = height;
    withHud_ = withHud;
    return true;
}

bool FirstPersonRenderer::presentFrame(bool withHud, int outputWidth, int outputHeight)
{
    const auto& frame = withHud ? composed_ : pixels_;
    if (!SDL_UpdateTexture(uploadTexture_.get(), nullptr, frame.data(),
                           width_ * int(sizeof(std::uint32_t)))) return false;
    if (!withHud) return SDL_RenderTexture(renderer_, uploadTexture_.get(), nullptr, nullptr);
    const auto fit = hud::presentation(outputWidth,outputHeight);
    const SDL_FRect destination{fit.x,fit.y,fit.width,fit.height};
    return SDL_RenderTexture(renderer_, uploadTexture_.get(), nullptr, &destination);
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
                               const original_map::Renderer* map, const original_map::State* mapState)
{
    if (!provisionalTexture && !materials && !planes && !spriteScene && !projectileScene && !hudRenderer) {
        return drawFirstPerson(renderer_, level, camera, doors);
    }
    if (provisionalTexture && !validTexture(*provisionalTexture)) {
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
    if (hudRenderer) { width = hud::width; height = hud::viewHeight; }
    if (!resizeFrame(width, height, hudRenderer != nullptr)) {
        return false;
    }
    if (hudRenderer && map && mapState && frameReady_) {
        if (!showingMap_) mapBackground_=composed_;
        showingMap_=true;
        composed_=mapBackground_;
        if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        return presentFrame(true,outputWidth,outputHeight);
    }
    showingMap_=false;
    fillBackground(pixels_, width, height);
    if (spriteScene || projectileScene || enemyScene) depth_.assign(pixels_.size(), std::numeric_limits<double>::infinity());
    const double focal = width / (2 * std::tan(fov * 0.5));
    if (planes) {
        planes::render(pixels_, camera, {width, height, focal, camera.viewHeight(), wallHeight}, *planes);
    }
    const bool wallsDrawn = renderVisibleColumns(level, camera, width, height, doors,
        [&](int column, const WallHit& hit, const ProjectedColumn& projected) {
            const auto* texture = selectWallTexture(hit, materials, provisionalTexture);
            rasterizeWallColumn(pixels_, width, height, column, hit, projected, texture,
                                (spriteScene || projectileScene || enemyScene) ? &depth_ : nullptr);
            return true;
        });
    if (!wallsDrawn) return false;
    if (spriteScene) sprites::render(pixels_, depth_, *spriteScene, camera,
                                    {width, height, focal, camera.viewHeight()});
    if (enemyScene) sprites::render(pixels_,depth_,*enemyScene,camera,
                                   {width,height,focal,camera.viewHeight()});
    if (enemyScene && enemyEffects) sprites::render(pixels_,depth_,enemyScene->bank,*enemyEffects,camera,
                                                   {width,height,focal,camera.viewHeight()});
    if (projectileScene) sprites::render(pixels_,depth_,*projectileScene,camera,
                                        {width,height,focal,camera.viewHeight()});
    if (hudRenderer) {
        std::copy(pixels_.begin(),pixels_.end(),composed_.begin());
        if (!hudRenderer->draw(composed_,hudState)) return SDL_SetError("Invalid HUD framebuffer");
        if (interactionRenderer && interactionState && !interactionRenderer->draw(composed_,*interactionState))
            return SDL_SetError("Invalid original interaction framebuffer");
        if (map && mapState) {
            mapBackground_=composed_; showingMap_=true;
            if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        }
    }
    frameReady_=true;
    return presentFrame(hudRenderer != nullptr,outputWidth,outputHeight);
}
