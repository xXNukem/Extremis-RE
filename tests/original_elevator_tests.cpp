#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>
#include "extremis/cinematics/original_elevator.h"
#include "extremis/cinematics/original_elevator_player.h"
#include "extremis/audio/game_audio.h"
#include "extremis/audio/audio_mixer.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
using namespace original_elevator;
namespace {
void check(bool ok,const char* text) {if(!ok) throw std::runtime_error(text);}
Resources fixture() {
    Resources r;r.background.palette.fill(0xffffffff);
    r.background.frames.emplace_back(64000,1);
    r.sprite={97,105,0,0,std::vector<std::uint8_t>(97*105,2)};
    r.sprite.indices[0]=0;return r;
}
void movement(int delta,int count,int last,int middle,int sum) {
    Presentation p(fixture(),delta);std::vector<int> positions,in,out;
    unsigned hold=0,starts=0,stops=0,steps=0;bool motor=false;
    while(p.phase()!=Phase::Done && ++steps<1000) {
        const auto before=p.phase();p.step();
        if(before==Phase::Move) positions.push_back(p.q());
        if(before==Phase::FadeIn) in.push_back(p.brightness());
        if(before==Phase::FadeOut) out.push_back(p.brightness());
        if(before==Phase::Hold) ++hold;
        if(p.motor()!=motor) {if(p.motor()) ++starts;else ++stops;motor=p.motor();}
        if(before==Phase::Move || (before==Phase::Hold && hold<30)) check(p.motor(),"Motor stopped before hold ended");
    }
    int total=0;for(auto q:positions) total+=q;
    check(positions.size()==unsigned(count) && positions.back()==last &&
          positions[positions.size()/2]==middle && total==sum,"Motion differs from recovered golden trajectory");
    check(positions.front()==(delta<0?(delta==-1?260:460):0),"Initial movement q not repeated");
    check(hold==30 && starts==1 && stops==1,"Hold or sound boundary changed");
    check(in.size()==51 && out.size()==51,"Fade length differs");
    for(int i=0;i<51;++i) check(in[i]==2*i && out[i]==100-2*i,"Fade brightness sequence differs");
    check(std::all_of(p.pixels().begin(),p.pixels().end(),[](auto c){return c==255;}),"Final frame not black");
    p.step();check(p.phase()==Phase::Done && !p.motor(),"Completion not stable");
}
void composition() {
    auto a=rectangle(0),b=rectangle(260),c=rectangle(460);
    check(a.x==81 && a.y==146 && a.w==69 && a.h==74,"q0 rectangle");
    check(b.x==105 && b.y==-36 && b.w==105 && b.h==114,"q260 rectangle");
    check(c.x==124 && c.y==-176 && c.w==133 && c.h==145,"q460 rectangle");
    Presentation p(fixture(),1);
    check(p.indices()[146*320+81]==1 && p.indices()[146*320+82]==2,"Transparency/scaling origin");
    while(p.phase()!=Phase::Hold) p.step();
    check(p.indices()[146*320+82]==1,"Old sprite trails were not erased");
    check(p.indices()[105]==2 && p.indices()[104]==1,"Top clipping/source offset");
    bool failed=false;auto r=fixture();r.sprite.indices.pop_back();
    try {Presentation bad(std::move(r),1);} catch(const std::runtime_error&) {failed=true;}
    check(failed,"Malformed asset accepted");
}
void originals(const char* directory) {
    auto r=loadResources(directory);
    for(int d:{1,3,-1,-3}) {Presentation p(r,d);for(int i=0;i<1000 && p.phase()!=Phase::Done;++i)p.step();check(p.phase()==Phase::Done,"Real resources failed");}
    vortex::audio::Mixer mixer(audio::mixerConfig);
    mixer.setSamples(audio::mixerSamples(audio::loadBank(std::filesystem::path(directory)/"BANK.DSS")));
    check(mixer.play(11),"Motor play failed");std::vector<float> pcm(20000);mixer.render(pcm.data(),pcm.size());
    check(mixer.playing(11) && mixer.statistics(11)->wraps>0,"BANK11 must loop natively");mixer.clear();
    check(!mixer.playing(11),"Motor failed to stop");
    // Exercise the actual SDL boundary: pending Escape is consumed, close aborts;
    // caller cannot mistake either missing resources or Quit for a committed trip.
    check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
    auto* window=SDL_CreateWindow("elevator test",320,200,SDL_WINDOW_HIDDEN);
    auto* surface=SDL_CreateSurface(320,200,SDL_PIXELFORMAT_RGBA8888);
    auto* renderer=SDL_CreateSoftwareRenderer(surface);check(window && renderer,"SDL fixture");
    SDL_SetRenderScale(renderer,2,3);SDL_SetRenderDrawColor(renderer,1,2,3,4);
    GameAudio sound;
    SDL_Event escape{};escape.type=SDL_EVENT_KEY_DOWN;escape.key.windowID=SDL_GetWindowID(window);escape.key.scancode=SDL_SCANCODE_ESCAPE;
    SDL_PushEvent(&escape);SDL_Event quit{};quit.type=SDL_EVENT_QUIT;SDL_PushEvent(&quit);
    check(showOriginalElevator(window,renderer,directory,1,sound)==StartupCreditsResult::Quit,"Escape skipped or close failed");
    float x,y;SDL_GetRenderScale(renderer,&x,&y);check(x==2 && y==3,"Renderer scale not restored");
    Uint8 red,green,blue,alpha;SDL_GetRenderDrawColor(renderer,&red,&green,&blue,&alpha);
    check(red==1 && green==2 && blue==3 && alpha==4,"Renderer color not restored");
    check(showOriginalElevator(window,renderer,"/missing-elevator-assets",1,sound)==StartupCreditsResult::Error,"Missing assets committed travel");
    SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_DestroyWindow(window);SDL_Quit();
}
}
int main(int argc,char** argv) {
    try {movement(1,58,260,120,7134);movement(3,75,458,190,15655);
        movement(-1,57,2,148,7946);movement(-3,75,2,270,18845);composition();
        if(argc==2) originals(argv[1]);
        std::cout<<"Elevator trajectories, fades, hold, audio boundaries and composition passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
