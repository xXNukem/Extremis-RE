#include <filesystem>
#include <stdexcept>
#include "extremis/cinematics/original_shuttle.h"
#include "extremis/cinematics/original_shuttle_player.h"
#include "extremis/resources/audio_data.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <string_view>
int main(int argc,char** argv) {
    if(argc<2 || argc>3 || (argc==3 && std::string_view(argv[2])!="--check" && std::string_view(argv[2])!="--inherited-escape")) {
        std::cerr<<"Usage: "<<argv[0]<<" path/to/DATA [--check|--inherited-escape]\n";return 2;
    }
    try {
        if(argc==3 && std::string_view(argv[2])=="--check") {
            original_shuttle::Presentation p(original_shuttle::loadResources(argv[1]));
            for(unsigned i=0;i<10;++i) {
                const auto& s=original_shuttle::stage(static_cast<original_shuttle::Scene>(i));
                auto sample=audio::loadSample(std::filesystem::path(argv[1])/s.sound);
                if(sample.rate!=5000 || sample.pcm.empty()) return 1;
            }
            unsigned steps=0;int last=-1;
            while(p.scene()!=original_shuttle::Scene::Done && ++steps<20000) {
                if(int(p.scene())!=last) {last=int(p.scene());std::cout<<last<<": "<<original_shuttle::stage(p.scene()).visual<<'\n';}
                p.step(3,true,true);
            }
            return p.scene()==original_shuttle::Scene::Done?0:1;
        }
        if(!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
        SDL_Window* w=nullptr;SDL_Renderer* r=nullptr;
        if(!SDL_CreateWindowAndRenderer("Extremis shuttle: level 22 to 23",960,720,0,&w,&r)) throw std::runtime_error(SDL_GetError());
        auto result=showOriginalShuttle(w,r,argv[1],argc==3);
        SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();
        return result==StartupCreditsResult::Error?1:0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
