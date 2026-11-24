#pragma once
#include <cstddef>
#include <cstdint>
#include "extremis/resources/spr_format.h"
#include "extremis/ui/utf8_glyph.h"
#include <algorithm>
#include <string_view>
#include <vector>
namespace localization {
template<class Map>
int measureIndexed(const std::vector<spr_format::Image>& font,std::string_view text,Map map) {
    int width=0;
    for(std::size_t i=0;i<text.size();) {
        const auto g=glyph(text,i);i+=g.bytes;
        const int n=map(g.base);
        width+=n>=0 && std::size_t(n)<font.size()?font[n].width+1:4;
        if(g.mark=='l') {const int e=map(g.base=='O'?'E':'e');if(e>=0 && std::size_t(e)<font.size()) width+=font[e].width;}
    }
    return width;
}
// The source movie still owns all clocks. This draws only a localized glyph layer.
template<class Map>
bool drawIndexed(std::vector<std::uint8_t>& canvas,const std::vector<spr_format::Image>& font,
                 std::string_view text,unsigned sourceProgress,unsigned sourceLength,
                 int left,int top,int right,int bottom,int lineHeight,Map map,bool slashes=false,std::uint8_t overrideInk=0)
{
    if(canvas.size()!=64000 || !sourceLength) return false;
    struct Placed { Glyph glyph; int x,y,index; };
    std::vector<Placed> placed;
    int x=left,y=top;
    for(std::size_t i=0;i<text.size();) {
        if(text[i]=='/' && slashes) {++i;x=left;y+=lineHeight;continue;}
        if(text[i]=='\r' || text[i]=='\n') {++i;x=left;y+=lineHeight;continue;}
        if(text[i]==' ') {++i;if(x>left) x+=4;continue;}
        // Measure the following word before placing it, so the reveal never reflows.
        std::size_t end=i;int wordWidth=0;
        while(end<text.size() && text[end]!=' ' && text[end]!='\r' && text[end]!='\n' && !(slashes && text[end]=='/')) {
            const auto g=glyph(text,end);end+=g.bytes;
            const int index=map(g.base);
            wordWidth+=index>=0 && std::size_t(index)<font.size()?font[index].width+1:4;
            if(g.mark=='l') {const int e=map(g.base=='O'?'E':'e');if(e>=0 && std::size_t(e)<font.size()) wordWidth+=font[e].width;}
        }
        if(wordWidth>right-left) return false;
        if(x>left && x+wordWidth>right) {x=left;y+=lineHeight;}
        while(i<end) {
            const auto g=glyph(text,i);i+=g.bytes;
            const int index=map(g.base);
            const int width=index>=0 && std::size_t(index)<font.size()?font[index].width:3;
            placed.push_back({g,x,y,index});x+=width+1;
            if(g.mark=='l') {const int e=map(g.base=='O'?'E':'e');if(e>=0 && std::size_t(e)<font.size()) x+=font[e].width;}
        }
    }
    if(y+lineHeight>bottom) return false;
    const std::size_t visible=std::min<std::size_t>(placed.size(),
        (placed.size()*std::min(sourceProgress,sourceLength)+sourceLength-1)/sourceLength);
    for(std::size_t i=0;i<visible;++i) {
        const auto& p=placed[i];if(p.index<0 || std::size_t(p.index)>=font.size()) continue;
        const auto& sprite=font[p.index];
        std::uint8_t ink=0;for(auto v:sprite.indices) if(v) {ink=v;break;}
        for(int row=0;row<sprite.height;++row) for(int col=0;col<sprite.width;++col) {
            const int xx=p.x+col,yy=p.y+row;
            if(xx<left || xx>=right || yy<0 || yy>=200) continue;
            const int sourceRow=p.glyph.mark=='i'?sprite.height-1-row:row;
            const int sourceCol=p.glyph.mark=='i'?sprite.width-1-col:col;
            const auto v=sprite.indices[std::size_t(sourceRow)*sprite.width+sourceCol];
            if(v) canvas[yy*320+xx]=overrideInk?overrideInk:v;
        }
        if(overrideInk) ink=overrideInk;
        const int middle=p.x+sprite.width/2;
        auto dot=[&](int xx,int yy){if(xx>=left && xx<right && yy>=0 && yy<200)canvas[yy*320+xx]=ink;};
        switch(p.glyph.mark) {
        case 'a':dot(middle+1,p.y-1);break;
        case 'g':dot(middle-1,p.y-1);break;
        case 'u':dot(middle-1,p.y-1);dot(middle+1,p.y-1);break;
        case 'e':case 't':dot(middle-1,p.y-1);dot(middle+1,p.y-2);break;
        case 'c':dot(middle,p.y+sprite.height);dot(middle-1,p.y+sprite.height+1);break;
        case 'l': {
            const int e=map(p.glyph.base=='O'?'E':'e');
            if(e>=0 && std::size_t(e)<font.size()) {
                const auto& second=font[e];
                for(int row=0;row<second.height;++row) for(int col=0;col<second.width;++col) {
                    const int xx=p.x+sprite.width+col,yy=p.y+row;
                    if(xx>=left && xx<right && yy>=0 && yy<200) {
                        const auto v=second.indices[std::size_t(row)*second.width+col];
                        if(v) canvas[yy*320+xx]=overrideInk?overrideInk:v;
                    }
                }
            }
            break;
        }
        case 'i':break;
        }
    }
    return true;
}
}
