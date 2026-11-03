#pragma once

namespace player {
enum class DeathCause { None, Health, Oxygen };

class Vitals {
public:
    static constexpr int maximumHealth = 1000;
    static constexpr int maximumOxygen = 288000;
    static constexpr int ticksPerSecond = 160; // Nominal PIT clock, divisor 7457.

    int health() const;
    int oxygen() const;
    DeathCause deathCause() const;
    bool alive() const;
    void reset(); // Verified new-game template; not a save/load implementation.
    void update(double seconds, bool simulationActive, bool increasedOxygenUse = false);
    void applyDamage(int internalUnits); // Caller supplies damage; no invented attack formula.
    void restoreHealth(); // Original injector operation restores to full.
    bool exchangeOxygen(int& reserve); // Original bottle operation swaps, rather than adds.
private:
    int health_ = maximumHealth;
    int oxygen_ = maximumOxygen;
    double fractionalTicks_ = 0;
    DeathCause death_ = DeathCause::None;
};
} // namespace player
