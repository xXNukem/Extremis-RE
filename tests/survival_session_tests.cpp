#include "survival_session.h"
#include "game_audio.h"
#include "player_input.h"
#include "item_session.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

SDL_Event key(SDL_Scancode scancode)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = 17;
    event.key.scancode = scancode;
    return event;
}

void bank(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    std::ofstream file(directory/"BANK.DSS",std::ios::binary);
    const auto put = [&](unsigned value, int count) {
        for (int i = 0; i < count; ++i) file.put(char(value>>(8*i)));
    };
    put(30,2);
    for (int id = 0; id < 30; ++id) {
        put(64,1); put(0,1); put(1,1);
        file.write("DSS1",4);
        for (int i = 0; i < 24; ++i) file.put(' ');
        put(5000,2); put(128,4); put(id == 26 ? 65535 : 0,2);
        put(0,4); put(id == 26 ? 127 : 0,4);
        for (int i = 0; i < 128; ++i) file.put(char(200));
    }
    check(bool(file),"Cannot write dummy bank");
}

void normalSession(GameAudio& audio)
{
    SurvivalSession session;
    for (auto scancode : {SDL_SCANCODE_F5,SDL_SCANCODE_F6,SDL_SCANCODE_F7,
                          SDL_SCANCODE_F8,SDL_SCANCODE_F9,SDL_SCANCODE_F10,SDL_SCANCODE_F11})
        session.handleEvent(key(scancode),17);
    check(session.hudState().health == 1000 && session.hudState().oxygen == 288000,
          "Diagnostics enabled in normal game");
    check(session.inventory().batteryCharge()==-1 && !session.inventory().detectorEnabled(),"Diagnostic battery leaked into normal gameplay");
    session.update(100,false,audio);
    session.update(100,true,audio); // Entry interval includes inactive time: discard.
    session.update(1,true,audio);
    check(session.hudState().oxygen == 287840,"HUD not connected to active vitals");
    session.update(100,false,audio);
    session.update(100,true,audio);
    check(session.hudState().oxygen == 287840,"Inactive/resume time caught up");
    SDL_Event focus{};
    focus.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    focus.window.windowID = 17;
    session.handleEvent(focus,17);
    focus.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    session.handleEvent(focus,17);
    session.update(100,true,audio);
    check(session.hudState().oxygen == 287840,"Focus cycle within one pump consumed suspended time");
}

void deathSession(GameAudio& audio)
{
    SurvivalSession session(true);
    auto wrongWindow = key(SDL_SCANCODE_F8); wrongWindow.key.windowID = 18;
    session.handleEvent(wrongWindow,17);
    auto repeat = key(SDL_SCANCODE_F8); repeat.key.repeat = true;
    session.handleEvent(repeat,17);
    check(session.alive(),"Foreign/repeated diagnostic event accepted");
    session.handleEvent(key(SDL_SCANCODE_F11),17);
    check(session.inventory().batteryCharge()==144000 && session.inventory().detectorEnabled(),
          "Explicit diagnostic battery/radar did not activate");

    session.handleEvent(key(SDL_SCANCODE_F5),17);
    check(session.hudState().health == 750,"Diagnostic damage not reflected in HUD");
    session.handleEvent(key(SDL_SCANCODE_F6),17);
    check(session.hudState().health == 1000,"Verified health recovery not reflected in HUD");
    session.handleEvent(key(SDL_SCANCODE_F8),17);
    check(!session.alive() && session.hudState().oxygen == 0,"Oxygen death failed");
    check(session.update(1,true,audio),"Death transition not reported");
    for (int i = 0; i < 100; ++i) session.update(1,true,audio);
    const auto loopDeadline = SDL_GetTicks()+1500;
    while (!audio.effectStatistics(26)->wraps && SDL_GetTicks() < loopDeadline) SDL_Delay(5);
    check(audio.effectStatistics(26)->requests == 1 && audio.effectStatistics(26)->started == 1,
          "Oxygen death loop restarted every frame");
    check(audio.effectStatistics(26)->wraps > 0,"Native oxygen death loop was not preserved");
    session.handleEvent(key(SDL_SCANCODE_F9),17);
    check(!session.alive(),"Bottle revived dead player");
    session.handleEvent(key(SDL_SCANCODE_F10),17);
    session.update(1,true,audio);
    const auto until = SDL_GetTicks()+1500;
    while (!audio.effectStatistics(26)->stopped && SDL_GetTicks() < until) SDL_Delay(5);
    check(session.alive() && audio.effectStatistics(26)->stopped == 1,
          "Reset failed to stop original oxygen loop");
    for (int i = 0; i < 4; ++i) session.handleEvent(key(SDL_SCANCODE_F5),17);
    session.update(1,true,audio);
    check(!session.alive() && audio.effectStatistics(25)->requests == 1,
          "Health death did not select original BANK[25]");
    const auto healthDeadline = SDL_GetTicks()+1500;
    while (!audio.effectStatistics(25)->completed && SDL_GetTicks() < healthDeadline) SDL_Delay(5);
    check(audio.effectStatistics(25)->completed == 1 && audio.effectStatistics(25)->wraps == 0,
          "Health death effect repeated or failed to finish");
    check(session.windowTitle("Port").find("DEAD") != std::string::npos,
          "Pending original death presentation not explicit");
}

void silentResourceLoss(GameAudio& audio)
{
    SurvivalSession session(true);
    session.handleEvent(key(SDL_SCANCODE_F5),17);
    // 29 nominal minutes: low oxygen, but still alive. No invented breathing cue.
    for (int i = 0; i < 29; ++i) session.handleEvent(key(SDL_SCANCODE_F7),17);
    session.update(0,false,audio);
    check(session.alive() && session.hudState().health == 750 && session.hudState().oxygen == 9600,
          "Nonfatal resource-loss fixture failed");
    for (std::size_t id : {14,25,26})
        check(audio.effectStatistics(id)->requests == 0,"Resource loss invented a survival/death sound");
}

void deadInput()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");
    check(SDL_InitSubSystem(SDL_INIT_VIDEO),"Cannot initialize dummy video");
    SDL_Window* window = SDL_CreateWindow("test",320,200,SDL_WINDOW_HIDDEN);
    check(window != nullptr,"Cannot create dummy window");
    {
        std::vector<doors::Door> doors;
        const collision::World world;
        game_lab::Level level;
        DoorControl control(world,doors,level);
        PlayerInput input(window,control);
        Camera camera{3500,900,1.0};
        const Camera before = camera;
        auto event = key(SDL_SCANCODE_E);
        event.key.windowID = SDL_GetWindowID(window);
        input.handleEvent(event);
        check(input.update(&camera,false,1),"Disabled player input failed");
        check(camera.x == before.x && camera.y == before.y &&
              camera.angle == before.angle && input.movementDistance() == 0,
              "Disabled player moved or produced displacement");
    }
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void modalPress(ItemSession& items, SDL_Scancode code, const Camera& camera,
                SurvivalSession& player, GameAudio& audio, DoorControl* control = nullptr)
{
    while (items.modal() && items.interfaceState()->phase!=interaction::Phase::Ready)
        items.update(&camera,true,{},player,audio,control,1.0);
    items.handleEvent(key(code),17);
    items.update(&camera,true,{},player,audio,control);
    auto release=key(code); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17);
}

void enterZoneCode(ItemSession& items, const Camera& camera, SurvivalSession& player, GameAudio& audio)
{
    for (char c : std::string("ABABBC")) {
        const unsigned target=unsigned(c-'A');
        while (items.interfaceState()->selection>target)
            modalPress(items,SDL_SCANCODE_LEFT,camera,player,audio);
        while (items.interfaceState()->selection<target)
            modalPress(items,SDL_SCANCODE_RIGHT,camera,player,audio);
        modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio);
    }
    modalPress(items,SDL_SCANCODE_DOWN,camera,player,audio);
    while (items.interfaceState()->selection<13)
        modalPress(items,SDL_SCANCODE_RIGHT,camera,player,audio);
    modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio);
    while (items.modal() && items.interfaceState()->phase!=interaction::Phase::Ready)
        items.update(&camera,true,{},player,audio,nullptr,1.0);
}

void itemSession(GameAudio& audio)
{
    game_lab::Level level{};
    level.accessCode=90;
    game_lab::Sector sector{};
    sector.vertices = {{100,200},{100,0}};
    sector.walls = {{0,1,0x3205,0x2064}};
    level.sectors.push_back(sector);
    ItemSession items;
    items.load(level);
    SurvivalSession player(true);
    const Camera camera{50,100,0};
    const collision::World obstacles;
    const auto requests = audio.effectStatistics(12)->requests;
    auto repeat = key(SDL_SCANCODE_E); repeat.key.repeat = true;
    items.handleEvent(repeat,17);
    items.update(&camera,true,obstacles,player,audio);
    check(player.hudState().injectors == 0,"Repeated key collected a panel");
    items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
    items.update(&camera,false,obstacles,player,audio);
    items.update(&camera,true,obstacles,player,audio);
    check(player.hudState().injectors == 0,"Inactive pickup request leaked into resumed game");
    items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
    items.update(&camera,true,obstacles,player,audio);
    check(items.modal() && player.hudState().injectors==0 && audio.effectStatistics(12)->requests==requests,
          "Opening original interface transferred content");
    enterZoneCode(items,camera,player,audio);
    check(audio.effectStatistics(28)->requests>=7,"Verified code-key audio missing");
    check(items.interfaceState()->screen==interaction::Screen::Supply,"Zone code did not open supply interface");
    modalPress(items,SDL_SCANCODE_ESCAPE,camera,player,audio);
    check(!items.modal() && player.hudState().injectors==0,"Cancel consumed injector");
    modalPress(items,SDL_SCANCODE_E,camera,player,audio);
    modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio);
    check(player.hudState().injectors == 1 && audio.effectStatistics(12)->requests == requests+1,
          "Accepted item did not reach HUD or original confirmation audio");
    while (items.modal())
        items.update(&camera,true,obstacles,player,audio,nullptr,1.0);
    items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
    items.update(&camera,true,obstacles,player,audio);
    check(audio.effectStatistics(12)->requests == requests+1,"Empty panel repeated confirmation sound");
    player.handleEvent(key(SDL_SCANCODE_F5),17);
    items.handleEvent(key(SDL_SCANCODE_Z),17);
    items.update(&camera,true,obstacles,player,audio);
    check(player.hudState().health == 1000 && player.inventory().injectors()[0] == 0 &&
          player.hudState().injectors == 1,"Real healing or empty-container HUD occupancy failed");
    items.handleEvent(key(SDL_SCANCODE_X),17);
    items.update(&camera,true,obstacles,player,audio);
    check(player.alive() && !player.hudState().oxygenBottle,"Absent bottle invented oxygen or caused death");
    items.load(level);
    player.handleEvent(key(SDL_SCANCODE_F8),17);
    items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
    items.update(&camera,true,obstacles,player,audio);
    check(player.inventory().injectors()[0] == 0,"Dead player transferred a new injector");
}
void runningOxygen(GameAudio& audio)
{
    SurvivalSession walking, running;
    walking.update(0,true,audio);running.update(0,true,audio);
    walking.update(1,true,audio);running.update(1,true,audio,true);
    check(walking.vitals().oxygen()==player::Vitals::maximumOxygen-160 &&
          running.vitals().oxygen()==player::Vitals::maximumOxygen-320,"Run oxygen modifier not applied");
    const auto before=running.vitals().oxygen();running.update(30,false,audio,true);
    check(running.vitals().oxygen()==before,"Running consumed oxygen in modal/inactive state");
}
void messageQueue()
{
    messages::Queue queue;
    queue.post(messages::Id::PortalLocked);
    queue.update(2,true);
    queue.post(messages::Id::PortalLocked);
    check(queue.visible()[1].empty(),"Duplicate original message was queued");
    queue.update(100,false);
    check(!queue.visible()[0].empty(),"Paused message expired");
    queue.update(3,true);
    check(queue.visible()[0].empty(),"Duplicate refreshed original 800-tick expiry");
    for (const int fps : {30,60,144}) {
        messages::Queue timed;
        timed.post(messages::Id::DoorUnlocked);
        for (int i = 0; i < fps*5; ++i) timed.update(1.0/fps,true);
        check(timed.visible()[0].empty(),"Message expiry depends on render FPS");
    }
    for (auto id : {messages::Id::PortalLocked,messages::Id::DoorUnlocked,
                    messages::Id::CardRejected,messages::Id::AlreadyDisabled,messages::Id::EmptyPanel})
        queue.post(id);
    check(queue.visible()[4] == messages::text(messages::Id::EmptyPanel),"Original five-message queue capacity differs");
}

void readerSession(GameAudio& audio)
{
    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100,200},{100,0}};
    sector.walls = {{0,1,0x3308,0x0107}};
    level.sectors.push_back(sector);
    ItemSession items; items.load(level);
    SurvivalSession player;
    const collision::World obstacles;
    std::vector<doors::Door> doors{{7,{}}};
    level.connectionStates.resize(8); level.connectionStates[7]=0x400;
    DoorControl control(obstacles,doors,level);
    Camera camera{50,100,0};
    const auto press = [&] {
        items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
        items.update(&camera,true,obstacles,player,audio,&control);
    };
    std::uint16_t card = 0x3001; player.inventory().takeSupply(card);
    press();
    check(level.connectionStates[7] == 0x0400 && items.modal() &&
          items.interfaceState()->cards[0]==1 && items.interfaceState()->notice.empty(),
          "E-only inventory granted security or failed to announce block");
    modalPress(items,SDL_SCANCODE_ESCAPE,camera,player,audio,&control);
    card = 0x4001; player.inventory().takeSupply(card);
    player.vitals().applyDamage(1000); press();
    check(level.connectionStates[7] == 0x0400,"Dead player used a security reader");
    player.vitals().reset();
    camera.angle = 3.141592653589793; press();
    check(level.connectionStates[7] == 0x0400,"Reader accepted card from behind facing direction");
    camera.angle = 0;
    items.handleEvent(key(SDL_SCANCODE_E),17);
    { auto release=key(SDL_SCANCODE_E); release.type=SDL_EVENT_KEY_UP; items.handleEvent(release,17); }
    const collision::World occluder{{{75,0},{75,200}}};
    items.update(&camera,true,occluder,player,audio,&control);
    check(level.connectionStates[7] == 0x0400,"Reader used through obstacle");
    press();
    check(items.modal() && level.connectionStates[7]==0x0400,"Opening selector auto-used a card");
    modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio,&control);
    check(level.connectionStates[7]==0x0400,"Selector accepted E card as C");
    modalPress(items,SDL_SCANCODE_DOWN,camera,player,audio,&control);
    modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio,&control);
    while (items.modal() && items.interfaceState()->phase!=interaction::Phase::Ready)
        items.update(&camera,true,obstacles,player,audio,&control,1.0);
    check(level.connectionStates[7] == 0 && player.inventory().cards()[1] == 0x1001 &&
          player.hudState().messages[0] == messages::text(messages::Id::DoorUnlocked),
          "Valid C card not retained or unlock message missing from HUD");
    modalPress(items,SDL_SCANCODE_SPACE,camera,player,audio,&control);
    press();
    check(level.connectionStates[7] == 0,"Repeated runtime reader use toggled security back on");
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    try {
        bank(argv[1]);
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
        GameAudio audio; audio.load(argv[1]);
        check(audio.effectStatistics(26).has_value(),"Cannot open dummy death audio");
        normalSession(audio); silentResourceLoss(audio); deathSession(audio); deadInput(); itemSession(audio);
        messageQueue(); runningOxygen(audio); readerSession(audio);
        GameAudio unavailable;
        SurvivalSession withoutAudio(true);
        withoutAudio.handleEvent(key(SDL_SCANCODE_F8),17);
        withoutAudio.update(1,true,unavailable);
        check(!withoutAudio.alive(),"Absent audio prevented logical death");
        std::cout << "Survival/HUD, diagnostic isolation, input gating and death audio passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
