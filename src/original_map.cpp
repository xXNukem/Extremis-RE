#include "original_map.h"
#include "interface_font.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace original_map {
namespace {
void validate(const Resources& r)
{
    if (r.maps.size()!=levelCount || r.compass.size()!=10)
        throw std::runtime_error("Expected 30 MAP and 10 BOUSSOLE images");
    for (const auto* bank:{&r.maps,&r.compass}) for (const auto& s:*bank)
        if (s.width<1 || s.height<1 || s.width>320 || s.height>200 ||
            s.indices.size()!=std::size_t(s.width)*s.height)
            throw std::runtime_error("Invalid original map image");
    if (r.compass[0].width!=32 || r.compass[0].height!=32 ||
        r.compass[9].width!=3 || r.compass[9].height!=3)
        throw std::runtime_error("Invalid original compass dimensions");
    for (unsigned i=1;i<9;++i)
        if (r.compass[i].width!=(i%2?7:6) || r.compass[i].height!=(i%2?7:6))
            throw std::runtime_error("Invalid compass direction image");
    interface_font::validate(r.font,8);
}
// EXE file 0x60ef and 0x1a58b; zero-based direction, one-based sprite ID = direction+2.
constexpr std::string_view zones="C200B101A002B103C204D305E406C207B108A009C210B111A012B113C214D315E416D317F518H719G620H721I822G623H724H725F526I827F528G629";
constexpr int compassOffsets[8][2]={{20,12},{18,8},{13,5},{8,8},{6,12},{8,18},{13,19},{20,18}};
}
Resources load(const std::filesystem::path& directory)
{
    Resources r;
    r.maps=spr_format::load(directory/"MAP.SPR");
    r.compass=spr_format::load(directory/"BOUSSOLE.SPR");
    r.palette=wall_textures::loadPalette(directory/"PAL_DI.PAL");
    r.font=interface_font::load(directory/"TYPO_3X8.BIN",8);
    validate(r);
    return r;
}
std::optional<Placement> placement(int width, int height, const Camera& camera)
{
    if (width<1 || width>320 || height<1 || height>200 ||
        !std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(camera.angle) ||
        camera.x<0 || camera.y<0 || camera.x>65535 || camera.y>65535) return {};
    constexpr double pi=3.14159265358979323846;
    int angle=int(std::lround(std::remainder(camera.angle,2*pi)*360/pi));
    angle=(angle+720+45)%720;
    const int x=((320-width)/2)&~1, y=((200-height)/2)&~1;
    return Placement{x,y,x+4*int(std::floor(camera.x/200))+1,
        y+height-4*int(std::floor(camera.y/200))-4,unsigned(angle/90)};
}
Renderer::Renderer(Resources resources):resources_(std::move(resources))
{
    validate(resources_);
    for (unsigned i=0;i<256;++i) {
        const auto c=resources_.palette[i];
        colors_[i]=(std::uint32_t(c.r)<<24)|(std::uint32_t(c.g)<<16)|(std::uint32_t(c.b)<<8)|255;
    }
}
void Renderer::image(std::vector<std::uint32_t>& pixels, const spr_format::Image& s, int x, int y) const
{
    // EXE 0x14092 ignores pivots; clipping is defensive, not map-rectangle clipping.
    for (int row=std::max(0,-y);row<s.height && y+row<200;++row)
        for (int col=std::max(0,-x);col<s.width && x+col<320;++col) {
            const auto index=s.indices[row*s.width+col];
            if (index) pixels[(y+row)*320+x+col]=colors_[index];
        }
}
bool Renderer::draw(std::vector<std::uint32_t>& pixels, const State& state) const
{
    if (pixels.size()!=320*200 || state.zone>=levelCount) return false;
    const auto& map=resources_.maps[state.zone];
    const auto p=placement(map.width,map.height,state.camera);
    if (!p) return false;
    // EXE 0xa6d1: 255-floor((R6+G6+B6)/6), with 255 replaced by 254.
    // The existing port frame is RGBA: quantize its components to VGA for this backdrop only.
    for (auto& pixel:pixels) {
        const unsigned r=((pixel>>24)*63+127)/255;
        const unsigned g=(((pixel>>16)&255)*63+127)/255;
        const unsigned b=(((pixel>>8)&255)*63+127)/255;
        pixel=colors_[std::min(254u,255-(r+g+b)/6)];
    }
    image(pixels,map,p->x,p->y);
    if (state.markerVisible) image(pixels,resources_.compass[9],p->markerX,p->markerY);
    image(pixels,resources_.compass[0],8,16);
    image(pixels,resources_.compass[p->direction+1],
          8+compassOffsets[p->direction][0],16+compassOffsets[p->direction][1]);
    interface_font::draw(pixels,resources_.font,colors_[102],
                         "ZONE "+std::string(zones.substr(state.zone*4,4)),6,52);
    return true;
}
}
