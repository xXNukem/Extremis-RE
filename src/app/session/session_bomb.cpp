#include "app_session.h"
#include "extremis/audio/game_audio.h"

#include <cmath>
#include <stdexcept>

namespace app {
void AppSession::updateBomb(const std::filesystem::path& directory, GameAudio& audio, std::uint16_t ticks)
{
    bombFrame = placedBomb.update(ticks);
    bombScene.objects.clear();
    if (bombFrame.shock) survival.postMessage(messages::Id::Shock);
    if (bombFrame.explosionSound) audio.bombExplosion();
    if (bombFrame.image < 0 && !bombFrame.damage) return;

    auto& level = data->level;
    const auto sector = placedBomb.sector();
    const auto origin = game_lab::bombOrigin(level,sector,placedBomb.x(),placedBomb.y());
    if (!origin) return;
    if (bombFrame.damage) {
        const game_lab::Vertex playerPosition{static_cast<std::uint16_t>(std::lround(camera->x)),
                                              static_cast<std::uint16_t>(std::lround(camera->y))};
        survival.vitals().applyDamage(5 * game_lab::blastStrength(*origin,playerPosition));
        if (enemySystem) {
            const auto disabled = enemySystem->blast(level,*origin);
            if (spriteScene) {
                for (const auto index : disabled) {
                    for (std::size_t i = 0; i < spriteScene->placements.size(); ++i) {
                        auto& placement = spriteScene->placements[i];
                        if (placement.sourceSector != index || placement.sourceEntity != 0) continue;
                        placement.type = level.sectors[index].entitiesA.front().type;
                        auto& object = spriteScene->objects[i];
                        object.image = 1;
                        if (const auto height = sprites::worldHeight(level.objetBank,1)) object.height = *height;
                    }
                }
            }
        }
    }
    if (bombFrame.image < 0 || sector >= level.sectors.size()) return;
    const auto visible = doors::connectedSectors(level,camera->sector.value_or(level.sectors.size()));
    if (!visible[sector]) return;
    if (bombScene.bank.images.empty() || bombPalette != level.palBank) {
        const auto palette = wall_textures::loadPalette(directory / ("PAL"+std::to_string(level.palBank)+".PAL"));
        auto bank = sprites::load(directory / "BOMBE.SPR",palette);
        if (bank.images.size() != 3) throw std::runtime_error("Expected three bomb sprites");
        bombScene.bank = std::move(bank);
        bombPalette = level.palBank;
    }
    bombScene.objects.push_back({double(origin->x),double(origin->y),0.,double(bombFrame.size),
                                 static_cast<std::size_t>(bombFrame.image)});
}
}
