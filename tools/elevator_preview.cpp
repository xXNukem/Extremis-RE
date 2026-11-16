#include "original_elevator_player.h"
#include "original_elevator.h"
#include "game_audio.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <string_view>

int main(int argc,char** argv) {
    if(argc<2 || argc>3 || (argc==3 && std::string_view(argv[2])!="--check")) {
        std::cerr<<"Usage: "<<argv[0]<<" path/to/DATA [--check]\n";return 2;
    }
    try {
        // Validate all graphics before creating a window or playing anything.
        auto resources=original_elevator::loadResources(argv[1]);
        if(argc==3) {
            for(int delta:{1,3,-1,-3}) {
                original_elevator::Presentation p(resources,delta);
                unsigned retraces=0;
                while(p.phase()!=original_elevator::Phase::Done && retraces<2000) {p.step();++retraces;}
                if(p.phase()!=original_elevator::Phase::Done) return 1;
                std::cout<<"delta "<<delta<<": "<<retraces<<" retraces, last q="<<p.q()<<'\n';
            }
            return 0;
        }
        if(!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
        SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
        if(!SDL_CreateWindowAndRenderer("Extremis elevator preview",960,720,0,&window,&renderer))
            throw std::runtime_error(SDL_GetError());
        int status=0;
        {
            GameAudio audio;audio.load(argv[1]);
            for(int delta:{1,3,-1,-3}) {
                const auto title=std::string("Ascensor: ")+(delta>0?"subir ":"bajar ")+
                    (delta==1 || delta==-1?"un piso":"varios pisos");
                SDL_SetWindowTitle(window,title.c_str());std::cout<<title<<std::endl;
                auto result=showOriginalElevator(window,renderer,argv[1],delta,audio);
                if(result!=StartupCreditsResult::Continue) {status=result==StartupCreditsResult::Error?1:0;break;}
            }
        }
        SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return status;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
