#pragma once
#include "app/session/item_session.h"
#include <filesystem>
#include <optional>

struct SDL_Window;
struct SDL_Renderer;
enum class StartupCreditsResult;
namespace player { class Inventory; }

namespace shuttle {
// Pure campaign decisions; presentation never owns inventory or countdown state.
enum class Step { Boarding, Start, MissingCard, Securities, Refused, Flight, Arrived };
ItemSession::Travel arrival();
class Session {
public:
    Step step() const;
    void answer(bool yes,const player::Inventory& inventory,const ItemSession& items);
    std::optional<ItemSession::Travel> complete(StartupCreditsResult result,ItemSession& items);
private:
    Step step_=Step::Boarding;
};
unsigned doorFade(const game_lab::Level& level,unsigned sector);
bool trigger(game_lab::Level& level,unsigned sector);
inline constexpr game_lab::PlayerStart refusal{38,300,900,0};
inline constexpr unsigned arrivalFloor=8;
// Continue + Flight means the caller can play the already validated movie.
StartupCreditsResult prompt(SDL_Window*,SDL_Renderer*,const std::filesystem::path&,
                            Session&,const player::Inventory&,const ItemSession&);
}
