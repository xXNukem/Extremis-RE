#include "extremis/gameplay/player/placed_bomb.h"

#include <iostream>
#include <stdexcept>

namespace {
void check(bool result, const char* message)
{
    if (!result) throw std::runtime_error(message);
}
void countdown()
{
    player::PlacedBomb bomb;
    check(!bomb.armed() && bomb.update(1).image == -1, "Idle bomb is visible");
    bomb.place(12, 300, 500);
    const auto fuse = bomb.update(799);
    check(bomb.armed() && fuse.alarm && fuse.image == 0 && fuse.size == 50 && !fuse.damage,
          "Fuse before tick 800");
    const auto ignition = bomb.update(1);
    check(!bomb.armed() && ignition.alarm && ignition.explosionSound && ignition.image == 1 &&
          ignition.size == 54 && ignition.white == 31 && !ignition.damage && !ignition.shock,
          "Ignition did not reuse this update's delta");
    const auto alternate = bomb.update(3);
    check(!alternate.alarm && !alternate.explosionSound && alternate.image == 2 &&
          alternate.size == 66 && alternate.shock, "Explosion image switch");
    const auto bright = bomb.update(95);
    check(bright.white == 100 && bright.image == 1 && bright.size == 446 && !bright.damage,
          "Last explosion frame");
    const auto blast = bomb.update(1);
    check(blast.damage && blast.white == 100 && blast.image == -1, "One-shot damage boundary");
    const auto fade = bomb.update(158);
    check(!fade.damage && fade.white == 1 && fade.shock, "Last afterglow frame");
    const auto clear = bomb.update(1);
    check(clear.white == 0 && clear.image == -1 && clear.shock, "Afterglow exit");
    check(bomb.update(61).shock && bomb.update(1).shock && !bomb.update(1).shock,
          "Shock must remain through signed timer -320 and stop after crossing");
}
void replacementAndLargeDelta()
{
    player::PlacedBomb bomb;
    bomb.place(3, 55, 20);
    bomb.update(790);
    bomb.place(14, 60, 400);
    check(bomb.sector() == 14 && bomb.x() == 60 && bomb.y() == 400 && bomb.update(10).alarm &&
          bomb.armed(), "Second placement did not replace live fuse and coordinates");
    const auto all = bomb.update(65535);
    check(all.alarm && all.explosionSound && all.damage && !all.white && all.image == -1,
          "A large elapsed interval must cross all three thresholds");
    check(!bomb.update(1).damage, "Idle bomb applied damage twice");
}
}
int main()
{
    try {
        countdown();
        replacementAndLargeDelta();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
