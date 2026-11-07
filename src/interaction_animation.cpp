#include "original_interactions.h"
#include <algorithm>

namespace interaction {
namespace {
// EXE supply descriptor 0x1835e and weapon descriptor 0x7fe2.
constexpr int supplyPosition[7][8]={{137,53,39,5,174,54,196,82},{108,55,41,10,146,61,197,85},
    {152,49,41,6,188,54,199,86},{146,69,37,9,181,75,195,86},
    {146,69,37,9,181,75,195,86},{146,69,37,9,181,75,195,86},{141,44,36,8,174,47,195,87}};
constexpr int weaponPosition[6][8]={{120,57,45,6,160,54,202,72},{107,61,34,6,139,61,199,76},
    {115,56,40,7,146,54,202,78},{108,42,38,9,139,47,193,75},
    {106,50,34,9,139,54,189,77},{109,58,40,4,146,54,200,76}};
void retainRectangle(std::vector<std::uint32_t>& pixels,const std::vector<std::uint32_t>& background,
                     int left,int top,int right,int bottom)
{
    for(int y=0;y<200;++y) for(int x=0;x<320;++x)
        if(x<left || x>=right || y<top || y>=bottom) pixels[y*320+x]=background[y*320+x];
}
}
void Renderer::scaled(std::vector<std::uint32_t>& pixels,const spr_format::Image& sprite,
                      int x,int y,int width,int height) const
{
    if(width<=0 || height<=0) return;
    for(int row=std::max(0,-y);row<height && y+row<150;++row)
        for(int col=std::max(0,-x);col<width && x+col<320;++col) {
            const auto index=sprite.indices[(row*sprite.height/height)*sprite.width+col*sprite.width/width];
            pixels[(y+row)*320+x+col]=colors_[index];
        }
}
void Renderer::shutters(std::vector<std::uint32_t>& pixels,const State& s) const
{
    const bool plan=s.screen==Screen::Plan, weapon=s.screen==Screen::Weapon;
    const auto& bank=plan?resources_.plan:weapon?resources_.weaponOpening:resources_.supplyOpening;
    image(pixels,bank[0],62,25,false);
    if(plan) {
        smallText(pixels,"TRANSFER ",142,51);
        const auto value=std::min(s.transfer,100u);
        const char digits[]{char('0'+value/100),char('0'+value/10%10),char('0'+value%10),0};
        smallText(pixels,digits,154,81); smallText(pixels,"ZONE C200",140,112);
    } else {
        const unsigned kind=s.contents>>12;
        if(kind<(weapon?6u:7u)) {
            const auto& pos=weapon?weaponPosition[kind]:supplyPosition[kind];
            image(pixels,bank[5+kind],pos[0],pos[1],true,25,125,100,219);
        }
    }
    const int offset=s.phase==Phase::Shutters?4*int(std::min(s.frame,22u)):88;
    image(pixels,bank[1],100-offset,30,true,25,125,100,219);
    image(pixels,bank[2],145+offset,30,true,25,125,100,219);
    image(pixels,bank[3],83,30,true,25,125,100,219);
    image(pixels,bank[4],192,30,true,25,125,100,219);
}
void Renderer::reader(std::vector<std::uint32_t>& pixels,const State& s) const
{
    const auto& bank=s.codeReader?resources_.codes:resources_.reader;
    image(pixels,bank[0],62,25,false);
    int x=85,y=52;
    if(s.phase==Phase::Opening) y=152;
    if(s.phase==Phase::CardInsert) {
        const int percent=5*int(std::min(s.frame,20u));
        x=62+23*percent/100; y=152-percent;
    } else if(s.phase==Phase::CardWithdraw) y+=8*int(s.frame+1);
    image(pixels,bank[1],x,y,true,65,125,62,258);
    if(!s.codeReader) {
        if(((s.frame>>4)&3)==3) image(pixels,bank[2],164,58);
        if(((s.frame>>2)&3)==3) image(pixels,bank[3],198,58);
        if(((s.frame>>2)&12)==12) image(pixels,bank[4],198,74);
    }
    const auto text=(s.phase==Phase::CardCheck || s.phase==Phase::CardResult)?
        std::string_view(s.notice).substr(0,std::min(s.notice.size(),std::size_t(s.frame/2))):std::string_view(s.notice);
    smallText(pixels,text,76,37);
}
void Renderer::movie(std::vector<std::uint32_t>& pixels,const State& s) const
{
    const unsigned frame=s.phase==Phase::MovieIn?0:s.phase==Phase::MovieOut?4:
        unsigned(std::clamp(s.movieSeconds*160/33,0.0,4.0));
    const unsigned percent=s.phase==Phase::MovieIn?std::min(100u,4*s.frame):
        s.phase==Phase::MovieOut?100-std::min(100u,2*s.frame):100;
    const auto& film=resources_.pickupMovie;
    for(unsigned i=0;i<64000;++i) {
        const auto color=film.palette[film.frames[frame][i]];
        const auto r=((color>>24)&255)*percent/100, g=((color>>16)&255)*percent/100,
                   b=((color>>8)&255)*percent/100;
        pixels[i]=(r<<24)|(g<<16)|(b<<8)|255;
    }
}
bool Renderer::sequence(std::vector<std::uint32_t>& pixels,const State& s) const
{
    if(s.phase==Phase::MovieIn || s.phase==Phase::Movie || s.phase==Phase::MovieOut) { movie(pixels,s); return true; }
    if(s.phase==Phase::Ready || s.phase==Phase::HandIn || s.phase==Phase::CodeResult || s.phase==Phase::HandOut ||
       s.phase==Phase::CardInsert || s.phase==Phase::CardCheck ||
       s.phase==Phase::CardResult || s.phase==Phase::CardWithdraw) return false;
    if(s.phase==Phase::Opening || s.phase==Phase::Closing) {
        const auto background=pixels;
        State stable=s; stable.phase=Phase::Ready;
        if(s.phase==Phase::Opening && (stable.screen==Screen::Reader || stable.screen==Screen::Elevator)) {
            stable.notice.clear(); stable.phase=Phase::CardInsert; stable.frame=0;
        }
        if(s.phase==Phase::Opening && stable.screen==Screen::Code) { stable.phase=Phase::HandIn; stable.frame=0; }
        draw(pixels,stable);
        const int percent=s.phase==Phase::Opening?4+4*int(std::min(s.frame,24u)):
            100-4*int(std::min(s.frame,24u));
        const bool pocket=s.screen==Screen::Cards || s.screen==Screen::Inventory;
        const int halfWidth=(pocket?96:102)*percent/100;
        const int halfHeight=(pocket?94:54)*percent/100, centerY=pocket?100:75;
        retainRectangle(pixels,background,160-halfWidth,centerY-halfHeight,160+halfWidth,centerY+halfHeight);
        return true;
    }
    const bool weapon=s.screen==Screen::Weapon;
    if(s.phase==Phase::PanelSlide || s.phase==Phase::PanelZoom) {
        const auto& door=s.screen==Screen::Plan?resources_.planDoor:weapon?resources_.weaponDoor:resources_.supplyDoor;
        if(s.phase==Phase::PanelSlide) {
            // Original keyboard is pushed down while the enlarged door enters.
            State keyboard=s; keyboard.screen=Screen::Code; keyboard.phase=Phase::Ready; keyboard.notice.clear();
            auto previous=pixels; code(previous,keyboard);
            const int down=std::min(100,4*int(s.frame));
            for(int y=25+down;y<125;++y)
                std::copy_n(previous.begin()+(y-down)*320+62,196,pixels.begin()+y*320+62);
            scaled(pixels,door[0],-36,-231+4*int(s.frame),392,256);
        } else {
            const int percent=100-2*int(std::min(s.frame,25u));
            scaled(pixels,door[0],160-196*percent/100,75-120*percent/100,
                   392*percent/100,256*percent/100);
        }
        return true;
    }
    if(s.phase==Phase::Shutters) { shutters(pixels,s); return true; }
    const auto& bank=weapon?resources_.weaponOpening:resources_.supplyOpening;
    const unsigned kind=s.contents>>12;
    if(kind>=(weapon?6u:7u)) return false;
    const auto& pos=weapon?weaponPosition[kind]:supplyPosition[kind];
    image(pixels,bank[0],62,25,false);
    if(s.phase==Phase::ContentExpand) {
        const int percent=4+2*int(std::min(s.frame,48u));
        const int x=160-102*percent/100,y=75-54*percent/100;
        for(int row=std::max(25,y);row<std::min(125,150-y);++row)
            std::fill(pixels.begin()+row*320+std::max(62,x),pixels.begin()+row*320+std::min(258,320-x),colors_[0]);
        image(pixels,bank[5+kind],pos[0],pos[1],true,25,125,62,258);
    } else if(s.phase==Phase::ContentMove) {
        for(int row=25;row<125;++row) std::fill_n(pixels.begin()+row*320+62,196,colors_[0]);
        const int percent=10+10*int(std::min(s.frame,9u));
        image(pixels,bank[5+kind],pos[0]+pos[2]*percent/100,
              pos[1]+pos[3]*percent/100,true,25,125,62,258);
    } else if(s.phase==Phase::ContentReveal) {
        // Reveal the final question/display from the object towards the panel edges.
        for(int row=25;row<125;++row) std::fill_n(pixels.begin()+row*320+62,196,colors_[0]);
        image(pixels,bank[5+kind],pos[0]+pos[2],pos[1]+pos[3],true,25,125,62,258);
        const auto background=pixels;
        supply(pixels,s,weapon);
        const int percent=2*int(std::min(s.frame,49u));
        const int cx=pos[6],cy=pos[7];
        retainRectangle(pixels,background,cx+(58-cx)*percent/100,cy+(21-cy)*percent/100,
                        cx+(262-cx)*percent/100,cy+(129-cy)*percent/100);
    }
    return true;
}
}
