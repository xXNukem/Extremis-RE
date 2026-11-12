#include "startup_credits.h"
#include "spr_format.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr float width=640.f, height=400.f;
constexpr double duration=27.4, firstEnd=7.5, secondEnd=16.8, titleStart=21.0, crtOffStart=26.05;
constexpr double pi=3.14159265358979323846;

struct Sweep { double at, travel; float damage; unsigned seed; };
constexpr std::array<Sweep,15> sweeps{{
    {.64,1.11,.08f, 0}, {2.42,1.08,.13f, 1}, {4.25,1.05,.16f, 2}, {6.02,.97,.21f, 3},
    {7.73,.98,.27f, 4}, {9.15,.92,.33f, 5}, {10.42,.88,.40f, 6},
    {11.66,.82,.49f, 7}, {12.81,.77,.56f, 8}, {13.89,.72,.65f, 9},
    {14.98,.72,.71f,10},
    {16.63,.89,.84f,11}, {17.58,.78,.92f,12},
    {18.46,.71,.98f,13}, {19.24,.66,1.00f,14}
}};

float fade(double t, double seconds)
{
    const double v=std::clamp(t/seconds,0.0,1.0);
    return float(v*v*(3.0-2.0*v));
}

Uint8 opacity(float value) { return Uint8(std::clamp(value,0.f,1.f)*255.f); }

struct Rgb { Uint8 red, green, blue; };
Rgb ecgTint(float damage)
{
    struct Stop { float position; Rgb color; };
    constexpr std::array<Stop,6> palette{{
        {0.f, {116,255,156}},
        {.28f,{154,255, 94}},
        {.53f,{240,216, 56}},
        {.72f,{255,134, 34}},
        {.86f,{255, 57, 49}},
        {1.f, {255, 22, 42}}
    }};
    const float progress=std::clamp((damage-.08f)/.92f,0.f,1.f);
    for (std::size_t i=1;i<palette.size();++i) {
        if (progress>palette[i].position) continue;
        const auto& from=palette[i-1], &to=palette[i];
        const float blend=(progress-from.position)/(to.position-from.position);
        const auto mix=[blend](Uint8 a,Uint8 b) {
            return Uint8(std::lround(float(a)+(float(b)-float(a))*blend));
        };
        return {mix(from.color.red,to.color.red),mix(from.color.green,to.color.green),
                mix(from.color.blue,to.color.blue)};
    }
    return palette.back().color;
}

bool color(SDL_Renderer* r, Uint8 red, Uint8 green, Uint8 blue, float alpha=1.f)
{
    return SDL_SetRenderDrawColor(r,red,green,blue,opacity(alpha));
}

// Original DOS font: CASQUE.SPR glyphs 42..85, exactly as used by the HUD.
// Every non-zero palette index is an opaque glyph pixel; tint is applied at draw time.
// Keep the original resource external: the port never ships game assets.
struct OriginalFont {
    std::array<SDL_Texture*,44> glyphs{};
    std::array<int,44> glyphWidths{};
    bool ready=false;

    ~OriginalFont() { clear(); }
    void clear()
    {
        for (auto*& texture : glyphs) {
            if (texture) SDL_DestroyTexture(texture);
            texture=nullptr;
        }
        ready=false;
    }

    bool load(SDL_Renderer* r, const std::filesystem::path& path)
    {
        clear();
        const auto sprites=spr_format::load(path);
        if (sprites.size()!=86) return false;
        for (std::size_t i=42; i<86; ++i) {
            const auto& sprite=sprites[i];
            if (sprite.width<=0 || sprite.width>32 || sprite.height!=10 ||
                sprite.indices.size()!=std::size_t(sprite.width)*sprite.height) {
                clear();
                return false;
            }
            std::vector<std::uint32_t> pixels(sprite.indices.size(),0);
            for (std::size_t p=0;p<pixels.size();++p)
                if (sprite.indices[p]) pixels[p]=0xffffffffu;
            auto* texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,
                                            SDL_TEXTUREACCESS_STATIC,sprite.width,10);
            if (!texture || !SDL_UpdateTexture(texture,nullptr,pixels.data(),sprite.width*4) ||
                !SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND) ||
                !SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST)) {
                if (texture) SDL_DestroyTexture(texture);
                clear();
                return false;
            }
            glyphs[i-42]=texture;
            glyphWidths[i-42]=sprite.width;
        }
        ready=true;
        return true;
    }

    static int index(unsigned char c)
    {
        if (c>='a' && c<='z') c=static_cast<unsigned char>(c-'a'+'A');
        if (c>='A' && c<='Z') return int(c-'A');
        if (c>='0' && c<='9') return 26+int(c-'0');
        switch (c) {
        case '?': return 36;
        case '!': return 37;
        case 39: return 38; // Apostrophe
        case '.': return 39;
        case ',': return 40;
        case ':': return 41;
        case '-': return 42;
        case 255: return 43;
        default: return -1;
        }
    }

    float advance(unsigned char c) const
    {
        if (c==' ') return 9.f;
        const int id=index(c);
        if (id>=0) return float(glyphWidths[std::size_t(id)]+1);
        return 8.f;
    }

    float extent(std::string_view text) const
    {
        float result=0.f;
        for (unsigned char c : text) result+=advance(c);
        return text.empty()?0.f:result-1.f;
    }

    bool glyph(SDL_Renderer* r, unsigned char c, float x, float y, float scale,
               Uint8 red, Uint8 green, Uint8 blue, float alpha) const
    {
        if (c==' ') return true;
        const int id=index(c);
        if (id>=0) {
            auto* texture=glyphs[std::size_t(id)];
            const SDL_FRect destination{x,y,float(glyphWidths[std::size_t(id)])*scale,10.f*scale};
            return SDL_SetTextureColorMod(texture,red,green,blue) &&
                   SDL_SetTextureAlphaMod(texture,opacity(alpha)) &&
                   SDL_RenderTexture(r,texture,nullptr,&destination);
        }
        // Parentheses are absent from CASQUE.SPR; construct their small
        // pixel-strokes instead of mixing a second font into the author credit.
        if (c=='(' || c==')') {
            constexpr std::array<int,10> columns{{2,1,1,0,0,0,0,1,1,2}};
            if (!color(r,red,green,blue,alpha)) return false;
            for (int row=0; row<10; ++row) {
                const int gx=c=='('?columns[std::size_t(row)]:3-columns[std::size_t(row)];
                const SDL_FRect pixel{x+(2.f+float(gx))*scale,y+float(row)*scale,scale,scale};
                if (!SDL_RenderFillRect(r,&pixel)) return false;
            }
            return true;
        }
        // Fallback for characters not present in the original game's charset.
        if (!SDL_SetRenderScale(r,scale,scale)) return false;
        const char symbol[]={char(c),0};
        const bool ok=color(r,red,green,blue,alpha) &&
                      SDL_RenderDebugText(r,x/scale,y/scale,symbol);
        return SDL_SetRenderScale(r,1.f,1.f) && ok;
    }
};

OriginalFont* introFont=nullptr;

float fontAdvance(unsigned char c) { return introFont?introFont->advance(c):8.f; }
float textExtent(std::string_view text) {
    return introFont?introFont->extent(text):float(text.size())*8.f;
}

bool textAt(SDL_Renderer* r, const char* str, float x, float y, float scale,
            Uint8 red, Uint8 green, Uint8 blue, float alpha)
{
    if (!introFont) {
        if (!SDL_SetRenderScale(r,scale,scale)) return false;
        const bool ok=color(r,red,green,blue,alpha) &&
                      SDL_RenderDebugText(r,x/scale,y/scale,str);
        return SDL_SetRenderScale(r,1.f,1.f) && ok;
    }
    for (const unsigned char c : std::string_view(str)) {
        if (!introFont->glyph(r,c,x,y,scale,red,green,blue,alpha)) return false;
        x+=fontAdvance(c)*scale; // Original DOS font is proportional.
    }
    return true;
}

bool digitalText(SDL_Renderer* r, const std::string& text, float x, float y, float scale,
                 Uint8 red, Uint8 green, Uint8 blue, float alpha,
                 float tracking=0.f)
{
    for (char c : text) {
        const char symbol[]={c,0};
        if (!textAt(r,symbol,x,y,scale,red,green,blue,alpha)) return false;
        x+=(fontAdvance(static_cast<unsigned char>(c))+tracking)*scale;
    }
    return true;
}

struct TextFault {
    bool active, blackout;
    float dx, dy, skew, intensity;
    unsigned phase;
};

TextFault textFault(double time, float severity)
{
    const unsigned tick=unsigned(std::floor(time*36.0));
    const bool fault=(tick%61>=8 && tick%61<=14) ||
                     (tick%43>=23 && tick%43<=28) ||
                     (tick%97>=63 && tick%97<=69) ||
                     (tick%131>=49 && tick%131<=53);
    const bool blackout=(tick%61==11 || tick%43==26 || tick%97==66 || tick%131==51);
    const float dx=fault?float(int((tick*37u)%31u)-15)*.52f*severity:0.f;
    const float dy=fault?float(int((tick*23u)%17u)-8)*.38f*severity:0.f;
    const float skew=fault?float(int((tick*11u)%9u)-4)*.65f*severity:0.f;
    const float intensity=blackout?.05f:(fault?.46f:1.f);
    return {fault,blackout,dx,dy,skew,intensity,tick};
}

bool unstableText(SDL_Renderer* r, std::string_view text, float x, float y, float scale,
                  Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                  float severity=1.f, bool digital=false)
{
    if (text.empty() || alpha<=0.f) return true;
    const auto fault=textFault(time,severity);
    const std::string copy(text);
    bool ok=true;

    auto renderLayer=[&](float dx,float dy, Uint8 rr, Uint8 gg, Uint8 bb, float a, float tracking=0.f) {
        return digital ? digitalText(r,copy,x+dx,y+dy,scale,rr,gg,bb,a,tracking)
                       : textAt(r,copy.c_str(),x+dx,y+dy,scale,rr,gg,bb,a);
    };

    if (fault.active) {
        ok = renderLayer(fault.dx-9.f, fault.dy+4.f, 44,255,122, alpha*.28f*fault.intensity, .6f) && ok;
        ok = renderLayer(-fault.dx*1.4f, -fault.dy*1.2f, 198,255,214, alpha*.18f*fault.intensity, .2f) && ok;
        ok = renderLayer(fault.skew*1.2f, fault.dy-3.f, 96,255,152, alpha*.20f*fault.intensity, .35f) && ok;
    }
    ok = renderLayer(fault.dx, fault.dy, red, green, blue, alpha*fault.intensity, .2f) && ok;
    if (!ok) return false;

    if (fault.active) {
        const float span=textExtent(copy)*scale;
        for (unsigned i=0;i<5;++i) {
            const float fx=x+float((fault.phase*29u+i*41u)%unsigned(std::max(2.f,span*.58f)));
            const float fy=y+float(int((fault.phase*17u+i*13u)%31u)-15);
            const float len=span*(.08f+.09f*float(i%3));
            // Screen tearing is HORIZONTAL; two-dimensional glitches come
            // from the displaced text layers, not diagonal scratch lines.
            if (!color(r,54,255,126,alpha*(fault.blackout?.20f:.30f)) ||
                !SDL_RenderLine(r,fx,fy,fx+len,fy)) return false;
        }
        for (unsigned i=0;i<3;++i) {
            const float bx=x+float((fault.phase*47u+i*19u)%unsigned(std::max(2.f,span*.45f)));
            const float by=y+float(int((fault.phase*31u+i*7u)%29u)-14);
            if (!color(r,220,255,228,alpha*.10f) ||
                !SDL_RenderLine(r,bx,by,bx+11.f+7.f*i,by)) return false;
        }
    }
    return true;
}

bool unstableCentered(SDL_Renderer* r, const std::string& text, float y, float scale,
                      Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                      float severity=1.f, bool digital=false)
{
    const float x=(width-textExtent(text)*scale)*.5f;
    return unstableText(r,text,x,y,scale,red,green,blue,alpha,time,severity,digital);
}

bool accentOnPerez(SDL_Renderer* r, float y, float scale, float alpha)
{
    constexpr std::string_view name="Carlos (xXNukem) de la Barrera Perez";
    const auto e=name.find("Perez")+1;
    const float x=(width-textExtent(name)*scale)*.5f+
                  textExtent(name.substr(0,e))*scale+4.f*scale;
    return color(r,195,255,209,alpha) && SDL_RenderLine(r,x-2,y-3,x+1,y-7);
}

float triangle(float x, float center, float halfWidth)
{
    return std::max(0.f,1.f-std::abs(x-center)/halfWidth);
}

// Small deterministic PRNG: one different waveform per pulse, and a different
// set of peak positions on every launch. The shape stays fixed WHILE sweeping.
std::uint32_t mixBits(std::uint32_t value)
{
    value^=value>>16;
    value*=0x7feb352du;
    value^=value>>15;
    value*=0x846ca68bu;
    return value^(value>>16);
}

float pulseRandom(unsigned beat, unsigned salt, std::uint32_t launchSeed)
{
    const auto n=mixBits(launchSeed ^ (beat+1u)*0x9e3779b9u ^
                         (salt+1u)*0x85ebca6bu);
    return float(n & 0xFFFFu)/65535.f;
}

struct WaveShape {
    float p, q, r, s, t, extra;
    float pAmp, rAmp, sAmp, tAmp, extraAmp;
    float pWidth, rWidth, sWidth, tWidth;
    unsigned variation;
};

WaveShape makeWaveShape(const Sweep& beat, std::uint32_t launchSeed)
{
    const auto rand=[&](unsigned salt, float a, float b) {
        return a+(b-a)*pulseRandom(beat.seed,salt,launchSeed);
    };
    const float r=rand(0,107.f,516.f); // QRS can strike virtually anywhere.
    return {
        std::max(25.f,r-rand(1,50.f,94.f)),
        r-rand(2,15.f,30.f),
        r,
        r+rand(3,14.f,34.f),
        std::min(615.f,r+rand(4,58.f,110.f)),
        rand(5,63.f,577.f), // Extra arrhythmic peak never has a fixed position.
        rand(6,.08f,.25f),
        rand(7,.70f,1.30f),
        rand(8,.42f,.90f),
        rand(9,.12f,.37f),
        rand(10,.16f,.45f),
        rand(11,23.f,45.f),
        rand(12,6.f,18.f)+beat.damage*7.f,
        rand(13,11.f,25.f),
        rand(14,25.f,62.f),
        (beat.seed+unsigned(launchSeed%7u))%6u
    };
}

float waveform(float x, const Sweep& beat, const WaveShape& shape)
{
    const float d=beat.damage;
    const float seed=float(beat.seed);
    float signal=shape.pAmp*triangle(x,shape.p,shape.pWidth)
                -.16f*triangle(x,shape.q,15.f)
                +shape.rAmp*triangle(x,shape.r,shape.rWidth)
                -shape.sAmp*triangle(x,shape.s,shape.sWidth)
                +shape.tAmp*triangle(x,shape.t,shape.tWidth);
    // Some beats are short, some double, some weak or inverted; random peak
    // positions alone are not enough to make the sweeps look different.
    if (shape.variation==1 || shape.variation==4)
        signal+=shape.extraAmp*(.35f+.65f*d)*triangle(x,shape.extra,11.f+d*6.f);
    if (shape.variation==2)
        signal-=shape.extraAmp*(.40f+.60f*d)*triangle(x,shape.extra,15.f);
    if (shape.variation==3)
        signal+=.56f*(.3f+.7f*d)*triangle(x,shape.extra,8.f)
               -.30f*d*triangle(x,shape.extra+23.f,16.f);
    if (shape.variation==5) signal*=.60f+.35f*d;
    if (d>.60f) {
        const float chaos=(d-.60f)/.40f;
        signal+=chaos*(.19f*std::sin(x*.23f+seed*1.7f)
                      +.12f*std::sin(x*.57f+seed*.9f)
                      +.07f*std::sin(x*.92f+seed*2.6f));
        signal+=chaos*(.43f*triangle(x,shape.extra,8.f)
                      -.29f*triangle(x,shape.extra+21.f,13.f));
        if (shape.variation==0) signal*=(1.f-.58f*chaos);
        if (shape.variation==4) signal=-signal*.79f;
    }
    return signal+.02f*std::sin(x*.026f+seed*1.1f);
}

float textFlicker(double time)
{
    const int tick=int(std::floor(time*26.0));
    const bool dropout=(tick%53>=11 && tick%53<=14) ||
                       (tick%37==17) ||
                       (tick%67>=39 && tick%67<=42) ||
                       (tick%89>=58 && tick%89<=61);
    const float brightness=dropout?.06f:(.80f+.20f*std::sin(float(time)*51.f));
    return brightness*(.76f+.24f*fade(time,.7));
}

bool blackBackground(SDL_Renderer* r)
{
    return color(r,0,0,0) && SDL_RenderClear(r);
}

bool phosphorSweep(SDL_Renderer* r, const Sweep& beat, double now, std::uint32_t launchSeed)
{
    const double age=now-beat.at;
    if (age<0.0 || age>=beat.travel+.38) return true;
    const float head=std::clamp(float(age/beat.travel)*width,0.f,width);
    if (head<2.f) return true;
    const float after=(1.f-fade(age-beat.travel,.38));
    const float flare=fade(age,.060)*after;
    const float gain=(1.f+.18f*beat.damage)*flare;
    const float amplitude=123.f+40.f*beat.damage;
    const auto tint=ecgTint(beat.damage);
    const auto shape=makeWaveShape(beat,launchSeed);

    std::array<SDL_FPoint,65> points{};
    for (int chunk=0; chunk<10; ++chunk) {
        const int begin=chunk*64, end=std::min(640,int(head));
        if (end<=begin) break;
        const int stop=std::min(begin+64,end);
        if (beat.damage>.8f && (unsigned(chunk)+beat.seed*3)%17==0) continue;
        const float midpoint=float(begin+stop)*.5f;
        const float distance=std::max(0.f,(head-midpoint)/width);
        const float afterglow=.18f+.82f*std::exp(-3.8f*distance);
        const float brightness=gain*afterglow;
        if (brightness<.02f) continue;
        for (int pass=0; pass<5; ++pass) {
            const float offset=pass==0?-7.f:pass==1?7.f:pass==2?-2.5f:pass==3?2.5f:0.f;
            const float strength=pass<2?.08f:pass<4?.21f:1.f;
            const float bleed=pass<2?.52f:pass<4?.84f:1.f;
            if (!color(r,Uint8(tint.red*bleed),Uint8(tint.green*bleed),Uint8(tint.blue*bleed),brightness*strength)) return false;
            const int n=stop-begin+1;
            for (int j=0; j<n; ++j) {
                const float x=float(begin+j);
                points[std::size_t(j)]={x,206.f-amplitude*waveform(x,beat,shape)+offset};
            }
            if (!SDL_RenderLines(r,points.data(),n)) return false;
        }
    }
    return true;
}

bool drawECG(SDL_Renderer* r, double time, std::uint32_t launchSeed)
{
    if (time>=titleStart) return true;
    for (const auto& beat:sweeps)
        if (!phosphorSweep(r,beat,time,launchSeed)) return false;
    return true;
}

bool drawCrtOff(SDL_Renderer* r, double time)
{
    const double local=time-crtOffStart;
    if (local<=0.0) return true;
    const float collapse=fade(local,.42);
    const float vanish=fade(local-.30,.28);
    const float centerY=height*.5f;
    const float half=(1.f-collapse)*height*.5f;
    const float glow=(1.f-vanish);
    if (glow<=0.f) return true;

    const SDL_FRect flash{0.f,0.f,width,height};
    if (!color(r,255,255,255,.05f*glow) || !SDL_RenderFillRect(r,&flash)) return false;
    if (!color(r,255,242,206,.85f*glow) ||
        !SDL_RenderLine(r,0.f,centerY-half,width,centerY-half) ||
        !SDL_RenderLine(r,0.f,centerY+half,width,centerY+half)) return false;
    if (!color(r,255,255,255,.95f*glow)) return false;
    if (half>1.5f) {
        for (float y=centerY-half;y<=centerY+half;y+=1.f)
            if (!SDL_RenderLine(r,width*.5f-1.f,y,width*.5f+1.f,y)) return false;
    } else {
        const float radius=(1.f-vanish)*10.f;
        if (!SDL_RenderLine(r,width*.5f-radius,centerY,width*.5f+radius,centerY) ||
            !SDL_RenderLine(r,width*.5f,centerY-radius,width*.5f,centerY+radius)) return false;
    }
    return true;
}

// Infrequent asynchronous dropouts, with different timings for the two
// title words and each bar. No incessant sine-wave strobing of the logo.
float logoBrightness(double local, unsigned channel)
{
    constexpr std::array<std::array<double,6>,4> events{{
        {{.47,1.31,2.09,3.37,4.29,4.82}}, // EXTREMIS
        {{.79,1.94,2.64,3.85,4.63,5.04}}, // RE
        {{.22,1.59,2.86,3.54,4.38,5.09}}, // Top bar
        {{.69,1.83,2.37,3.92,4.71,5.19}}  // Bottom bar
    }};
    float brightness=1.f;
    for (const double at:events[channel]) {
        const double age=local-at;
        if (age>=0.0 && age<.075) brightness=std::min(brightness,.16f);
        else if (age>=.075 && age<.12) brightness=std::min(brightness,.52f);
        else if (age>=.12 && age<.17) brightness=std::min(brightness,.86f);
    }
    return brightness;
}

// After the title has fully appeared, a few long synchronization failures
// displace narrow slices of the actual glyphs. The smooth envelopes avoid
// rapid flashing: the intact logo is visible between each failure.
struct TitleRupture { float strength; unsigned phase; };
TitleRupture titleRupture(double local)
{
    struct Event { double at, length; };
    constexpr std::array<Event,4> events{{
        {1.18,.57}, {2.09,.93}, {3.33,.64}, {4.41,.71}
    }};
    for (unsigned i=0;i<events.size();++i) {
        const auto& event=events[i];
        const double age=local-event.at;
        if (age>=0.0 && age<event.length) {
            const float envelope=std::sin(float(pi*age/event.length));
            return {envelope*envelope,i};
        }
    }
    return {0.f,0};
}

// Repaint only two horizontally torn pieces of the title, leaving most
// of the lettering readable. Clipping keeps the displaced text confined
// to the corrupted scan bands; the original two title bars remain aligned.
bool drawTitleRupture(SDL_Renderer* r, float left, float y, float scale,
                      float wordX, float reX, float wordY, float reY,
                      float alpha, TitleRupture rupture)
{
    const float strength=rupture.strength;
    if (strength<.015f || alpha<=0.f) return true;
    constexpr std::array<int,3> offsets{{0,13,-7}};
    const float glyphHeight=10.f*scale;
    for (unsigned band=0;band<2;++band) {
        const int bandY=int(y+6.f+float((rupture.phase*11u+band*19u)%24u));
        const int bandH=6+int(7.f*strength)+(band==1?2:0);
        const SDL_Rect clip{int(left)-5,bandY,int(textExtent("EXTREMIS RE")*scale)+10,bandH};
        // Erase at logical coordinates; texture glyphs use that same space.
        if (!SDL_SetRenderClipRect(r,&clip)) return false;
        const SDL_FRect dark{float(clip.x),float(clip.y),float(clip.w),float(clip.h)};
        const bool erased=color(r,0,0,0,.95f*strength) && SDL_RenderFillRect(r,&dark);
        const bool unclipped=SDL_SetRenderClipRect(r,nullptr);
        if (!erased || !unclipped) return false;
        const SDL_Rect glyphClip=clip;
        if (!SDL_SetRenderClipRect(r,&glyphClip)) return false;
        const float direction=(band==0?1.f:-1.f);
        const float displacement=direction*(7.f+13.f*strength)+float(offsets[rupture.phase]);
        const bool ok=textAt(r,"EXTREMIS",wordX+displacement,wordY+direction*2.f*strength,
                             scale,255,72,89,alpha*.86f*strength) &&
                      textAt(r,"RE",reX+displacement,reY-direction*2.f*strength,
                             scale,255,72,89,alpha*.86f*strength);
        const bool restored=SDL_SetRenderClipRect(r,nullptr);
        if (!ok || !restored) return false;
    }

    // Slow scar-like tears drift out from the lettering in several directions.
    for (unsigned i=0;i<7;++i) {
        const std::uint32_t seed=mixBits((rupture.phase+1u)*137u ^ (i+1u)*941u);
        const float x=left-18.f+float(seed%420u);
        const float row=y+float((seed>>11)%unsigned(glyphHeight+18.f))-6.f;
        const float dx=float(int((seed>>19)&31u)-15)*(.55f+.95f*strength);
        const float dy=float(int((seed>>25)&15u)-7)*(.35f+.65f*strength);
        const float length=12.f+float((seed>>7)%74u)*(.25f+.75f*strength);
        if (!color(r,238,42,63,alpha*strength*(i%2==0?.34f:.18f)) ||
            !SDL_RenderLine(r,x,row,std::min(width-16.f,x+length+dx),row+dy)) return false;
    }
    return true;
}


struct GlyphWarp {
    bool active;
    float dx, dy, alpha, smear;
};

GlyphWarp glyphWarp(double local, unsigned channel, unsigned glyph)
{
    const std::uint32_t h0=mixBits((channel+1u)*0x9e3779b9u ^ (glyph+3u)*0x85ebca6bu ^ 0x7f4a7c15u);
    const double speed=2.05 + double((h0>>4)&7u)*0.19 + double(channel)*0.07;
    const double offset=double((h0>>13)&1023u)/1024.0;
    const double t=local*speed + offset;
    const double cycle=t-std::floor(t);
    const double window=.11 + double((h0>>25)&3u)*.045;
    if (cycle>window) return {false,0.f,0.f,0.f,0.f};
    const float envelope=std::sin(float(pi*cycle/window));
    const unsigned burst=unsigned(std::floor(t));
    const std::uint32_t h=mixBits(h0 ^ (burst+1u)*0xc2b2ae35u);
    const float sx=float(int((h>>7)&31u)-15);
    const float sy=float(int((h>>15)&31u)-15);
    const float dx=sx*(.18f+.78f*envelope);
    const float dy=sy*(.14f+.72f*envelope);
    const float alpha=.18f+.52f*envelope;
    const float smear=.35f+.95f*envelope;
    return {true,dx,dy,alpha,smear};
}

bool corruptedWord(SDL_Renderer* r, std::string_view word, float x, float y, float scale,
                   Uint8 red, Uint8 green, Uint8 blue, float alpha,
                   double local, unsigned channel)
{
    if (alpha<=0.f) return true;
    float gx=x;
    bool ok=true;
    for (unsigned i=0;i<word.size();++i) {
        const std::string glyph(1,word[i]);
        const float step=fontAdvance(static_cast<unsigned char>(word[i]))*scale;
        const auto warp=glyphWarp(local,channel,i);
        if (warp.active) {
            ok = ok && textAt(r,glyph.c_str(),gx+warp.dx*1.55f,y+warp.dy*1.35f,scale,
                              255,92,104,alpha*warp.alpha*.36f);
            ok = ok && textAt(r,glyph.c_str(),gx-warp.dx*.82f,y-warp.dy*.68f,scale,
                              120,8,18,alpha*warp.alpha*.24f);
            for (unsigned j=0;j<4;++j) {
                const std::uint32_t lineSeed=mixBits((channel+1u)*97u ^ (i+1u)*313u ^ (j+1u)*911u ^ unsigned(std::floor(local*53.0))*17u);
                const float lx=gx+float(int((lineSeed>>3)&31u)-15);
                const float ly=y+float(int((lineSeed>>11)&31u)-15);
                const float ex=lx+float(int((lineSeed>>19)&31u)-15)*warp.smear;
                const float ey=ly+float(int((lineSeed>>25)&31u)-15)*warp.smear;
                ok = ok && color(r,255,60,74,alpha*.15f*warp.alpha) && SDL_RenderLine(r,lx,ly,ex,ey);
            }
            for (unsigned j=0;j<3;++j) {
                const std::uint32_t cutSeed=mixBits((channel+1u)*701u ^ (i+1u)*1499u ^ (j+1u)*3203u ^ unsigned(std::floor(local*41.0))*19u);
                const float cx=gx+step*.5f+float(int((cutSeed>>4)&15u)-7);
                const float cy=y+4.f*scale+float(int((cutSeed>>12)&15u)-7);
                const float dx=float(int((cutSeed>>20)&31u)-15)*(.35f+.55f*warp.smear);
                const float dy=float(int((cutSeed>>26)&15u)-7)*(.55f+.75f*warp.smear);
                ok = ok && color(r,0,0,0,alpha*.16f*warp.alpha);
                for (unsigned k=0;k<3;++k) {
                    const float ox=float(k)-1.f;
                    ok = ok && SDL_RenderLine(r,cx+ox,cy+ox*.35f,cx+dx+ox,cy+dy+ox*.35f);
                }
                if (!ok) return false;
            }
        }
        ok = ok && textAt(r,glyph.c_str(),gx,y,scale,red,green,blue,alpha);
        if (!ok) return false;
        gx+=step;
    }
    return ok;
}

bool drawCredit(SDL_Renderer* r, double time, std::uint32_t launchSeed)
{
    if (!blackBackground(r) || !drawECG(r,time,launchSeed)) return false;
    if (time<firstEnd) {
        const float appear=fade(time-.95,1.2);
        const float visible=appear*(1.f-fade(time-(firstEnd-1.50),1.35));
        if (!unstableCentered(r,"CYANIDE SOFTWARE PRESENTS",172.f,2.55f,
                              178,255,196,visible*textFlicker(time),time,1.05f,true)) return false;
    } else if (time<secondEnd) {
        const double local=time-firstEnd;
        const float appear=fade(local-.85,1.15);
        const float visible=appear*(1.f-fade(time-(secondEnd-1.55),1.35));
        const float intensity=visible*textFlicker(time);
        if (!unstableCentered(r,"A SOURCE PORT BY",142.f,2.75f,
                              175,252,198,intensity,time,1.28f,true)) return false;
        constexpr std::string_view name="Carlos (xXNukem) de la Barrera Perez";
        const float scale=1.85f;
        const unsigned count=unsigned(std::clamp((local-1.48)*8.1,0.0,double(name.size())));
        const float origin=(width-textExtent(name)*scale)*.5f;
        const std::string typed(name.substr(0,count));
        if (!unstableText(r,typed,origin,209.f,scale,192,255,212,
                          intensity,time,1.45f,false)) return false;
        if (count>name.find("Perez")+1 && !accentOnPerez(r,209.f,scale,intensity)) return false;
        if (count<name.size() && count>0 && std::fmod(local*1.42,1.0)<.52) {
            float caret=origin;
            for (unsigned i=0;i<count;++i) caret+=fontAdvance(static_cast<unsigned char>(name[i]))*scale;
            if (!color(r,145,255,177,intensity) ||
                !SDL_RenderLine(r,caret,209.f,caret,227.f)) return false;
        }
    } else if (time>=titleStart) {
        const double local=time-titleStart;
        constexpr double hitAt=.25; // Same instant as the existing audio stinger.
        const double impact=local-hitAt;

        // A nervous red charging line announces the strike in the silence.
        if (impact<0.0) {
            const float charge=fade(local-.05,.13);
            const float flicker=(int(local*90.0)%5==0)?.25f:1.f;
            if (!color(r,215,32,45,charge*.25f*flicker) ||
                !SDL_RenderLine(r,270.f,200.f,370.f,200.f)) return false;
        } else {
            // The entire title hits in one blow, oversized at first, then
            // settles into the centre. Neither word strobes continuously.
            const float recoil=1.f-fade(impact,.33);
            const float flash=1.f-fade(impact,.42);
            const float visibility=1.f-fade(time-(crtOffStart-.65),.55);
            const float scale=std::min(5.20f+.50f*recoil,
                                       (width-50.f)/textExtent("EXTREMIS RE"));
            const float span=textExtent("EXTREMIS RE")*scale;
            const float left=(width-span)*.5f;
            const float y=(height-10.f*scale)*.5f;
            const float upper=y-16.f;
            const float lower=y+10.f*scale+16.f; // Identical spacing above/below glyphs.
            const float radius=span*.5f+23.f;
            const float reveal=fade(impact,.29);
            const float extent=radius*reveal;
            const float dx=4.f*std::sin(float(impact)*84.f)*recoil;
            const float dy=2.f*std::sin(float(impact)*63.f)*recoil;
            const auto fault=textFault(time,1.25f);
            const float wordX=left+dx+fault.dx*.22f;
            const float reX=left+(textExtent("EXTREMIS ")+1.f)*scale+dx-fault.dx*.16f;
            const float wordY=y+dy+fault.dy*.15f;
            const float reY=y-dy-fault.dy*.12f;
            const float a=visibility*logoBrightness(local,0);
            const float b=visibility*logoBrightness(local,1);
            const float top=visibility*logoBrightness(local,2);
            const float bottom=visibility*logoBrightness(local,3);

            // A single red shock-front lights the monitor at the impact.
            if (flash>.001f) {
                const SDL_FRect shock{0.f,0.f,width,height};
                if (!color(r,200,8,25,flash*.105f*visibility) ||
                    !SDL_RenderFillRect(r,&shock) ||
                    !color(r,255,114,118,flash*.62f*visibility) ||
                    !SDL_RenderLine(r,0.f,200.f,width,200.f)) return false;
            }

            // The two bars grow outwards from the exact horizontal centre.
            // Glow is the same distance above and below the title.
            if (!color(r,200,20,40,top*.18f) ||
                !SDL_RenderLine(r,320.f-extent,upper-2.f,320.f+extent,upper-2.f) ||
                !color(r,200,20,40,bottom*.18f) ||
                !SDL_RenderLine(r,320.f-extent,lower+2.f,320.f+extent,lower+2.f) ||
                !color(r,239,39,57,top*.92f) ||
                !SDL_RenderLine(r,320.f-extent,upper,320.f+extent,upper) ||
                !color(r,239,39,57,bottom*.92f) ||
                !SDL_RenderLine(r,320.f-extent,lower,320.f+extent,lower)) return false;

            // Deep red phosphor bleed, plus a brief, offset after-image that
            // snaps into place with the impact sound.
            const float ghostOffset=12.f*recoil;
            if (!textAt(r,"EXTREMIS",wordX-3.f,wordY+1.f,scale,160,16,28,a*.25f) ||
                !textAt(r,"EXTREMIS",wordX+2.f,wordY-1.f,scale,238,52,65,a*.20f) ||
                !textAt(r,"EXTREMIS",wordX-ghostOffset,wordY+2.f*recoil,scale,
                        255,124,132,a*.33f*recoil) ||
                !corruptedWord(r,"EXTREMIS",wordX,wordY,scale,252,69,76,a,
                               local,0) ||
                !textAt(r,"RE",reX-2.f,reY+1.f,scale,160,16,28,b*.25f) ||
                !textAt(r,"RE",reX+3.f,reY-1.f,scale,238,52,65,b*.20f) ||
                !textAt(r,"RE",reX+ghostOffset,reY-2.f*recoil,scale,
                        255,124,132,b*.33f*recoil) ||
                !corruptedWord(r,"RE",reX,reY,scale,252,69,76,b,
                               local,1)) return false;

            // An ominous, slowly crawling red phosphor after-image persists
            // under the complete logo. Nothing strobes or removes the main text.
            const float decay=fade(impact-.65,1.25);
            if (decay>.001f && visibility>.001f) {
                const float drift=1.6f*std::sin(float(local)*2.7f);
                if (!textAt(r,"EXTREMIS",wordX+3.f+drift,wordY+3.f,scale,
                            119,9,21,a*.16f*decay) ||
                    !textAt(r,"RE",reX-3.f-drift,reY+3.f,scale,
                            119,9,21,b*.16f*decay)) return false;
            }

            // Three distinct, sustained signal breakdowns after the complete
            // title and bars settle. The actual letter strokes are sheared,
            // not just decorated with lines; between breakdowns it stays calm.
            if (impact>.95) {
                const auto rupture=titleRupture(local);
                if (!drawTitleRupture(r,left,y,scale,wordX,reX,wordY,reY,
                                      visibility,rupture)) return false;
                if (rupture.strength>.015f) {
                    // Tiny missing sections in each red bar, with no change
                    // to their position or symmetrical spacing.
                    for (unsigned i=0;i<3;++i) {
                        const float x=left+float((rupture.phase*59u+i*97u)%360u);
                        const float len=(4.f+11.f*rupture.strength);
                        if (!color(r,0,0,0,rupture.strength*.95f) ||
                            !SDL_RenderLine(r,x,upper,x+len,upper) ||
                            !SDL_RenderLine(r,width-x-len,lower,width-x,lower)) return false;
                    }
                }
            }

            // Preserve V8's short horizontal video-memory tears, with a
            // burst of extra corruption only during the initial impact.
            if (fault.active || flash>.25f) {
                for (unsigned i=0;i<8;++i) {
                    const float x=left+float((fault.phase*19u+i*37u)%270u);
                    const float row=y+float(int((fault.phase*7u+i*9u)%43u)-5);
                    const float len=35.f+float((fault.phase*11u+i*13u)%125u);
                    if (!color(r,255,57,68,visibility*(fault.active?.46f:.28f)*
                               (i%3==0?1.f:.60f)) ||
                        !SDL_RenderLine(r,x,row,std::min(width-50.f,x+len),row)) return false;
                }
            }
        }
    }
    if (!drawCrtOff(r,time)) return false;
    return SDL_RenderPresent(r);
}

double trackerChirp(double t, float damage)
{
    if (t<0.0 || t>.26) return 0.0;
    const double high=1450.0+damage*210.0, low=680.0+damage*130.0;
    const double phase=2*pi*(low*t+(high-low)*(1.0-std::exp(-29.0*t))/29.0);
    const double envelope=std::min(1.0,t/.003)*std::exp(-t*(16.0+damage*2.0))
                         *(1.0-fade(t-.20,.06));
    const double tone=std::sin(phase+.14*std::sin(2*pi*30.0*t))
                     +.20*std::sin(phase*2.03+.3);
    const double click=.16*std::sin(2*pi*3100.0*t)*std::exp(-t*85.0);
    return envelope*(.090*tone+.02*click);
}

double logoStinger(double t, double grit)
{
    if (t<0.0 || t>=2.85) return 0.0;
    const double attack=std::min(1.0,t/.008);
    const double subPhase=2*pi*(39.0*t+56.0*(1.0-std::exp(-4.4*t))/4.4);
    const double punchPhase=2*pi*(60.0*t+115.0*(1.0-std::exp(-18.0*t))/18.0);
    const double sub=.32*std::sin(subPhase)*std::exp(-1.30*t)*attack;
    const double punch=.26*std::sin(punchPhase)*std::exp(-5.1*t)*attack;
    const double metal=(.12*std::sin(2*pi*181.0*t+.45*std::sin(2*pi*9.0*t))+
                        .13*std::sin(2*pi*259.0*t)+
                        .075*std::sin(2*pi*307.0*t))*std::exp(-2.0*t)*attack;
    const double blast=.23*grit*std::exp(-19.0*t)+
                       .045*grit*std::exp(-2.4*t);
    return sub+punch+metal+blast;
}

struct IntroAudio {
    SDL_AudioStream* stream=nullptr;
    bool initialized=false;

    void start()
    {
        if (!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
            initialized=true;
        }
        constexpr int rate=24000;
        const SDL_AudioSpec spec{SDL_AUDIO_F32,1,rate};
        stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
        if (!stream) return;
        std::vector<float> pcm(std::size_t(duration*rate),0.f);
        for (std::size_t i=0;i<pcm.size();++i) {
            const double t=double(i)/rate;
            const float envelope=fade(t,.6f)*(1.f-fade(t-(duration-1.1),1.1f));
            pcm[i]=envelope*float(.006*std::sin(2*pi*53*t)+.003*std::sin(2*pi*71*t));
        }
        for (const auto& beat:sweeps) {
            const std::size_t start=std::size_t(beat.at*rate);
            for (int j=0;j<int(rate*.44) && start+std::size_t(j)<pcm.size();++j) {
                const double t=double(j)/rate;
                const double ping=trackerChirp(t,beat.damage)
                                 +.28*trackerChirp(t-.095,beat.damage)
                                 +.12*trackerChirp(t-.188,beat.damage);
                const float volume=1.f+.14f*beat.damage;
                pcm[start+std::size_t(j)]+=float(volume*ping);
            }
        }
        const std::size_t titleSample=std::size_t((titleStart+.25)*rate);
        std::uint32_t random=0xFA71FACEu;
        double lowNoise=0.;
        for (int j=0;j<int(rate*2.85) && titleSample+std::size_t(j)<pcm.size();++j) {
            const double t=double(j)/rate;
            random=random*1664525u+1013904223u;
            const double noise=double((random>>8)&0xFFFFu)/32768.0-1.0;
            lowNoise+=.13*(noise-lowNoise);
            pcm[titleSample+std::size_t(j)]+=float(logoStinger(t,noise-lowNoise));
        }
        for (float& sample:pcm) sample=std::clamp(sample,-.75f,.75f);
        if (!SDL_PutAudioStreamData(stream,pcm.data(),int(pcm.size()*sizeof(float))) ||
            !SDL_ResumeAudioStreamDevice(stream)) {
            SDL_DestroyAudioStream(stream);
            stream=nullptr;
        }
    }

    ~IntroAudio()
    {
        if (stream) SDL_DestroyAudioStream(stream);
        if (initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
};
} // namespace

StartupCreditsResult showStartupCredits(SDL_Window* window, SDL_Renderer* renderer,
                                        const std::filesystem::path& resourceDirectory)
{
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

    OriginalFont originalFont;
    if (result==StartupCreditsResult::Continue && !resourceDirectory.empty()) {
        try {
            if (originalFont.load(renderer,resourceDirectory/"CASQUE.SPR")) {
                introFont=&originalFont;
            } else {
                SDL_Log("Intro: invalid CASQUE.SPR font; using SDL fallback");
            }
        } catch (const std::exception& error) {
            SDL_Log("Intro: cannot read CASQUE.SPR font (%s); using SDL fallback",error.what());
        }
    }

    IntroAudio audio;
    if (result==StartupCreditsResult::Continue) audio.start();

    const auto windowId=SDL_GetWindowID(window);
    auto previous=SDL_GetTicksNS();
    const std::uint32_t launchSeed=std::uint32_t(mixBits(std::uint32_t(previous)));
    double elapsed=0;
    bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    if (!focused && audio.stream) SDL_PauseAudioStreamDevice(audio.stream);
    while (result==StartupCreditsResult::Continue && elapsed<duration) {
        SDL_Event event;
        bool done=false;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT ||
                (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==windowId)) {
                result=StartupCreditsResult::Quit;
                done=true;
            } else if (event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId && !event.key.repeat) {
                done=true;
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==windowId) {
                focused=false;
                if (audio.stream) SDL_PauseAudioStreamDevice(audio.stream);
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==windowId) {
                focused=true;
                if (audio.stream) SDL_ResumeAudioStreamDevice(audio.stream);
            }
        }
        if (done) break;
        const auto now=SDL_GetTicksNS();
        if (focused) elapsed+=double(now-previous)/1e9;
        previous=now;
        if (elapsed>=duration) break;
        if (!drawCredit(renderer,elapsed,launchSeed)) {
            result=StartupCreditsResult::Error;
            break;
        }
        SDL_Delay(16);
    }

    SDL_ResetKeyboard();
    const bool scaleOK=SDL_SetRenderScale(renderer,oldScaleX,oldScaleY);
    const bool blendOK=SDL_SetRenderDrawBlendMode(renderer,oldBlend);
    const bool colorOK=SDL_SetRenderDrawColor(renderer,oldR,oldG,oldB,oldA);
    const bool modeOK=SDL_SetRenderLogicalPresentation(renderer,oldWidth,oldHeight,oldMode);
    if (!scaleOK || !blendOK || !colorOK || !modeOK)
        result=StartupCreditsResult::Error;
    introFont=nullptr;
    return result;
}
