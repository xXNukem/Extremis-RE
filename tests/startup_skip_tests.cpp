#include "app/startup/startup_credits.h"
#include "extremis/cinematics/original_intro_player.h"
#include "platform/sdl/input/startup_skip.h"
#include <iostream>
#include <stdexcept>

namespace {
SDL_WindowID windowId;
SDL_Scancode key=SDL_SCANCODE_UNKNOWN;
unsigned polls=0,injectAt=0;
Uint64 ticks=0;
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void prepare(SDL_Scancode value,unsigned at=1) {
    key=value;polls=0;injectAt=at;
    SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
}
}
// Only the test target's player translation units use these host-clock/event
// substitutes. Controllers, rendering, resource ownership and audio are real.
extern "C" Uint64 SDLCALL startupTestTicks() { return ticks+=100000000; }
extern "C" void SDLCALL startupTestDelay(Uint32) { if(key==SDL_SCANCODE_UNKNOWN) SDL_DelayNS(1000000); }
extern "C" bool SDLCALL startupTestPoll(SDL_Event* event) {
    if(++polls==injectAt && key!=SDL_SCANCODE_UNKNOWN) {
        *event={};event->type=SDL_EVENT_KEY_DOWN;event->key.windowID=windowId;
        event->key.scancode=key;
        // A queued repeat must not reach gameplay after the skip.
        SDL_Event repeat=*event;repeat.key.repeat=true;SDL_PushEvent(&repeat);
        return true;
    }
    if(polls==1 || polls==3) {
        *event={};event->type=SDL_EVENT_WINDOW_FOCUS_GAINED;
        event->window.windowID=windowId;return true;
    }
    check(polls<100000,"Playback did not finish");
    return false;
}
int main(int argc,char** argv) {
    try {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        auto* window=SDL_CreateWindow("startup test",320,200,0);
        check(window!=nullptr,SDL_GetError());windowId=SDL_GetWindowID(window);
        auto* renderer=SDL_CreateRenderer(window,nullptr);check(renderer!=nullptr,SDL_GetError());
        for(auto scan:{SDL_SCANCODE_SPACE,SDL_SCANCODE_E}) {
            SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.windowID=windowId;e.key.scancode=scan;
            check(startupSkipRequested(e,windowId),"Skip key rejected");
            e.key.repeat=true;check(!startupSkipRequested(e,windowId),"Repeat accepted");
            e.key.repeat=false;check(!startupSkipRequested(e,windowId+1),"Foreign window accepted");
            e.type=SDL_EVENT_KEY_UP;check(!startupSkipRequested(e,windowId),"Key up accepted");
            for(unsigned at:{1u,20u,90u,180u,230u}) {
                prepare(scan,at);
                check(showStartupCredits(window,renderer)==StartupCreditsResult::SkipAll,"Custom credits skip lost");
                check(!SDL_HasEvent(SDL_EVENT_KEY_DOWN),"Skip repeat leaked");
                check(SDL_WasInit(SDL_INIT_AUDIO)==0,"Custom audio leaked");
            }
            prepare(scan);
            check(showOriginalIntro(window,renderer,{})==StartupCreditsResult::SkipAll,"PSEQ entry skip lost");
            check(!SDL_HasEvent(SDL_EVENT_KEY_DOWN),"PSEQ entry input leaked");
        }
        prepare(SDL_SCANCODE_ESCAPE);
        check(showStartupCredits(window,renderer)==StartupCreditsResult::Continue,"Custom Escape changed");
        prepare(SDL_SCANCODE_UNKNOWN);
        check(showStartupCredits(window,renderer)==StartupCreditsResult::Continue,"Normal custom playback failed");
        check(polls>200,"Normal custom playback ended early");
        if(argc>1) {
            for(auto player:{showOriginalPresentation,showOriginalIntro}) {
                for(auto scan:{SDL_SCANCODE_SPACE,SDL_SCANCODE_E}) {
                    for(unsigned at:{2u,30u,150u,400u}) {
                        prepare(scan,at);
                        check(player(window,renderer,argv[1])==StartupCreditsResult::SkipAll,"Original player skip lost");
                        check(!SDL_HasEvent(SDL_EVENT_KEY_DOWN),"Original player input leaked");
                        check(SDL_WasInit(SDL_INIT_AUDIO)==0,"Original audio leaked");
                        int w,h;SDL_RendererLogicalPresentation mode;
                        SDL_GetRenderLogicalPresentation(renderer,&w,&h,&mode);
                        check(mode==SDL_LOGICAL_PRESENTATION_DISABLED,"Renderer state leaked");
                    }
                }
                prepare(SDL_SCANCODE_UNKNOWN);
                check(player(window,renderer,argv[1])==StartupCreditsResult::Continue,"Normal original playback failed");
            }
        }
        SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
        std::cout<<"Startup skip/input/cleanup and normal playback passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
