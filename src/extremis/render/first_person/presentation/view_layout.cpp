#include "extremis/render/first_person/presentation/view_scale.h"
#include "view_layout.h"
#include "extremis/render/first_person/presentation/video_render_settings.h"
#include "extremis/ui/original_hud.h"

#include <algorithm>
#include <cmath>

namespace extremis::render_detail {
ViewLayout viewLayout(int outputWidth, int outputHeight, bool withHud,
                      const VideoRenderSettings& video)
{
    int width = outputWidth;
    int height = outputHeight;
    if (withHud) {
        const double aspect = extremis::selectedAspect(video.preferences.aspectChoice, outputWidth, outputHeight);
        // Width is measured in the original DOS game's non-square pixels;
        // its native 320x200 image corresponds to a 4:3 physical display.
        width=extremis::dosViewWidth(aspect);
        height=hud::viewHeight;
    }
    // Keep 1x fallback, enable 2x at 1080p, up to 3x at 1440p/4K.
    // VORTEX_VIDEO_RESOLUTION_V2: internal 3D follows the selected
    // physical display mode, not merely an unchanging Retina backbuffer.
    // DOS HUD remains 320x200 and the whole view preserves its aspect.
    // The 6x safety limit prevents unbounded software rendering at 5K/8K.
    const int baseViewWidth=width;
    const bool nativeMax=withHud && video.mode.nativeMax;
    const int renderScale=withHud?std::max(1,video.mode.renderScale):1;
    if(nativeMax && video.mode.nativeWidth>0 && video.mode.nativeHeight>0) {
        const double virtualAspect=(4.0/3.0)*baseViewWidth/hud::width;
        const double physicalWidth=std::min(double(video.mode.nativeWidth),
                                            double(video.mode.nativeHeight)*virtualAspect);
        const double physicalHeight=physicalWidth/virtualAspect;
        height=std::max(hud::viewHeight,int(std::lround(physicalHeight*hud::viewHeight/240.0)));
        width=std::max(baseViewWidth,int(std::lround(double(baseViewWidth)*height/hud::viewHeight)));
        width=std::min(width,std::max(baseViewWidth,int(std::floor(physicalWidth))));
    } else {
        width*=renderScale;
        height*=renderScale;
    }
    const int projectionReferenceWidth=withHud
        ? int(std::lround(double(width)*hud::width/baseViewWidth)) : width;
    return {width, height, projectionReferenceWidth, renderScale, nativeMax};
}
} // namespace extremis::render_detail

namespace extremis {
RequestedVideoSize composedViewSize(int outputWidth, int outputHeight, const VideoRenderSettings& video)
{
    const auto layout = render_detail::viewLayout(outputWidth,outputHeight,true,video);
    return {layout.width, int(std::lround(double(layout.height)*hud::height/hud::viewHeight))};
}
}
