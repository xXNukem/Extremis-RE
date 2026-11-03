#include "map_view.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
SDL_Color wallColor(std::uint16_t type)
{
    switch (type & 0xf000) {
    case 0x0000: return {170, 170, 170, 255};
    case 0x1000: return {70, 140, 255, 255};
    case 0x2000: return {255, 220, 60, 255};
    case 0x3000: return {255, 100, 190, 255};
    default: return {255, 255, 255, 255};
    }
}

bool setColor(SDL_Renderer* renderer, SDL_Color color)
{
    return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

bool drawPlayerMarker(SDL_Renderer* renderer, SDL_FPoint position, double angle)
{
    const float dx = float(std::cos(angle)), dy = -float(std::sin(angle));
    const SDL_FPoint arrow[] = {
        {position.x + dx * 10, position.y + dy * 10},
        {position.x - dx * 5 - dy * 5, position.y - dy * 5 + dx * 5},
        {position.x - dx * 5 + dy * 5, position.y - dy * 5 - dx * 5},
        {position.x + dx * 10, position.y + dy * 10}};
    return setColor(renderer, {255, 255, 255, 255}) &&
           SDL_RenderLines(renderer, arrow, 4) &&
           SDL_RenderLine(renderer, position.x, position.y,
                          position.x + dx * 20, position.y + dy * 20);
}
} // namespace

bool drawMap(SDL_Renderer* renderer, const game_lab::Level& level, bool showPolygons,
             const Camera* camera)
{
    float minX = std::numeric_limits<float>::max();
    float minY = minX;
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = maxX;
    bool hasVertices = false;
    for (const auto& sector : level.sectors) {
        for (const auto& vertex : sector.vertices) {
            const float x = std::uint32_t(sector.gx) + vertex.x;
            const float y = std::uint32_t(sector.gy) + vertex.y;
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
            hasVertices = true;
        }
    }
    if (!hasVertices) {
        return true;
    }

    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(renderer, &width, &height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return true;
    }
    // Recalculate each frame so resizing and pixel-density changes fit naturally.
    const float margin = std::min(32.0f, std::min(width, height) * 0.1f);
    const float spanX = maxX - minX;
    const float spanY = maxY - minY;
    const float scale = std::min((width - 2 * margin) / std::max(spanX, 1.0f),
                                 (height - 2 * margin) / std::max(spanY, 1.0f));
    const float left = (width - spanX * scale) * 0.5f;
    const float top = (height - spanY * scale) * 0.5f;
    const auto projectWorld = [&](double x, double y) {
        return SDL_FPoint{float(left + (x - minX) * scale),
                         float(top + (maxY - y) * scale)};
    };
    const auto project = [&](const game_lab::Sector& sector,
                             const game_lab::Vertex& vertex) {
        const float x = std::uint32_t(sector.gx) + vertex.x;
        const float y = std::uint32_t(sector.gy) + vertex.y;
        return projectWorld(x, y);
    };

    // Draw contours first so wall colors and entity markers remain intact.
    if (showPolygons) {
        if (!setColor(renderer, {80, 210, 220, 255})) {
            return false;
        }
        for (const auto& sector : level.sectors) {
            for (const auto& polygon : sector.polygons) {
                if (polygon.vertices.size() < 2) {
                    continue;
                }
                for (std::size_t i = 0; i < polygon.vertices.size(); ++i) {
                    const auto a = project(sector, sector.vertices[polygon.vertices[i]]);
                    const auto b = project(sector, sector.vertices[
                        polygon.vertices[(i + 1) % polygon.vertices.size()]]);
                    if (!SDL_RenderLine(renderer, a.x, a.y, b.x, b.y)) {
                        return false;
                    }
                }
            }
        }
    }
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            const auto a = project(sector, sector.vertices[wall.vertexA]);
            const auto b = project(sector, sector.vertices[wall.vertexB]);
            if (!setColor(renderer, wallColor(wall.type)) ||
                !SDL_RenderLine(renderer, a.x, a.y, b.x, b.y)) {
                return false;
            }
        }
    }
    // Draw markers after walls so entities remain visible at wall vertices.
    const float markerSize = std::min(6.0f, margin);
    for (const auto& sector : level.sectors) {
        for (const auto& entity : sector.entitiesA) {
            SDL_Color color{255, 255, 255, 255};
            switch (entity.type & 0xf000) {
            case 0x4000: color = {255, 70, 70, 255}; break;
            case 0x5000: color = {70, 230, 100, 255}; break;
            }
            const auto point = project(sector, sector.vertices[entity.vertex]);
            const SDL_FRect marker{point.x - markerSize * 0.5f,
                                   point.y - markerSize * 0.5f,
                                   markerSize, markerSize};
            if (!setColor(renderer, color) || !SDL_RenderFillRect(renderer, &marker)) {
                return false;
            }
        }
    }
    return !camera || drawPlayerMarker(renderer, projectWorld(camera->x, camera->y), camera->angle);
}
