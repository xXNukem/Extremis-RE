#include "destination_level.h"

#include "extremis/render/first_person/camera/camera_start.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace app {
DestinationLevel prepareDestination(const char* labPath, game_lab::Level1 levelData,
                                    const game_lab::PlayerStart& arrival)
{
    auto& level=levelData.level;
    level.playerStart=arrival;
    const auto camera=initialCamera(level);
    const auto directory=std::filesystem::path(labPath).parent_path()/"DATA";
    auto spriteScene=sprites::loadScene(level,directory);
    enemies::System enemySystem;
    enemySystem.load(level,directory);
    auto hudRenderer=hud::Renderer(hud::load(directory,level.palBank));
    player::WeaponFire weapon;
    weapon.loadGraphics(directory,level.palBank);
    TexturePreview textures;
    textures.load(directory,level.dalleBank,level.palBank);
    PlaneAppearance planes;
    planes.load(directory,level.dalleBank,level.palBank);
    if (!textures.materials())
        throw std::runtime_error("Missing destination wall textures");
    auto world=collision::staticWorld(level);
    auto levelDoors=doors::closedDoors(level);
    return {std::move(levelData),camera,std::move(spriteScene),std::move(enemySystem),
            std::move(hudRenderer),std::move(weapon),std::move(textures),std::move(planes),
            std::move(world),std::move(levelDoors)};
}
}
