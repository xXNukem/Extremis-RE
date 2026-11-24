#include <string_view>
#include <cstddef>
#include "extremis/diagnostics/shot_trace.h"
#include "extremis/ui/localization.h"
#include "extremis/ui/utf8_glyph.h"
#include "app_session.h"

#include "app/diagnostics/map_view.h"
#include "app/session/shuttle_session.h"
#include "extremis/cinematics/level22_communication.h"
#include "extremis/render/first_person/presentation/first_person_view.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "../diagnostics/gameplay_fps.h"
#include "app/diagnostics/debug_menu.h"
#include "platform/sdl/video/frame_profiler.h"
#endif

#include <SDL3/SDL_render.h>
#include <algorithm>

namespace {
bool drawLocalizedDebugText(SDL_Renderer* renderer,std::string_view text,float centerX,float y)
{
    unsigned glyphs=0;for(std::size_t i=0;i<text.size();) {const auto decoded=localization::glyph(text,i);i+=decoded.bytes;++glyphs;}
    float x=centerX-4.f*glyphs;
    for(std::size_t i=0;i<text.size();) {
        const auto decoded=localization::glyph(text,i);i+=decoded.bytes;
        const char str[]{char(decoded.base),0};
        if(!SDL_RenderDebugText(renderer,x,y,str)) return false;
        if(decoded.mark && decoded.mark!='i') {
            const float m=x+4.f;
            if(decoded.mark=='c') {if(!SDL_RenderLine(renderer,m,y+8.f,m-1.f,y+9.f)) return false;}
            else if(!SDL_RenderLine(renderer,m-1.f,y-1.f,m+1.f,y-2.f)) return false;
        }
        x+=8.f;
    }
    return true;
}
bool drawContinuationNotice(SDL_Renderer* renderer, const continuation::Checkpoint& checkpoint)
{
    SDL_Rect area{};
    float oldX=1, oldY=1;
    if (!SDL_GetRenderViewport(renderer,&area) || !SDL_GetRenderScale(renderer,&oldX,&oldY))
        return false;
    const float scale=std::max(1.f,float(area.w)/320.f);
    if (!SDL_SetRenderScale(renderer,oldX*scale,oldY*scale)) return false;
    const float x=area.w/(2.f*scale), y=area.h/(2.f*scale);
    const SDL_FRect panel{x-96,y-27,192,54};
    const bool drawn=SDL_SetRenderDrawColor(renderer,8,17,13,255) &&
        SDL_RenderFillRect(renderer,&panel) &&
        SDL_SetRenderDrawColor(renderer,159,233,117,255) &&
        SDL_RenderRect(renderer,&panel) &&
        drawLocalizedDebugText(renderer,localization::byId("password.quick_code",localization::current()),x,y-18) &&
        SDL_RenderDebugText(renderer,x-32,y-4,checkpoint.code.data()) &&
        drawLocalizedDebugText(renderer,localization::byId("password.press_any_key",localization::current()),x,y+12);
    return SDL_SetRenderScale(renderer,oldX,oldY) && drawn;
}
}

namespace app {
bool AppSession::presentFrame(SDL_Renderer* renderer, FirstPersonRenderer& firstPerson,
    const hud::State& hudState, const enemies::System* activeEnemies,
    bool show3D, bool showPolygons, bool modalBlocked, extremis::PaletteTint tint
#ifdef EXTREMIS_DEBUG_MENU
    , DebugMenu& debugMenu, double gameplayFps, platform::sdl::video_profiling::FrameProfiler& profiler
#endif
)
{
    auto renderCamera=camera;
    if (renderCamera && !modalBlocked && bombFrame.shock) {
        renderCamera->x += bombFrame.shakeX;
        renderCamera->y += bombFrame.shakeY;
        renderCamera->postureHeight = std::clamp(renderCamera->postureHeight + bombFrame.shakeHeight,100.,165.);
    }
    if (bombFrame.white) tint = {bombFrame.white,{63,63,63},tint.optics};
    const auto shuttleDoorFade=[&]() {
        const auto amount=data && camera && show3D && !modalBlocked ?
            (shuttle::doorFade(data->level,camera->sector.value_or(~0u)) +
             communication::doorFade(data->level,camera->sector.value_or(~0u))):0;
        if(!amount) return true;
        auto faded=firstPerson.frame();
        for(auto& c:faded) c=(((c>>24)*(100-amount)/100)<<24)|
            ((((c>>16)&255)*(100-amount)/100)<<16)|((((c>>8)&255)*(100-amount)/100)<<8)|255;
        return firstPerson.drawFrame(faded);
    };
    if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
        !SDL_RenderClear(renderer) ||
        (data && !(show3D && camera
            ? firstPerson.draw(data->level, *renderCamera, texturePreview.selectedTexture(),
                               texturePreview.materials(), &levelDoors, planeAppearance.materials(),
                               spriteScene ? &*spriteScene : nullptr,
                               originalHud ? &*originalHud : nullptr, hudState,
                               interactionRenderer ? &*interactionRenderer : nullptr,items.interfaceState(),
                               weaponGraphicsReady?&weaponFire.scene():nullptr,
                               activeEnemies?&activeEnemies->scene():nullptr,
                               activeEnemies?&activeEnemies->effects().objects():nullptr,
                               originalMap?&*originalMap:nullptr,mapSession.state(),modalBlocked ? extremis::PaletteTint{} : tint, bombScene.objects.empty() ? nullptr : &bombScene)
            : drawMap(renderer, data->level, showPolygons, camera ? &*camera : nullptr))) ||
        !shuttleDoorFade()
#ifdef EXTREMIS_DEBUG_MENU
        || !debugMenu.draw(renderer,data?data->level.zoneIndex:0) ||
        (data && show3D && camera && !debugMenu.open() && !mapSession.state() &&
         !diagnostics::drawGameplayFps(renderer,gameplayFps,profiler.snapshot()))
#endif
        || (pendingDisclosure && !drawContinuationNotice(renderer,*pendingDisclosure))
    ) return false;
    extremis::shot_trace::event("render", "view3d=%d modal=%d flash=%d barrel=%u sprites=%zu", show3D, modalBlocked,
                    hudState.firing,hudState.barrelPhase,
                    weaponFire.scene().objects.size());
    const bool presented =
#ifdef EXTREMIS_DEBUG_MENU
        profiler.timedPresent(renderer);
#else
        SDL_RenderPresent(renderer);
#endif
    extremis::shot_trace::event("present", "ok=%d", presented);
    return presented;
}
}
