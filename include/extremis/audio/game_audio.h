#pragma once

#include "extremis/audio/playback_stats.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace audio { class Player; }
namespace doors { struct Door; }
namespace game_lab { struct Level; }

class GameAudio {
public:
    GameAudio();
    ~GameAudio();
    GameAudio(const GameAudio&) = delete;
    GameAudio& operator=(const GameAudio&) = delete;

    void stopEffects();
    void pause(bool paused);
    bool enabled() const;
    enum class DeathSound { Health, Oxygen, SelfDestruction };
    void destructionAlarm(bool requested);
    void load(const std::filesystem::path& directory);
    void update(const game_lab::Level& level, const std::vector<doors::Door>& doors,
                unsigned footsteps = 0, bool active = true);
    void beginDeath(DeathSound cause);
    void endDeath();
    void confirmItemSelection();
    void interfaceKey();
    void acceptCard();
    void interfaceMotor();
    void elevatorMotor(); // BANK[11], native loop until stopEffects().
    void pickupTransfer();
    void insertCard();
    void interfaceShutters();
    void fireWeapon(unsigned weapon);
    void emptyWeapon();
    void enemyEffect(unsigned bankId);
    std::optional<audio::PlaybackStats> doorStatistics();
    std::optional<audio::PlaybackStats> effectStatistics(std::size_t id);
private:
    std::unique_ptr<audio::Player> player_;
    bool doorRequested_ = false, destructionAlarmRequested_ = false;
    std::uint64_t reportedCompletions_ = 0;
    bool enabled_ = false;
    std::optional<DeathSound> death_;
    void updateDoors(const game_lab::Level& level, const std::vector<doors::Door>& doors, bool active);
    void updateFootsteps(unsigned footsteps);
};
