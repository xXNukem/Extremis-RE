#include "player_vitals.h"

#include <algorithm>
#include <cmath>

namespace player {
int Vitals::health() const { return health_; }
int Vitals::oxygen() const { return oxygen_; }
DeathCause Vitals::deathCause() const { return death_; }
bool Vitals::alive() const { return death_ == DeathCause::None; }

void Vitals::reset()
{
    health_ = maximumHealth;
    oxygen_ = maximumOxygen;
    fractionalTicks_ = 0;
    death_ = DeathCause::None;
}

void Vitals::update(double seconds, bool simulationActive, bool increasedOxygenUse)
{
    if (!alive() || !simulationActive || !std::isfinite(seconds) || seconds <= 0) return;
    const int multiplier = increasedOxygenUse ? 2 : 1;
    const int ticksToDeath = (oxygen_ + multiplier - 1) / multiplier;
    // Bound before multiplication, including unusually long suspended frames.
    if (seconds >= double(ticksToDeath) / ticksPerSecond) {
        oxygen_ = 0;
        death_ = DeathCause::Oxygen;
        fractionalTicks_ = 0;
        return;
    }
    const double total = fractionalTicks_ + seconds * ticksPerSecond;
    const int ticks = int(std::floor(total + 1e-9));
    fractionalTicks_ = std::max(0.0,total-ticks);
    oxygen_ = std::max(0,oxygen_-ticks*multiplier);
    if (!oxygen_) death_ = DeathCause::Oxygen;
}

void Vitals::applyDamage(int internalUnits)
{
    if (!alive() || internalUnits <= 0) return;
    health_ -= std::min(health_,internalUnits);
    if (!health_) death_ = DeathCause::Health;
}

void Vitals::restoreHealth()
{
    if (alive()) health_ = maximumHealth;
}

bool Vitals::exchangeOxygen(int& reserve)
{
    if (!alive() || reserve < 0) return false;
    const int previous = oxygen_;
    oxygen_ = std::min(reserve,maximumOxygen);
    reserve = previous;
    if (!oxygen_) death_ = DeathCause::Oxygen;
    return true;
}
} // namespace player
