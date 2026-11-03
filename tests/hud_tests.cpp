#include "original_hud.h"
#include "weapon_fire.h"
#include "enemy_system.h"
#include "first_person_view.h"
#include "sprite_scene.h"
#include "player_vitals.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

hud::Resources fixture()
{
    hud::Resources r{};
    constexpr int sizes[][2] = {
        {1,150},{320,50},{60,10},{60,10},{46,29},{46,29},{46,29},
        {46,29},{46,29},{46,29},{46,29},{46,29},{46,29},
        {5,5},{3,3},{1,1},{43,10},{43,10},{43,10},{31,41},
        {9,19},{9,19},{11,21},{11,21},{11,15},{13,9}};
    for (int i = 0; i < 86; ++i) {
        const int w = i < 26 ? sizes[i][0] : i < 36 ? 3 : 5;
        const int h = i < 26 ? sizes[i][1] : i < 36 ? 6 : 10;
        r.casque.push_back({w,h,0,0,std::vector<std::uint8_t>(w*h,std::uint8_t(i+1))});
    }
    for (int i = 0; i < 256; ++i) r.palette[i] = {std::uint8_t(i),0,0};
    r.casque[36] = {18,38,0,0,std::vector<std::uint8_t>(18*38,37)};
    r.casque[37] = {7,10,0,0,std::vector<std::uint8_t>(7*10,38)};
    r.casque[38] = {5,17,0,0,std::vector<std::uint8_t>(5*17,39)};
    for (int i = 0; i < 6; ++i)
        r.weaponIcons[i] = {57,30,0,0,std::vector<std::uint8_t>(57*30,std::uint8_t(100+i))};
    constexpr int views[6][2]{{25,28},{33,37},{26,34},{27,50},{46,33},{25,47}};
    for (unsigned i=0;i<6;++i)
        r.weaponViews[i]={views[i][0],views[i][1],0,0,
            std::vector<std::uint8_t>(views[i][0]*views[i][1],std::uint8_t(110+i))};
    constexpr int effects[6][4]{{14,11,21,21},{13,12,21,21},{13,13,27,27},
        {14,21,21,21},{28,18,37,37},{15,25,25,24}};
    for (unsigned id=0;id<6;++id) for(unsigned i=0;i<2;++i) {
        const int w=effects[id][i*2], h=effects[id][i*2+1];
        r.weaponEffects[id][i]={w,h,0,0,std::vector<std::uint8_t>(w*h,std::uint8_t(120+id*2+i))};
    }
    return r;
}

std::uint32_t color(unsigned index) { return (index<<24) | 255; }

void radarComposition()
{
    const hud::Renderer renderer(fixture()); hud::State state;
    state.detector.phase=4; state.detector.count=1; state.detector.contacts[0]={24,188,0};
    std::vector<std::uint32_t> pixels(320*200,0);
    check(renderer.draw(pixels,state),"Detector HUD composition failed");
    check(pixels[188*320+24]==color(14),"Native5x5 detector marker not at its original coordinates");
    state.detector.contacts[0]={-3,199,0};
    renderer.draw(pixels,state);
    check(pixels[199*320]==color(14),"Radar clipping lost partially visible marker");
    state.detector.phase.reset(); state.detector.count=10;
    renderer.draw(pixels,state);
    check(pixels[188*320+24]==color(5),"Off radar retained a stale detection");
}
void slimeComposition()
{
    auto resources=fixture();
    for(unsigned i=0;i<16;++i) resources.slime.push_back({5,51,0,0,std::vector<std::uint8_t>(255,200+i)});
    const hud::Renderer renderer(resources); hud::State state;
    std::vector<std::uint32_t> normal(320*200,0x123456ff);
    check(renderer.draw(normal,state),"Reference HUD failed");
    for(unsigned phase=0;phase<30;++phase) {
        state.slimePhase=phase; auto frame=std::vector<std::uint32_t>(320*200,0x123456ff);
        check(renderer.draw(frame,state),"Slime HUD failed");
        check(std::equal(frame.begin()+320*150,frame.end(),normal.begin()+320*150),"Slime touched bottom HUD");
        if(phase==0) check(frame[77*320+7]==color(208),"Original first GOUTTE position/parity differs");
    }
    state.slimePhase=30; auto clean=std::vector<std::uint32_t>(320*200,0x123456ff);
    renderer.draw(clean,state); check(clean==normal,"Finished slime left framebuffer residue");
}
void weaponViews()
{
    auto r=fixture();
    r.weaponViews[0].indices[0]=0;
    const hud::Renderer renderer(r);
    constexpr int pos[6][2]{{148,122},{144,113},{147,116},{147,100},{137,117},{148,103}};
    for(unsigned id=0;id<6;++id) {
        hud::State state; state.weapon=id;
        std::vector<std::uint32_t> pixels(320*200,0xabcdef12);
        check(renderer.draw(pixels,state),"Idle weapon composition failed");
        const int x=pos[id][0], y=pos[id][1];
        check(pixels[y*320+x+1]==color(110+id),"Idle weapon uses wrong frame/position/palette");
        check(pixels[(y-1)*320+x]==0xabcdef12,"Idle weapon moved above original rectangle");
        if(id==0) check(pixels[y*320+x]==0xabcdef12,"Transparent weapon pixel destroyed scene");
        check(pixels[75*320+160]==color(121+id*2),"Idle reticle missing");
        state.health=0; pixels.assign(320*200,0xabcdef12); renderer.draw(pixels,state);
        check(pixels[y*320+x+1]==0xabcdef12,"Dead player retained first-person weapon");
    }
}
void firingComposition()
{
    const hud::Renderer renderer(fixture());
    for(unsigned id=0;id<6;++id) for(unsigned phase=0;phase<4;++phase) {
        const auto& d=player::weaponDefinitions[id];
        hud::State state;state.weapon=id;state.firing=true;state.barrelPhase=phase;
        std::vector<std::uint32_t> pixels(320*200,0xabcdef12);
        check(renderer.draw(pixels,state),"Weapon fire composition failed");
        const auto offset=d.muzzle[d.alternating?(phase==3?1:phase):0];
        check(pixels[75*320+160]==color(121+id*2),"Optical-axis reticle missing during firing");
        check(pixels[(d.gunY+5)*320+d.gunX+offset[0]]==color(d.alternating?110+id:120+id*2),
              "Muzzle/gun overlap order differs from original alternating flag");
        check(pixels[150*320+160]==color(2),"Firing animation overwrote HUD panel");
    }
}
void composition()
{
    auto resources = fixture();
    resources.casque[42].indices[1] = 0; // Transparent A pixel must preserve the scene.
    const hud::Renderer renderer(resources);
    hud::State state;
    std::vector<std::uint32_t> pixels(320*200,0xabcdef12);
    check(renderer.draw(pixels,state), "HUD rejected complete framebuffer");
    check(pixels[150*320+160] == color(2) && pixels[149*320+160] == 0xabcdef12,
          "Panel escaped reserved rows or scene was cleared");
    check(pixels[162*320+68] == color(18) && pixels[178*320+68] == color(17),
          "Oxygen/health position or graphic swapped");
    check(pixels[155*320+125] == color(20) && pixels[171*320] == color(5) &&
          pixels[155*320+183] == color(0), "Body, radar or inventory rectangle shifted");
    check(pixels[0] == color(3) && pixels[319] == color(3) &&
          pixels[9*320+3] == color(3) && pixels[9*320+4] == 0xabcdef12 &&
          pixels[140*320+3] == color(4) && pixels[140*320+4] == 0xabcdef12 &&
          pixels[149*320+59] == color(4) && pixels[149*320+60] == 0xabcdef12,
          "Mirrored helmet corners or row-prefix limits differ");
    state.oxygen = 144000; state.health = 500;
    renderer.draw(pixels,state);
    for (int x = 0; x < 43; ++x) {
        check(pixels[162*320+68+x] == color(x < 21 ? 19 : 18) &&
              pixels[178*320+68+x] == color(x < 21 ? 19 : 17),
              "Original empty-left indicator direction/rounding changed");
    }
    state.health = INT_MIN; state.oxygen = INT_MAX;
    renderer.draw(pixels,state);
    check(pixels[178*320+110] == color(19) && pixels[162*320+68] == color(18),
          "Indicator bounds overflowed or did not clamp");
    state.weapon = 0; state.ammunition = 123;
    renderer.draw(pixels,state);
    check(pixels[170*320+263] == color(100) && pixels[188*320+265] == color(28) &&
          pixels[188*320+269] == color(29) && pixels[188*320+273] == color(30),
          "Weapon icon, decimal glyphs or original number coordinates differ");
    state.weapon = UINT_MAX; state.detector.phase = UINT_MAX;
    state.message = "A\r\nB";
    renderer.draw(pixels,state);
    check(pixels[170*320+263] == color(0) && pixels[171*320] == color(13),
          "Invalid visual selection read outside assets");
    check(pixels[16*320+8] == color(43) && pixels[16*320+9] == 0xabcdef12 &&
          pixels[27*320+8] == color(44), "Font mapping, transparency or CR/LF differs");
    std::string longText(100000,'A');
    state.message = longText;
    renderer.draw(pixels,state);
    check(pixels[150*320+160] == color(2), "Message overwrote the panel");
    std::vector<std::uint32_t> wrong(320*200+1,123);
    check(!renderer.draw(wrong,state) && std::all_of(wrong.begin(),wrong.end(),[](auto p){return p==123;}),
          "Invalid framebuffer was written");
    state = {};
    renderer.draw(pixels,state);
    check(pixels[178*320+68] == color(17), "Indicator did not recover on next composition");
    state.oxygenBottle = true; state.cards = 1; state.injectors = 1;
    renderer.draw(pixels,state);
    check(pixels[157*320+184] == color(37) && pixels[171*320+205] == color(38) &&
          pixels[168*320+215] == color(39),"Single equipment coordinates differ from EXE");
    state.cards = 2; state.injectors = 2;
    renderer.draw(pixels,state);
    check(pixels[161*320+205] == color(38) && pixels[181*320+205] == color(38) &&
          pixels[157*320+215] == color(39) && pixels[178*320+215] == color(39),
          "Multiple equipment coordinates differ from EXE tables");
    state.cards = UINT_MAX; state.injectors = UINT_MAX;
    renderer.draw(pixels,state);
    check(pixels[157*320+205] == color(38) && pixels[171*320+205] == color(38) &&
          pixels[185*320+205] == color(38),"Card count did not clamp to original three slots");
    state = {};
    renderer.draw(pixels,state);
    check(pixels[157*320+184] == color(0) && pixels[171*320+205] == color(0) &&
          pixels[178*320+215] == color(0),"Removed inventory left stale HUD icons");
    state.messages[0] = "A\r\nB"; state.messages[1] = "C";
    renderer.draw(pixels,state);
    check(pixels[16*320+8] == color(43) && pixels[27*320+8] == color(44) &&
          pixels[41*320+8] == color(45),"Queued messages lost original CR and 14-row separation");
}

void validation()
{
    for (int kind = 0; kind < 5; ++kind) {
        auto r = fixture();
        if (kind == 0) r.casque.pop_back();
        if (kind == 1) r.casque[1].width = 319;
        if (kind == 2) r.casque[42].indices.pop_back();
        if (kind == 3) r.weaponIcons[0].height = 31;
        if (kind == 4) r.casque[36].height = 37;
        bool failed = false;
        try { hud::Renderer invalid(std::move(r)); } catch (const std::runtime_error&) { failed = true; }
        check(failed,"Malformed HUD assets accepted");
    }
    bool failed = false;
    try { hud::load("/nonexistent-inextremis-hud-data"); }
    catch (const std::runtime_error&) { failed = true; }
    check(failed,"Missing HUD assets accepted");
}

void realVitalsBars()
{
    player::Vitals vitals;
    vitals.update(60,true);
    vitals.applyDamage(500);
    hud::State state;
    state.health = vitals.health();
    state.oxygen = vitals.oxygen();
    const hud::Renderer renderer(fixture());
    std::vector<std::uint32_t> pixels(320*200,0);
    check(renderer.draw(pixels,state),"Real vitals HUD composition failed");
    check(pixels[162*320+68] == color(19) && pixels[162*320+69] == color(18) &&
          pixels[178*320+88] == color(19) && pixels[178*320+89] == color(17),
          "Live internal units do not match original bar quantization");
    vitals.update(1800,true);
    state.oxygen = vitals.oxygen();
    renderer.draw(pixels,state);
    check(pixels[162*320+110] == color(19),"Oxygen death did not empty entire HUD bar");
}

void presentation()
{
    for (const auto size : {SDL_Point{320,200},{960,600},{1920,1080},{600,960},{1,1}}) {
        const auto p = hud::presentation(size.x,size.y);
        check(p.x >= 0 && p.y >= 0 && p.x+p.width <= size.x+0.001 &&
              p.y+p.height <= size.y+0.001 && std::abs(p.width/p.height-4.0f/3) < 1e-5,
              "HUD presentation stretches aspect or exceeds output");
    }
    check(hud::presentation(0,600).width == 0,"Zero-size output not handled");
}

using Surface = std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
using SdlRenderer = std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;

void integration()
{
    const hud::Renderer hudRenderer(fixture());
    for (const auto size : {SDL_Point{320,240},SDL_Point{960,600},SDL_Point{300,500}}) {
        Surface surface(SDL_CreateSurface(size.x,size.y,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
        SdlRenderer renderer(SDL_CreateSoftwareRenderer(surface.get()),SDL_DestroyRenderer);
        check(bool(surface) && bool(renderer),"Cannot create software HUD test target");
        FirstPersonRenderer view(renderer.get());
        game_lab::Level level{};
        const wall_textures::PlaneBank planes{{1,1,{{10,20,30}}},{1,1,{{40,50,60}}}};
        const sprites::Scene scene{};
        SDL_SetRenderDrawColor(renderer.get(),0,0,0,255);
        SDL_RenderClear(renderer.get());
        check(view.draw(level,{},nullptr,nullptr,nullptr,&planes,&scene,&hudRenderer) &&
              SDL_RenderPresent(renderer.get()),"3D/HUD composition failed");
        Uint8 r,g,b,a;
        const auto fit = hud::presentation(size.x,size.y);
        const int x = int(fit.x+fit.width*160/320), y = int(fit.y+fit.height*150.5/200);
        check(SDL_ReadSurfacePixel(surface.get(),x,y,&r,&g,&b,&a) && r == 2 && g == 0,
              "HUD panel moved during SDL scaling");
        // Exercise texture and framebuffer replacement in both directions.
        check(view.draw(level,{},nullptr,nullptr,nullptr,&planes) &&
              view.draw(level,{},nullptr,nullptr,nullptr,&planes,&scene,&hudRenderer),
              "Switching full-window/helmet framebuffers failed");
    }
}

std::vector<std::uint8_t> bytes(const std::filesystem::path& path)
{
    std::ifstream file(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}

void originals(const std::filesystem::path& directory)
{
    const auto resources = hud::load(directory);
    const auto raw = bytes(directory/"CASQUE.SPR");
    for (std::size_t i = 0; i < resources.casque.size(); ++i) {
        const auto p = 4+4*i;
        const std::size_t start = 4 + (unsigned(raw[p]) | (unsigned(raw[p+1])<<8) |
                                      (unsigned(raw[p+2])<<16) | (unsigned(raw[p+3])<<24));
        check(std::equal(resources.casque[i].indices.begin(), resources.casque[i].indices.end(),
                         raw.begin()+start+8),"Original HUD decoded pixels changed");
    }
    const auto palette = bytes(directory/"PAL1.PAL");
    for (std::size_t i = 0; i < 256; ++i)
        check(resources.palette[i].r == (palette[3*i]*255+31)/63 &&
              resources.palette[i].g == (palette[3*i+1]*255+31)/63 &&
              resources.palette[i].b == (palette[3*i+2]*255+31)/63,"HUD palette differs from PAL1");
    const hud::Renderer hudRenderer(resources);
    const auto level = game_lab::loadLevel1(directory.parent_path()/"GAME.LAB").level;
    const auto materials = wall_textures::load(directory,1,1);
    const auto planes = wall_textures::loadPlanes(directory,1,1);
    const auto scene = sprites::loadScene(level,directory);
    const auto doors = doors::closedDoors(level);
    Surface surface(SDL_CreateSurface(960,720,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
    SdlRenderer renderer(SDL_CreateSoftwareRenderer(surface.get()),SDL_DestroyRenderer);
    FirstPersonRenderer view(renderer.get());
    hud::State preview; preview.weapon = 0;
    check(view.draw(level,initialCamera(level),nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview)
          && SDL_RenderPresent(renderer.get()),"Original level/HUD integration failed");
    std::filesystem::create_directories("build/hud-debug");
    check(SDL_SaveBMP(surface.get(),"build/hud-debug/level1-hud.bmp"),"Cannot save diagnostic HUD frame");
    for(unsigned id=0;id<6;++id) {
        preview.weapon=id;
        check(view.draw(level,initialCamera(level),nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview)
              && SDL_RenderPresent(renderer.get()),"Original idle weapon/3D composition failed");
        check(SDL_SaveBMP(surface.get(),("build/hud-debug/step201-weapon-"+std::to_string(id)+".bmp").c_str()),
              "Cannot save original idle weapon composition");
    }
    for(unsigned id=0;id<6;++id) for(unsigned phase=0;phase<(id==2?4u:1u);++phase) {
        preview.weapon=id; preview.firing=true; preview.barrelPhase=phase;
        check(view.draw(level,initialCamera(level),nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview)
              && SDL_RenderPresent(renderer.get()),"Original firing layers/scene/HUD composition failed");
        check(SDL_SaveBMP(surface.get(),("build/hud-debug/fire-"+std::to_string(id)+"-"+std::to_string(phase)+".bmp").c_str()),
              "Cannot save original firing composition");
    }
    // Exercise the new projectile Scene through the existing depth/sprite renderer.
    player::WeaponFire fire; fire.loadGraphics(directory,1);
    player::Inventory ammo; auto contents=std::uint16_t(0x400a); ammo.exchangeWeapon(contents);
    fire.update({}, {},ammo,{true,true},true,0); fire.update({}, {},ammo,{},true,0);
    preview.weapon=4; preview.firing=false;
    const game_lab::Level empty{};
    const Camera lookingAside{0,0,0.25};
    check(view.draw(empty,lookingAside,nullptr,nullptr,nullptr,&planes,nullptr,&hudRenderer,preview)
          && SDL_RenderPresent(renderer.get()),"Projectile reference render failed");
    const auto* before=static_cast<const std::uint8_t*>(surface->pixels);
    const std::vector<std::uint8_t> without(before,before+surface->pitch*surface->h);
    check(view.draw(empty,lookingAside,nullptr,nullptr,nullptr,&planes,nullptr,&hudRenderer,preview,
                    nullptr,nullptr,&fire.scene()) && SDL_RenderPresent(renderer.get()),"Projectile Scene render failed");
    const auto* after=static_cast<const std::uint8_t*>(surface->pixels);
    check(!std::equal(without.begin(),without.end(),after),"Original projectile pixels were not composed");
    check(SDL_SaveBMP(surface.get(),"build/hud-debug/fire-projectile.bmp"),"Cannot save projectile composition");
    // Native MONS1 scenes pass through the same production compositor/depth buffer.
    enemies::System enemies; enemies.load(level,directory);
    player::Vitals vitals;
    const Camera combatView{4100,2530,-1.5707963267948966};
    const auto world=collision::staticWorld(level);
    preview.weapon=0; preview.firing=false;
    enemies.update(level,combatView,world,vitals,0,true);
    for(unsigned stage=0;stage<5;++stage) {
        if(stage==1) for(unsigned tick=0;tick<4;++tick) enemies.update(level,combatView,world,vitals,1./18,true);
        if(stage==2) { enemies.hit(0,0,0); enemies.update(level,combatView,world,vitals,1./18,true); }
        if(stage==3) { enemies.hit(0,4,0); enemies.update(level,combatView,world,vitals,1./18,true); }
        if(stage==4) for(unsigned tick=0;tick<8;++tick) enemies.update(level,combatView,world,vitals,1./18,true);
        check(view.draw(level,combatView,nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview,
                        nullptr,nullptr,nullptr,&enemies.scene()) && SDL_RenderPresent(renderer.get()),
              "Native enemy state composition failed");
        check(SDL_SaveBMP(surface.get(),("build/hud-debug/step203-enemy-"+std::to_string(stage)+".bmp").c_str()),
              "Cannot save native enemy state composition");
    }
    // Paso204: full original death/GOUTTE composition and detector through walls.
    enemies.load(level,directory); vitals.reset();
    const Camera deathView{4100,2400,-1.5707963267948966};
    enemies.hit(0,4,0); enemies.update(level,deathView,world,vitals,0,true);
    for(unsigned tick=0;tick<8;++tick) enemies.update(level,deathView,world,vitals,1./18,true);
    preview.batteryCharge=144000; preview.detectorEnabled=true;
    for(unsigned stage=0;stage<=42;++stage) {
        if(stage) enemies.update(level,deathView,world,vitals,1./18,true);
        preview.slimePhase=enemies.effects().slimePhase();
        preview.detector=detector::scan(level,enemies.records(),deathView,enemies.simulationTicks(),true);
        if(stage==0 || stage==1 || stage==2 || stage==9 || stage==10 || stage==14 || stage==25 || stage==42) {
            check(view.draw(level,deathView,nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview,
                            nullptr,nullptr,nullptr,&enemies.scene(),&enemies.effects().objects()) && SDL_RenderPresent(renderer.get()),
                  "Original explosion/slime/radar composition failed");
            check(SDL_SaveBMP(surface.get(),("build/hud-debug/step204-death-"+std::to_string(stage)+".bmp").c_str()),
                  "Cannot save original204 effects");
        }
    }
    enemies.load(level,directory);
    preview.slimePhase.reset(); preview.detector=detector::scan(level,enemies.records(),initialCamera(level),3,true);
    check(view.draw(level,initialCamera(level),nullptr,&materials,&doors,&planes,&scene,&hudRenderer,preview,
                    nullptr,nullptr,nullptr,&enemies.scene()) && SDL_RenderPresent(renderer.get()),"Native detector level view failed");
    check(SDL_SaveBMP(surface.get(),"build/hud-debug/step204-radar.bmp"),"Cannot save native radar");
    preview.detector={}; preview.batteryCharge=-1; preview.detectorEnabled=false;
    player::WeaponFire muzzle; muzzle.loadGraphics(directory,1); player::Inventory muzzleAmmo;
    const collision::World target{{{800,-1000},{800,1000}}};
    for(unsigned stage=0;stage<6;++stage) {
        muzzle.update({},target,muzzleAmmo,stage==0?player::FireTrigger{true,true}:player::FireTrigger{},true,0);
        preview.firing=muzzle.firing(); preview.barrelPhase=muzzle.barrelPhase();
        preview.ammunition=muzzleAmmo.ammunition();
        check(view.draw(empty,{},nullptr,nullptr,nullptr,&planes,nullptr,&hudRenderer,preview,
                        nullptr,nullptr,&muzzle.scene()) && SDL_RenderPresent(renderer.get()),
              "Native muzzle flight composition failed");
        check(SDL_SaveBMP(surface.get(),("build/hud-debug/step203-flight-"+std::to_string(stage)+".bmp").c_str()),
              "Cannot save native muzzle flight composition");
    }
    preview.weapon=0; preview.firing=false;
    const interaction::Renderer interfaces(interaction::load(directory));
    interaction::State modal;
    check(view.draw(level,initialCamera(level),nullptr,&materials,&doors,&planes,&scene,&hudRenderer,
                    preview,&interfaces,&modal) && SDL_RenderPresent(renderer.get()),
          "Original modal/3D/HUD integration failed");
    check(SDL_SaveBMP(surface.get(),"build/panel-debug/level1-code-interface.bmp"),
          "Cannot save original interaction integrated frame");
    std::cout << "Original CASQUE pixels/PAL1 and level 1 integration verified.\n";
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        validation(); composition(); radarComposition(); slimeComposition(); weaponViews(); firingComposition(); realVitalsBars(); presentation(); integration();
        if (argc == 2) originals(argv[1]);
        std::cout << "Original HUD layout, limits, glyphs and software presentation passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
