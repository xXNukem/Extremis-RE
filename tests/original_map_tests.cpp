#include "original_map.h"
#include "map_session.h"
#include "item_session.h"
#include "survival_session.h"
#include "game_audio.h"
#include "enemy_system.h"
#include "first_person_view.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
template<class F> void rejects(F operation)
{
    bool failed=false;
    try { operation(); } catch (const std::runtime_error&) { failed=true; }
    check(failed,"Invalid resource accepted");
}
spr_format::Image sprite(int w,int h,unsigned color)
{
    return {w,h,0,0,std::vector<std::uint8_t>(w*h,std::uint8_t(color))};
}
std::uint32_t color(unsigned i) { return (i<<24)|255; }
original_map::Resources fixture()
{
    original_map::Resources r;
    for (unsigned i=0;i<30;++i) r.maps.push_back(sprite(89+i,69+i,i+1));
    r.compass.push_back(sprite(32,32,50));
    for (unsigned i=1;i<9;++i) r.compass.push_back(sprite(i%2?7:6,i%2?7:6,50+i));
    r.compass.push_back(sprite(3,3,60));
    r.font.assign(634,0); r.font[0]=3; r.font[1]=8;
    std::fill(r.font.begin()+2,r.font.begin()+98,255);
    r.font[2+'Z'-32]=0; r.font[98]=0x80;
    for (unsigned i=0;i<256;++i) r.palette[i]={std::uint8_t(i),0,0};
    return r;
}
void write(const std::filesystem::path& path,const std::vector<std::uint8_t>& bytes)
{
    std::ofstream stream(path,std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(stream),"Fixture write failed");
}
std::vector<std::uint8_t> encode(const std::vector<spr_format::Image>& images)
{
    std::vector<std::uint8_t> bytes(4+4*images.size());
    bytes[2]=images.size(); bytes[3]=images.size()>>8;
    const auto word=[&](unsigned v) { bytes.push_back(v&255); bytes.push_back(v>>8); };
    for (unsigned i=0;i<images.size();++i) {
        const unsigned offset=bytes.size()-4;
        for(unsigned j=0;j<4;++j) bytes[4+4*i+j]=(offset>>(8*j))&255;
        const auto& s=images[i]; word(s.width); word(s.height); word(s.anchorX); word(s.anchorY);
        bytes.insert(bytes.end(),s.indices.begin(),s.indices.end());
    }
    return bytes;
}
void loading(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
    const auto r=fixture(); auto encoded=encode(r.maps);
    write(path/"MAP.SPR",encoded); write(path/"BOUSSOLE.SPR",encode(r.compass));
    write(path/"TYPO_3X8.BIN",r.font); write(path/"PAL_DI.PAL",std::vector<std::uint8_t>(768,63));
    auto loaded=original_map::load(path);
    for(unsigned i=0;i<30;++i)
        check(loaded.maps[i].width==89+int(i) && loaded.maps[i].height==69+int(i) &&
              loaded.maps[i].indices==r.maps[i].indices,"SPR records reordered or corrupted");
    check(loaded.palette[0].r==255,"Six-bit palette conversion incorrect");
    encoded.pop_back(); write(path/"MAP.SPR",encoded);
    rejects([&]{original_map::load(path);});
    write(path/"MAP.SPR",encode(r.maps));
    rejects([&]{original_map::load(path/"missing");});
    auto bad=r; bad.maps.pop_back(); rejects([&]{original_map::Renderer renderer(bad);});
    bad=r; bad.compass.pop_back(); rejects([&]{original_map::Renderer renderer(bad);});
    bad=r; bad.maps[0].width=321; rejects([&]{original_map::Renderer renderer(bad);});
    bad=r; bad.maps[0].indices.pop_back(); rejects([&]{original_map::Renderer renderer(bad);});
    bad=r; bad.font[2]=254; rejects([&]{original_map::Renderer renderer(bad);});
}
void graphics()
{
    auto r=fixture(); r.maps[0].indices[0]=0; r.maps[0].anchorX=42;
    const original_map::Renderer renderer(r);
    constexpr double pi=3.14159265358979323846;
    const Camera camera{3500,320,pi/2};
    auto p=original_map::placement(89,69,camera);
    check(p && p->x==114 && p->y==64 && p->markerX==183 && p->markerY==125 && p->direction==2,
          "Native center, marker or radians/720 conversion differs");
    for(unsigned direction=0;direction<8;++direction) {
        p=original_map::placement(89,69,{200,200,direction*pi/4});
        check(p && p->direction==direction,"Compass direction differs");
    }
    check(original_map::placement(89,69,{199.999,200,0})->markerX==115 &&
          original_map::placement(89,69,{200,200,0})->markerX==119,"Cell boundary shifted");
    check(original_map::placement(89,69,{0,0,-pi/4})->direction==7 &&
          original_map::placement(89,69,{0,0,2*pi})->direction==0,"Angle wrap failed");
    check(!original_map::placement(89,69,{-1,0,0}) &&
          !original_map::placement(89,69,{0,0,std::numeric_limits<double>::quiet_NaN()}),"Invalid position accepted");
    constexpr int offsets[8][2]={{20,12},{18,8},{13,5},{8,8},{6,12},{8,18},{13,19},{20,18}};
    for(unsigned zone=0;zone<30;++zone) {
        std::vector<std::uint32_t> pixels(64000,0xffffffff);
        const unsigned direction=zone%8;
        original_map::State state{zone,{0,0,direction*pi/4},false};
        check(renderer.draw(pixels,state),"Map draw failed");
        const auto q=*original_map::placement(89+zone,69+zone,state.camera);
        check(pixels[q.y*320+q.x+1]==color(zone+1),"Wrong map record/palette");
        check(pixels[0]==color(224),"Original gray background mapping failed");
        if (!zone) check(pixels[q.y*320+q.x]==color(224),"Index zero or map pivot mishandled");
        check(pixels[16*320+8]==color(50),"Compass base not at 8,16");
        check(pixels[(16+offsets[direction][1])*320+8+offsets[direction][0]]==color(51+direction),
              "Wrong compass ID or position");
        check(pixels[52*320+6]==color(102),"Original label font/position missing");
    }
    std::vector<std::uint32_t> off(64000,0),on=off;
    renderer.draw(off,{0,camera,false}); renderer.draw(on,{0,camera,true});
    check(off[125*320+183]!=color(60) && on[125*320+183]==color(60),"Marker visibility failed");
    auto edge=on; check(renderer.draw(edge,{0,{65535,65535,0},true}),"Defensive clipping failed");
    for (unsigned size:{0u,63999u,64001u}) {
        std::vector<std::uint32_t> invalid(size,123),before=invalid;
        check(!renderer.draw(invalid,{}) && invalid==before,"Invalid framebuffer written");
    }
    auto before=on; check(!renderer.draw(on,{30,camera,true}) && on==before,"Invalid zone changed framebuffer");
}
SDL_Event key(SDL_Scancode code,bool down=true,std::uint32_t window=17)
{
    SDL_Event e{}; e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    e.key.scancode=code; e.key.windowID=window; return e;
}
void session()
{
    original_map::Session map; SurvivalSession player; GameAudio audio;
    Camera camera{3500,320,0}; map.beginFrame(0);
    const auto send=[&](SDL_Event e,bool permission,bool blocked=false,bool available=true,bool gameplay=true) {
        return map.handleEvent(e,17,&camera,gameplay,permission,blocked,available,player);
    };
    send(key(SDL_SCANCODE_M),false);
    check(!map.modal() && player.hudState().messages[0]==messages::text(messages::Id::MapUnavailable),"No-plan denial failed");
    send(key(SDL_SCANCODE_M,false),false);
    send(key(SDL_SCANCODE_M),true,true);
    check(!map.modal(),"Enemy block ignored"); send(key(SDL_SCANCODE_M,false),true);
    send(key(SDL_SCANCODE_M),true,false,false);
    check(!map.modal(),"Invisible map opened"); send(key(SDL_SCANCODE_M,false),true);
    send(key(SDL_SCANCODE_M),true,false,true,false);
    check(!map.modal(),"Map opened over another context"); send(key(SDL_SCANCODE_M,false),true);
    send(key(SDL_SCANCODE_M,true,99),true); check(!map.modal(),"Foreign window opened map");
    auto repeat=key(SDL_SCANCODE_M); repeat.key.repeat=true; send(repeat,true);
    check(!map.modal(),"Key repeat opened map"); send(key(SDL_SCANCODE_M,false),true);
    // No battery and no cards: the transferred plan alone grants access.
    send(key(SDL_SCANCODE_SEMICOLON),true);
    check(map.modal() && !map.state()->markerVisible && player.inventory().batteryCharge()==-1,
          "Native physical key or access changed inventory");
    check(send(key(SDL_SCANCODE_TAB),true) && send(key(SDL_SCANCODE_ESCAPE),true) && map.modal(),
          "Unconfirmed close control or debug toggle leaked through map");
    map.beginFrame(0); map.update(20./160,true); check(!map.state()->markerVisible,"Blink used >=20 instead of >20");
    map.update(1./160,true); check(map.state()->markerVisible,"21st tick did not toggle marker");
    map.update(21./160,true); check(!map.state()->markerVisible,"Second blink did not restore");
    map.update(21./160,false); check(!map.state()->markerVisible,"Unfocused map clock advanced");
    const int health=player.vitals().health(), ammunition=player.inventory().ammunition();
    std::uint16_t battery=900; player.inventory().takeSupply(battery); player.inventory().toggleDetector();
    player.update(0,true,audio); const int oxygen=player.vitals().oxygen(), charge=player.inventory().batteryCharge();
    player.update(60,!map.blocksSimulation(),audio);
    check(player.vitals().oxygen()==oxygen && player.inventory().batteryCharge()==charge &&
          player.vitals().health()==health && player.inventory().ammunition()==ammunition,"Map did not pause vitals/inventory");
    camera.x=9999; check(map.state()->camera.x==3500,"Map did not snapshot camera");
    send(key(SDL_SCANCODE_SPACE),true); check(!map.modal() && map.blocksSimulation(),"Close transition not isolated");
    check(send(key(SDL_SCANCODE_E),true),"Close-frame interaction leaked");
    player.update(60,!map.blocksSimulation(),audio);
    map.beginFrame(0); player.update(60,!map.blocksSimulation(),audio);
    check(player.vitals().oxygen()==oxygen && player.inventory().batteryCharge()==charge,"Resume charged paused interval");
    for(unsigned i=0;i<5;++i) {
        send(key(SDL_SCANCODE_SPACE,false),true); send(key(SDL_SCANCODE_M,false),true);
        send(key(SDL_SCANCODE_M),true); check(map.modal(),"Reopening failed");
        send(key(SDL_SCANCODE_SPACE),true); map.beginFrame(0);
    }
    send(key(SDL_SCANCODE_M,false),true); send(key(SDL_SCANCODE_M),true);
    map.beginFrame(1); check(!map.modal() && map.blocksSimulation(),"Level change retained old map");
    // Equal elapsed time at distinct rendering rates has the same marker phase.
    for(unsigned rate:{30u,60u,120u}) {
        original_map::Session clock; clock.beginFrame(0);
        clock.handleEvent(key(SDL_SCANCODE_M),17,&camera,true,true,false,true,player);
        clock.beginFrame(0);
        for(unsigned frame=0;frame<rate;++frame) clock.update(1./rate,true);
        check(clock.state()->markerVisible,"Blink depends on render FPS");
    }
}
void transfer(ItemSession& items, SurvivalSession& player, GameAudio& audio,
              const Camera& camera,const collision::World& walls)
{
    const auto tick=[&]{items.update(&camera,true,walls,player,audio);};
    const auto settle=[&]{for(unsigned i=0;i<600 && items.modal() && items.interfaceState()->phase!=interaction::Phase::Ready;++i) tick();};
    const auto press=[&](SDL_Scancode c){items.handleEvent(key(c),17);tick();items.handleEvent(key(c,false),17);settle();};
    press(SDL_SCANCODE_E);
    check(items.interfaceState() && items.interfaceState()->screen==interaction::Screen::Code,"Plan terminal not found");
    for (char c:std::string("IJEKAH")+'!') {
        const unsigned target=c=='!'?13:unsigned(c-'A')+(c>='G');
        while(items.interfaceState()->selection/7>target/7) press(SDL_SCANCODE_UP);
        while(items.interfaceState()->selection/7<target/7) press(SDL_SCANCODE_DOWN);
        while(items.interfaceState()->selection%7>target%7) press(SDL_SCANCODE_LEFT);
        while(items.interfaceState()->selection%7<target%7) press(SDL_SCANCODE_RIGHT);
        press(SDL_SCANCODE_SPACE);
    }
    check(!items.planTransferred(),"Code acceptance granted plan before transfer completed");
    for(unsigned i=0;i<500 && items.modal();++i) tick();
    check(items.planTransferred() && !items.modal(),"Plan transfer did not complete");
}
void permissions()
{
    game_lab::Level level; level.accessCode=90;
    game_lab::Sector sector{}; sector.vertices={{100,200},{100,0}}; sector.walls={{0,1,0x3606,0}};
    level.sectors.push_back(sector);
    ItemSession items; SurvivalSession player; GameAudio audio; items.load(level);
    check(!items.planTransferred(),"New session already has plan");
    transfer(items,player,audio,{50,100,0},{});
    level.zoneIndex=29; items.load(level);
    check(!items.planTransferred() && items.planTransferred(0),"Zone permissions shared");
    transfer(items,player,audio,{50,100,0},{});
    level.zoneIndex=0; items.load(level);
    check(items.planTransferred() && items.planTransferred(29) && !items.planTransferred(1),"Zone return lost permission");
    items.resetPlans(); check(!items.planTransferred(0) && !items.planTransferred(29),"New-game plan reset failed");
    level.zoneIndex=30; rejects([&]{items.load(level);});
}
std::vector<std::uint8_t> surfaceBytes(SDL_Surface* surface)
{
    const auto* bytes=static_cast<const std::uint8_t*>(surface->pixels);
    return {bytes,bytes+surface->pitch*surface->h};
}
void originals(const std::filesystem::path& directory)
{
    const auto resources=original_map::load(directory);
    const auto decoded=spr_format::load(directory/"MAP.SPR");
    constexpr int sizes[30][2]={{89,69},{97,73},{97,93},{57,81},{57,85},{65,109},{77,77},{85,97},
        {73,101},{125,97},{81,53},{145,113},{117,113},{101,97},{69,93},{77,157},{109,57},{113,49},
        {89,57},{33,21},{133,49},{233,157},{181,81},{69,69},{49,81},{109,113},{113,73},{105,81},{77,85},{93,93}};
    for(unsigned i=0;i<30;++i)
        check(resources.maps[i].width==sizes[i][0] && resources.maps[i].height==sizes[i][1] &&
              resources.maps[i].indices==decoded[i].indices,"Original MAP records differ");
    const original_map::Renderer map(resources);
    const auto level=game_lab::loadLevel1(directory.parent_path()/"GAME.LAB").level;
    ItemSession items; SurvivalSession player; GameAudio audio; items.load(level);
    const auto walls=collision::staticWorld(level);
    transfer(items,player,audio,{3250,1100,3.14159265358979323846},walls);
    const auto camera=initialCamera(level);
    original_map::Session session; session.beginFrame(level.zoneIndex);
    session.handleEvent(key(SDL_SCANCODE_M),17,&camera,true,items.planTransferred(),false,true,player);
    check(session.modal(),"Real transfer did not unlock playable map");
    enemies::System enemies; enemies.load(level,directory);
    check(!enemies.blocksInterface(level,camera),"Initial location incorrectly blocked by alien");
    enemies.update(level,camera,walls,player.vitals(),0,true);
    const auto beforeTicks=enemies.simulationTicks();
    enemies.update(level,camera,walls,player.vitals(),10,!session.blocksSimulation());
    check(enemies.simulationTicks()==beforeTicks,"Map advanced enemy simulation");
    const hud::Renderer hud(hud::load(directory));
    const auto materials=wall_textures::load(directory,1,1);
    const auto planes=wall_textures::loadPlanes(directory,1,1);
    const auto scene=sprites::loadScene(level,directory);
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_CreateSurface(320,240,SDL_PIXELFORMAT_RGBA8888),SDL_DestroySurface);
    check(bool(surface),"SDL surface failed");
    std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> renderer(SDL_CreateSoftwareRenderer(surface.get()),SDL_DestroyRenderer);
    check(bool(renderer),"Software renderer failed");
    FirstPersonRenderer view(renderer.get());
    const auto draw=[&](const original_map::State* state) {
        check(view.draw(level,camera,nullptr,&materials,nullptr,&planes,&scene,&hud,player.hudState(),
            nullptr,nullptr,nullptr,&enemies.scene(),nullptr,&map,state) && SDL_RenderPresent(renderer.get()),"Map framebuffer integration failed");
    };
    draw(nullptr); const auto normal=surfaceBytes(surface.get());
    draw(session.state()); const auto firstMap=surfaceBytes(surface.get());
    check(firstMap!=normal,"Map did not replace gameplay presentation");
    draw(session.state()); check(surfaceBytes(surface.get())==firstMap,"Map backdrop accumulated conversion");
    std::filesystem::create_directories("build/map-debug");
    check(SDL_SaveBMP(surface.get(),"build/map-debug/level1-map.bmp"),"Cannot save map preview");
    session.beginFrame(0); session.update(21./160,true); draw(session.state());
    check(SDL_SaveBMP(surface.get(),"build/map-debug/level1-marker.bmp"),"Cannot save marker preview");
    draw(nullptr); check(surfaceBytes(surface.get())==normal,"Closing map did not restore HUD/scene/palette");
    draw(session.state()); check(surfaceBytes(surface.get())!=normal,"Reopened map missing");
    draw(nullptr); check(surfaceBytes(surface.get())==normal,"Repeated close left map pixels");
    std::cout<<"30 original maps, real IJEKAH transfer, modal enemies and SDL palette/HUD restoration passed\n";
}
}
int main(int argc,char** argv)
{
    try {
        check(argc>=2,"Fixture output directory required");
        loading(argv[1]); graphics(); session(); permissions();
        if(argc>2) originals(argv[2]);
        std::cout<<"Original map resources, geometry, permissions and modal clock passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
