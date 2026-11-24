#pragma once

#include "extremis/gameplay/player/player_vitals.h"
#include <array>
#include <cstdint>

namespace player {
enum class BatteryMode { Off, Radar, Optics };
enum class Transfer { Taken, Exchanged, Empty, Capacity, Unsupported };

// Original fixed slots, not a generic bag. Empty containers remain occupied.
class Inventory {
public:
    struct Loadout {
        std::array<int,3> cards;
        std::array<int,2> injectors;
        int mine, oxygenReserve, battery;
        unsigned weapon;
        int ammunition;
    };
    void startWith(const Loadout& loadout);
    Transfer takeSupply(std::uint16_t& contents, bool exchangeChargedBattery = false);
    Transfer exchangeWeapon(std::uint16_t& panelContents);
    Transfer replaceCard(std::uint16_t& panelContents, unsigned slot);
    bool useInjector(Vitals& vitals);
    bool useInjector(Vitals& vitals, unsigned slot);
    bool useOxygen(Vitals& vitals);
    const std::array<int,3>& cards() const;
    const std::array<int,2>& injectors() const;
    int oxygenReserve() const;
    int mine() const;
    bool useMine();
    unsigned weapon() const;
    int ammunition() const;
    bool consumeShot();
    int batteryCharge() const;
    BatteryMode batteryMode() const;
    bool detectorEnabled() const;
    bool setBatteryMode(BatteryMode mode);
    bool toggleDetector();
    void updateBattery(std::uint16_t gameplayTicks);
private:
    Transfer takeInjector(std::uint16_t& contents);
    Transfer takeCard(std::uint16_t& contents);
    std::array<int,3> cards_{-1,-1,-1};
    std::array<int,2> injectors_{-1,-1};
    int oxygenReserve_ = -1;
    int mine_ = -1;
    int battery_ = -1; // Original new-game template0x1d8be DWORD -1.
    BatteryMode batteryMode_ = BatteryMode::Off;
    unsigned weapon_ = 0;
    int ammunition_ = 500; // Original new-game template.
};
} // namespace player
