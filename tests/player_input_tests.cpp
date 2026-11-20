#include "platform/sdl/input/player_input.h"
#include "app/session/item_session.h"
#include "extremis/gameplay/player/survival_session.h"
#include "extremis/audio/game_audio.h"
#include "app/session/map_session.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <memory>
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
void testPlayerDeviceTransitions()
{
    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("PlayerInput test", 64, 64, 0), SDL_DestroyWindow);
    check(window != nullptr, "Cannot create input test window");
    SDL_PumpEvents();
    check(SDL_GetKeyboardFocus() == window.get(), "Input test window has no keyboard focus");

    const collision::World walls;
    const std::vector<doors::Door> doors;
    game_lab::Level level;
    DoorControl control(walls, doors, level);
    Camera camera{};
    constexpr double tick = 1.0 / 18;
    {
        PlayerInput input(window.get(), control);
        SDL_Event mouse{};
        mouse.button.windowID = SDL_GetWindowID(window.get());
        mouse.button.button = SDL_BUTTON_LEFT;
        check(input.update(&camera, true, tick, true) && !input.fireReady(), "Capture must wait one frame");
        check(input.update(&camera, true, tick, true) && input.fireReady(), "Capture did not become ready");

        mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        input.handleEvent(mouse, true);
        auto trigger = input.takeFireTrigger();
        check(trigger.held && trigger.pressed, "First gameplay click did not arm firing");
        trigger = input.takeFireTrigger();
        check(trigger.held && !trigger.pressed, "Held click repeated its initial edge");

        mouse.type = SDL_EVENT_MOUSE_BUTTON_UP;
        input.handleEvent(mouse, false);
        trigger = input.takeFireTrigger();
        check(!trigger.held && !trigger.pressed, "Modal release did not clear firing");
        mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        input.handleEvent(mouse, false);
        input.handleEvent(mouse, true);
        trigger = input.takeFireTrigger();
        check(!trigger.held && !trigger.pressed, "Modal click leaked into gameplay");

        int moves = 0;
        const player::MovementConstraint countMove = [&](const Camera&, Camera&) { ++moves; };
        input.move(camera, tick, false, countMove);
        check(moves == 1, "Ready input did not reach movement");
        input.releaseRelativeMouse();
        check(!SDL_GetWindowRelativeMouseMode(window.get()), "Pause did not release the cursor");
        input.invalidateContext();
        input.move(camera, tick, false, countMove);
        check(moves == 1 && !input.fireReady(), "Invalidation left movement enabled");
        check(input.update(&camera, false, tick, true) && !input.fireReady(), "Modal frame enabled input");
        check(input.update(&camera, true, tick, true) && !input.fireReady(), "Resuming skipped the capture guard");
        check(input.update(&camera, true, tick, true) && input.fireReady(), "Input did not resume");
        input.move(camera, tick, false, countMove);
        check(moves == 2 && camera.x == 0 && camera.y == 0, "Idle resume moved the player");

        input.handleEvent(mouse, true);
        trigger = input.takeFireTrigger();
        check(trigger.held && trigger.pressed, "Fresh click after recapture did not arm firing");

        auto tab = keyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_TAB);
        tab.key.windowID = mouse.button.windowID;
        tab.key.key = SDLK_TAB;
        input.handleEvent(tab, true);
        input.handleEvent(tab, true);
        check(input.fireReady(), "TAB invalidated readiness before the update");
        check(input.update(&camera, true, tick, true), "Update after TAB failed");
#ifdef EXTREMIS_DEBUG_MENU
        check(!input.fireReady(), "Same-pump Debug TABs did not force the capture guard");
        check(input.update(&camera, true, tick, true) && input.fireReady(), "Debug TAB left input blocked");
#else
        check(input.fireReady(), "Release TAB unexpectedly reset capture");
#endif
    }
    check(!SDL_GetWindowRelativeMouseMode(window.get()), "PlayerInput destruction left the cursor captured");
}
} // namespace

int main()
{
    try {
        check(SDL_Init(SDL_INIT_EVENTS), "SDL event initialization failed");
        testQueuedEventsToDoorState(); testModalMouseOwnership(); testMapOwnsQueuedActions();
        check(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE),
              "Cannot select dummy video driver");
        check(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL video initialization failed");
        testPlayerDeviceTransitions();
        SDL_Quit();
        std::cout << "Modal ownership, gameplay device transitions and E exclusion from ordinary doors passed.\n";
    } catch (const std::exception& error) {
        SDL_Quit();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
