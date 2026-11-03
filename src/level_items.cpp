#include "level_items.h"

#include <cmath>
#include <limits>

namespace items {
World::World(const game_lab::Level& level)
{
    for (std::size_t s = 0; s < level.sectors.size(); ++s) {
        const auto& sector = level.sectors[s];
        for (std::size_t w = 0; w < sector.walls.size(); ++w) {
            const auto& wall = sector.walls[w];
            const auto family = wall.type & 0xff00;
            const unsigned kind = wall.extra >> 12;
            if (family != 0x3000 && family != 0x3100 && family != 0x3300 && family != 0x3500 && family != 0x3600 &&
                !(family == 0x3200 && kind <= 5)) continue;
            if (wall.vertexA >= sector.vertices.size() || wall.vertexB >= sector.vertices.size()) continue;
            const auto a = sector.vertices[wall.vertexA], b = sector.vertices[wall.vertexB];
            panels_.push_back({s,w,{{double(sector.gx)+a.x,double(sector.gy)+a.y},
                                    {double(sector.gx)+b.x,double(sector.gy)+b.y}},
                               family == 0x3100,wall.extra,family == 0x3300,family == 0x3600,family==0x3500,family==0x3000});
        }
    }
}

const std::vector<Panel>& World::panels() const { return panels_; }

std::optional<double> World::reachable(const Panel& panel, const Camera& camera,
                                      const collision::World& obstacles) const
{
    const double dx = panel.face.b.x-panel.face.a.x, dy = panel.face.b.y-panel.face.a.y;
    const double length = std::hypot(dx,dy);
    if (length <= 0) return std::nullopt;
    const collision::Point tangent{dx/length,dy/length}, normal{dy/length,-dx/length};
    const double px = camera.x-panel.face.a.x, py = camera.y-panel.face.a.y;
    const double distance = px*normal.x+py*normal.y;
    const double along = px*tangent.x+py*tangent.y;
    // EXE 0x10bc1..0x10bda: 0<=normal<90, 0<=along<wall length.
    if (distance < 0 || distance >= 90 || along < 0 || along >= length) return std::nullopt;
    // EXE angles have 720 units per revolution: [120,240) around the tangent
    // is a 60-degree facing cone. Floating geometry avoids DOS fixed-point loss.
    const double facing = -std::cos(camera.angle)*normal.x-std::sin(camera.angle)*normal.y;
    if (facing < std::cos(3.14159265358979323846/6)) return std::nullopt;
    const collision::Point towardWall{-normal.x,-normal.y};
    const auto hit = collision::rayDistance(obstacles,{camera.x,camera.y},towardWall,distance);
    if (hit && *hit < distance-1e-6) return std::nullopt;
    return distance;
}

Collection World::collect(const Camera& camera, bool alive, player::Inventory& inventory,
                          const collision::World& obstacles)
{
    if (!alive) return {};
    const auto chosen=target(camera,obstacles);
    return chosen ? Collection{chosen,take(*chosen,inventory)} : Collection{};
}

std::optional<std::size_t> World::target(const Camera& camera, const collision::World& obstacles) const
{
    if (!std::isfinite(camera.x) || !std::isfinite(camera.y) ||
        !std::isfinite(camera.angle)) return {};
    std::optional<std::size_t> chosen;
    double nearest = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto distance = reachable(panels_[i],camera,obstacles);
        if (distance && *distance < nearest) { chosen = i; nearest = *distance; }
    }
    return chosen;
}

player::Transfer World::take(std::size_t index, player::Inventory& inventory)
{
    if (index>=panels_.size()) return player::Transfer::Unsupported;
    auto& panel=panels_[index];
    if (panel.cardReader || panel.codePanel || panel.codeReader || panel.elevator) return player::Transfer::Unsupported;
    return panel.weapon ? inventory.exchangeWeapon(panel.contents) : inventory.takeSupply(panel.contents);
}
player::Transfer World::replaceCard(std::size_t index, player::Inventory& inventory, unsigned slot)
{
    if (index>=panels_.size() || panels_[index].weapon || panels_[index].cardReader ||
        panels_[index].codePanel || panels_[index].codeReader || panels_[index].elevator)
        return player::Transfer::Unsupported;
    return inventory.replaceCard(panels_[index].contents,slot);
}
} // namespace items
