#pragma once

#include <cstdint>

namespace player {
struct BombFrame {
    bool alarm = false;
    bool explosionSound = false;
    bool damage = false;
    bool shock = false;
    int image = -1; // Zero-based BOMBE.SPR index.
    unsigned size = 0;
    unsigned white = 0;
    int shakeX = 0, shakeY = 0, shakeHeight = 0;
};

class PlacedBomb {
public:
    void place(std::uint16_t sector, std::uint16_t localX, std::uint16_t localY);
    BombFrame update(std::uint16_t ticks);
    bool armed() const;
    std::uint16_t sector() const;
    std::uint16_t x() const;
    std::uint16_t y() const;
private:
    enum class Phase { Idle, Armed, Explosion, Afterglow };
    Phase phase_ = Phase::Idle;
    unsigned elapsed_ = 0;
    int shockTimer_ = 0;
    std::uint16_t sector_ = 0, x_ = 0, y_ = 0;
    std::uint32_t randomState_ = 0x5b090d29;
    unsigned randomWord();
};
}
