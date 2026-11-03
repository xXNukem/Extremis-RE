#pragma once

#include "player_vitals.h"
#include <array>
#include <cstdint>

namespace player {
enum class Transfer { Taken, Exchanged, Empty, Capacity, Unsupported };

// Original fixed slots, not a generic bag. Empty containers remain occupied.
class Inventory {
public:
    Transfer takeSupply(std::uint16_t& panelContents);
    Transfer exchangeWeapon(std::uint16_t& panelContents);
    Transfer replaceCard(std::uint16_t& panelContents, unsigned slot);
    bool useInjector(Vitals& vitals);
    bool useInjector(Vitals& vitals, unsigned slot);
    bool useOxygen(Vitals& vitals);
    const std::array<int,3>& cards() const;
    const std::array<int,2>& injectors() const;
    int oxygenReserve() const;
    unsigned weapon() const;
    int ammunition() const;
    bool consumeShot();
    int batteryCharge() const { return battery_; }
    bool detectorEnabled() const { return detector_; }
    bool toggleDetector();
    void updateBattery(double seconds, bool active);
private:
    Transfer takeInjector(std::uint16_t& contents);
    Transfer takeCard(std::uint16_t& contents);
    std::array<int,3> cards_{-1,-1,-1};
    std::array<int,2> injectors_{-1,-1};
    int oxygenReserve_ = -1;
    int battery_ = -1; // Original new-game template0x1d8be DWORD -1.
    bool detector_ = false;
    double batteryFraction_ = 0;
    unsigned weapon_ = 0;
    int ammunition_ = 500; // Original new-game template.
};
} // namespace player
