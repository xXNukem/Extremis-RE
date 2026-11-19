#include "extremis/diagnostics/shot_trace.h"
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

namespace app {
bool AppSession::presentFrame(SDL_Renderer* renderer, FirstPersonRenderer& firstPerson,
    const hud::State& hudState, const enemies::System* activeEnemies,
    bool show3D, bool showPolygons, bool modalBlocked
#ifdef EXTREMIS_DEBUG_MENU
    , DebugMenu& debugMenu, double gameplayFps, platform::sdl::video_profiling::FrameProfiler& profiler
#endif
)
{
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
            ? firstPerson.draw(data->level, *camera, texturePreview.selectedTexture(),
                               texturePreview.materials(), &levelDoors, planeAppearance.materials(),
                               spriteScene ? &*spriteScene : nullptr,
                               originalHud ? &*originalHud : nullptr, hudState,
                               interactionRenderer ? &*interactionRenderer : nullptr,items.interfaceState(),
                               weaponGraphicsReady?&weaponFire.scene():nullptr,
                               activeEnemies?&activeEnemies->scene():nullptr,
                               activeEnemies?&activeEnemies->effects().objects():nullptr,
                               originalMap?&*originalMap:nullptr,mapSession.state())
            : drawMap(renderer, data->level, showPolygons, camera ? &*camera : nullptr))) ||
        !shuttleDoorFade()
#ifdef EXTREMIS_DEBUG_MENU
        || !debugMenu.draw(renderer,data?data->level.zoneIndex:0) ||
        (data && show3D && camera && !debugMenu.open() && !mapSession.state() &&
         !diagnostics::drawGameplayFps(renderer,gameplayFps,profiler.snapshot()))
#endif
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
