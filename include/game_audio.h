#pragma once

#include "audio_player.h"
#include "door.h"
#include "footstep_cadence.h"

class GameAudio {
public:
    enum class DeathSound { Health, Oxygen };
    void load(const std::filesystem::path& directory);
    void update(const std::vector<doors::Door>& doors, double effectiveDistance = 0);
    void beginDeath(DeathSound cause);
    void endDeath();
    void confirmItemSelection();
    void interfaceKey();
    void acceptCard();
    void interfaceMotor();
    void fireWeapon(unsigned weapon);
    void emptyWeapon();
    void enemyEffect(unsigned bankId);
    std::optional<audio::PlaybackStats> doorStatistics();
    std::optional<audio::PlaybackStats> effectStatistics(std::size_t id);
private:
    audio::Player player_;
    bool doorRequested_ = false;
    std::uint64_t reportedCompletions_ = 0;
    audio::FootstepCadence footsteps_;
    std::optional<DeathSound> death_;
    void updateDoors(const std::vector<doors::Door>& doors);
    void updateFootsteps(double effectiveDistance);
};
