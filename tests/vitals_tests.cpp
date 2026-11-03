#include "player_vitals.h"

#include <climits>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

void initialAndDamage()
{
    player::Vitals state;
    check(state.health() == 1000 && state.oxygen() == 288000 && state.alive(),
          "New-game template differs from EXE");
    state.applyDamage(2); // Subtraction in original 0x1663f..0x16657.
    check(state.health() == 998 && state.oxygen() == 288000,"Damage changed wrong resource");
    state.applyDamage(-1); state.applyDamage(INT_MIN); state.applyDamage(0);
    check(state.health() == 998,"Invalid damage healed player or overflowed");
    state.restoreHealth();
    check(state.health() == 1000,"Injector did not restore full health");
    state.applyDamage(INT_MAX);
    check(state.health() == 0 && !state.alive() &&
          state.deathCause() == player::DeathCause::Health,"Lethal damage not latched");
    state.update(100,true); state.restoreHealth();
    int reserve = 288000;
    check(!state.exchangeOxygen(reserve) && state.oxygen() == 288000 && state.health() == 0,
          "Dead player updated, recovered or consumed a reserve");
    state.reset();
    check(state.health() == 1000 && state.oxygen() == 288000 && state.alive(),"Reset retained death state");
}

void clockAndPause()
{
    for (int fps : {1,18,30,60,144,240}) {
        player::Vitals state;
        for (int i = 0; i < 60*fps; ++i) state.update(1.0/fps,true);
        check(state.oxygen() == 288000-9600 && state.health() == 1000,
              "Nominal 160 Hz consumption depends on rendering FPS");
    }
    player::Vitals state;
    state.update(0.5/160,true);
    state.update(1800,false);
    check(state.oxygen() == 288000,"Paused simulation consumed oxygen");
    state.update(0.5/160,true);
    check(state.oxygen() == 287999,"Pause lost fractional active tick");
    for (double invalid : {0., -1., std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) state.update(invalid,true);
    check(state.oxygen() == 287999,"Invalid time consumed oxygen");
    player::Vitals increased;
    increased.update(60,true,true);
    check(increased.oxygen() == 288000-19200,"Original modifier did not double use");
}

void depletionAndExchange()
{
    player::Vitals state;
    state.update(60,true);
    int bottle = 144000;
    check(state.exchangeOxygen(bottle) && state.oxygen() == 144000 && bottle == 278400,
          "Bottle was added instead of exchanged");
    bottle = -1;
    check(!state.exchangeOxygen(bottle) && state.oxygen() == 144000,"Absent reserve accepted");
    bottle = INT_MAX;
    check(state.exchangeOxygen(bottle) && state.oxygen() == 288000 && bottle == 144000,
          "Oxygen maximum not bounded");
    state.update(1799,true);
    check(state.alive() && state.oxygen() == 160,"Oxygen reached zero early");
    state.update(1,true);
    check(!state.alive() && state.oxygen() == 0 && state.health() == 1000 &&
          state.deathCause() == player::DeathCause::Oxygen,"Oxygen death altered health/cause");
    state.update(1e300,true); state.applyDamage(1000);
    check(state.health() == 1000 && state.deathCause() == player::DeathCause::Oxygen,
          "Death cause/state changed after death");
    state.reset();
    state.update(1e300,true);
    check(state.oxygen() == 0,"Long active interval overflowed");
    state.reset();
    bottle = 0;
    state.exchangeOxygen(bottle);
    check(!state.alive() && bottle == 288000,"Empty bottle exchange failed to reach zero");
}
} // namespace

int main()
{
    try {
        initialAndDamage(); clockAndPause(); depletionAndExchange();
        std::cout << "Original vitals template, clock, damage, recovery and death passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
