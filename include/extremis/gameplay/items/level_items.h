#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "extremis/gameplay/player/camera.h"
#include "extremis/gameplay/player/player_inventory.h"
#include "extremis/world/player_collision.h"

namespace items {
struct Panel {
    std::size_t sector, wall;
    collision::Segment face;
    bool weapon = false;
    std::uint16_t contents = 0xffff;
    bool cardReader = false;
    bool codePanel = false;
    bool codeReader = false;
    bool elevator = false;
    bool securityPanel = false; // Logical type 4: N-card security, not a door.
};

struct Collection {
    std::optional<std::size_t> panel;
    player::Transfer transfer = player::Transfer::Unsupported;
};

class World {
public:
    explicit World(const game_lab::Level& level);
    const std::vector<Panel>& panels() const;
    void storePanelContents(game_lab::Level& level) const;
    Collection collect(const Camera& camera, bool alive, player::Inventory& inventory,
                       const collision::World& obstacles);
    std::optional<std::size_t> target(const Camera& camera, const collision::World& obstacles) const;
    player::Transfer take(std::size_t index, player::Inventory& inventory, bool exchangeChargedBattery = false);
    player::Transfer replaceCard(std::size_t index, player::Inventory& inventory, unsigned slot);
private:
    std::optional<double> reachable(const Panel& panel, const Camera& camera,
                                    const collision::World& obstacles) const;
    std::vector<Panel> panels_;
};
} // namespace items
