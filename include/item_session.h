#pragma once
#include "level_items.h"
#include "original_interactions.h"
#include <cstdint>
#include <array>
#include <SDL3/SDL_scancode.h>

union SDL_Event;
class SurvivalSession;
class GameAudio;
class DoorControl;

class ItemSession {
public:
    void load(const game_lab::Level& level);
    // Returns true when the modal interface owns this event.
    bool handleEvent(const SDL_Event& event, std::uint32_t windowId);
    void update(const Camera* camera, bool active, const collision::World& obstacles,
                SurvivalSession& player, GameAudio& audio, DoorControl* security = nullptr);
    bool modal() const;
    bool planTransferred() const;
    const interaction::State* interfaceState() const;
    void setAvailable(bool available);
private:
    enum class Action { None, Left, Right, Up, Down, Confirm, Cancel };
    std::optional<items::World> world_;
    std::optional<std::size_t> panel_;
    std::optional<interaction::State> interface_;
    std::optional<std::uint16_t> accessCode_;
    std::vector<std::uint16_t> codeTargets_;
    bool authorized_ = false, available_ = true;
    bool planTransferred_ = false;
    bool take_ = false, injector_ = false, oxygen_ = false;
    bool inventoryRequested_ = false;
    Action action_ = Action::None;
    unsigned blink_ = 0;
    std::array<bool, SDL_SCANCODE_COUNT> keysDown_{};
    void open(std::size_t index, SurvivalSession& player);
    void showContents(const SurvivalSession& player);
    void navigate(Action action);
    void confirm(SurvivalSession& player, GameAudio& audio, DoorControl* security);
    void confirmCode(SurvivalSession& player);
    void confirmCard(SurvivalSession& player, GameAudio& audio, DoorControl* security);
    void showCode();
    void refreshInventory(const SurvivalSession& player);
    void confirmInventory(SurvivalSession& player);
};
