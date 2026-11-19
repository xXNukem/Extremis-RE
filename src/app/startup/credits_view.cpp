#include "credits_view.h"
#include "credits_timeline.h"
#include "extremis/resources/spr_format.h"

#include <cmath>
#include <string>
#include <vector>

namespace startup_credits {
namespace {
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

} // namespace

// Original DOS font: CASQUE.SPR glyphs 42..85, exactly as used by the HUD.
// Every non-zero palette index is an opaque glyph pixel; tint is applied at draw time.
// Keep the original resource external: the port never ships game assets.
CreditsFont::~CreditsFont() { clear(); }
void CreditsFont::clear()
{
    for (auto*& texture : glyphs) {
        if (texture) SDL_DestroyTexture(texture);
        texture=nullptr;
    }
}

bool CreditsFont::load(SDL_Renderer* r, const std::filesystem::path& path)
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
    return true;
}

int CreditsFont::index(unsigned char c)
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

float CreditsFont::advance(unsigned char c) const
{
    if (c==' ') return 9.f;
    const int id=index(c);
    if (id>=0) return float(glyphWidths[std::size_t(id)]+1);
    return 8.f;
}

float CreditsFont::extent(std::string_view text) const
{
    float result=0.f;
    for (unsigned char c : text) result+=advance(c);
    return text.empty()?0.f:result-1.f;
}

bool CreditsFont::glyph(SDL_Renderer* r, unsigned char c, float x, float y, float scale,
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

namespace {
float fontAdvance(const CreditsFont* font, unsigned char c) { return font?font->advance(c):8.f; }
float textExtent(const CreditsFont* font, std::string_view text) {
    return font?font->extent(text):float(text.size())*8.f;
}

bool textAt(SDL_Renderer* r, const CreditsFont* font, const char* str, float x, float y, float scale,
            Uint8 red, Uint8 green, Uint8 blue, float alpha)
{
    if (!font) {
        if (!SDL_SetRenderScale(r,scale,scale)) return false;
        const bool ok=color(r,red,green,blue,alpha) &&
                      SDL_RenderDebugText(r,x/scale,y/scale,str);
        return SDL_SetRenderScale(r,1.f,1.f) && ok;
    }
    for (const unsigned char c : std::string_view(str)) {
        if (!font->glyph(r,c,x,y,scale,red,green,blue,alpha)) return false;
        x+=fontAdvance(font,c)*scale; // Original DOS font is proportional.
    }
    return true;
}

bool digitalText(SDL_Renderer* r, const CreditsFont* font, const std::string& text, float x, float y, float scale,
                 Uint8 red, Uint8 green, Uint8 blue, float alpha,
                 float tracking=0.f)
{
    for (char c : text) {
        const char symbol[]={c,0};
        if (!textAt(r,font,symbol,x,y,scale,red,green,blue,alpha)) return false;
        x+=(fontAdvance(font,static_cast<unsigned char>(c))+tracking)*scale;
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

bool unstableText(SDL_Renderer* r, const CreditsFont* font, std::string_view text, float x, float y, float scale,
                  Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                  float severity=1.f, bool digital=false)
{
    if (text.empty() || alpha<=0.f) return true;
    const auto fault=textFault(time,severity);
    const std::string copy(text);
    bool ok=true;

    auto renderLayer=[&](float dx,float dy, Uint8 rr, Uint8 gg, Uint8 bb, float a, float tracking=0.f) {
        return digital ? digitalText(r,font,copy,x+dx,y+dy,scale,rr,gg,bb,a,tracking)
                       : textAt(r,font,copy.c_str(),x+dx,y+dy,scale,rr,gg,bb,a);
    };

    if (fault.active) {
        ok = renderLayer(fault.dx-9.f, fault.dy+4.f, 148,255,65, alpha*.28f*fault.intensity, .6f) && ok;
        ok = renderLayer(-fault.dx*1.4f, -fault.dy*1.2f, 214,255,170, alpha*.18f*fault.intensity, .2f) && ok;
        ok = renderLayer(fault.skew*1.2f, fault.dy-3.f, 169,255,88, alpha*.20f*fault.intensity, .35f) && ok;
    }
    ok = renderLayer(fault.dx, fault.dy, red, green, blue, alpha*fault.intensity, .2f) && ok;
    if (!ok) return false;

    if (fault.active) {
        const float span=textExtent(font,copy)*scale;
        for (unsigned i=0;i<5;++i) {
            const float fx=x+float((fault.phase*29u+i*41u)%unsigned(std::max(2.f,span*.58f)));
            const float fy=y+float(int((fault.phase*17u+i*13u)%31u)-15);
            const float len=span*(.08f+.09f*float(i%3));
            // Screen tearing is HORIZONTAL; two-dimensional glitches come
            // from the displaced text layers, not diagonal scratch lines.
            if (!color(r,183,255,90,alpha*(fault.blackout?.20f:.30f)) ||
                !SDL_RenderLine(r,fx,fy,fx+len,fy)) return false;
        }
        for (unsigned i=0;i<3;++i) {
            const float bx=x+float((fault.phase*47u+i*19u)%unsigned(std::max(2.f,span*.45f)));
            const float by=y+float(int((fault.phase*31u+i*7u)%29u)-14);
            if (!color(r,234,255,197,alpha*.10f) ||
                !SDL_RenderLine(r,bx,by,bx+11.f+7.f*i,by)) return false;
        }
    }
    return true;
}

bool unstableCentered(SDL_Renderer* r, const CreditsFont* font, const std::string& text, float y, float scale,
                      Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                      float severity=1.f, bool digital=false)
{
    const float x=(width-textExtent(font,text)*scale)*.5f;
    return unstableText(r,font,text,x,y,scale,red,green,blue,alpha,time,severity,digital);
}

bool accentOnPerez(SDL_Renderer* r, const CreditsFont* font, float y, float scale, float alpha)
{
    constexpr std::string_view name="Carlos (xXNukem) de la Barrera Perez";
    const auto e=name.find("Perez")+1;
    const float x=(width-textExtent(font,name)*scale)*.5f+
                  textExtent(font,name.substr(0,e))*scale+4.f*scale;
    return color(r,220,255,167,alpha) && SDL_RenderLine(r,x-2,y-3,x+1,y-7);
}

float triangle(float x, float center, float halfWidth)
{
    return std::max(0.f,1.f-std::abs(x-center)/halfWidth);
}

// Small deterministic PRNG: one different waveform per pulse, and a different
// set of peak positions on every launch. The shape stays fixed WHILE sweeping.
} // namespace

std::uint32_t mixBits(std::uint32_t value)
{
    value^=value>>16;
    value*=0x7feb352du;
    value^=value>>15;
    value*=0x846ca68bu;
    return value^(value>>16);
}

namespace {
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
    if (!color(r,201,255,133,.85f*glow) ||
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
bool drawTitleRupture(SDL_Renderer* r, const CreditsFont* font, float left, float y, float scale,
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
        const SDL_Rect clip{int(left)-5,bandY,int(textExtent(font,"EXTREMIS RE")*scale)+10,bandH};
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
        const bool ok=textAt(r,font,"EXTREMIS",wordX+displacement,wordY+direction*2.f*strength,
                             scale,207,255,100,alpha*.86f*strength) &&
                      textAt(r,font,"RE",reX+displacement,reY-direction*2.f*strength,
                             scale,207,255,100,alpha*.86f*strength);
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
        if (!color(r,154,243,55,alpha*strength*(i%2==0?.34f:.18f)) ||
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

bool corruptedWord(SDL_Renderer* r, const CreditsFont* font, std::string_view word, float x, float y, float scale,
                   Uint8 red, Uint8 green, Uint8 blue, float alpha,
                   double local, unsigned channel)
{
    if (alpha<=0.f) return true;
    float gx=x;
    bool ok=true;
    for (unsigned i=0;i<word.size();++i) {
        const std::string glyph(1,word[i]);
        const float step=fontAdvance(font,static_cast<unsigned char>(word[i]))*scale;
        const auto warp=glyphWarp(local,channel,i);
        if (warp.active) {
            ok = ok && textAt(r,font,glyph.c_str(),gx+warp.dx*1.55f,y+warp.dy*1.35f,scale,
                              210,255,129,alpha*warp.alpha*.36f);
            ok = ok && textAt(r,font,glyph.c_str(),gx-warp.dx*.82f,y-warp.dy*.68f,scale,
                              70,127,19,alpha*warp.alpha*.24f);
            for (unsigned j=0;j<4;++j) {
                const std::uint32_t lineSeed=mixBits((channel+1u)*97u ^ (i+1u)*313u ^ (j+1u)*911u ^ unsigned(std::floor(local*53.0))*17u);
                const float lx=gx+float(int((lineSeed>>3)&31u)-15);
                const float ly=y+float(int((lineSeed>>11)&31u)-15);
                const float ex=lx+float(int((lineSeed>>19)&31u)-15)*warp.smear;
                const float ey=ly+float(int((lineSeed>>25)&31u)-15)*warp.smear;
                ok = ok && color(r,192,255,92,alpha*.15f*warp.alpha) && SDL_RenderLine(r,lx,ly,ex,ey);
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
        ok = ok && textAt(r,font,glyph.c_str(),gx,y,scale,red,green,blue,alpha);
        if (!ok) return false;
        gx+=step;
    }
    return ok;
}

// Vortex Engine emblem: the clean inverted V transforms into its final mark.
// Only SDL primitives: no new assets, audio or third-party dependencies.
bool drawVortexEngine(SDL_Renderer* r, const CreditsFont* font, double local)
{
    constexpr float cx=320.f, cy=176.f;
    constexpr float ringRadius=55.f, rayLength=56.f;
    constexpr int rays=16, circlePieces=96, rayPieces=11;
    constexpr double hold=.56, turn=1.18;
    const float born=fade(local,.22);
    const float exit=1.f-fade(local-5.91,.29);
    const float visibility=born*exit;
    if (visibility<=.001f) return true;

    // The emblem holds slightly longer before the turn, then remains readable
    // in its final state until the main title arrives.
    const float progress=fade(local-hold,turn);
    const float angle=-float(pi)*progress; // clockwise rotation
    const float sine=std::sin(angle), cosine=std::cos(angle);
    const auto rotated=[&](float x, float y) -> SDL_FPoint {
        return {cx+x*cosine-y*sine,cy+x*sine+y*cosine};
    };
    const auto pointOnRay=[&](int ray, float distance, float amount) -> SDL_FPoint {
        const float base=2.f*float(pi)*float(ray)/float(rays);
        const float twist=-progress*(.58f*amount + .26f*amount*amount);
        const float a=base+twist;
        return {cx+std::cos(a)*distance, cy+std::sin(a)*distance};
    };

    for (int pass=0; pass<3; ++pass) {
        const float shift=pass==0?2.0f:pass==1?-.8f:0.f;
        const float alpha=visibility*(pass==0?.14f:pass==1?.34f:.96f);
        const Uint8 red=pass==0?102:pass==1?165:213;
        const Uint8 green=255, blue=pass==0?48:pass==1?73:135;
        if (!color(r,red,green,blue,alpha)) return false;

        std::array<SDL_FPoint,circlePieces+1> ring{};
        for (int i=0;i<=circlePieces;++i) {
            const float a=2.f*float(pi)*float(i)/circlePieces;
            ring[std::size_t(i)]={cx+(ringRadius+shift)*std::cos(a),
                                  cy+(ringRadius+shift)*std::sin(a)};
        }
        if (!SDL_RenderLines(r,ring.data(),int(ring.size()))) return false;

        for (int ray=0;ray<rays;++ray) {
            std::array<SDL_FPoint,rayPieces+1> stroke{};
            for (int i=0;i<=rayPieces;++i) {
                const float u=float(i)/rayPieces;
                stroke[std::size_t(i)]=pointOnRay(ray,ringRadius+shift+rayLength*u,u);
            }
            if (!SDL_RenderLines(r,stroke.data(),int(stroke.size()))) return false;
        }

        const auto apex=rotated(0.f,-52.f);
        const auto left=rotated(-44.f,27.f);
        const auto right=rotated(44.f,27.f);
        const float dx=pass==0?2.f:pass==1?-1.f:0.f;
        // Give the inner V more body without changing the overall silhouette.
        for (const float ox : {-1.4f,0.f,1.4f}) {
            for (const float oy : {-0.5f,0.f,0.5f}) {
                if (!SDL_RenderLine(r,left.x+dx+ox,left.y+oy,apex.x+dx+ox,apex.y+oy) ||
                    !SDL_RenderLine(r,apex.x+dx+ox,apex.y+oy,right.x+dx+ox,right.y+oy)) return false;
            }
        }
    }

    const float letters=visibility*fade(local-.92,.54);
    return unstableCentered(r,font,"VORTEX ENGINE",295.f,2.7f,
                            183,255,90,letters,secondEnd+local,.40f,false);
}

} // namespace

bool drawCredit(SDL_Renderer* r, const CreditsFont* font, double time, std::uint32_t launchSeed)
{
    if (!blackBackground(r) || !drawECG(r,time,launchSeed)) return false;
    if (time<firstEnd) {
        // First card: only the port author alias, no interim studio branding.
        const float appear=fade(time-.65,.95);
        const float visible=appear*(1.f-fade(time-(firstEnd-1.45),1.25));
        const float intensity=visible*textFlicker(time);
        if (!unstableCentered(r,font,"A SOURCE PORT BY",138.f,2.75f,
                              183,255,90,intensity,time,1.05f,true) ||
            !unstableCentered(r,font,"XXNUKEM",212.f,3.7f,
                              211,255,153,intensity,time,1.25f,false)) return false;
    } else if (time<secondEnd) {
        // Original developer and publisher are credited together.
        const double local=time-firstEnd;
        const float visible=fade(local-.55,.85)*
                            (1.f-fade(time-(secondEnd-1.30),1.15));
        const float first=visible*textFlicker(time);
        const float second=first*fade(local-1.25,.85);
        if (!unstableCentered(r,font,"ORIGINALLY DEVELOPED BY",106.f,2.10f,
                              183,255,90,first,time,1.05f,true) ||
            !unstableCentered(r,font,"BLUE SPHERE",153.f,3.15f,
                              215,255,163,first,time,1.20f,false) ||
            !unstableCentered(r,font,"PUBLISHED BY",237.f,2.10f,
                              183,255,90,second,time,1.05f,true) ||
            !unstableCentered(r,font,"U.S. GOLD",282.f,3.15f,
                              215,255,163,second,time,1.20f,false)) return false;
    } else if (time<titleStart) {
        if (!drawVortexEngine(r,font,time-secondEnd)) return false;
    } else if (time>=titleStart) {
        const double local=time-titleStart;
        constexpr double hitAt=.25; // Same instant as the existing audio stinger.
        const double impact=local-hitAt;

        // A nervous pistachio charging line announces the strike in the silence.
        if (impact<0.0) {
            const float charge=fade(local-.05,.13);
            const float flicker=(int(local*90.0)%5==0)?.25f:1.f;
            if (!color(r,138,240,47,charge*.25f*flicker) ||
                !SDL_RenderLine(r,270.f,200.f,370.f,200.f)) return false;
        } else {
            // The entire title hits in one blow, oversized at first, then
            // settles into the centre. Neither word strobes continuously.
            const float recoil=1.f-fade(impact,.33);
            const float flash=1.f-fade(impact,.42);
            const float visibility=1.f-fade(time-(crtOffStart-.65),.55);
            const float scale=std::min(5.20f+.50f*recoil,
                                       (width-50.f)/textExtent(font,"EXTREMIS RE"));
            const float span=textExtent(font,"EXTREMIS RE")*scale;
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
            const float reX=left+(textExtent(font,"EXTREMIS ")+1.f)*scale+dx-fault.dx*.16f;
            const float wordY=y+dy+fault.dy*.15f;
            const float reY=y-dy-fault.dy*.12f;
            const float a=visibility*logoBrightness(local,0);
            const float b=visibility*logoBrightness(local,1);
            const float top=visibility*logoBrightness(local,2);
            const float bottom=visibility*logoBrightness(local,3);

            // A single green shock-front lights the monitor at the impact.
            if (flash>.001f) {
                const SDL_FRect shock{0.f,0.f,width,height};
                if (!color(r,71,154,11,flash*.105f*visibility) ||
                    !SDL_RenderFillRect(r,&shock) ||
                    !color(r,217,255,129,flash*.62f*visibility) ||
                    !SDL_RenderLine(r,0.f,200.f,width,200.f)) return false;
            }

            // The two bars grow outwards from the exact horizontal centre.
            // Glow is the same distance above and below the title.
            if (!color(r,77,165,25,top*.18f) ||
                !SDL_RenderLine(r,320.f-extent,upper-2.f,320.f+extent,upper-2.f) ||
                !color(r,77,165,25,bottom*.18f) ||
                !SDL_RenderLine(r,320.f-extent,lower+2.f,320.f+extent,lower+2.f) ||
                !color(r,183,255,90,top*.92f) ||
                !SDL_RenderLine(r,320.f-extent,upper,320.f+extent,upper) ||
                !color(r,183,255,90,bottom*.92f) ||
                !SDL_RenderLine(r,320.f-extent,lower,320.f+extent,lower)) return false;

            // Deep green phosphor bleed, plus a brief, offset after-image that
            // snaps into place with the impact sound.
            const float ghostOffset=12.f*recoil;
            if (!textAt(r,font,"EXTREMIS",wordX-3.f,wordY+1.f,scale,75,128,23,a*.25f) ||
                !textAt(r,font,"EXTREMIS",wordX+2.f,wordY-1.f,scale,161,223,63,a*.20f) ||
                !textAt(r,font,"EXTREMIS",wordX-ghostOffset,wordY+2.f*recoil,scale,
                        229,255,158,a*.33f*recoil) ||
                !corruptedWord(r,font,"EXTREMIS",wordX,wordY,scale,183,255,90,a,
                               local,0) ||
                !textAt(r,font,"RE",reX-2.f,reY+1.f,scale,75,128,23,b*.25f) ||
                !textAt(r,font,"RE",reX+3.f,reY-1.f,scale,161,223,63,b*.20f) ||
                !textAt(r,font,"RE",reX+ghostOffset,reY-2.f*recoil,scale,
                        229,255,158,b*.33f*recoil) ||
                !corruptedWord(r,font,"RE",reX,reY,scale,183,255,90,b,
                               local,1)) return false;

            // An ominous, slowly crawling green phosphor after-image persists
            // under the complete logo. Nothing strobes or removes the main text.
            const float decay=fade(impact-.65,1.25);
            if (decay>.001f && visibility>.001f) {
                const float drift=1.6f*std::sin(float(local)*2.7f);
                if (!textAt(r,font,"EXTREMIS",wordX+3.f+drift,wordY+3.f,scale,
                            61,114,16,a*.16f*decay) ||
                    !textAt(r,font,"RE",reX-3.f-drift,reY+3.f,scale,
                            61,114,16,b*.16f*decay)) return false;
            }

            // Three distinct, sustained signal breakdowns after the complete
            // title and bars settle. The actual letter strokes are sheared,
            // not just decorated with lines; between breakdowns it stays calm.
            if (impact>.95) {
                const auto rupture=titleRupture(local);
                if (!drawTitleRupture(r,font,left,y,scale,wordX,reX,wordY,reY,
                                      visibility,rupture)) return false;
                if (rupture.strength>.015f) {
                    // Tiny missing sections in each green bar, with no change
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
                    if (!color(r,170,255,74,visibility*(fault.active?.46f:.28f)*
                               (i%3==0?1.f:.60f)) ||
                        !SDL_RenderLine(r,x,row,std::min(width-50.f,x+len),row)) return false;
                }
            }
        }
    }
    if (!drawCrtOff(r,time)) return false;
    return SDL_RenderPresent(r);
}

} // namespace startup_credits
