#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include "../session/app_session.h"

#include "extremis/render/first_person/camera/camera_start.h"
#include "extremis/cinematics/level22_communication.h"
#include "app/session/shuttle_session.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_messagebox.h>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace app {
bool AppSession::loadInitialLevel(const LaunchOptions& options)
{
    if (options.labPath) {
        try {
            data = game_lab::loadLevel(std::filesystem::u8path(options.labPath),(options.destructionDebug || options.shuttleDebug || options.communicationDebug)?21:0);
            if(options.destructionDebug) data->level.playerStart=game_lab::PlayerStart{0,280,300,360};
            if(options.communicationDebug) {
                data->level.playerStart=communication::returnPosition;
                data->level.playerStart->angle=540; // Face connection44; DOS return still uses 180.
            }
            if(options.shuttleDebug) {
                data->level.playerStart=shuttle::refusal;
                data->level.playerStart->angle=180; // Debug spawn faces the actual door.
            }
            const auto& level = data->level;
            items.load(level);
#ifdef EXTREMIS_DEBUG_MENU
            if(options.destructionDebug) {
                items.debugCompleteSecurities();
                SDL_Log("SELF-DESTRUCTION DEBUG: level22, real security flags completed, 48000 units. F12 shortens remaining budget to 1600; I/map/Debug/focus pause it.");
            }
#endif
            try {
                interactionRenderer.emplace(interaction::load(std::filesystem::u8path(options.labPath).parent_path()/"DATA"));
            } catch (const std::exception& error) {
                items.setAvailable(false);
                std::cerr << "Original interaction resources unavailable: " << error.what() << '\n';
            }
            try {
                originalHud.emplace(hud::load(std::filesystem::u8path(options.labPath).parent_path() / "DATA",
                                              level.palBank));
                std::cout << "Player vitals: original initial health=1000, oxygen=288000; "
                             "160 nominal ticks/second during active 3D simulation.\n";
            } catch (const std::exception& error) {
                std::cerr << "HUD unavailable: " << error.what() << '\n';
            }
            items.setAvailable(interactionRenderer.has_value() && originalHud.has_value());
            try {
                originalMap.emplace(original_map::load(std::filesystem::u8path(options.labPath).parent_path()/"DATA"));
            } catch (const std::exception& error) {
                std::cerr << "Original map unavailable: " << error.what() << '\n';
            }
            try {
                weaponFire.loadGraphics(std::filesystem::u8path(options.labPath).parent_path()/"DATA",level.palBank);
                weaponGraphicsReady=true;
            } catch (const std::exception& error) {
                std::cerr<<"Weapon graphics unavailable; firing disabled: "<<error.what()<<'\n';
            }
            collisionWorld = collision::staticWorld(level);
            levelDoors = doors::closedDoors(level);
            std::cout << "Verified doors (initially closed): " << levelDoors.size() << '\n';
            texturePreview.load(std::filesystem::u8path(options.labPath).parent_path() / "DATA",
                                level.dalleBank, level.palBank);
            planeAppearance.load(std::filesystem::u8path(options.labPath).parent_path() / "DATA",
                                 level.dalleBank, level.palBank);
            try {
                spriteScene = sprites::loadScene(level, std::filesystem::u8path(options.labPath).parent_path() / "DATA");
                std::cout << "OBJET sprites: " << spriteScene->bank.images.size()
                          << " images, " << spriteScene->objects.size() << " instances, "
                          << spriteScene->skipped << " unsupported records.\n";
            } catch (const std::exception& error) {
                std::cerr << "Sprites unavailable: " << error.what() << '\n';
            }
            try {
                enemies::System enemies;
                enemies.load(level, std::filesystem::u8path(options.labPath).parent_path() / "DATA");
                enemySystem = std::move(enemies);
                std::cout << "MONS1: " << enemySystem->records().size() << " original enemies.\n";
                SDL_Log("MONS load: bank=%u records=%zu images=%zu instances=%zu",
                    unsigned(level.monsBank),enemySystem->records().size(),
                    enemySystem->scene().bank.images.size(),enemySystem->scene().objects.size());
            } catch (const std::exception& error) {
                std::cerr << "Enemies unavailable: " << error.what() << '\n';
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"MONS load failed: %s",error.what());
                const std::string message="Enemy resources could not be loaded:\n"+std::string(error.what());
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Extremis RE: enemies unavailable",
                    message.c_str(),nullptr);
                return false;
            }
            try {
                camera = initialCamera(level);
                std::cout << "Initial camera: x=" << camera->x << " y=" << camera->y
                          << " z=128 angle=" << camera->angle
                          << " rad (0 = +X; positive = towards +Y)\n";
            } catch (const std::exception& error) {
                std::cerr << "3D view unavailable: " << error.what() << '\n';
            }
            std::size_t walls = 0, entities = 0, polygons = 0, connections = 0;
            for (const auto& sector : level.sectors) {
                walls += sector.walls.size();
                entities += sector.entitiesA.size();
                polygons += sector.polygons.size();
                for (const auto& polygon : sector.polygons) {
                    connections += polygon.connections.size();
                }
            }
            std::cout << "GAME.LAB: " << data->header.levelCount << " levels\n"
                      << "Level 1 banks: PAL=" << level.palBank
                      << " DALLE=" << level.dalleBank << " OBJET=" << level.objetBank
                      << " MONS=" << level.monsBank << '\n'
                      << "Sectors: " << level.sectors.size() << " | Walls: " << walls
                      << " | Entities A: " << entities << '\n'
                      << "Polygons B: " << polygons << " | Connections: " << connections << '\n'
                      << "M / physical semicolon: toggle original map after plan transfer; Space: close map.\n"
                      << "Left/Right: rotate in 3D.\n"
                      << "WASD: move/navigate; Shift: run; mouse: look; left button: fire equipped weapon.\n"
                      << "E: interact/confirm; I: toggle inventory; Z: heal; X: oxygen; Escape: port menu; Backspace: close interface.\n"
                      << "F: toggle floor and ceiling textures together.\n"
                      << "Entity sectors use zero-based indices.\n";
            for (std::size_t i = 0; i < level.sectors.size(); ++i) {
                const auto& sector = level.sectors[i];
                for (const auto& entity : sector.entitiesA) {
                    const auto& vertex = sector.vertices[entity.vertex];
                    std::cout << "  sector=" << i << " type=0x" << std::hex
                              << std::setw(4) << std::setfill('0') << entity.type
                              << std::dec << std::setfill(' ')
                              << " x=" << (std::uint32_t(sector.gx) + vertex.x)
                              << " y=" << (std::uint32_t(sector.gy) + vertex.y)
                              << " param1=" << entity.param1
                              << " param2=" << entity.param2 << '\n';
                }
            }
            std::cout << std::flush;
        } catch (const std::exception& error) {
            std::cerr << "GAME.LAB load failed: " << error.what() << '\n';
            return false;
        }
    }
    return true;
}
}
