#include "extremis/ui/frame_composition.h"

#include "extremis/ui/original_hud.h"

#include <algorithm>
#include <cmath>

namespace extremis {
namespace {
constexpr int cornerCounts[10] = {60, 48, 40, 32, 24, 20, 16, 12, 8, 4};
} // namespace

void copyHudView(const std::vector<std::uint32_t>& pixels, std::vector<std::uint32_t>& composed,
                 int width, int height, bool nativeMax, int renderScale)
{
    if (nativeMax && height>hud::viewHeight) {
        const double scale=double(height)/hud::viewHeight;
        const double inset=(double(width)/scale-hud::width)*.5;
        for(int y=0;y<hud::viewHeight;++y) {
            const int sy=std::clamp(int((y+.5)*scale),0,height-1);
            for(int x=0;x<hud::width;++x) {
                const int sx=std::clamp(int((inset+x+.5)*scale),0,width-1);
                composed[std::size_t(y)*hud::width+x]=pixels[std::size_t(sy)*width+sx];
            }
        }
    } else if (height>hud::viewHeight) {
        // The original HUD receives a native 320x150 snapshot while the
        // high-res, possibly fisheye-warped 3D remains untouched.
        const int inset=(width/renderScale-hud::width)/2;
        for(int y=0;y<hud::viewHeight;++y)
            for(int x=0;x<hud::width;++x)
                composed[std::size_t(y)*hud::width+x]=
                    pixels[std::size_t(y*renderScale+renderScale/2)*width+
                            (inset+x)*renderScale+renderScale/2];
    } else if (width==hud::width) {
        std::copy(pixels.begin(),pixels.end(),composed.begin());
    } else {
        // The DOS interface still receives precisely 320x200 pixels.
        // Its view area is a centred crop of the expanded world.
        const int inset=(width-hud::width)/2;
        for(int y=0;y<hud::viewHeight;++y)
            std::copy_n(pixels.data()+std::size_t(y)*width+inset,hud::width,
                        composed.data()+std::size_t(y)*hud::width);
    }
}

namespace {

ComposedFrame composeNativeHud(const std::vector<std::uint32_t>& pixels,
    const std::vector<std::uint32_t>& composed, std::vector<std::uint32_t>& wideComposed,
    int width, int height, const hud::Renderer* hudRenderer)
{
    if(!hudRenderer) return {nullptr,0,"Missing HUD for native resolution"};
    const double scale=double(height)/hud::viewHeight;
    const double baseWidth=double(width)/scale;
    const double inset=(baseWidth-hud::width)*.5;
    const int fullHeight=int(std::lround(double(height)*hud::height/hud::viewHeight));
    if(wideComposed.size()!=std::size_t(width)*fullHeight)
        return {nullptr,0,"Invalid native video framebuffer"};
    std::copy(pixels.begin(),pixels.end(),wideComposed.begin());
    for(int y=0;y<hud::viewHeight;++y) {
        const int corner=y<10?cornerCounts[y]:(y>=140?cornerCounts[9-(y-140)]:0);
        const int startY=std::clamp(int(std::lround(y*scale)),0,height);
        const int endY=std::clamp(int(std::lround((y+1)*scale)),0,height);
        const int sourceY=std::clamp(int((y+.5)*scale),0,height-1);
        for(int x=0;x<hud::width;++x) {
            const auto overlay=composed[std::size_t(y)*hud::width+x];
            const int sourceX=std::clamp(int((inset+x+.5)*scale),0,width-1);
            if(overlay==pixels[std::size_t(sourceY)*width+sourceX]) continue;
            double placedX=inset+x;
            if(baseWidth>hud::width) {
                if(x<corner) placedX=x;
                else if(x>=hud::width-corner) placedX=baseWidth-hud::width+x;
            }
            const int startX=std::clamp(int(std::lround(placedX*scale)),0,width);
            const int endX=std::clamp(int(std::lround((placedX+1)*scale)),0,width);
            for(int yy=startY;yy<endY;++yy) {
                auto* row=wideComposed.data()+std::size_t(yy)*width;
                std::fill(row+startX,row+std::max(startX,endX),overlay);
            }
        }
    }
    // Row-correct extension of the original 320x50 instrument panel.
    int previousPanelY=-1;
    for(int y=height;y<fullHeight;++y) {
        const int panelY=std::clamp(int((y-height)/scale),0,hud::height-hud::viewHeight-1);
        auto* row=wideComposed.data()+std::size_t(y)*width;
        // Consecutive physical rows sampling the same HUD row are identical.
        if(panelY==previousPanelY) {
            std::copy_n(row-width,width,row);
            continue;
        }
        previousPanelY=panelY;
        for(int x=0;x<width;++x) {
            const double logicalX=x/scale-inset;
            if(logicalX<0) row[x]=hudRenderer->panelBackgroundEdge(panelY,false);
            else if(logicalX>=hud::width) row[x]=hudRenderer->panelBackgroundEdge(panelY,true);
            else row[x]=composed[std::size_t(panelY+hud::viewHeight)*hud::width+int(logicalX)];
        }
    }
    const float ratio=(4.f/3.f)*(float(width)/(float(hud::width)*float(scale)));
    return {wideComposed.data(),ratio,nullptr};
}

ComposedFrame composeScaledHud(const std::vector<std::uint32_t>& pixels,
    const std::vector<std::uint32_t>& composed, std::vector<std::uint32_t>& wideComposed,
    int width, int height, const hud::Renderer* hudRenderer)
{
    if (!hudRenderer) return {nullptr,0,"Missing original HUD for high-resolution view"};
    const int scale = height / hud::viewHeight;
    if (scale < 2 || width % scale) return {nullptr,0,"Invalid internal rendering scale"};
    const int baseWidth = width / scale;
    if (baseWidth < hud::width || wideComposed.size() !=
        std::size_t(width)*(height+(hud::height-hud::viewHeight)*scale))
        return {nullptr,0,"Invalid high-resolution composition dimensions"};

    const int inset = (baseWidth - hud::width)/2;
    std::copy(pixels.begin(), pixels.end(), wideComposed.begin());

    // DOS overlay is captured independently of any 3D postprocess, so
    // fisheye never distorts HUD, crosshair, weapon or interaction text.
    for (int y=0; y<hud::viewHeight; ++y) {
        const int corner=(y<10) ? cornerCounts[y]
            : (y>=140 ? cornerCounts[9-(y-140)] : 0);
        for (int x=0; x<hud::width; ++x) {
            const auto overlay = composed[std::size_t(y)*hud::width+x];
            const int sourceX = ((inset+x)*scale+scale/2);
            const int sourceY = (y*scale+scale/2);
            if (overlay==pixels[std::size_t(sourceY)*width+sourceX]) continue;
            int placedX = inset+x;
            if (baseWidth>hud::width) {
                if (x<corner) placedX=x;
                else if (x>=hud::width-corner) placedX=baseWidth-hud::width+x;
            }
            for (int dy=0;dy<scale;++dy) {
                auto* row=wideComposed.data()+std::size_t(y*scale+dy)*width;
                std::fill_n(row+placedX*scale,scale,overlay);
            }
        }
    }

    // The 50-line original HUD panel is centred, not stretched sideways.
    for (int panelY=0;panelY<hud::height-hud::viewHeight;++panelY) {
        for (int x=0;x<baseWidth;++x) {
            const auto color=x<inset
                ? hudRenderer->panelBackgroundEdge(panelY,false)
                : (x>=inset+hud::width
                    ? hudRenderer->panelBackgroundEdge(panelY,true)
                    : composed[std::size_t(panelY+hud::viewHeight)*hud::width+(x-inset)]);
            for (int dy=0;dy<scale;++dy) {
                auto* row=wideComposed.data()+
                          std::size_t(height+panelY*scale+dy)*width;
                std::fill_n(row+x*scale,scale,color);
            }
        }
    }
    const float ratio=(4.f/3.f)*(float(baseWidth)/float(hud::width));
    return {wideComposed.data(),ratio,nullptr};
}

ComposedFrame composeWideHud(const std::vector<std::uint32_t>& pixels,
    const std::vector<std::uint32_t>& composed, std::vector<std::uint32_t>& wideComposed,
    int width, const hud::Renderer* hudRenderer)
{
    if (!hudRenderer) return {nullptr,0,"Missing original HUD for widescreen panel"};
    // The world is the full width of the screen. Only the DOS artwork
    // remains at its original 320-pixel width in the centre.
    const int inset=(width-hud::width)/2;
    for (int y=0;y<hud::viewHeight;++y) {
        auto* target=wideComposed.data()+std::size_t(y)*width;
        const auto* world=pixels.data()+std::size_t(y)*width;
        std::copy_n(world,width,target);
        for (int x=0;x<hud::width;++x) {
            const auto overlay=composed[std::size_t(y)*hud::width+x];
            if (overlay==world[inset+x]) continue;
            // Keep the original DOS helmet corner shapes at the edges
            // of the *extended* viewport rather than in the old 4:3 area.
            const int corner=(y<10)?cornerCounts[y]:
                             (y>=140?cornerCounts[9-(y-140)]:0);
            if (x<corner) target[x]=overlay;
            else if (x>=hud::width-corner) target[width-hud::width+x]=overlay;
            else target[inset+x]=overlay;
        }
    }
    // Extend the actual CASQUE[1] background in each row, without
    // stretching or repeating the original instruments or helmet geometry.
    // The DOS 320x50 panel remains pixel-for-pixel unchanged in the middle.
    // The right-side count also handles odd widescreen widths correctly.
    for (int y=hud::viewHeight;y<hud::height;++y) {
        const int panelRow=y-hud::viewHeight;
        auto* destination=wideComposed.data()+std::size_t(y)*width;
        const auto* original=composed.data()+std::size_t(y)*hud::width;
        std::fill_n(destination,inset,hudRenderer->panelBackgroundEdge(panelRow,false));
        std::copy_n(original,hud::width,destination+inset);
        std::fill_n(destination+inset+hud::width,width-inset-hud::width,
                    hudRenderer->panelBackgroundEdge(panelRow,true));
    }
    const float ratio = (4.f/3.f)*(float(width)/float(hud::width));
    return {wideComposed.data(), ratio, nullptr};
}

} // namespace

ComposedFrame composeFrame(const std::vector<std::uint32_t>& pixels,
    const std::vector<std::uint32_t>& composed, std::vector<std::uint32_t>& wideComposed,
    int width, int height, bool withHud, bool nativeComposed, const hud::Renderer* hudRenderer)
{
    if (withHud && nativeComposed && height > hud::viewHeight)
        return composeNativeHud(pixels, composed, wideComposed, width, height, hudRenderer);
    if (withHud && height > hud::viewHeight)
        return composeScaledHud(pixels, composed, wideComposed, width, height, hudRenderer);
    if (withHud && width > hud::width)
        return composeWideHud(pixels, composed, wideComposed, width, hudRenderer);

    // 320x200 was displayed on a 4:3 CRT (non-square VGA pixels).
    const float ratio = withHud ? (4.f/3.f)*(float(width)/float(hud::width)) : 0.f;
    return {(withHud ? composed : pixels).data(), ratio, nullptr};
}

} // namespace extremis
