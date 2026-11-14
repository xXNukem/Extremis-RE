#include "player_input.h"
#include "item_session.h"
#include "survival_session.h"
#include "game_audio.h"
#include "map_session.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

SDL_Event keyEvent(SDL_EventType type, SDL_Scancode scancode, bool repeat = false)
{
    SDL_Event event{};
    event.type = type;
    event.key.windowID = 31;
    event.key.scancode = scancode;
    event.key.key = SDLK_SPACE; // Physical E must work independently of logical key mapping.
    event.key.repeat = repeat;
    return event;
}

void testQueuedEventsToDoorState()
{
    const collision::World walls;
    const doors::Door first{7, {{{{100, 100}, {100, -100}, 23},
                                  {{100, -100}, {100, 100}, 33}}}};
    auto second = first;
    second.stateIndex = 8;
    for (auto& face : second.faces) { face.a.x = 180; face.b.x = 180; }
    std::vector<doors::Door> doors{first, second};
    game_lab::Level level; level.connectionStates.resize(9);
    DoorControl control(walls, doors, level);
    const Camera camera{50, 0, 0};

    ItemSession items; SurvivalSession player; GameAudio audio;
    items.load(level);
    auto foreign=keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_E);
    check(!items.handleEvent(foreign,32),"Another window can interact");
    for (auto event : {keyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_E),
                       keyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_E, true),
                       keyEvent(SDL_EVENT_KEY_UP, SDL_SCANCODE_E)})
        check(SDL_PushEvent(&event),"Cannot enqueue SDL key event");
    SDL_Event event{};
    while (SDL_PollEvent(&event)) items.handleEvent(event,31);
    items.update(&camera,true,walls,player,audio,&control);
    check(level.connectionStates[7]==0 && level.connectionStates[8]==0,
          "E must leave ordinary door words unchanged");
    items.update(&camera,true,walls,player,audio,&control);
    check(level.connectionStates[7]==0,"Consumed E changed an ordinary door");
    check(level.connectionStates[7]==0, "E must not activate an ordinary door");
}
void testModalMouseOwnership()
{
    ItemSession items; game_lab::Level level; items.load(level);
    SurvivalSession player; GameAudio audio; Camera camera{};
    auto open=keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_I);
    items.handleEvent(open,31);items.update(&camera,true,{},player,audio);
    check(items.modal(),"Mouse ownership fixture failed to open inventory");
    for(auto type:{SDL_EVENT_MOUSE_BUTTON_DOWN,SDL_EVENT_MOUSE_BUTTON_UP}) {
        SDL_Event event{};event.type=type;event.button.windowID=31;event.button.button=SDL_BUTTON_LEFT;
        check(items.handleEvent(event,31),"Modal leaked a left mouse event to gameplay");
        event.button.windowID=32;
        check(!items.handleEvent(event,31),"Modal captured another window's mouse event");
    }
    check(player.inventory().ammunition()==500,"UI click consumed ammunition");
}
void testMapOwnsQueuedActions()
{
    ItemSession items; game_lab::Level level; items.load(level);
    SurvivalSession player; GameAudio audio; Camera camera{50,0,0};
    const collision::World walls;
    std::vector<doors::Door> doors{{7,{{{{100,100},{100,-100},23},{{100,-100},{100,100},33}}}}};
    level.connectionStates.resize(8);
    DoorControl control(walls,doors,level);
    original_map::Session map; map.beginFrame(0);
    // Synthetic permission fixture: real transfer authorization has separate integration tests.
    for (const auto code:{SDL_SCANCODE_M,SDL_SCANCODE_E,SDL_SCANCODE_I,SDL_SCANCODE_Z,
                         SDL_SCANCODE_SPACE,SDL_SCANCODE_E}) {
        auto event=keyEvent(SDL_EVENT_KEY_DOWN,code);
        if (!map.handleEvent(event,31,&camera,!items.busy(),true,false,true,player))
            items.handleEvent(event,31);
    }
    check(!map.modal() && map.blocksSimulation(),"Same-pump close lost transition guard");
    items.update(&camera,!map.blocksSimulation(),walls,player,audio,&control);
    check(!items.modal() && level.connectionStates[7]==0,"Queued map keys interacted with world");
    map.beginFrame(0); items.update(&camera,true,walls,player,audio,&control);
    check(!items.modal() && level.connectionStates[7]==0,"Map left pending gameplay actions");
    // A pending inventory request must win over a later map-open key in the same pump.
    items.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_I),31);
    map.handleEvent(keyEvent(SDL_EVENT_KEY_UP,SDL_SCANCODE_M),31,&camera,false,true,false,true,player);
    map.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_M),31,&camera,!items.busy(),true,false,true,player);
    items.update(&camera,true,walls,player,audio,&control);
    check(items.modal() && !map.modal(),"Map opened over pending inventory");
}
} // namespace

int main()
{
    try {
        check(SDL_Init(SDL_INIT_EVENTS), "SDL event initialization failed");
        testQueuedEventsToDoorState(); testModalMouseOwnership(); testMapOwnsQueuedActions();
        SDL_Quit();
        std::cout << "Modal input ownership and E exclusion from ordinary doors passed.\n";
    } catch (const std::exception& error) {
        SDL_Quit();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
