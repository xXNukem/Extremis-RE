#pragma once

#include "extremis/gameplay/player/player_inventory.h"
#include "extremis/resources/game_lab.h"

#include <bitset>
#include <string_view>

namespace continuation {

struct Checkpoint {
    std::string_view code;
    unsigned zone;
    game_lab::PlayerStart start;
    unsigned floor;
    player::Inventory::Loadout inventory;
    int health, oxygen;
};

const Checkpoint* find(std::string_view code);
char normalize(char character);

class Disclosures {
public:
    void consume(const Checkpoint& checkpoint);
    const Checkpoint* arrive(unsigned zone, unsigned sector);
private:
    std::bitset<30> consumed_;
};

} // namespace continuation
