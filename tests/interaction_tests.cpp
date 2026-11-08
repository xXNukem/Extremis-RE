#include "original_interactions.h"
#include "original_travel.h"
#include "item_session.h"
#include "survival_session.h"
#include "game_audio.h"
#include "door_control.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
interaction::Resources fixture()
{
    interaction::Resources r;
    r.pickupMovie.frames.assign(5,std::vector<std::uint8_t>(64000));
    const auto fill=[](unsigned count) {
        std::vector<spr_format::Image> result;
        for(unsigned i=0;i<count;++i) result.push_back({1,1,0,0,{std::uint8_t(i+1)}});
        return result;
    };
    r.keyboard=fill(10); r.supplies=fill(31); r.weapons=fill(26); r.inventory=fill(27);
    r.letters=fill(76); r.reader=fill(5); r.plan=fill(5);
    r.supplyDoor=fill(1); r.weaponDoor=fill(1); r.planDoor=fill(1);
    r.supplyOpening=fill(12); r.weaponOpening=fill(11);
    r.codes=fill(10); r.elevator=fill(10);
    const auto resize=[](spr_format::Image& s,int w,int h,std::uint8_t color) {
        s={w,h,0,0,std::vector<std::uint8_t>(w*h,color)};
    };
    for(auto* bank:{&r.supplyDoor,&r.weaponDoor,&r.planDoor}) resize((*bank)[0],196,128,1);
    resize(r.supplyOpening[0],196,100,1); resize(r.weaponOpening[0],196,100,1);
    resize(r.keyboard[0],196,100,1); resize(r.supplies[12],196,100,13);
    resize(r.weapons[12],196,100,13); resize(r.inventory[0],184,180,1);
    resize(r.reader[0],196,100,1); resize(r.plan[0],196,100,1);
    resize(r.codes[0],196,100,1); resize(r.elevator[0],196,100,1);
    r.tinyFont.assign(278,0); r.tinyFont[0]=3; r.tinyFont[1]=5;
    std::fill(r.tinyFont.begin()+2,r.tinyFont.begin()+98,255);
    r.smallFont.assign(634,0); r.smallFont[0]=3; r.smallFont[1]=8;
    std::fill(r.smallFont.begin()+2,r.smallFont.begin()+98,255);
    for(unsigned i=0;i<256;++i) r.palette[i]={std::uint8_t(i),0,0};
    return r;
}
std::uint32_t color(unsigned i) {return (i<<24)|255;}
void graphics()
{
    const auto resources=fixture(); const interaction::Renderer renderer(resources);
    for(auto screen:{interaction::Screen::Code,interaction::Screen::Supply,interaction::Screen::Weapon,
                     interaction::Screen::Cards,interaction::Screen::Reader,interaction::Screen::Plan,interaction::Screen::Codes,interaction::Screen::Inventory,interaction::Screen::Elevator}) {
        interaction::State s; s.screen=screen; s.contents=0x2064; s.cards={1,0x1001,-1};
        std::vector<std::uint32_t> pixels(320*200,0xabcdef12);
        check(renderer.draw(pixels,s),"Original modal draw failed");
        if(screen==interaction::Screen::Supply)
            check(pixels[32*320+75]==color(10),"Supply label confused one-based EXE sprite IDs with indices");
        if(screen!=interaction::Screen::Cards && screen!=interaction::Screen::Inventory) {
            check(pixels[25*320+62]==color(screen==interaction::Screen::Supply || screen==interaction::Screen::Weapon?13:1),
                  "Modal base not at recovered (62,25)");
            check(pixels[150*320+160]==0xabcdef12,"Viewport modal overwrote HUD");
        } else check(pixels[10*320+68]==color(1),"Inventory modal base not at (68,10)");
        check(pixels.front()==0xabcdef12 && pixels.back()==0xabcdef12,"Modal escaped framebuffer bounds");
        s.selection=~0u; s.contents=0xffff; s.entered=std::string(1000,'A');
        s.notice=std::string(10000,'A'); s.cards={0x7fff,0xffff,0x2001};
        check(renderer.draw(pixels,s),"Defensive malformed-state modal draw failed");
        for (int size:{0,319*200,320*200+1}) {
            std::vector<std::uint32_t> wrong(size,123);
            check(!renderer.draw(wrong,s) && std::all_of(wrong.begin(),wrong.end(),[](auto p){return p==123;}),
                  "Wrong framebuffer modified");
        }
    }
    bool failed=false;
    auto bad=resources; bad.smallFont[2]=254;
    try {interaction::Renderer invalid(bad);} catch(const std::runtime_error&) {failed=true;}
    check(failed,"Invalid font bitmap mapping accepted");
    failed=false; bad=resources; bad.supplies[12].indices.pop_back();
    try {interaction::Renderer invalid(bad);} catch(const std::runtime_error&) {failed=true;}
    check(failed,"Truncated modal sprite accepted");
    failed=false;
    try {interaction::load("/missing-original-interface-assets");} catch(const std::runtime_error&) {failed=true;}
    check(failed,"Missing modal resources accepted");
}
void checksum()
{
    check(interaction::codeChecksum("ABABBC")==90,"EXE weighted byte checksum differs");
    check(interaction::codeChecksum("LLLLLL")==std::uint8_t(11*63),"Checksum did not wrap BL");
    check(!interaction::codeChecksum("ABC") && !interaction::codeChecksum("ABCDEM"),"Invalid code accepted");
    check(interaction::cardName(1)=="E001" && interaction::cardName(0x1001)=="C001",
          "Original card type/decimal encoding differs");
}
struct Session {
    GameAudio audio;
    SurvivalSession player;
    ItemSession items;
    Camera camera{50,100,0};
    collision::World obstacles;
    std::vector<doors::Door> doors{{7,0x400,{}}};
    DoorControl control{obstacles,doors};
    game_lab::Level level;
    Session(std::uint16_t type=0x3205,std::uint16_t contents=0x2064) {
        game_lab::Sector sector{};
        sector.vertices={{100,200},{100,0}}; sector.walls={{0,1,type,contents}};
        level.sectors.push_back(sector); level.accessCode=90; level.codeTargets={0,1}; items.load(level);
    }
    void tick(bool active=true) {items.update(&camera,active,obstacles,player,audio,&control);}
    void settle() {
        for(unsigned i=0;i<600 && items.modal() &&
            items.interfaceState()->phase!=interaction::Phase::Ready &&
            (items.interfaceState()->phase!=interaction::Phase::CardResult || items.interfaceState()->cardAccepted);++i) tick();
    }
    void press(SDL_Scancode code) {
        SDL_Event e{}; e.type=SDL_EVENT_KEY_DOWN; e.key.windowID=17; e.key.scancode=code;
        items.handleEvent(e,17); tick();
        e.type=SDL_EVENT_KEY_UP; items.handleEvent(e,17);
        settle();
    }
    void select(unsigned key) {
        while(items.interfaceState()->selection/7>key/7) press(SDL_SCANCODE_UP);
        while(items.interfaceState()->selection/7<key/7) press(SDL_SCANCODE_DOWN);
        while(items.interfaceState()->selection%7>key%7) press(SDL_SCANCODE_LEFT);
        while(items.interfaceState()->selection%7<key%7) press(SDL_SCANCODE_RIGHT);
    }
    void code(std::string_view value="IJEKAH") {
        for(char c:value) {select(unsigned(c-'A')+(c>='G'?1:0)); press(SDL_SCANCODE_SPACE);}
        select(13); press(SDL_SCANCODE_SPACE);
    }
};
void flows()
{
    Session s;
    s.player.vitals().applyDamage(250); // Test fixture, not a newly invented attack.
    s.press(SDL_SCANCODE_E);
    check(s.items.modal() && s.player.hudState().injectors==0,"Open action transferred injector");
    s.press(SDL_SCANCODE_Z); s.press(SDL_SCANCODE_X); s.press(SDL_SCANCODE_E);
    check(s.items.interfaceState()->screen==interaction::Screen::Code,"Gameplay key leaked into modal");
    s.code("AAAAAA");
    check(s.items.interfaceState()->notice=="WRONG CODE!!..." && s.player.hudState().injectors==0,
          "Incorrect zone code bypassed supply access");
    s.press(SDL_SCANCODE_ESCAPE); check(!s.items.modal(),"Cancel did not close code panel");
    s.press(SDL_SCANCODE_E); s.code();
    check(s.items.interfaceState()->screen==interaction::Screen::Supply,"Canonical C200 code not accepted");
    s.press(SDL_SCANCODE_RIGHT); s.press(SDL_SCANCODE_SPACE);
    check(!s.items.modal() && s.player.hudState().injectors==0,"No option consumed an object");
    s.press(SDL_SCANCODE_E);
    check(s.items.interfaceState()->screen==interaction::Screen::Supply,"Zone authorization not persistent");
    s.press(SDL_SCANCODE_SPACE);
    check(!s.items.modal() && s.player.inventory().injectors()[0]==100,"Yes did not transfer once");
    check(s.player.vitals().health()==750,"Storing injector healed automatically");
    s.press(SDL_SCANCODE_E);
    check(!s.items.modal() && s.player.hudState().messages[0]=="THIS COMPARTMENT IS EMPTY!",
          "Empty compartment reopened or failed to announce English message");
    s.player.vitals().applyDamage(500); s.press(SDL_SCANCODE_Z);
    check(s.player.vitals().health()==1000 && s.player.inventory().injectors()[0]==0,
          "Modal injector pickup disconnected real Vitals recovery");
    Session weapon(0x3104,0x11f4); weapon.press(SDL_SCANCODE_E); weapon.code();
    check(weapon.player.inventory().weapon()==0,"Weapon equipped before confirmation");
    weapon.press(SDL_SCANCODE_SPACE);
    check(weapon.player.inventory().weapon()==1,"Weapon confirmation failed");
    weapon.press(SDL_SCANCODE_E);
    check(weapon.items.interfaceState()->contents==0x01f4,"Weapon cabinet lost deposited weapon/ammunition");
    weapon.press(SDL_SCANCODE_ESCAPE);
    check(weapon.player.inventory().weapon()==1,"Cancel exchanged weapon");
    Session bottle(0x3205,0x1708); // Verified bottle branch; no invented placement in real level 1.
    bottle.player.vitals().update(60,true);
    const auto oxygen=bottle.player.vitals().oxygen();
    bottle.press(SDL_SCANCODE_E); bottle.code(); bottle.press(SDL_SCANCODE_SPACE);
    check(bottle.player.inventory().oxygenReserve()==288000 && bottle.player.vitals().oxygen()==oxygen,
          "Bottle modal restored oxygen immediately instead of storing reserve");
    bottle.press(SDL_SCANCODE_X);
    check(bottle.player.vitals().oxygen()==288000 && bottle.player.inventory().oxygenReserve()==oxygen,
          "Modal bottle pickup duplicated or disconnected the real oxygen exchange");
}
void selectionRegressions()
{
    for (auto type:{0x3205,0x3104}) {
        Session s(std::uint16_t(type),type==0x3205?0x2064:0x11f4);
        s.press(SDL_SCANCODE_E); s.code();
        check(s.items.interfaceState()->selection==0,"Initial selection must be Yes");
        s.press(SDL_SCANCODE_LEFT); check(s.items.interfaceState()->selection==0,"Left from Yes wrapped");
        s.press(SDL_SCANCODE_RIGHT); check(s.items.interfaceState()->selection==1,"Right must select No");
        s.press(SDL_SCANCODE_RIGHT); check(s.items.interfaceState()->selection==1,"Right from No wrapped");
        s.press(SDL_SCANCODE_LEFT); check(s.items.interfaceState()->selection==0,"Left must select Yes");
        s.press(SDL_SCANCODE_RIGHT); s.press(SDL_SCANCODE_SPACE);
        check(!s.items.modal() && s.player.inventory().injectors()[0]==-1 && s.player.inventory().weapon()==0,
              "No mutated inventory");
        s.press(SDL_SCANCODE_E); check(s.items.interfaceState()->selection==0,"Reopen did not reset selection");
        s.press(SDL_SCANCODE_ESCAPE); s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_SPACE);
        SDL_Event repeat{}; repeat.type=SDL_EVENT_KEY_DOWN; repeat.key.windowID=17;
        repeat.key.scancode=SDL_SCANCODE_SPACE; repeat.key.repeat=true;
        s.items.handleEvent(repeat,17); s.tick();
        check(!s.items.modal(),"Repeated confirmation reopened modal");
        if(type==0x3205) check(s.player.inventory().injectors()==std::array<int,2>{100,-1},"Pickup duplicated");
        else check(s.player.inventory().weapon()==1,"Yes failed weapon exchange");
    }
    auto r=fixture();
    for (auto* bank:{&r.supplies,&r.weapons}) {
        (*bank)[13]={60,45,0,0,std::vector<std::uint8_t>(60*45,0)};
        (*bank)[13].indices[36*60+15]=221; (*bank)[13].indices[36*60+42]=221;
    }
    r.keyboard[7]={21,20,20,19,std::vector<std::uint8_t>(420,8)};
    const interaction::Renderer renderer(r);
    std::vector<std::uint32_t> pixels(320*200);
    for(auto screen:{interaction::Screen::Supply,interaction::Screen::Weapon}) {
        interaction::State state; state.screen=screen; state.contents=0x2064;
        for(unsigned choice=0;choice<2;++choice) {
            state.selection=choice; renderer.draw(pixels,state);
            check(pixels[96*320+90]==color(choice?15:221) && pixels[96*320+117]==color(choice?221:16),
                  "Bright Yes/No disagrees with logical selection");
        }
    }
    for(unsigned key=0;key<14;++key) {
        interaction::State state; state.selection=key; renderer.draw(pixels,state);
        const int x=70+15*int(key%7)+42-20, y=70+17*int(key/7)+9-19;
        check(pixels[y*320+x]==color(8),"Finger pivot not subtracted at selected key");
        check(pixels[124*320+257]!=color(8),"Finger escaped clipping window");
    }
    interaction::State cards; cards.screen=interaction::Screen::Cards; cards.cards={1,0x1001,0x2001};
    cards.notice="Incompatible card type"; renderer.draw(pixels,cards);
    check(std::all_of(pixels.begin()+157*320+72,pixels.begin()+157*320+248,
                     [](auto p){return p==color(254);}),"Rejection overlaps card menu rows");
}
void detectorInventory()
{
    Session s; std::uint16_t battery=900;
    s.player.inventory().takeSupply(battery);
    s.press(SDL_SCANCODE_I); s.press(SDL_SCANCODE_E);
    check(s.items.interfaceState()->inventoryUse,"Battery section did not open detector action");
    s.press(SDL_SCANCODE_E);
    check(s.player.inventory().detectorEnabled(),"Inventory action did not enable original detector");
    const int charge=s.player.inventory().batteryCharge();
    s.press(SDL_SCANCODE_ESCAPE);
    check(!s.items.modal() && s.player.inventory().batteryCharge()==charge,"Inventory close consumed battery");
    s.press(SDL_SCANCODE_I); s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_E);
    check(!s.player.inventory().detectorEnabled(),"Inventory could not turn detector off");
    s.press(SDL_SCANCODE_I); check(!s.items.modal(),"Detector inventory could not close");
}
void inventoryAndElevator()
{
    Session s;
    std::uint16_t injector=0x2064, card=0x4001;
    s.player.inventory().takeSupply(injector); s.player.inventory().takeSupply(card);
    s.player.vitals().applyDamage(500);
    s.player.postMessage(messages::Id::PortalLocked);
    s.press(SDL_SCANCODE_I);
    check(s.player.hudState().messages[0].empty(),"Inventory retained stale HUD message");
    check(s.items.modal() && s.items.interfaceState()->screen==interaction::Screen::Inventory,
          "Original I binding did not open inventory");
    s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_Z);
    check(s.player.vitals().health()==500,"Gameplay use leaked into inventory");
    s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_SPACE);
    check(s.items.interfaceState()->inventoryUse,"Injector action confirmation missing");
    s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_SPACE);
    check(s.player.vitals().health()==500,"Do not use consumed injector");
    s.press(SDL_SCANCODE_SPACE); s.press(SDL_SCANCODE_SPACE);
    check(s.player.vitals().health()==1000 && s.player.inventory().injectors()[0]==0,
          "Inventory did not use existing consumable effect");
    s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_SPACE);
    check(s.player.inventory().cards()[0]==0x1001,"General inventory used security card on world");
    s.press(SDL_SCANCODE_ESCAPE); s.press(SDL_SCANCODE_I);
    check(s.items.interfaceState()->selection==0 && s.items.interfaceState()->injectors[0]==0,
          "Inventory reopen lost state or category reset");
    s.press(SDL_SCANCODE_ESCAPE);
    injector=0x2064; s.player.inventory().takeSupply(injector);
    injector=0x2064; s.player.inventory().takeSupply(injector);
    s.press(SDL_SCANCODE_I); s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_DOWN);
    s.press(SDL_SCANCODE_SPACE); s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_SPACE);
    check(s.player.inventory().injectors()==std::array<int,2>{100,0},"Selected injector-2 consumed injector-1");
    s.press(SDL_SCANCODE_ESCAPE);
    Session lift(0x3002,1); lift.press(SDL_SCANCODE_E);
    check(lift.items.interfaceState()->notice=="No elevator card","Elevator missing E-card restriction");
    lift.press(SDL_SCANCODE_SPACE); check(!lift.items.modal(),"No-card elevator exit failed");
    card=0x3001; lift.player.inventory().takeSupply(card); lift.press(SDL_SCANCODE_E);
    lift.press(SDL_SCANCODE_SPACE);
    check(lift.items.interfaceState()->screen==interaction::Screen::Elevator,"E001 failed elevator access");
    lift.press(SDL_SCANCODE_UP); lift.press(SDL_SCANCODE_SPACE);
    const auto travel=lift.items.takeTravel();
    check(travel && travel->zone==1 && travel->arrival.sector==0 &&
          travel->arrival.x==300 && travel->arrival.y==280 && travel->arrival.angle==540 &&
          lift.player.inventory().cards()[0]==1,
          "Pending level transition consumed E001");
    check(!lift.items.modal() && !lift.items.takeTravel(),"Elevator request was not consumed");
}
void originalTravel()
{
    using namespace interaction::original;
    check(!route(25,0) && !route(0,4),"Invalid elevator/row accepted");
    check(!admitted(*route(2,2),9,1,14),"E009 enabled nonexistent route");
    check(admitted(*route(2,1),2,1,14),"Current destination requires a listed permission");
    check(!admitted(*route(2,0),2,1,14),"E002 authorized A002");
    check(admitted(*route(2,0),9,1,14),"E009 did not bypass destination filter");
    check(!admitted(*route(2,0),0,1,14) && !admitted(*route(2,0),10,1,14),
          "Invalid E-card identifier accepted");
    for (unsigned index=0;index<lifts.size();++index) for (unsigned row=0;row<4;++row) {
        const auto& r=*route(index,row);
        if (!r.exists) continue;
        Session s(0x3002,std::uint16_t(index));
        std::uint16_t card=0x3009; s.player.inventory().takeSupply(card);
        s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_SPACE);
        for(unsigned i=0;i<row;++i) s.press(SDL_SCANCODE_UP);
        s.press(SDL_SCANCODE_SPACE);
        const auto travel=s.items.takeTravel();
        check(travel && travel->zone==r.zone && travel->arrival.sector==r.sector &&
              travel->arrival.x==lifts[index].x && travel->arrival.y==lifts[index].y &&
              travel->arrival.angle==lifts[index].angle,"Table route not reachable from interface");
    }
    interaction::Renderer renderer(fixture());
    interaction::State menu; menu.screen=interaction::Screen::Elevator;
    menu.elevatorIndex=2; menu.elevatorZone=1; menu.elevatorSector=14; menu.useSelection=1;
    std::vector<std::uint32_t> pixels(320*200);
    renderer.draw(pixels,menu);
    check(pixels[79*320+142]==color(2) && pixels[63*320+142]==color(4),
          "A002 or current B101 marker missing from shared table menu");
    menu.useSelection=2; renderer.draw(pixels,menu);
    check(pixels[79*320+142]==color(1) && pixels[63*320+142]==color(4),
          "Menu permissions disagree with current-destination exception");
    Session next(0x3002,2); next.level.zoneIndex=1; next.items.load(next.level);
    std::uint16_t card=0x3001; next.player.inventory().takeSupply(card);
    next.press(SDL_SCANCODE_E); next.press(SDL_SCANCODE_SPACE); next.press(SDL_SCANCODE_SPACE);
    auto travel=next.items.takeTravel();
    check(travel && travel->zone==2 && travel->arrival.sector==0 &&
          travel->arrival.x==300 && travel->arrival.y==280 && travel->arrival.angle==540 &&
          next.player.inventory().cards()[0]==1,"Level 2 to 3 with E001 failed");
    Session denied(0x3002,2); card=0x3002; denied.player.inventory().takeSupply(card);
    denied.press(SDL_SCANCODE_E); denied.press(SDL_SCANCODE_SPACE); denied.press(SDL_SCANCODE_SPACE);
    check(denied.items.modal() && !denied.items.takeTravel(),"Denied route closed modal or travelled");
    Session code(0x3508,1); code.level.codeTargets={2}; code.items.load(code.level);
    card=0x4001; code.player.inventory().takeSupply(card);
    code.press(SDL_SCANCODE_E); code.press(SDL_SCANCODE_SPACE);
    check(code.items.interfaceState()->notice=="CODE ZONE A002  CJBAIK","A002 code missing");
}
void missingVariants()
{
    // DOS uses CH_OBJ4A[11] for the mine name (0x1985a..0x1986e).
    auto resources=fixture(); const interaction::Renderer renderer(resources);
    interaction::State visual; visual.screen=interaction::Screen::Supply; visual.contents=0x6064;
    std::vector<std::uint32_t> pixels(320*200); renderer.draw(pixels,visual);
    check(pixels[32*320+75]==color(12),"Mine rendered as a card");
    visual.screen=interaction::Screen::Inventory; visual.selection=4; visual.mine=100;
    renderer.draw(pixels,visual);
    check(pixels[60*320+198]==color(23),"Owned mine absent from inventory");
    Session mine(0x3205,0x6064);
    mine.press(SDL_SCANCODE_E); mine.code();
    check(mine.items.modal() && mine.items.interfaceState()->screen==interaction::Screen::Supply,
          "Mine cabinet was filtered out");
    mine.press(SDL_SCANCODE_E);
    check(mine.player.inventory().mine()==100,"Mine was not transferred into original fixed slot");
    mine.press(SDL_SCANCODE_E);
    check(!mine.items.modal(),"Empty mine cabinet reopened");
    mine.items.load(mine.level); mine.press(SDL_SCANCODE_E); mine.code(); mine.press(SDL_SCANCODE_E);
    check(mine.items.interfaceState()->notice=="ALREADY HAVE A MINE" &&
          mine.items.interfaceState()->contents==0x6064,"Full mine slot consumed cabinet");
    for(std::uint16_t contents:{0x0384,0x1384,0x2064}) {
        Session full(0x3205,contents);
        auto held=contents; full.player.inventory().takeSupply(held);
        if((contents>>12)==2) { held=contents; full.player.inventory().takeSupply(held); }
        const auto battery=full.player.inventory().batteryCharge();
        const auto oxygen=full.player.inventory().oxygenReserve();
        const auto injectors=full.player.inventory().injectors();
        full.press(SDL_SCANCODE_E); full.code(); full.press(SDL_SCANCODE_E);
        check(full.items.modal() && full.items.interfaceState()->contents==contents &&
              full.player.inventory().batteryCharge()==battery && full.player.inventory().oxygenReserve()==oxygen &&
              full.player.inventory().injectors()==injectors,"Full inventory lost a supply");
        full.press(SDL_SCANCODE_ESCAPE); full.press(SDL_SCANCODE_E);
        check(full.items.interfaceState()->contents==contents,"Full inventory consumed world contents");
    }
    Session empty(0x3205,0xffff); empty.press(SDL_SCANCODE_E);
    check(!empty.items.modal() && empty.player.hudState().messages[0]=="THIS COMPARTMENT IS EMPTY!",
          "Initially empty cabinet disappeared from interaction detection");
    Session n(0x3422,0x0101);
    std::uint16_t card=0x4001; n.player.inventory().takeSupply(card);
    card=0x5002; n.player.inventory().takeSupply(card);
    card=0x5001; n.player.inventory().takeSupply(card);
    n.press(SDL_SCANCODE_E); n.press(SDL_SCANCODE_E);
    check(n.items.interfaceState()->notice=="Incompatible card type","N reader accepted C card");
    n.press(SDL_SCANCODE_DOWN); n.press(SDL_SCANCODE_E);
    check(n.items.interfaceState()->notice=="IMPOSSIBLE TO DEACTIVATE THE SECURITY",
          "N reader accepted wrong identifier");
    n.press(SDL_SCANCODE_ESCAPE); n.press(SDL_SCANCODE_E);
    n.press(SDL_SCANCODE_DOWN); n.press(SDL_SCANCODE_DOWN); n.press(SDL_SCANCODE_E);
    check(!n.items.modal() && n.player.inventory().cards()[2]==0x2001 && n.doors[0].originalState==0x400,
          "N reader consumed card or altered a door");
    n.items.load(n.level); n.press(SDL_SCANCODE_E);
    check(!n.items.modal(),"Disabled N security lost state on level return");
    n.items.resetPlans(); n.press(SDL_SCANCODE_E);
    check(n.items.modal(),"New game retained disabled N security");
}
void allPanelsLab(const char* path)
{
    unsigned count=0, empty=0, disabled=0, cards=0;
    std::map<unsigned,unsigned> types, supplies;
    for(unsigned zone=0;zone<30;++zone) {
        const auto level=game_lab::loadLevel(path,zone).level;
        items::World world(level);
        unsigned raw=0, tested=0;
        for(const auto& sector:level.sectors) for(const auto& wall:sector.walls)
            if((wall.type&0xf000)==0x3000) ++raw;
        check(world.panels().size()==raw,"Original interactive wall variant omitted");
        for(std::size_t index=0;index<world.panels().size();++index) {
            const auto& panel=world.panels()[index];
            ++types[level.sectors[panel.sector].walls[panel.wall].type];
            if(panel.elevator) continue; // Travel implementation is outside this audit.
            Session s; s.level=level; s.items.load(level);
            s.obstacles=collision::staticWorld(level); s.doors=doors::closedDoors(level);
            const double dx=panel.face.b.x-panel.face.a.x, dy=panel.face.b.y-panel.face.a.y;
            const double length=std::hypot(dx,dy);
            check(length>0,"Degenerate original panel");
            s.camera={(panel.face.a.x+panel.face.b.x)/2+50*dy/length,
                      (panel.face.a.y+panel.face.b.y)/2-50*dx/length,std::atan2(dx,-dy)};
            s.control.resetLevel(); s.control.prepareFrame(&s.camera,0,{});
            s.obstacles=s.control.movementWorld();
            check(world.target(s.camera,s.obstacles)==index,"Original panel occluded or unreachable");
            for(const auto& wall:s.obstacles)
                check(!collision::overlaps(wall,{s.camera.x,s.camera.y},30),"Panel test pose inside collision");
            const auto before=s.player.inventory().cards();
            s.press(SDL_SCANCODE_E); ++tested; ++count;
            if(panel.contents==0xffff && !panel.cardReader && !panel.codeReader && !panel.codePanel && !panel.securityPanel) {
                check(!s.items.modal(),"Empty cabinet opened"); ++empty; continue;
            }
            if(panel.cardReader && s.control.useCardReader(panel.contents,{})==DoorControl::CardResult::AlreadyDisabled) {
                check(!s.items.modal(),"Already disabled door reader opened"); ++disabled; continue;
            }
            check(s.items.modal(),"E did not start original panel interaction");
            check(s.items.interfaceState()->zoneName==interaction::original::codes[zone].zone,
                  "Panel displays another zone's code name");
            check(s.player.inventory().cards()==before,"Opening invented a card");
            if(panel.cardReader || panel.codeReader || panel.securityPanel) {
                check(s.items.interfaceState()->screen==interaction::Screen::Cards,"Reader skipped selection");
                s.press(SDL_SCANCODE_ESCAPE);
                std::uint16_t card=std::uint16_t((panel.securityPanel?0x5000:0x4000)|
                    (panel.codeReader?(panel.contents&255):(panel.contents>>8)));
                s.player.inventory().takeSupply(card);
                s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_E);
                if(panel.codeReader) check(s.items.interfaceState() &&
                    s.items.interfaceState()->screen==interaction::Screen::Codes,"Correct C card rejected");
                else if(panel.cardReader) check(s.control.useCardReader(panel.contents,{})==DoorControl::CardResult::AlreadyDisabled,
                    "Correct C card did not unlock original target");
                else { s.press(SDL_SCANCODE_E); check(!s.items.modal(),"N security did not remain disabled"); }
                continue;
            }
            check(level.accessCode==interaction::codeChecksum(interaction::original::codes[zone].letters),
                  "Zone code catalogue differs from GAME.LAB");
            if(zone<4) {
                s.code(*level.accessCode==0?"BBBBBB":"AAAAAA");
                check(s.items.interfaceState() && !s.items.interfaceState()->codeAccepted &&
                      s.items.interfaceState()->notice=="WRONG CODE!!...","Original zone access bypassed");
                s.press(SDL_SCANCODE_ESCAPE); s.press(SDL_SCANCODE_E);
            }
            s.code(interaction::original::codes[zone].letters);
            if(panel.codePanel) {
                for(unsigned i=0;i<500 && s.items.modal();++i) s.tick();
                check(s.items.planTransferred(),"Original plan transfer failed"); continue;
            }
            const unsigned kind=panel.contents>>12;
            if(!panel.weapon) ++supplies[kind];
            check(s.items.modal() && s.items.interfaceState()->screen==
                (panel.weapon?interaction::Screen::Weapon:interaction::Screen::Supply),"Correct zone code rejected");
            s.press(SDL_SCANCODE_E);
            check(!s.items.modal(),"Original object did not transfer to empty inventory");
            if(!panel.weapon && kind>=3 && kind<=5) {
                check(s.player.inventory().cards()[0]==int(panel.contents)-0x3000,"Original card was changed or lost");
                ++cards;
            }
        }
        std::cout<<"Zone "<<zone+1<<": "<<tested<<" non-elevator panels verified.\n";
    }
    for(const auto& [type,n]:types) std::cout<<"Type "<<std::hex<<type<<std::dec<<": "<<n<<"\n";
    for(const auto& [kind,n]:supplies) std::cout<<"Supply class "<<kind<<": "<<n<<"\n";
    std::cout<<count<<" panels; "<<cards<<" cards transferred; "<<empty<<" empty; "<<disabled<<" disabled readers.\n";
}
void originalTravelLab(const char* path)
{
    using namespace interaction::original;
    std::array<bool,3> seen{};
    unsigned destinations=0;
    for(unsigned zone=0;zone<3;++zone) {
        const auto data=game_lab::loadLevel(path,zone);
        for(const auto& sector:data.level.sectors) for(const auto& wall:sector.walls)
            if(wall.type==0x3002) {
                check(wall.extra<lifts.size(),"GAME.LAB references unknown lift"); if(wall.extra<seen.size()) seen[wall.extra]=true;
            }
        for(unsigned index=0;index<lifts.size();++index) for(const auto& r:lifts[index].routes) {
            if(!r.exists || r.zone!=zone) continue;
            check(r.sector<data.level.sectors.size(),"Arrival sector missing");
            const auto& sector=data.level.sectors[r.sector];
            // Elevator cabins need not have enemy navigation polygons. Validate
            // the original arrival against the sector's vertex bounds instead.
            const auto xBounds=std::minmax_element(sector.vertices.begin(),sector.vertices.end(),
                [](const auto& a,const auto& b){return a.x<b.x;});
            const auto yBounds=std::minmax_element(sector.vertices.begin(),sector.vertices.end(),
                [](const auto& a,const auto& b){return a.y<b.y;});
            check(!sector.vertices.empty() && lifts[index].x>=xBounds.first->x &&
                  lifts[index].x<=xBounds.second->x && lifts[index].y>=yBounds.first->y &&
                  lifts[index].y<=yBounds.second->y,"Arrival outside sector bounds");
            check(std::any_of(sector.walls.begin(),sector.walls.end(),[&](const auto& w) {
                return w.type==0x3002 && w.extra==index;
            }),"Destination has no matching elevator panel");
            ++destinations;
        }
    }
    check(std::all_of(seen.begin(),seen.end(),[](bool value){return value;}),"Unreferenced elevator table entry");
    std::cout<<"Verified first three levels and "<<destinations<<" arrivals against GAME.LAB.\n";
}
void unifiedControls()
{
    for(bool populated:{false,true}) {
        Session s;
        if(populated) {std::uint16_t card=0x4001; s.player.inventory().takeSupply(card);}
        const auto before=s.player.inventory().cards();
        for(int cycle=0;cycle<8;++cycle) {
            s.press(SDL_SCANCODE_I); check(s.items.modal(),"Inventory toggle failed to open");
            s.press(SDL_SCANCODE_S); check(s.items.interfaceState()->selection==1,"S did not navigate inventory");
            s.press(SDL_SCANCODE_W); check(s.items.interfaceState()->selection==0,"W did not navigate inventory");
            s.press(cycle%2?SDL_SCANCODE_I:SDL_SCANCODE_ESCAPE);
            check(!s.items.modal() && s.player.inventory().cards()==before,"Inventory close lost state or trapped player");
        }
        s.press(SDL_SCANCODE_I);
        SDL_Event cancel{}; cancel.type=SDL_EVENT_KEY_DOWN; cancel.key.windowID=17; cancel.key.scancode=SDL_SCANCODE_ESCAPE;
        s.items.handleEvent(cancel,17); s.tick(false);
        check(!s.items.modal(),"Inventory close was discarded during capture/focus transition");
    }
    Session pending;
    SDL_Event queued{};queued.type=SDL_EVENT_KEY_DOWN;queued.key.windowID=17;queued.key.scancode=SDL_SCANCODE_I;
    check(pending.items.handleEvent(queued,17),"Inventory opening was not captured");
    queued.key.scancode=SDL_SCANCODE_ESCAPE;
    check(pending.items.handleEvent(queued,17),"Escape leaked to application exit before inventory update");
    pending.tick();check(!pending.items.modal(),"Canceled pending inventory still opened");
    Session code; code.press(SDL_SCANCODE_E);
    code.press(SDL_SCANCODE_S); code.press(SDL_SCANCODE_D); code.press(SDL_SCANCODE_A); code.press(SDL_SCANCODE_W);
    check(code.items.interfaceState()->selection==0,"WASD code grid directions disagree");
    SDL_Event held{};held.type=SDL_EVENT_KEY_DOWN;held.key.windowID=17;held.key.scancode=SDL_SCANCODE_E;
    code.items.handleEvent(held,17); code.tick();
    code.items.handleEvent(held,17); code.tick(); // Duplicate non-repeat KEY_DOWN without KEY_UP.
    check(code.items.interfaceState()->entered=="A","Held E confirmed twice without a release");
    held.type=SDL_EVENT_KEY_UP;code.items.handleEvent(held,17);
    code.press(SDL_SCANCODE_E);check(code.items.interfaceState()->entered=="AA","Released E failed next press");
    Session opened; opened.items.handleEvent(held,17);
    held.type=SDL_EVENT_KEY_DOWN; opened.items.handleEvent(held,17);opened.tick();
    opened.items.handleEvent(held,17); opened.tick();
    check(opened.items.interfaceState()->entered.empty(),"Opening E also pressed a panel key");
    Session pickup;pickup.press(SDL_SCANCODE_E);pickup.code();
    pickup.press(SDL_SCANCODE_D);check(pickup.items.interfaceState()->selection==1,"D must select No on right");
    pickup.press(SDL_SCANCODE_E);check(pickup.player.inventory().injectors()[0]==-1,"E on No collected object");
    pickup.press(SDL_SCANCODE_E);pickup.press(SDL_SCANCODE_A);pickup.press(SDL_SCANCODE_E);
    check(pickup.player.inventory().injectors()==std::array<int,2>{100,-1},"E on Yes failed or duplicated pickup");
    Session priority;
    priority.doors[0].faces={{{{150,200},{150,0},23},{{150,0},{150,200},33}}};
    priority.press(SDL_SCANCODE_E);
    check(priority.items.modal() && priority.doors[0].state==doors::State::Closed,
          "One E opened a panel and also activated a door behind it");
    Session obsolete;obsolete.press(SDL_SCANCODE_G);
    check(!obsolete.items.modal(),"G still owns normal interaction");
}
void cardCountsAndLiftDirections()
{
    for(unsigned count=0;count<=3;++count) {
        Session reader(0x3308,0x0107);
        for(unsigned i=0;i<count;++i) {std::uint16_t card=std::uint16_t((3+i)*4096+1);reader.player.inventory().takeSupply(card);}
        reader.press(SDL_SCANCODE_E);
        const auto* state=reader.items.interfaceState();
        check(state && unsigned(std::count_if(state->cards.begin(),state->cards.end(),[](int c){return c>=0;}))==count,
              "Reader hid one/multiple owned cards");
        if(!count) {check(state->notice=="No security card","Empty reader warning missing");continue;}
        reader.press(SDL_SCANCODE_E);
        check(reader.items.interfaceState()->notice=="Incompatible card type","Single E001 was misreported as absent");
        if(count>1) {
            reader.press(SDL_SCANCODE_S);reader.press(SDL_SCANCODE_E);
            check(reader.doors[0].originalState==0,"C001 did not unlock after contextual rejection");
        }
    }
    Session onlyC(0x3308,0x0107);std::uint16_t card=0x4001;onlyC.player.inventory().takeSupply(card);
    onlyC.press(SDL_SCANCODE_E);onlyC.press(SDL_SCANCODE_E);
    check(onlyC.doors[0].originalState==0 && onlyC.player.inventory().cards()[0]==0x1001,
          "Exactly one C001 failed door7 or was consumed");
    Session lift(0x3002,1);card=0x3001;lift.player.inventory().takeSupply(card);
    lift.press(SDL_SCANCODE_E);lift.press(SDL_SCANCODE_E);
    for(unsigned row=0;row<4;++row) {
        check(lift.items.interfaceState()->selection==row,"W selected wrong elevator index");
        if(row<3) lift.press(SDL_SCANCODE_W);
    }
    lift.press(SDL_SCANCODE_W);check(lift.items.interfaceState()->selection==3,"Elevator exceeded top limit");
    for(int row=3;row>=0;--row) {
        check(lift.items.interfaceState()->selection==unsigned(row),"S selected wrong elevator index");
        if(row) lift.press(SDL_SCANCODE_S);
    }
    lift.press(SDL_SCANCODE_S);check(lift.items.interfaceState()->selection==0,"Elevator exceeded bottom limit");
    lift.press(SDL_SCANCODE_W);lift.press(SDL_SCANCODE_E);
    const auto travel=lift.items.takeTravel();
    check(travel && travel->zone==1,"Visible B101 row confirmed a different destination");
    auto resources=fixture(); const interaction::Renderer renderer(resources);
    std::vector<std::uint32_t> pixels(320*200);
    for(unsigned row=0;row<4;++row) {
        interaction::State state;state.screen=interaction::Screen::Elevator;state.selection=row;
        renderer.draw(pixels,state);
        check(pixels[(91-16*row)*320+136]==color(7),"Elevator logical index disagrees with hand position");
    }
}
void capacityAndPlan()
{
    Session s(0x3205,0x3002);
    for(std::uint16_t contents:{0x3001,0x4001,0x5001}) s.player.inventory().takeSupply(contents);
    s.press(SDL_SCANCODE_E); s.code(); s.press(SDL_SCANCODE_SPACE);
    check(s.items.interfaceState()->replacing,"Full card pocket did not request replacement choice");
    for(int i=0;i<3;++i) s.press(SDL_SCANCODE_DOWN);
    s.press(SDL_SCANCODE_SPACE);
    check(s.player.inventory().cards()[1]==0x1001,"Exit replaced a card");
    s.press(SDL_SCANCODE_E); s.press(SDL_SCANCODE_SPACE); s.press(SDL_SCANCODE_DOWN);
    s.press(SDL_SCANCODE_SPACE);
    check(s.player.inventory().cards()[1]==2,"Selected card slot not exchanged");
    s.press(SDL_SCANCODE_E);
    check(s.items.interfaceState()->contents==0x4001,"Replaced card not left in cabinet");
    Session plan(0x3606,0); plan.press(SDL_SCANCODE_E); plan.code();
    check(plan.items.interfaceState()->screen==interaction::Screen::Plan,"Initial panel not a plan transfer");
    for(int i=0;i<500 && plan.items.modal();++i) plan.tick();
    check(plan.items.planTransferred() && !plan.items.modal() && plan.player.hudState().injectors==0,
          "Plan transfer invented supply or failed to return to world");
    plan.items.load(plan.level);
    check(plan.items.planTransferred(),"Returning to zone lost transferred plan");
    plan.level.zoneIndex=1; plan.items.load(plan.level);
    check(!plan.items.planTransferred() && plan.items.planTransferred(0),"Plan leaked across zones");
    plan.level.zoneIndex=0; plan.items.load(plan.level);
    check(plan.items.planTransferred(),"Zone permission did not survive return");
    plan.items.resetPlans();
    check(!plan.items.planTransferred(),"New game retained plan-transfer state");
}
void pauseAndFailures()
{
    Session s;
    s.player.update(0,true,s.audio); s.player.update(1,true,s.audio);
    const int before=s.player.vitals().oxygen();
    s.press(SDL_SCANCODE_E);
    for(int i=0;i<3;++i) s.player.update(100,!s.items.modal(),s.audio);
    check(s.player.vitals().oxygen()==before,"Modal time consumed oxygen");
    s.press(SDL_SCANCODE_ESCAPE); s.player.update(100,true,s.audio);
    check(s.player.vitals().oxygen()==before,"Resumed game caught up modal elapsed time");
    s.press(SDL_SCANCODE_E); s.player.vitals().applyDamage(1000); s.tick();
    check(!s.items.modal() && s.player.hudState().injectors==0,"Death left interface active or granted contents");
    Session unavailable; unavailable.items.setAvailable(false); unavailable.press(SDL_SCANCODE_E);
    check(!unavailable.items.modal(),"Missing interface resources allowed invisible modal");
    Session unknown; unknown.level.accessCode.reset(); unknown.items.load(unknown.level);
    unknown.press(SDL_SCANCODE_E); unknown.code();
    check(unknown.items.interfaceState()->screen==interaction::Screen::Code,
          "Missing code data granted arbitrary access");
    Session paused; paused.press(SDL_SCANCODE_E);
    SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.windowID=17;e.key.scancode=SDL_SCANCODE_SPACE;
    paused.items.handleEvent(e,17); paused.tick(false); paused.tick();
    check(paused.items.interfaceState()->entered.empty(),"Unfocused confirmation leaked into resume");
    paused.items.handleEvent(e,17);
    SDL_Event focus{}; focus.type=SDL_EVENT_WINDOW_FOCUS_LOST; focus.window.windowID=17;
    paused.items.handleEvent(focus,17); paused.tick();
    check(paused.items.interfaceState()->entered.empty(),"Focus loss/regain leaked queued confirmation");
}
void codeTerminal()
{
    Session s(0x3508,1);
    s.player.postMessage(messages::Id::PortalLocked);
    std::uint16_t card=0x4001; s.player.inventory().takeSupply(card);
    s.press(SDL_SCANCODE_E);
    check(s.player.hudState().messages[0].empty(),"Old security alert remained frozen behind new modal");
    check(s.items.interfaceState()->screen==interaction::Screen::Cards,"Code reader skipped card selection");
    s.press(SDL_SCANCODE_SPACE);
    check(s.items.interfaceState()->screen==interaction::Screen::Codes &&
          s.items.interfaceState()->notice=="CODE ZONE C200  IJEKAH" && s.doors[0].originalState==0x400,
          "Code terminal opened a door or returned invented zone data");
    s.press(SDL_SCANCODE_SPACE);
    check(s.items.interfaceState()->notice=="CODE ZONE B101  JBHGAD","Original code-target list not browsable");
    s.press(SDL_SCANCODE_DOWN); s.press(SDL_SCANCODE_SPACE);
    check(!s.items.modal() && s.player.inventory().cards()[0]==0x1001,"Code reader consumed card or failed to exit");
    Session wrong(0x3508,1); card=0x4002; wrong.player.inventory().takeSupply(card);
    wrong.press(SDL_SCANCODE_E); wrong.press(SDL_SCANCODE_SPACE);
    check(wrong.items.interfaceState()->screen==interaction::Screen::Reader &&
          wrong.items.interfaceState()->notice=="CARD REFUSED...","Code terminal accepted wrong C identifier");
}
void sequenceTiming()
{
    using interaction::Phase;
    const auto raw=[](Session& s,SDL_Scancode key) {
        SDL_Event e{}; e.type=SDL_EVENT_KEY_DOWN; e.key.windowID=17; e.key.scancode=key;
        s.items.handleEvent(e,17); s.tick(); e.type=SDL_EVENT_KEY_UP; s.items.handleEvent(e,17);
    };
    const auto enter=[&](Session& s) {
        s.press(SDL_SCANCODE_E);
        for(char c:std::string("IJEKAH")) {s.select(unsigned(c-'A')+(c>='G'));s.press(SDL_SCANCODE_SPACE);}
        s.select(13);raw(s,SDL_SCANCODE_SPACE);
        check(s.items.interfaceState()->phase==Phase::CodeResult && s.items.interfaceState()->notice=="OK...",
              "Accepted code feedback skipped");
    };
    Session canceled(0x3606,0); enter(canceled); raw(canceled,SDL_SCANCODE_ESCAPE);
    check(!canceled.items.modal() && !canceled.items.planTransferred(),"Canceled code feedback granted plan");
    for(int rate:{30,60,120,160}) {
        Session s(0x3606,0); enter(s);
        for(unsigned i=0;i<300 && s.items.interfaceState()->phase!=Phase::Shutters;++i) s.tick();
        check(s.items.interfaceState()->phase==Phase::Shutters,"Plan never opened shutters");
        check(s.items.interfaceState()->transfer==100 && !s.items.planTransferred(),"Animation granted plan early");
        double time=0;
        while(time+1.0/rate<900.0/160-1e-8) {
            s.items.update(&s.camera,true,s.obstacles,s.player,s.audio,&s.control,1.0/rate); time+=1.0/rate;
            check(!s.items.planTransferred(),"Transfer completed before 900 DOS ticks");
        }
        s.items.update(&s.camera,true,s.obstacles,s.player,s.audio,&s.control,900.0/160-time);
        check(s.items.planTransferred() && s.items.modal() && s.items.interfaceState()->transfer==0,
              "Zero-count transfer did not grant exactly once before closing");
        for(unsigned i=0;i<30 && s.items.modal();++i) s.tick();
        check(!s.items.modal() && s.player.hudState().injectors==0,"Transfer failed to close or invented pickup");
    }
    Session opening(0x3606,0);raw(opening,SDL_SCANCODE_E);
    raw(opening,SDL_SCANCODE_SPACE);
    check(opening.items.interfaceState()->entered.empty(),"Opening confirmation leaked into keyboard");
    const auto frame=opening.items.interfaceState()->frame;
    opening.items.update(&opening.camera,false,opening.obstacles,opening.player,opening.audio,&opening.control,100);
    opening.items.update(&opening.camera,true,opening.obstacles,opening.player,opening.audio,&opening.control,100);
    check(opening.items.interfaceState()->frame==frame,"Unfocused animation caught up on resume");
    Session reader(0x3308,0x0107);std::uint16_t card=0x4001;reader.player.inventory().takeSupply(card);
    reader.press(SDL_SCANCODE_E);raw(reader,SDL_SCANCODE_SPACE);
    check(reader.doors[0].originalState==0x400,"Card granted access before insertion");
    for(unsigned i=0;i<200 && reader.items.interfaceState()->phase!=Phase::CardResult;++i) reader.tick();
    check(reader.doors[0].originalState==0 && reader.player.inventory().cards()[0]==0x1001,
          "Inserted card did not unlock once or was consumed");
    for(unsigned i=0;i<250 && reader.items.modal();++i) reader.tick();
    check(!reader.items.modal(),"Reader result failed to withdraw and close automatically");
}
void originals(const std::filesystem::path& directory)
{
    const auto resources=interaction::load(directory);
    const interaction::Renderer renderer(resources);
    std::filesystem::create_directories("build/panel-debug");
    unsigned i=0;
    for(auto screen:{interaction::Screen::Code,interaction::Screen::Supply,interaction::Screen::Weapon,
                     interaction::Screen::Cards,interaction::Screen::Reader,interaction::Screen::Plan,interaction::Screen::Codes,interaction::Screen::Inventory,interaction::Screen::Elevator}) {
        interaction::State state; state.screen=screen; state.contents=screen==interaction::Screen::Weapon?0x11f4:0x3001;
        state.cards={1,0x1001,-1};
        if(screen==interaction::Screen::Reader) state.notice="VALID CARD..... DOOR UNLOCKING!";
        if(screen==interaction::Screen::Codes) state.notice="CODE ZONE C200  IJEKAH";
        std::vector<std::uint32_t> pixels(320*200,0x121212ff);
        check(renderer.draw(pixels,state),"Original resource modal composition failed");
        auto* surface=SDL_CreateSurfaceFrom(320,200,SDL_PIXELFORMAT_RGBA8888,pixels.data(),320*4);
        check(surface!=nullptr,"Cannot create diagnostic modal surface");
        const auto path="build/panel-debug/interface-"+std::to_string(i++)+".bmp";
        const bool saved=SDL_SaveBMP(surface,path.c_str()); SDL_DestroySurface(surface);
        check(saved,"Cannot save modal diagnostic");
    }
    // Real overlays are darkening masks, not illuminated labels.
    for(const auto* bank:{&resources.supplies,&resources.weapons})
        for(unsigned index:{14u,15u})
            check(std::all_of((*bank)[index].indices.begin(),(*bank)[index].indices.end(),
                             [](auto c){return c==0 || c==220;}),"Original option overlay palette changed");
    const auto save=[&](const interaction::State& state,const std::string& name) {
        std::vector<std::uint32_t> pixels(320*200,0x121212ff);
        check(renderer.draw(pixels,state),"Variant composition failed");
        auto* surface=SDL_CreateSurfaceFrom(320,200,SDL_PIXELFORMAT_RGBA8888,pixels.data(),320*4);
        check(surface!=nullptr,"Cannot create variant surface");
        const bool saved=SDL_SaveBMP(surface,("build/panel-debug/"+name+".bmp").c_str());
        SDL_DestroySurface(surface); check(saved,"Cannot save variant");
    };
    for(unsigned selection=0;selection<2;++selection) {
        interaction::State state; state.screen=interaction::Screen::Supply; state.contents=0x4001;
        state.selection=selection; save(state,selection?"pickup-no":"pickup-yes");
    }
    for(unsigned phase=0;phase<4;++phase) {
        interaction::State state; state.selection=0; state.pressedKey=0; state.fingerPhase=phase;
        save(state,"finger-"+std::to_string(phase));
    }
    for(auto screen:{interaction::Screen::Plan,interaction::Screen::Supply,interaction::Screen::Weapon}) {
        for(auto phase:{interaction::Phase::PanelSlide,interaction::Phase::PanelZoom,interaction::Phase::Shutters,
                        interaction::Phase::ContentMove,interaction::Phase::ContentReveal}) {
            if(screen==interaction::Screen::Plan && (phase==interaction::Phase::ContentMove || phase==interaction::Phase::ContentReveal)) continue;
            interaction::State state;state.screen=screen;state.phase=phase;state.frame=10;
            state.contents=screen==interaction::Screen::Weapon?0x11f4:0x4001;
            save(state,"sequence-"+std::to_string(int(screen))+"-"+std::to_string(int(phase)));
        }
    }
    {
        interaction::State plan;plan.screen=interaction::Screen::Plan;
        std::vector<std::uint32_t> pixels(64000,0);renderer.draw(pixels,plan);
        const auto c=resources.palette[102];const auto ink=(std::uint32_t(c.r)<<24)|(std::uint32_t(c.g)<<16)|(std::uint32_t(c.b)<<8)|255;
        check(std::count(pixels.begin()+51*320+142,pixels.begin()+59*320,ink)>0,"Original TRANSFER text is invisible");
    }
    for(unsigned frame=0;frame<5;++frame) {
        interaction::State movie;movie.phase=interaction::Phase::Movie;movie.movieSeconds=frame*33.0/160;
        save(movie,"weapon-movie-"+std::to_string(frame));
    }
    interaction::State rejected; rejected.screen=interaction::Screen::Cards;
    rejected.cards={1,0x1001,0x2001}; rejected.notice="Incompatible card type";
    save(rejected,"cards-rejected");
    for(unsigned section=0;section<6;++section) {
        interaction::State state; state.screen=interaction::Screen::Inventory;
        state.selection=section; state.cards={1,0x1001,-1}; state.injectors={100,0}; state.oxygenReserve=144000;
        save(state,"inventory-"+std::to_string(section));
    }
    interaction::State lift; lift.screen=interaction::Screen::Elevator; lift.elevatorIndex=1;
    for(unsigned row=0;row<4;++row) { lift.selection=row; save(lift,"elevator-"+std::to_string(row)); }
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
    Session audible; audible.audio.load(directory); audible.press(SDL_SCANCODE_I);
    const auto motor=audible.audio.effectStatistics(11);
    check(motor && motor->requests==1,"Inventory opening did not request original BANK[11]");
    for(int frame=0;frame<100;++frame) audible.tick();
    check(audible.audio.effectStatistics(11)->requests==1,"Visible modal retriggered interface motor");
    audible.press(SDL_SCANCODE_ESCAPE);
    check(audible.audio.effectStatistics(11)->requests==2,"Inventory close motor missing or duplicated");
    std::cout<<"Original modal banks, PAL_DI, fonts and nine compositions verified.\n";
}
}
int main(int argc,char* argv[])
{
    try { graphics(); checksum(); flows(); selectionRegressions(); detectorInventory(); inventoryAndElevator(); originalTravel(); unifiedControls(); cardCountsAndLiftDirections(); capacityAndPlan(); pauseAndFailures(); codeTerminal(); sequenceTiming(); missingVariants(); if(argc==2) originals(argv[1]); if(argc==3 && std::string_view(argv[1])=="--lab") { originalTravelLab(argv[2]); allPanelsLab(argv[2]); }
        std::cout<<"Original interaction coordinates, bounds, resources and code format passed.\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
