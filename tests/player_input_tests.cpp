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

void testClassic()
{
    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Classic controls", 64, 64, 0), SDL_DestroyWindow);
    check(bool(window) && SDL_GetKeyboardFocus() == window.get(), "Classic window has no focus");
    const auto id = SDL_GetWindowID(window.get());
    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100,200},{100,0}};
    sector.walls = {{0,1,0x3205,0x2064}};
    level.sectors.push_back(sector);
    const collision::World walls;
    const std::vector<doors::Door> doors;
    DoorControl control(walls,doors,level);
    PlayerInput input(window.get(),control);
    Camera camera{0,100,0};
    constexpr double tick = 1.0 / 18;
    input.setClassicControls(true);
    check(input.update(&camera,true,0) && input.fireReady(), "Classic waited for relative capture");
    const auto key = [&](SDL_Scancode scan, bool down=true, bool gameplay=true) {
        auto event = keyEvent(down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP,scan);
        event.key.windowID = id;
        return input.handleEvent(event,gameplay);
    };
    SDL_Event mouse{};
    mouse.type = SDL_EVENT_MOUSE_MOTION;
    mouse.motion.windowID = id;
    mouse.motion.xrel = 1000;
    input.handleEvent(mouse);
    mouse = {};
    mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    mouse.button.windowID = id;
    mouse.button.button = SDL_BUTTON_LEFT;
    input.handleEvent(mouse);
    key(SDL_SCANCODE_W);
    check(input.update(&camera,true,tick) && !SDL_GetWindowRelativeMouseMode(window.get()) &&
          camera.x == 0 && camera.angle == 0 && !input.takeFireTrigger().held,
          "Classic accepted Modern movement or the mouse");

    key(SDL_SCANCODE_UP);
    input.update(&camera,true,tick);
    const auto walked = camera.x;
    check(walked > 0, "Up did not advance");
    key(SDL_SCANCODE_LALT);
    input.update(&camera,true,tick);
    check(input.running() && camera.x - walked > walked, "Left Alt did not run");
    key(SDL_SCANCODE_LALT,false);
    key(SDL_SCANCODE_UP,false);
    const auto beforeReverse = camera.x;
    key(SDL_SCANCODE_DOWN);
    input.update(&camera,true,tick);
    check(camera.x < beforeReverse, "Down did not reverse");
    key(SDL_SCANCODE_DOWN,false);
    key(SDL_SCANCODE_LEFT);
    input.update(&camera,true,tick);
    check(camera.angle > 0, "DOS left turn must increase the heading");
    const auto leftHeading = camera.angle;
    key(SDL_SCANCODE_RIGHT);
    input.update(&camera,true,tick*2);
    check(camera.angle < 0 && camera.angle < leftHeading, "DOS right turn must win when both arrows are held");
    key(SDL_SCANCODE_LEFT,false);
    key(SDL_SCANCODE_RIGHT,false);
    key(SDL_SCANCODE_SPACE);
    input.update(&camera,true,tick);
    check(camera.postureHeight < Camera::eyeHeight, "Space did not crouch");
    key(SDL_SCANCODE_SPACE,false);

    check(!key(SDL_SCANCODE_LCTRL), "Ctrl alone fired");
    check(key(SDL_SCANCODE_UP), "Ctrl + Up did not arm");
    key(SDL_SCANCODE_UP,false);
    key(SDL_SCANCODE_LCTRL,false);
    const auto tap = input.takeFireTrigger();
    check(tap.pressed && !tap.held, "Short Classic chord was lost in the event pump");
    player::WeaponFire weapon;
    SurvivalSession player;
    weapon.update(camera,walls,player.inventory(),{},true,tick);
    const auto ammunition = player.inventory().ammunition();
    check(weapon.update(camera,walls,player.inventory(),tap,true,tick) == player::FireResult::Fired &&
          player.inventory().ammunition() == ammunition - 1, "Classic chord did not fire the equipped weapon");
    check(key(SDL_SCANCODE_RETURN) && input.takeFireTrigger().held,
          "Classic keyboard fallback did not arm");
    key(SDL_SCANCODE_RETURN,false);

    ItemSession items;
    GameAudio audio;
    items.load(level);
    camera = {50,100,0};
    key(SDL_SCANCODE_UP);
    check(key(SDL_SCANCODE_LCTRL) && items.requestContextualAction(camera,walls),
          "Classic chord failed to select a reachable panel");
    input.suppressClassicFire();
    items.update(&camera,true,walls,player,audio);
    const auto suppressed = input.takeFireTrigger();
    check(items.modal() && !suppressed.pressed && !suppressed.held,
          "A panel interaction also armed the weapon");
    input.update(&camera,false,tick);
    key(SDL_SCANCODE_UP,false,false);
    key(SDL_SCANCODE_LCTRL,false,false);
    key(SDL_SCANCODE_UP,true,false);
    key(SDL_SCANCODE_LCTRL,true,false);
    check(input.update(&camera,true,0) && !input.takeFireTrigger().held,
          "Modal input leaked into Classic gameplay");

    input.setClassicControls(false);
    check(input.update(&camera,true,0) && !input.fireReady() &&
          input.update(&camera,true,0) && input.fireReady() &&
          SDL_GetWindowRelativeMouseMode(window.get()), "Modern capture failed after switching profiles");
    input.setClassicControls(true);
    check(!SDL_GetWindowRelativeMouseMode(window.get()) && !input.takeFireTrigger().held,
          "Switching to Classic kept Modern capture or shooting state");
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
        testPlayerDeviceTransitions(); testClassic();
        SDL_Quit();
        std::cout << "Modal ownership, gameplay device transitions and E exclusion from ordinary doors passed.\n";
    } catch (const std::exception& error) {
        SDL_Quit();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
