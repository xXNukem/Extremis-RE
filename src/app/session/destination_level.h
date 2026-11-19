#pragma once

#include "extremis/gameplay/player/camera.h"
#include "extremis/world/door.h"
#include "extremis/gameplay/enemies/enemy_system.h"
#include "extremis/resources/game_lab.h"
#include "extremis/ui/original_hud.h"
#include "extremis/render/first_person/materials/plane_appearance.h"
#include "extremis/world/player_collision.h"
#include "extremis/render/first_person/materials/texture_preview.h"
#include "extremis/gameplay/player/weapon_fire.h"

#include <vector>

namespace app {
// All resources needed to publish a different level. The active session is
// unchanged if preparation fails or the elevator sequence is interrupted.
struct DestinationLevel {
    game_lab::Level1 levelData;
    Camera camera;
    sprites::Scene sprites;
    enemies::System enemies;
    hud::Renderer hud;
    player::WeaponFire weapon;
    TexturePreview textures;
    PlaneAppearance planes;
    collision::World world;
    std::vector<doors::Door> doors;
};

DestinationLevel prepareDestination(const char* labPath, unsigned zone,
                                    const game_lab::PlayerStart& arrival);
}
