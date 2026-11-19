#pragma once

#include "../startup/launch_options.h"
#include "extremis/gameplay/player/camera.h"
#include "extremis/render/first_person/sprites/decorative_animation.h"
#include "extremis/world/door.h"
#include "extremis/gameplay/enemies/enemy_system.h"
#include "extremis/resources/game_lab.h"
#include "app/session/item_session.h"
#include "app/session/map_session.h"
#include "extremis/ui/original_hud.h"
#include "extremis/ui/original_interactions.h"
#include "extremis/ui/original_map.h"
#include "extremis/render/first_person/materials/plane_appearance.h"
#include "extremis/world/player_collision.h"
#include "extremis/render/first_person/sprites/sprite_animation.h"
#include "extremis/gameplay/player/survival_session.h"
#include "extremis/render/first_person/materials/texture_preview.h"
#include "extremis/gameplay/player/weapon_fire.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct PortMenuVideoState;
class DebugMenu;
class DoorControl;
class FirstPersonRenderer;
class GameAudio;
class PlayerInput;
class VictorySession;
namespace player { class DeathPresentation; }
namespace platform::sdl::video_profiling { class FrameProfiler; }
enum class StartupCreditsResult;

namespace app {
namespace diagnostics { class GameplayFps; }
class AppSession {
public:
    explicit AppSession(bool vitalsDebug);
    bool loadInitialLevel(const LaunchOptions& options);
    bool hasLevel() const;
    std::string windowTitle() const;
    void run(SDL_Window* window, SDL_Renderer* renderer, const LaunchOptions& options,
             const std::filesystem::path& dataDirectory, PortMenuVideoState& video, int& exitCode);

private:
    struct EventBatch {
        bool running;
        bool restartRequested;
        bool deathKey;
        bool showPolygons;
        enemies::System* activeEnemies;
    };

    EventBatch pollEvents(SDL_Window* window, SDL_Renderer* renderer,
        const LaunchOptions& options, const std::filesystem::path& dataDirectory,
        PortMenuVideoState& video, int& exitCode, std::uint64_t& previousTicks,
        PlayerInput& playerInput, GameAudio& gameAudio, player::DeathPresentation* deathPresentation,
        const VictorySession& victory, bool show3D, EventBatch result
#ifdef EXTREMIS_DEBUG_MENU
        , DebugMenu& debugMenu, platform::sdl::video_profiling::FrameProfiler& profiler,
        diagnostics::GameplayFps& fps
#endif
    );

    void transferLevel(SDL_Window* window, SDL_Renderer* renderer,
        const LaunchOptions& options, const ItemSession::Travel& travel,
        bool restartRequested, bool arrivingByShuttle, bool& running,
        FirstPersonRenderer& firstPerson, DoorControl& doorControl, PlayerInput& playerInput,
        GameAudio& gameAudio, player::DeathPresentation* deathPresentation, VictorySession& victory
#ifdef EXTREMIS_DEBUG_MENU
        , DebugMenu& debugMenu
#endif
    );

    bool presentFrame(SDL_Renderer* renderer, FirstPersonRenderer& firstPerson,
        const hud::State& hudState, const enemies::System* activeEnemies,
        bool show3D, bool showPolygons, bool modalBlocked
#ifdef EXTREMIS_DEBUG_MENU
        , DebugMenu& debugMenu, double gameplayFps, platform::sdl::video_profiling::FrameProfiler& profiler
#endif
    );

    enum class DeathFrame { Inactive, Presented, Error };
    DeathFrame advanceDeathPresentation(SDL_Window* window, SDL_Renderer* renderer,
        FirstPersonRenderer& firstPerson, player::DeathPresentation& deathPresentation,
        GameAudio& gameAudio, PlayerInput& playerInput, DoorControl& doorControl,
        const VictorySession& victory, double elapsedSeconds, bool deathKey);
    DeathFrame beginDeathPresentation(SDL_Renderer* renderer,
        const char* labPath, FirstPersonRenderer& firstPerson, player::DeathPresentation* deathPresentation,
        GameAudio& gameAudio, PlayerInput& playerInput, DoorControl& doorControl);
    StartupCreditsResult playVictory(SDL_Window* window, SDL_Renderer* renderer,
        const char* labPath, FirstPersonRenderer& firstPerson, GameAudio& gameAudio,
        PlayerInput& playerInput, DoorControl& doorControl, enemies::System& activeEnemies,
        player::DeathPresentation* deathPresentation, VictorySession& victory);

    StartupCreditsResult playCommunication(SDL_Window* window, SDL_Renderer* renderer,
        const char* labPath, FirstPersonRenderer& firstPerson, GameAudio& gameAudio,
        PlayerInput& playerInput, DoorControl& doorControl, std::uint64_t& previousTicks);
    StartupCreditsResult playShuttle(SDL_Window* window, SDL_Renderer* renderer,
        const char* labPath, GameAudio& gameAudio, PlayerInput& playerInput,
        DoorControl& doorControl, std::optional<ItemSession::Travel>& shuttleTravel);

    std::optional<game_lab::Level1> data;
    std::optional<Camera> camera;
    TexturePreview texturePreview;
    PlaneAppearance planeAppearance;
    std::optional<sprites::Scene> spriteScene;
    sprites::Animation spriteAnimation;
    sprites::DecorativeAnimation decorativeAnimation;
    std::optional<enemies::System> enemySystem;
    std::optional<hud::Renderer> originalHud;
    std::optional<interaction::Renderer> interactionRenderer;
    SurvivalSession survival;
    ItemSession items;
    original_map::Session mapSession;
    std::optional<original_map::Renderer> originalMap;
    player::WeaponFire weaponFire;
    bool weaponGraphicsReady=false;
    std::vector<doors::Door> levelDoors;
    collision::World collisionWorld;
};
}
