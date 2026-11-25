#include <cstddef>
#include <cstdint>
#include "port_menu_view.h"

#include "app/startup/port_menu_state.h"
#include "extremis/ui/original_font.h"
#include "extremis/ui/utf8_glyph.h"
#include "port_menu_layout.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>

namespace app::menu_detail {
namespace {
constexpr SDL_Color phosphor{183,255,90,255};
constexpr SDL_Color pale{214,255,171,255};
constexpr SDL_Color dim{93,125,77,255};
constexpr SDL_Color frame{62,112,52,255};
constexpr SDL_Color dark{4,10,6,255};

float pulse(double seconds, float speed, float strength)
{
    return 1.f-strength*.5f+strength*.5f*std::sin(float(seconds)*speed);
}

bool ink(SDL_Renderer* r, SDL_Color c, Uint8 opacity=255)
{
    return SDL_SetRenderDrawColor(r,c.r,c.g,c.b,opacity);
}

bool line(SDL_Renderer* r, float x1,float y1,float x2,float y2,
          SDL_Color c,Uint8 alpha=255)
{
    return ink(r,c,alpha) && SDL_RenderLine(r,x1,y1,x2,y2);
}

bool fill(SDL_Renderer* r, SDL_FRect rect, SDL_Color c, Uint8 alpha)
{
    return ink(r,c,alpha) && SDL_RenderFillRect(r,&rect);
}

bool textAt(SDL_Renderer* r, const original_font::Font* font, std::string_view text,
            float x,float y,float scale, SDL_Color tint,float opacity=1.f)
{
    if (font) return font->draw(r,text,x,y,scale,tint,opacity);
    // SDL's debug font draws ASCII. Use the same UTF-8 decoder and accent strokes as CASQUE.
    if (!ink(r,tint,Uint8(std::clamp(opacity,0.f,1.f)*255.f)) ||
        !SDL_SetRenderScale(r,scale,scale)) return false;
    x/=scale;
    y/=scale;
    std::string base;
    base.reserve(text.size());
    bool drawn=true;
    for (std::size_t i=0;i<text.size() && drawn;) {
        const auto glyph=localization::glyph(text,i);
        i+=glyph.bytes;
        if (glyph.mark)
            drawn=original_font::drawAccent(r,glyph.mark,x+8.f*float(base.size())+4.f,y,1.f);
        base.push_back(char(glyph.base));
    }
    drawn=drawn && SDL_RenderDebugText(r,x,y,base.c_str());
    return SDL_SetRenderScale(r,1.f,1.f) && drawn;
}

float textWidth(const original_font::Font* font,std::string_view s,float scale)
{
    if (font) return font->measure(s)*scale;
    unsigned count=0;
    for (std::size_t i=0;i<s.size();i+=localization::glyph(s,i).bytes) ++count;
    return float(count)*8.f*scale;
}

bool drawCRT(SDL_Renderer* r,double seconds,std::uint32_t seed)
{
    if (!ink(r,dark) || !SDL_RenderClear(r)) return false;

    // Entire screen is a CRT surface: no frame, grid, or moving full-width band.
    for (int y=0;y<400;y+=4) {
        const float center=1.f-std::abs(float(y)-196.f)/235.f;
        const float drift=.5f+.5f*std::sin(float(seconds)*.65f+float(y)*.019f);
        const Uint8 alpha=Uint8(16.f+15.f*std::max(0.f,center)+7.f*drift);
        if (!fill(r,{0.f,float(y),640.f,4.f},{21,50,15,255},alpha)) return false;
    }
    for (int y=0;y<400;y+=3)
        if (!line(r,0.f,float(y),640.f,float(y),{0,0,0,255},32)) return false;

    // Soft edge falloff; it must not read as a decorative rectangular bezel.
    for (int x=0;x<640;x+=8) {
        const float edge=std::abs(float(x+4)-320.f)/320.f;
        if (!fill(r,{float(x),0.f,8.f,400.f},{0,0,0,255},
                  Uint8(36.f*edge*edge))) return false;
    }
    for (int y=0;y<400;y+=8) {
        const float edge=std::abs(float(y+4)-200.f)/200.f;
        if (!fill(r,{0.f,float(y),640.f,8.f},{0,0,0,255},
                  Uint8(29.f*edge*edge))) return false;
    }

    // Independent irregular video-memory faults. Each slot has its own random
    // position, duration, count, and width; no modulo-scheduled repeated sweep.
    const auto tick=std::uint32_t(std::floor(seconds*15.));
    const auto event=hash32(seed ^ (tick*0x9e3779b9u));
    if ((event&255u)<76u) {
        const unsigned count=2u+((event>>12)&7u);
        for (unsigned i=0;i<count;++i) {
            const auto h=hash32(event ^ (i+1u)*0x85ebca6bu);
            const float y=float((h>>13)%398u)+.5f;
            const float x=float((h>>4)%608u);
            const float len=10.f+float((h>>20)%155u);
            const auto alpha=Uint8(13u+(h&47u));
            if (!line(r,x,y,std::min(639.f,x+len),y,phosphor,alpha)) return false;
            if ((h&7u)==1u &&
                !line(r,x+3.f,y+1.f,std::min(639.f,x+len*.75f),y+1.f,
                      {0,0,0,255},65)) return false;
        }
    }
    return true;
}

bool drawTitle(SDL_Renderer* r,const original_font::Font* font,double time,
               std::uint32_t seed)
{
    constexpr std::string_view title="IN EXTREMIS";
    const float scale=3.25f;
    const float span=textWidth(font,title,scale);
    const float x=320.f-span*.5f;
    const float y=74.f;
    const auto tick=std::uint32_t(std::floor(time*20.));
    const auto fault=hash32(seed ^ tick*0x9e3779b9u);
    const bool rupture=(fault&15u)<6u; // Short, frequent irregular corruption.
    const float jitter=rupture?float(int((fault>>12)%17u)-8)*.72f:0.f;
    const float brightness=pulse(time,2.15f,.095f)*(rupture && (fault&255u)<22u?.48f:1.f);

    // Stable phosphor logo, with a displaced image only during signal failure.
    if (!textAt(r,font,title,x-2.f+jitter*.4f,y+1.f,scale,{48,111,47,255},.43f) ||
        !textAt(r,font,title,x+2.f-jitter*.4f,y-1.f,scale,{116,216,83,255},.23f) ||
        !textAt(r,font,title,x+jitter*.16f,y,scale,phosphor,brightness)) return false;

    if (!rupture) return true;
    // True horizontal glyph corruption: black out short scan bands, then
    // reprint their clipped strokes at unpredictable horizontal offsets.
    const unsigned bands=2u+((fault>>8)&3u);
    for (unsigned i=0;i<bands;++i) {
        const auto h=hash32(fault ^ (i+1u)*0xc2b2ae35u);
        const int bandY=int(y)+int((h>>14)%32u);
        const int bandHeight=2+int((h>>24)%6u);
        const SDL_Rect clip{int(x)-18,bandY,int(span)+36,bandHeight};
        if (!SDL_SetRenderClipRect(r,&clip)) return false;
        const float displacement=float(int((h>>4)%27u)-13);
        const bool bandOK=
            fill(r,{float(clip.x),float(clip.y),float(clip.w),float(clip.h)},dark,235) &&
            textAt(r,font,title,x+displacement,y,scale,
                   (h&1u)?phosphor:pale,.70f);
        const bool restoreOK=SDL_SetRenderClipRect(r,nullptr);
        if (!bandOK || !restoreOK) return false;
    }
    // Stray memory shears stay local to the title rather than forming long
    // UI-decoration bars beneath it.
    for (unsigned i=0;i<5;++i) {
        const auto h=hash32(fault ^ (i+7u)*0x27d4eb2du);
        const float sx=x+float(h%std::uint32_t(std::max(1.f,span)));
        const float sy=y+float((h>>13)%36u);
        const float length=5.f+float((h>>21)%48u);
        if (!line(r,sx,sy,std::min(x+span,sx+length),sy,phosphor,
                  Uint8(40u+(h&63u)))) return false;
    }
    return true;
}

// Only submenus have this header. Its restrained ghosting and momentary
// horizontal shears belong to the same damaged phosphor display as the logo.
bool drawPageHeader(SDL_Renderer* r,const original_font::Font* font,
                    const PortMenuState& state,double time,std::uint32_t seed)
{
    if (state.page()==PortMenuPage::Main) return true;
    const std::string_view heading=state.pageTitle();
    constexpr float x=52.f,y=37.f;
    const float scale=std::min(2.45f, (626.f-x)/std::max(1.f,textWidth(font,heading,1.f)));
    const float width=textWidth(font,heading,scale);
    const auto tick=std::uint32_t(std::floor(time*18.));
    const auto noise=hash32(seed ^ (tick+5u)*0x85ebca6bu);
    const bool fault=(noise&15u)<4u;
    const float offset=fault?float(int((noise>>10)&15u)-7)*.42f:0.f;
    const float brightness=pulse(time,1.8f,.08f);

    // Short side phosphor mark, not a decorative box or horizontal rule.
    if (!fill(r,{31.f,y+1.f,4.f,23.f},phosphor,Uint8(115.f*brightness)) ||
        !fill(r,{38.f,y+5.f,2.f,13.f},frame,90) ||
        !textAt(r,font,heading,x+1.5f,y+1.5f,scale,{36,91,39,255},.58f) ||
        !textAt(r,font,heading,x+offset,y,scale,phosphor,brightness)) return false;
    if (!fault) return true;

    // Two narrow displaced slices briefly corrupt the actual heading glyphs.
    for (unsigned i=0;i<2;++i) {
        const auto h=hash32(noise ^ (i+1u)*0x9e3779b9u);
        const int row=int(y)+3+int((h>>9)%14u);
        const SDL_Rect clip{int(x)-3,row,int(width)+12,3};
        if (!SDL_SetRenderClipRect(r,&clip)) return false;
        const float dx=float(int((h>>20)&15u)-7)*.8f;
        const bool drawn=textAt(r,font,heading,x+dx,y,scale,pale,.75f);
        const bool restored=SDL_SetRenderClipRect(r,nullptr);
        if (!drawn || !restored) return false;
    }
    return true;
}

// Each focus change seeds a different complete waveform. The trace grows
// left-to-right through the full selection rectangle, then fades away.
// A fresh, asymmetric spike pattern on every focus change. One fast pass
// across the whole row; its sharp peaks decay before the glow settles.
bool drawSelectionECG(SDL_Renderer* r,float rowY,double age,std::uint32_t seed,
                      float leftEdge,float rightEdge)
{
    constexpr double sweep=.56, end=.75;
    if (age<0. || age>=end) return true;
    constexpr unsigned samples=361;
    const float progress=std::clamp(float(age/sweep),0.f,1.f);
    const int count=std::clamp(int(progress*float(samples-1))+1,2,int(samples));
    const float fadeOut=1.f-std::clamp(float((age-.54)/.21),0.f,1.f);
    const float left=leftEdge+6.f,span=rightEdge-leftEdge-12.f;
    const float baseline=rowY+rowHeight*.51f;
    const unsigned spikes=3u+((seed>>11)&3u);
    std::array<SDL_FPoint,samples> points{};
    for (int i=0;i<count;++i) {
        const float t=float(i)/float(samples-1);
        const float x=left+t*span;
        float wave=.20f*std::sin(t*58.f+float(seed&255u)*.047f);
        for (unsigned j=0;j<spikes;++j) {
            const auto h=hash32(seed ^ (j+1u)*0x9e3779b9u);
            const float jitter=(float(h&255u)/255.f-.5f)*.048f;
            const float center=.10f+.80f*(float(j)+.5f)/float(spikes)+jitter;
            const float halfWidth=.009f+float((h>>8)&255u)/26000.f;
            const float distance=std::abs((t-center)/halfWidth);
            // Narrow triangular peaks, never the smooth, rounded blobs of v4.
            const float apex=std::max(0.f,1.f-distance);
            const float amplitude=8.4f+float((h>>16)&255u)/90.f;
            const float polarity=(h&0x1000000u)?-1.f:1.f;
            wave+=polarity*amplitude*apex;
            const float undershoot=std::max(0.f,1.f-std::abs((t-center-halfWidth*1.45f)/halfWidth));
            wave-=polarity*amplitude*.29f*undershoot;
        }
        points[std::size_t(i)]={x,baseline-std::clamp(wave,-12.f,12.f)};
    }
    if (!ink(r,phosphor,Uint8(60.f*fadeOut)) ||
        !SDL_RenderLines(r,points.data(),count) ||
        !ink(r,pale,Uint8(160.f*fadeOut)) ||
        !SDL_RenderLines(r,points.data(),count)) return false;
    const auto head=points[std::size_t(count-1)];
    const SDL_FRect spark{head.x-1.f,head.y-1.f,2.f,2.f};
    return fill(r,spark,pale,Uint8(120.f*fadeOut));
}

bool drawOptions(SDL_Renderer* r,const original_font::Font* font,
                 const PortMenuState& state,double time,double selectionAge,
                 std::uint32_t seed)
{
    const bool submenu=state.page()!=PortMenuPage::Main;
    const float left=submenu?submenuLeft:menuLeft;
    const float right=submenu?submenuRight:menuRight;
    const float width=right-left;
    const auto count=state.rowCount();
    // A single fixed scale for all submenu values, including their longest
    // alternatives. Changing resolution, language or scheme never resizes text.
    constexpr std::array<std::string_view,5> largestLabels{{
        "RESOLUTION : 3840 X 2160 RETINA", "FLOOR AND CEILING : OFF",
        "CONTROL SCHEME : CLASSIC", "SPANISH - SELECTED",
        "MASTER VOLUME : 100"
    }};
    float reference=1.f;
    for (const auto label:largestLabels) reference=std::max(reference,textWidth(font,label,1.f));
    const float glyphScale=submenu?std::min(1.90f,(width-28.f)/reference):2.05f;
    for (unsigned i=0;i<count;++i) {
        const float y=rowTop(i,count,submenu);
        const bool selected=state.selection()==i;
        const auto label=state.label(i);
        const auto tint=selected?phosphor:pale; // No dimmed placeholder entries.
        if (selected) {
            const float phase=float(time*3.1+double(i)*1.8);
            const float opacity=.64f+.36f*std::sin(phase);
            // Same transparent green field, noise and ECG as the approved v5.
            if (!fill(r,{left,y,width,rowHeight},frame,
                      Uint8(43+21*opacity)) ||
                !fill(r,{left+10.f,y+8.f,3.f,18.f},phosphor,
                      Uint8(140+60*opacity))) return false;
            const float scan=left+24.f+float(std::fmod(time*86.,width-50.f));
            if (!line(r,scan,y+2.f,scan,y+rowHeight-2.f,phosphor,42)) return false;
            const auto tick=std::uint32_t(std::floor(time*24.));
            const auto fault=hash32(seed ^ (tick+1u)*0x85ebca6bu);
            if ((fault&7u)<3u) {
                for (unsigned j=0;j<3;++j) {
                    const auto h=hash32(fault ^ (j+1u)*0xc2b2ae35u);
                    const float x=left+8.f+float((h>>5)%unsigned(width-66.f));
                    const float len=7.f+float((h>>15)%37u);
                    const float tearY=y+4.f+float((h>>21)%26u);
                    if (!line(r,x,tearY,std::min(right-4.f,x+len),tearY,dark,165) ||
                        !line(r,x+float(int((h>>24)%9u)-4),tearY+1.f,
                              std::min(right-4.f,x+len),tearY+1.f,
                              phosphor,120)) return false;
                }
            }
            if (!drawSelectionECG(r,y,selectionAge,seed,left,right)) return false;
        }
        // Scale is fixed for the page; only horizontal centering follows the value.
        const float fittedScale=std::min(glyphScale,(width-32.f)/std::max(1.f,textWidth(font,label,1.f)));
        const float x=320.f-textWidth(font,label,fittedScale)*.5f;
        const float textY=y+(rowHeight-10.f*fittedScale)*.5f;
        if (selected && !textAt(r,font,label,x+2.f,textY+1.f,fittedScale,phosphor,.20f))
            return false;
        if (!textAt(r,font,label,x,textY,fittedScale,tint,selected?1.f:.80f)) return false;
    }
    if (state.page()==PortMenuPage::Video) {
        const std::string_view notice = localization::byEnglish(state.vsyncChangeFailed()
            ? "VSYNC SWITCH FAILED"
            : !state.videoModeAvailable()
            ? "DISPLAY MODE SWITCH FAILED"
            : state.aspectChoice()==0 ? "4:3 ORIGINAL DOS VIEW"
            : state.aspectChoice()==1 ? "16:9 EXPANDED GAME VIEW"
            : state.aspectChoice()==2 ? "21:9 EXPANDED GAME VIEW"
                                      : "AUTO: MATCH MONITOR ASPECT",state.language());
        const float x=320.f-textWidth(font,notice,1.f)*.5f;
        if (!textAt(r,font,notice,x,355.f,1.f,pale,.88f)) return false;
    }
    return true;
}

} // namespace

bool renderPortMenu(SDL_Renderer* r,const original_font::Font* font,
            const PortMenuState& state,double seconds,double selectionAge,
            std::uint32_t effectSeed,std::uint32_t selectionSeed)
{
    if (!drawCRT(r,seconds,effectSeed) ||
        !(state.page()==PortMenuPage::Main
            ? drawTitle(r,font,seconds,effectSeed)
            : drawPageHeader(r,font,state,seconds,effectSeed)) ||
        !drawOptions(r,font,state,seconds,selectionAge,selectionSeed)) return false;

    // Only the unobtrusive source-port credit remains in the bottom-right corner.
    constexpr std::string_view credit="EXTREMIS RE SOURCE PORT";
    const float scale=.68f;
    const float x=626.f-textWidth(font,credit,scale);
    if (!textAt(r,font,credit,x,382.f,scale,dim,.65f)) return false;
    return SDL_RenderPresent(r);
}
} // namespace app::menu_detail

namespace app::menu_detail {

std::uint32_t hash32(std::uint32_t x)
{
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}

} // namespace app::menu_detail
