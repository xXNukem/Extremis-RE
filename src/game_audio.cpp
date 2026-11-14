#include "game_audio.h"
#include <algorithm>
#include <iostream>

void GameAudio::load(const std::filesystem::path& directory)
{
    doorRequested_ = false;
    reportedCompletions_ = 0;
    enabled_ = false;
    death_.reset();
    std::string error;
    if (!player_.open(directory / "BANK.DSS",error)) {
        std::cerr << "Audio unavailable: " << error << ". Continuing without sound.\n";
        return;
    }
    enabled_ = true;
    std::cout << "Audio: " << player_.effectCount() << " original effects; 5000 Hz mono, "
                 "4 voices. Door movement: BANK[27]; footsteps: BANK[7].\n";
}

void GameAudio::beginDeath(DeathSound cause)
{
    if (death_) return;
    death_ = cause;
    // Original direct death branches: AX=0 -> BANK[25], AX=1 -> BANK[26].
    // Native DSS loop semantics are retained; no forced repeat or invented timer.
    player_.play(cause == DeathSound::Oxygen ? 26 : 25);
}

void GameAudio::endDeath()
{
    if (!death_) return;
    player_.stop(*death_ == DeathSound::Oxygen ? 26 : 25);
    death_.reset();
}

void GameAudio::confirmItemSelection()
{
    // Original cabinet confirmation: 0x18f97..0x18f9f, AX=12, voice 1.
    player_.play(12);
}
void GameAudio::pickupTransfer() { player_.play(26,false,audio::Playback::Once); }
void GameAudio::insertCard() { player_.play(18,false,audio::Playback::Once); }
void GameAudio::interfaceShutters() { player_.play(27,false,audio::Playback::Once); }
void GameAudio::interfaceKey() { player_.play(28); }
void GameAudio::acceptCard() { player_.play(19); }
void GameAudio::interfaceMotor()
{
    // Inventory zoom: 0x1baa7 / 0x1c038; elevator panel: 0xcfcb / 0xd539.
    // DOS stops voice 0 after each zoom (0xd03a -> 0x13090 -> 0x464a).
    // Stable views omit that animation: play its complete 3853-frame payload once.
    player_.play(11,false,audio::Playback::Once);
}

void GameAudio::update(const game_lab::Level& level, const std::vector<doors::Door>& doors, unsigned footsteps, bool active)
{
    updateDoors(level,doors,active);
    updateFootsteps(footsteps);
}

void GameAudio::updateFootsteps(unsigned footsteps)
{
    if (footsteps != 0 && !player_.playing(7)) {
        // Do not queue a burst after a long frame or overlap an existing step.
        // Keep the original bank volume (16/64) and play the complete one-shot.
        player_.play(7);
    }
}

void GameAudio::updateDoors(const game_lab::Level& level, const std::vector<doors::Door>& doors, bool active)
{
    const bool moving = active && std::any_of(doors.begin(),doors.end(),[&](const auto& door) {
        if (door.stateIndex>=level.connectionStates.size()) return false;
        const auto phase=doors::dos::phase(level.connectionStates[door.stateIndex]);
        return phase>0 && phase<60;
    });
    // One full effect per movement episode. Keep the latch until movement ends,
    // even when the shorter audio has already finished; never force a repeat or
    // truncate its tail when the door reaches an endpoint.
    if (moving && !doorRequested_) {
        doorRequested_ = player_.play(27);
        if (doorRequested_) {
            const auto stats = player_.statistics(27);
            if (stats) std::cout << "Door audio ID=27 request=" << stats->requests
                                 << " decodedFrames=" << stats->decodedFrames << " repeat=off\n";
        }
    }
    if (!moving) doorRequested_ = false;
    const auto stats = player_.statistics(27);
    if (stats && stats->completed != reportedCompletions_) {
        reportedCompletions_ = stats->completed;
        std::cout << "Door audio ID=27 totals: started=" << stats->started
                  << " completed=" << stats->completed << " sourceFrames=" << stats->sourceFrames
                  << " wraps=" << stats->wraps << '\n';
    }
}

std::optional<audio::PlaybackStats> GameAudio::doorStatistics()
{
    return effectStatistics(27);
}

std::optional<audio::PlaybackStats> GameAudio::effectStatistics(std::size_t id)
{
    return player_.statistics(id);
}

void GameAudio::fireWeapon(unsigned weapon)
{
    if (weapon<6) player_.play(weapon+1); // EXE 0x17ad8..0x17ae0.
}
void GameAudio::emptyWeapon()
{
    // Original queued IDs are deduplicated; do not stack empty clicks every frame.
    if (!player_.playing(0)) player_.play(0);
}

void GameAudio::enemyEffect(unsigned bankId)
{
    if (bankId==9 || bankId==10 || bankId==16 || bankId==17)
        player_.play(bankId,false,audio::Playback::Once);
}
