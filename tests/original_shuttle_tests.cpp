#include "extremis/cinematics/original_shuttle.h"
#include "extremis/resources/audio_data.h"
#include "extremis/cinematics/original_shuttle_player.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
using namespace original_shuttle;
void require(bool b,const char* message) {if(!b) throw std::runtime_error(message);}
Resources synthetic() {
    Resources r;
    for(unsigned i=0;i<10;++i) {
        r.visuals[i].frames.assign(stage(static_cast<Scene>(i)).frames,std::vector<std::uint8_t>(64000,0));
        r.visuals[i].palette.fill(0xffffffff);
    }
    unsigned counts[]{69,89,16,7};
    for(unsigned i=0;i<4;++i) {r.sprites[i].assign(counts[i],{1,1,0,0,{1}});r.text[i]=std::string(20,'A');}
    return r;
}
void journey(Resources r) {
    Presentation p(std::move(r),true);std::vector<int> scenes,cues;int last=-1;
    for(unsigned i=0;i<20000 && p.scene()!=Scene::Done;++i) {
        if(int(p.scene())!=last) {last=int(p.scene());scenes.push_back(last);}
        auto c=p.takeCues();cues.insert(cues.end(),c.begin(),c.end());p.step(3,true,true);
    }
    auto c=p.takeCues();cues.insert(cues.end(),c.begin(),c.end());
    require(p.scene()==Scene::Done,"journey did not terminate");require(scenes.size()==10,"missing scene");
    for(int i=0;i<10;++i) {require(scenes[i]==i,"scene order");require(cues[2*i]==-1 && cues[2*i+1]==i,"voice restart order");}
    require(cues.size()==21 && cues.back()==-1,"VENT stop before fade");
    require(!p.escapeEnabled(),"normal ANM completion must clear inherited flag");
    for(auto pixel:p.pixels()) require(pixel==255,"final fade must end black");
}
int main(int argc,char** argv) {
    try {
        const char* visuals[]{"NAV_INTE.VGA","NAV_HANG.VGA","NAV_SEQ3.ANM","NAV_SEQ4.ANM","VAIS_EXP.ANM","NAV_INTE.VGA","NAV_SEQ7.ANM","NAV_SEQ8.ANM","ARR_BASE.ANM","FSEQ2_1.VGA"};
        const char* sounds[]{"ALARME4.DSS","ALARME3.DSS","DEPART.DSS","BARGE2.DSS","EXPLO.DSS","CABINE.DSS","BARGE2.DSS","BARGE1.DSS","BARGE1.DSS","VENT.DSS"};
        unsigned frames[]{1,1,61,49,60,1,60,62,62,1},intervals[]{20,26,16,19,11,20,21,20,20,10};
        for(unsigned i=0;i<10;++i) {auto& s=stage(static_cast<Scene>(i));require(std::string(s.visual)==visuals[i] && std::string(s.sound)==sounds[i] && s.frames==frames[i] && s.interval==intervals[i],"resource/timing contract");}
        journey(synthetic());
        for(bool inherited:{false,true}) {
            Presentation p(synthetic(),inherited);
            for(unsigned i=0;i<2000 && p.scene()!=Scene::Nav3;++i) p.step(27,true,true,true);
            require(p.scene()==Scene::Nav3,"procedural scenes ignore Escape");
            p.step(0,true,true);p.step(0,true,true); // black cut then first frame
            p.step(16,true,true);require(p.frame()==0,"strict PIT threshold");
            p.step(0,true,true,true);
            if(inherited) {require(p.scene()==Scene::Nav4 && p.escapeEnabled(),"Escape skips only current ANM");}
            else {
                require(p.scene()==Scene::Nav3,"disabled Escape");p.step(1,true,true);require(p.frame()==1,"frame wait");
                for(unsigned i=1;i<60;++i) p.step(17,true,true);
                require(p.frame()==60 && p.scene()==Scene::Nav3,"last frame displayed");
                p.step(16,true,true);require(p.scene()==Scene::Nav3,"last frame wait");
                p.step(1,true,true);require(p.scene()==Scene::Nav4,"normal completion");
            }
        }
        {
            auto r=synthetic();
            for(unsigned i=0;i<16;++i) r.sprites[2][i].indices[0]=i+2;
            for(unsigned i=0;i<7;++i) r.sprites[3][i].indices[0]=i+20;
            Presentation p(std::move(r));
            while(p.scene()!=Scene::Hangar) p.step(3,true,true);
            p.step(0,true,true); // black cut
            unsigned strip=0;
            for(unsigned step=0;step<36;++step) {
                p.step(0,true,true);
                require(p.indices()[145*320+91]==strip+2,"hangar lower strip cycle");
                if(step>=12 && step<24) require(p.indices()[21*320+16]==step%12+6,"hangar middle phase");
                if(step>=24) require(p.indices()[21*320+16]==17,"hangar background must persist");
                p.step(26,true,true);require(p.scene()==Scene::Hangar,"hangar strict threshold");
                p.step(1,true,true);if(++strip==4) strip=2;
            }
            require(p.scene()==Scene::Nav3,"three phases of twelve steps");
            while(p.scene()!=Scene::Base) p.step(3,true,true);
            p.step(0,true,true);p.step(0,true,true);
            int y=150,pose=1,direction=1;unsigned step=0;
            const int dy[]{-1,-3,-2,-1,0,0,-2,-2,-1,-1,-1,-1};
            while(y>100) {
                require(p.indices()[y*320+154]==pose+19,"base sprite trajectory");
                p.step(10,true,true);require(p.indices()[y*320+154]==pose+19,"base strict wait");
                p.step(1,true,true);y+=dy[step++%12];pose+=direction;
                if(pose==7) direction=-1;if(pose==1) direction=1;
            }
            for(unsigned i=0;i<51;++i) p.step(0,true,false);
            require(p.scene()==Scene::Done,"51 final fade retraces");
        }
        if(argc>1) {
            journey(loadResources(argv[1]));
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
            require(SDL_Init(SDL_INIT_VIDEO),"SDL init");
            SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
            require(SDL_CreateWindowAndRenderer("test",320,200,0,&window,&renderer),"SDL window");
            SDL_SetRenderScale(renderer,2,3);
            SDL_Event quit{};quit.type=SDL_EVENT_QUIT;SDL_PushEvent(&quit);
            require(showOriginalShuttle(window,renderer,argv[1])==StartupCreditsResult::Quit,"host quit");
            float x=0,y=0;SDL_GetRenderScale(renderer,&x,&y);require(x==2 && y==3,"renderer restoration");
            require(showOriginalShuttle(window,renderer,"/missing/shuttle") == StartupCreditsResult::Error,"missing resources");
            SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
            const unsigned lengths[]{29164,36278,18278,17954,30509,20792,17954,17986,17986,25395};
            for(unsigned i=0;i<10;++i) {
                auto sample=audio::loadSample(std::filesystem::path(argv[1])/sounds[i]);
                require(sample.rate==5000 && sample.pcm.size()==lengths[i],"original sample identity");
                bool loop=i==0 || i==1 || i==2 || i==5 || i==9;
                require(bool(sample.loopFlag)==loop,"native sample loop");
            }
        }
        std::cout<<"Shuttle tests passed\n";return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
