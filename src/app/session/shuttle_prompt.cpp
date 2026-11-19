#include "app/session/shuttle_session.h"
#include "extremis/cinematics/playback_result.h"
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace shuttle {
namespace {
int glyph(unsigned char c) {
    if(c>='0' && c<='9') return c-'0';
    if(c>='A' && c<='Z') return c-'A'+10;
    if(c>='a' && c<='z') return c-'a'+36;
    const std::string punctuation=":-?!',.\x85\x82\x8a\x88\x93\x96\x97";
    const auto p=punctuation.find(char(c));
    return p==std::string::npos?-1:int(p)+62;
}
struct Restore {
    SDL_Renderer* r;
    int w=0,h=0; float x=1,y=1; Uint8 red=0,green=0,blue=0,alpha=255;
    SDL_RendererLogicalPresentation mode=SDL_LOGICAL_PRESENTATION_DISABLED;
    Restore(SDL_Renderer* renderer):r(renderer) {
        SDL_GetRenderLogicalPresentation(r,&w,&h,&mode); SDL_GetRenderScale(r,&x,&y);
        SDL_GetRenderDrawColor(r,&red,&green,&blue,&alpha);
    }
    ~Restore() {
        SDL_SetRenderLogicalPresentation(r,w,h,mode); SDL_SetRenderScale(r,x,y);
        SDL_SetRenderDrawColor(r,red,green,blue,alpha); SDL_ResetKeyboard();
    }
};
}
StartupCreditsResult prompt(SDL_Window* window,SDL_Renderer* renderer,const std::filesystem::path& directory,
                            Session& session,const player::Inventory& inventory,const ItemSession& items) {
    try {
        Restore restore(renderer);
        const auto hangar=anm_format::loadVga(directory/"NAV_HANG.VGA");
        const auto interior=anm_format::loadVga(directory/"NAV_INTE.VGA");
        const auto instruments=spr_format::load(directory/"NAV_INS2.SPR");
        const auto font=spr_format::load(directory/"CON_VUET.SPR");
        if(instruments.size()<89 || font.size()<69 || hangar.frames.empty() || interior.frames.empty())
            throw std::runtime_error("Incomplete boarding resources");
        std::array<std::string,6> text;
        const unsigned offsets[]{0x22d71,0x22d98,0x22d9c,0x22da0,0x22dc2,0x22dfb};
        std::ifstream exe(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
        for(unsigned i=0;i<6;++i) {
            exe.seekg(offsets[i]);
            for(unsigned n=0;;++n) {
                const int c=exe.get();
                if(c<0 || n>=128) throw std::runtime_error("Missing boarding text in EXTREMIS.EXE");
                if(!c) break;
                text[i]+=char(c);
            }
        }
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        std::vector<std::uint8_t> canvas;
        std::vector<std::uint32_t> pixels(64000);
        const auto image=[&](const spr_format::Image& s,int x,int y) {
            for(int row=0;row<s.height;++row) for(int col=0;col<s.width;++col) {
                const int xx=x+col,yy=y+row;const auto c=s.indices[row*s.width+col];
                if(c && xx>=0 && xx<320 && yy>=0 && yy<200) canvas[yy*320+xx]=c;
            }
        };
        const auto label=[&](const std::string& str,int y) {
            int width=0;
            for(unsigned char c:str) { const int g=glyph(c);width+=g>=0 && unsigned(g)<font.size()?font[g].width+1:4; }
            int x=(320-width)/2;
            for(unsigned char c:str) {
                const int g=glyph(c);
                if(g>=0 && unsigned(g)<font.size()) { image(font[g],x,y);x+=font[g].width+1; }
                else x+=4;
            }
        };
        bool yes=true,focused=SDL_GetKeyboardFocus()==window;
        // Require release across modal boundaries, including two queued confirmations.
        bool confirmHeld=SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_E] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE];
        unsigned brightness=0; bool fadingIn=true,fadingOut=false;
        Step displayed=session.step(); double retrace=0,blink=0;
        auto previous=SDL_GetTicksNS();
        const auto id=SDL_GetWindowID(window);
        for(;;) {
            bool confirm=false;
            SDL_Event e;
            while(SDL_PollEvent(&e)) {
                if(e.type==SDL_EVENT_QUIT || (e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && e.window.windowID==id))
                    return StartupCreditsResult::Quit;
                if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST && e.window.windowID==id) {focused=false;confirmHeld=true;}
                if(e.type==SDL_EVENT_WINDOW_FOCUS_GAINED && e.window.windowID==id) focused=true;
                if((e.type!=SDL_EVENT_KEY_UP && e.type!=SDL_EVENT_KEY_DOWN) || e.key.windowID!=id) continue;
                const auto k=e.key.scancode;
                if(k==SDL_SCANCODE_E || k==SDL_SCANCODE_SPACE) {
                    if(e.type==SDL_EVENT_KEY_UP) confirmHeld=false;
                    else { if(!confirmHeld && !e.key.repeat && focused && !fadingIn && !fadingOut) confirm=true; confirmHeld=true; }
                }
                if(e.type!=SDL_EVENT_KEY_DOWN || e.key.repeat || !focused || fadingIn || fadingOut) continue;
                if(k==SDL_SCANCODE_W || k==SDL_SCANCODE_UP) yes=true;
                if(k==SDL_SCANCODE_S || k==SDL_SCANCODE_DOWN) yes=false;
            }
            const auto now=SDL_GetTicksNS();const double dt=focused?double(now-previous)/1e9:0;previous=now;
            blink+=dt*160;while(blink>=160) blink-=160;
            if(confirm) {
                session.answer(yes,inventory,items);
                if(session.step()==Step::Flight) return StartupCreditsResult::Continue;
                if(session.step()==Step::MissingCard || session.step()==Step::Securities) displayed=session.step();
                else fadingOut=true;
            }
            retrace+=dt*70;
            while(retrace>=1) {
                retrace-=1;
                if(fadingIn) { brightness+=2;if(brightness>=100) {brightness=100;fadingIn=false;} }
                else if(fadingOut) {
                    if(brightness>=2) brightness-=2;
                    else {
                        if(session.step()!=Step::Start) return StartupCreditsResult::Continue;
                        displayed=Step::Start;yes=true;blink=0;fadingOut=false;fadingIn=true;
                    }
                }
            }
            const auto& background=displayed==Step::Boarding?hangar:interior;
            canvas=background.frames[0];
            if(displayed!=Step::Boarding) image(instruments[88],0,39);
            if(displayed==Step::MissingCard || displayed==Step::Securities) label(text[displayed==Step::MissingCard?4:5],20);
            else {
                label(text[displayed==Step::Boarding?0:3],20);
                if(!yes || blink<80) label(text[1],30);
                if(yes || blink<80) label(text[2],40);
            }
            for(unsigned i=0;i<64000;++i) {
                const auto c=background.palette[canvas[i]];
                pixels[i]=(((c>>24)*brightness/100)<<24)|((((c>>16)&255)*brightness/100)<<16)|
                          ((((c>>8)&255)*brightness/100)<<8)|255;
            }
            if(!SDL_UpdateTexture(texture.get(),nullptr,pixels.data(),320*4) ||
               !SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
               !SDL_RenderTexture(renderer,texture.get(),nullptr,nullptr) || !SDL_RenderPresent(renderer))
                throw std::runtime_error(SDL_GetError());
            SDL_Delay(1);
        }
    } catch(const std::exception& error) {
        SDL_Log("Shuttle boarding failed: %s",error.what());return StartupCreditsResult::Error;
    }
}
}
