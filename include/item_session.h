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
    struct Travel { unsigned zone; game_lab::PlayerStart arrival; };
    std::optional<Travel> takeTravel();
    void load(const game_lab::Level& level);
    // Returns true when the modal interface owns this event.
    bool handleEvent(const SDL_Event& event, std::uint32_t windowId);
    void update(const Camera* camera, bool active, const collision::World& obstacles,
                SurvivalSession& player, GameAudio& audio, DoorControl* security = nullptr, double seconds = 1.0/70.0);
    bool modal() const;
    bool planTransferred() const;
    bool planTransferred(unsigned zone) const;
    void resetPlans(); // New game, not level switching.
    bool busy() const { return modal() || inventoryRequested_ || take_; }
    void clearInput();
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
    std::array<bool,30> plans_{};
    unsigned zone_ = 0;
    std::optional<Travel> travel_;
    bool take_ = false, injector_ = false, oxygen_ = false;
    bool inventoryRequested_ = false;
    Action action_ = Action::None;
    unsigned blink_ = 0;
    double animationTime_ = 0, planTicks_ = 0;
    bool resumeAnimation_ = false;
    std::optional<std::uint16_t> pendingCard_;
    void startPhase(interaction::Phase phase);
    void closePanel(GameAudio& audio);
    bool animate(double seconds, SurvivalSession& player, GameAudio& audio, DoorControl* security);
    void finishPhase(SurvivalSession& player, GameAudio& audio, DoorControl* security);
    void usePendingCard(SurvivalSession& player, GameAudio& audio, DoorControl* security);
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
