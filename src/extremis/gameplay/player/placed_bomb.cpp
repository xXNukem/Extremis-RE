#include <cstdint>
#include "extremis/gameplay/player/placed_bomb.h"

#include <algorithm>

namespace player {
void PlacedBomb::place(std::uint16_t sector, std::uint16_t localX, std::uint16_t localY)
{
    sector_ = sector;
    x_ = localX;
    y_ = localY;
    phase_ = Phase::Armed;
    elapsed_ = 0;
}

bool PlacedBomb::armed() const { return phase_ == Phase::Armed; }
std::uint16_t PlacedBomb::sector() const { return sector_; }
std::uint16_t PlacedBomb::x() const { return x_; }
std::uint16_t PlacedBomb::y() const { return y_; }

unsigned PlacedBomb::randomWord()
{
    // Host stream for the three shock draws. DOS shares its RNG with other systems.
    randomState_ ^= randomState_ << 13;
    randomState_ ^= randomState_ >> 17;
    randomState_ ^= randomState_ << 5;
    return randomState_ & 65535u;
}

BombFrame PlacedBomb::update(std::uint16_t ticks)
{
    BombFrame frame;
    // The player consumes the previous shock before the bomb updates its state.
    if (shockTimer_) {
        frame.shock = true;
        frame.shakeX = int(randomWord() & 31) - 16;
        frame.shakeY = int(randomWord() & 31) - 16;
        frame.shakeHeight = int(randomWord() & 47) - 32;
        shockTimer_ -= ticks;
        if (shockTimer_ < -320) shockTimer_ = 0;
    }
    // DOS reuses the current frame delta on transition into each next phase.
    if (phase_ == Phase::Armed) {
        frame.alarm = true;
        elapsed_ += ticks;
        if (elapsed_ < 800) {
            frame.image = 0;
            frame.size = 50;
            return frame;
        }
        elapsed_ = 0;
        phase_ = Phase::Explosion;
        frame.explosionSound = true;
    }
    if (phase_ == Phase::Explosion) {
        elapsed_ += ticks;
        if (elapsed_ < 100) {
            frame.image = (elapsed_ & 4) ? 2 : 1;
            frame.size = 50 + 4 * elapsed_;
            frame.white = std::min(100u, 30 + elapsed_);
            if (shockTimer_ == 0) shockTimer_ = -1;
            return frame;
        }
        elapsed_ = 0;
        phase_ = Phase::Afterglow;
        frame.damage = true;
    }
    if (phase_ == Phase::Afterglow) {
        elapsed_ += ticks;
        if (elapsed_ < 160) frame.white = std::min(100u, 160 - elapsed_);
        else phase_ = Phase::Idle;
    }
    return frame;
}
}
