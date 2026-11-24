#include "extremis/gameplay/player/player_inventory.h"
#include <algorithm>

namespace player {
void Inventory::startWith(const Loadout& loadout)
{
    cards_ = loadout.cards;
    injectors_ = loadout.injectors;
    mine_ = loadout.mine;
    oxygenReserve_ = loadout.oxygenReserve;
    battery_ = loadout.battery;
    weapon_ = loadout.weapon;
    ammunition_ = loadout.ammunition;
    batteryMode_ = BatteryMode::Off;
}

Transfer Inventory::replaceCard(std::uint16_t& contents, unsigned slot)
{
    const auto kind=contents>>12;
    if (slot>=cards_.size() || cards_[slot]<0 || kind<3 || kind>5) return Transfer::Unsupported;
    const auto previous=cards_[slot];
    cards_[slot]=int(contents)-0x3000;
    contents=std::uint16_t(previous+0x3000);
    return Transfer::Exchanged;
}
Transfer Inventory::takeSupply(std::uint16_t& contents, bool exchangeChargedBattery)
{
    if (contents == 0xffff) return Transfer::Empty;
    const unsigned kind = contents >> 12, amount = contents & 0xfff;
    if (kind==0) {
        if (battery_>0 && !exchangeChargedBattery) return Transfer::Capacity;
        const int previous=battery_;
        battery_=int(amount)*Vitals::ticksPerSecond;
        contents=previous<0 ? 0xffff : std::uint16_t(previous/Vitals::ticksPerSecond);
        return previous<0?Transfer::Taken:Transfer::Exchanged;
    }
    // EXE 0x191a8..0x191d2: one mine slot, initialized to 100 on pickup.
    if (kind == 6) {
        if (mine_>=0) return Transfer::Capacity;
        mine_=100; contents=0xffff; return Transfer::Taken;
    }
    if (kind == 2) return takeInjector(contents);
    if (kind >= 3 && kind <= 5) return takeCard(contents);
    if (kind != 1 || amount > 1800) return Transfer::Unsupported;
    if (oxygenReserve_ > 0) return Transfer::Capacity; // Original replacement choice pending.
    const int previous = oxygenReserve_;
    oxygenReserve_ = int(amount) * Vitals::ticksPerSecond;
    contents = previous < 0 ? 0xffff : 0x1000;
    return previous < 0 ? Transfer::Taken : Transfer::Exchanged;
}

Transfer Inventory::takeInjector(std::uint16_t& contents)
{
    for (auto& slot : injectors_) {
        if (slot > 0) continue;
        const int previous = slot;
        slot = contents & 0xfff;
        contents = previous < 0 ? 0xffff : 0x2000;
        return previous < 0 ? Transfer::Taken : Transfer::Exchanged;
    }
    return Transfer::Capacity;
}

Transfer Inventory::takeCard(std::uint16_t& contents)
{
    for (auto& slot : cards_) {
        if (slot >= 0) continue;
        slot = int(contents)-0x3000; // (kind-3)<<12 | card identifier.
        contents = 0xffff;
        return Transfer::Taken;
    }
    return Transfer::Capacity;
}

Transfer Inventory::exchangeWeapon(std::uint16_t& contents)
{
    if (contents == 0xffff) return Transfer::Empty;
    const unsigned next = contents >> 12;
    if (next > 5) return Transfer::Unsupported;
    const unsigned ammunition = contents & 0xfff;
    contents = std::uint16_t((weapon_<<12) | unsigned(ammunition_));
    weapon_ = next;
    ammunition_ = int(ammunition);
    return Transfer::Exchanged;
}

bool Inventory::useInjector(Vitals& vitals)
{
    for (unsigned slot=0;slot<injectors_.size();++slot)
        if (useInjector(vitals,slot)) return true;
    return false;
}
bool Inventory::useInjector(Vitals& vitals, unsigned slot)
{
    if (!vitals.alive() || slot>=injectors_.size() || injectors_[slot]<=0) return false;
    vitals.restoreHealth(); injectors_[slot]=0; return true;
}

bool Inventory::useOxygen(Vitals& vitals)
{
    // EXE 0x1bd21..0x1bd36 excludes both absent and empty bottles from use.
    return oxygenReserve_ > 0 && vitals.exchangeOxygen(oxygenReserve_);
}

const std::array<int,3>& Inventory::cards() const { return cards_; }
const std::array<int,2>& Inventory::injectors() const { return injectors_; }
int Inventory::oxygenReserve() const { return oxygenReserve_; }
unsigned Inventory::weapon() const { return weapon_; }
int Inventory::ammunition() const { return ammunition_; }
bool Inventory::consumeShot()
{
    if (weapon_>=6 || ammunition_<=0) return false;
    --ammunition_; return true;
}
} // namespace player

namespace player {
bool Inventory::setBatteryMode(BatteryMode mode)
{
    if (mode!=BatteryMode::Off && battery_<=0) return false;
    batteryMode_=mode;
    return true;
}

bool Inventory::toggleDetector()
{
    if (battery_<=0) return false;
    return setBatteryMode(detectorEnabled() ? BatteryMode::Off : BatteryMode::Radar);
}

void Inventory::updateBattery(std::uint16_t gameplayTicks)
{
    if (batteryMode_==BatteryMode::Off) return;
    battery_=std::max(0,battery_-int(gameplayTicks));
    if (!battery_) batteryMode_=BatteryMode::Off;
}
}

namespace player {
bool Inventory::useMine()
{
    if (mine_<0) return false;
    mine_=-1;
    return true;
}

int Inventory::mine() const
{
    return mine_;
}

int Inventory::batteryCharge() const
{
    return battery_;
}

BatteryMode Inventory::batteryMode() const
{
    return batteryMode_;
}
bool Inventory::detectorEnabled() const
{
    return batteryMode_==BatteryMode::Radar;
}
} // namespace player
