#include <cstdint>
#include <filesystem>
#include "app/startup/startup_credits.h"
#include "platform/sdl/input/startup_skip.h"
#include "credits_audio.h"
#include "credits_timeline.h"
#include "credits_view.h"

#include <exception>

StartupCreditsResult showStartupCredits(SDL_Window* window, SDL_Renderer* renderer,
                                        const std::filesystem::path& resourceDirectory, float gain)
{
    using namespace startup_credits;

    int oldWidth=0,oldHeight=0;
    float oldScaleX=1.f,oldScaleY=1.f;
    Uint8 oldR=0,oldG=0,oldB=0,oldA=255;
    SDL_RendererLogicalPresentation oldMode;
    SDL_BlendMode oldBlend;
    if (!SDL_GetRenderLogicalPresentation(renderer,&oldWidth,&oldHeight,&oldMode) ||
        !SDL_GetRenderScale(renderer,&oldScaleX,&oldScaleY) ||
        !SDL_GetRenderDrawColor(renderer,&oldR,&oldG,&oldB,&oldA) ||
        !SDL_GetRenderDrawBlendMode(renderer,&oldBlend) ||
        !SDL_SetRenderLogicalPresentation(renderer,640,400,SDL_LOGICAL_PRESENTATION_LETTERBOX))
        return StartupCreditsResult::Error;

    auto result=StartupCreditsResult::Continue;
    if (!SDL_SetRenderScale(renderer,1.f,1.f) ||
        !SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND))
        result=StartupCreditsResult::Error;

    CreditsFont originalFont;
    const CreditsFont* font=nullptr;
    if (result==StartupCreditsResult::Continue && !resourceDirectory.empty()) {
        try {
            if (originalFont.load(renderer,resourceDirectory/"CASQUE.SPR")) {
                font=&originalFont;
            } else {
                SDL_Log("Intro: invalid CASQUE.SPR font; using SDL fallback");
            }
        } catch (const std::exception& error) {
            SDL_Log("Intro: cannot read CASQUE.SPR font (%s); using SDL fallback",error.what());
        }
    }

    CreditsAudio audio;
    if (result==StartupCreditsResult::Continue) audio.start(gain);

    const auto windowId=SDL_GetWindowID(window);
    auto previous=SDL_GetTicksNS();
    const std::uint32_t launchSeed=std::uint32_t(mixBits(std::uint32_t(previous)));
    double elapsed=0;
    bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    if (!focused) audio.setFocused(false);
    while (result==StartupCreditsResult::Continue && elapsed<duration) {
        SDL_Event event;
        bool done=false;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT ||
                (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==windowId)) {
                result=StartupCreditsResult::Quit;
                done=true;
            } else if (event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId && !event.key.repeat) {
                if (result==StartupCreditsResult::Continue && startupSkipRequested(event,windowId))
                    result=StartupCreditsResult::SkipAll;
                done=true;
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==windowId) {
                focused=false;
                audio.setFocused(false);
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==windowId) {
                focused=true;
                audio.setFocused(true);
            }
        }
        if (done) break;
        const auto now=SDL_GetTicksNS();
        if (focused) elapsed+=double(now-previous)/1e9;
        previous=now;
        if (elapsed>=duration) break;
        if (!drawCredit(renderer,font,elapsed,launchSeed)) {
            result=StartupCreditsResult::Error;
            break;
        }
        SDL_Delay(16);
    }

    if (result==StartupCreditsResult::SkipAll) clearStartupSkipInput();
    else SDL_ResetKeyboard();
    const bool scaleOK=SDL_SetRenderScale(renderer,oldScaleX,oldScaleY);
    const bool blendOK=SDL_SetRenderDrawBlendMode(renderer,oldBlend);
    const bool colorOK=SDL_SetRenderDrawColor(renderer,oldR,oldG,oldB,oldA);
    const bool modeOK=SDL_SetRenderLogicalPresentation(renderer,oldWidth,oldHeight,oldMode);
    if (!scaleOK || !blendOK || !colorOK || !modeOK)
        result=StartupCreditsResult::Error;
    return result;
}
