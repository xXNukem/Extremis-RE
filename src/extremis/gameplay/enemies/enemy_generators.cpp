#include "extremis/gameplay/enemies/enemy_system.h"

#include <algorithm>

namespace enemies {
namespace {
const game_lab::Vertex* generatorVertex(const game_lab::Sector& sector)
{
    if (sector.entitiesA.empty()) return nullptr;
    const auto& first = sector.entitiesA.front();
    if ((first.type & 255) != 0 || first.vertex >= sector.vertices.size()) return nullptr;
    return &sector.vertices[first.vertex];
}
}

void System::updateGenerators(const game_lab::Level& level, const Camera& camera,
                              std::uint16_t ticks, std::uint16_t& campaignAccumulator)
{
    if (level.generatorSectors.empty()) return;
    campaignAccumulator = std::uint16_t(campaignAccumulator + ticks);
    if (campaignAccumulator < 800 || campaignAccumulator >= 0x8000) return;
    campaignAccumulator = 0;
    auto source = camera.sector;
    if (!source) {
        const auto location = collision::locate(level,{camera.x,camera.y});
        if (location) source = location->sector;
    }
    const auto visible = doors::connectedSectors(level,source.value_or(level.sectors.size()));
    auto slot = records_.begin();
    for (const auto sectorIndex : level.generatorSectors) {
        if (sectorIndex >= level.sectors.size() || visible[sectorIndex]) continue;
        const auto& sector = level.sectors[sectorIndex];
        const auto* vertex = generatorVertex(sector);
        if (!vertex) continue;
        slot = std::find_if(slot,records_.end(),[](const auto& enemy) {
            return enemy.health == 0xffff && enemy.behavior == 4;
        });
        if (slot == records_.end()) break;
        slot->health = level.generatorHealth;
        slot->state = 6;
        slot->phase = 0;
        slot->sector = sectorIndex;
        slot->x = vertex->x;
        slot->y = vertex->y;
        slot->polygon = 0xffff;
        const auto index = std::size_t(slot - records_.begin());
        origins_[index] = {double(sector.gx),double(sector.gy)};
        recoil5_[index] = 0;
        ++slot;
    }
    rebuildScene();
}

std::vector<unsigned> System::blast(game_lab::Level& level, game_lab::Vertex origin)
{
    std::vector<unsigned> disabled;
    for (auto index : level.generatorSectors) {
        if (index >= level.sectors.size()) continue;
        auto& sector = level.sectors[index];
        const auto* vertex = generatorVertex(sector);
        if (!vertex) continue;
        const game_lab::Vertex world{std::uint16_t(sector.gx + vertex->x),
                                     std::uint16_t(sector.gy + vertex->y)};
        if (game_lab::blastStrength(origin,world) >= 75) {
            sector.entitiesA.front().type |= 1;
            disabled.push_back(index);
        }
    }
    for (auto& enemy : records_) {
        if (enemy.health == 0xffff || enemy.sector >= level.sectors.size()) continue;
        const auto& sector = level.sectors[enemy.sector];
        const game_lab::Vertex world{std::uint16_t(sector.gx + enemy.x),
                                     std::uint16_t(sector.gy + enemy.y)};
        const auto damage = game_lab::blastStrength(origin,world) * 20 / 100;
        enemy.health = damage > enemy.health ? 0xffff : std::uint16_t(enemy.health - damage);
    }
    rebuildScene();
    return disabled;
}
}
