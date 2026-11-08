#pragma once
#ifdef EXTREMIS_DEBUG_MENU
#include "item_session.h"
#include <SDL3/SDL.h>
#include <array>
#include <string>
class DebugMenu {
public:
    void load(const std::filesystem::path& lab);
    bool event(const SDL_Event& event, player::Inventory& inventory, unsigned zone);
    bool draw(SDL_Renderer* renderer, unsigned zone);
    bool open() const { return open_; }
    bool enemiesEnabled() const { return enemies_; }
    std::optional<ItemSession::Travel> takeTravel();
private:
    friend struct DebugMenuChecks;
    struct Entry { std::string label; std::uint16_t contents=0; std::optional<ItemSession::Travel> travel; };
    std::array<std::vector<Entry>,6> entries_;
    std::vector<game_lab::Level> levels_;
    void refresh(unsigned zone);
    bool open_=false, enemies_=true;
    unsigned category_=0, row_=0, zone_=999, slot_=0;
    std::optional<std::uint16_t> replacement_;
    std::optional<ItemSession::Travel> travel_;
    std::string notice_;
    std::array<int,3> cards_{};
};
#endif
