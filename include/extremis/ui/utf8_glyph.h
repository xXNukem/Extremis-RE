#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
namespace localization {
struct Glyph { unsigned char base; char mark; unsigned bytes; };
inline Glyph glyph(std::string_view text, std::size_t i) {
    const auto c=static_cast<unsigned char>(text[i]);
    if(c<128) return {c,0,1};
    if(i+1>=text.size()) return {'?',0,1};
    const auto d=static_cast<unsigned char>(text[i+1]);
    if(c==0xc5 && (d==0x92 || d==0x93)) return {static_cast<unsigned char>(d==0x92?'O':'o'),'l',2};
    if(c==0xe2 && i+2<text.size() && d==0x80 && static_cast<unsigned char>(text[i+2])==0x99) return {'\'',0,3};
    if(c==0xc2 && d==0xbf) return {'?', 'i',2};
    if(c==0xc2 && d==0xa1) return {'!', 'i',2};
    if(c==0xc3) {
        const auto letter=[&](char upper,char mark)->Glyph {
            return {static_cast<unsigned char>(d>=0xa0?upper-'A'+'a':upper),mark,2};
        };
        if(d==0x91 || d==0xb1) return letter('N','t');
        if(d==0x87 || d==0xa7) return letter('C','c');
        if(d==0x86 || d==0xa6) return letter('A','e');
        if(d==0x92) return letter('O','e');
        if(d==0x80 || d==0xa0 || d==0x81 || d==0xa1 || d==0x82 || d==0xa2) return letter('A',d==0x80||d==0xa0?'g':d==0x81||d==0xa1?'a':'e');
        if(d==0x88 || d==0xa8 || d==0x8a || d==0xaa || d==0x89 || d==0xa9 || d==0x8b || d==0xab) return letter('E',d==0x89||d==0xa9?'a':d==0x8b||d==0xab?'u':d==0x88||d==0xa8?'g':'e');
        if(d==0x8c || d==0xac || d==0x8e || d==0xae || d==0x8f || d==0xaf || d==0x8d || d==0xad) return letter('I',d==0x8d||d==0xad?'a':d==0x8f||d==0xaf?'u':d==0x8c||d==0xac?'g':'e');
        if(d==0x93 || d==0xb3 || d==0x94 || d==0xb4 || d==0x96 || d==0xb6) return letter('O',d==0x93||d==0xb3?'a':d==0x96||d==0xb6?'u':'e');
        if(d==0x99 || d==0xb9 || d==0x9a || d==0xba || d==0x9b || d==0xbb || d==0x9c || d==0xbc) return letter('U',d==0x9a||d==0xba?'a':d==0x9c||d==0xbc?'u':d==0x99||d==0xb9?'g':'e');
    }
    return {'?',0,2};
}
inline void markPixels(std::vector<std::uint32_t>& pixels,int x,int y,int w,int h,std::uint32_t color,char mark) {
    auto dot=[&](int xx,int yy){if(xx>=0 && xx<320 && yy>=0 && yy<200) pixels[yy*320+xx]=color;};
    const int m=x+w/2;
    if(mark=='a') dot(m+1,y-1);
    else if(mark=='g') dot(m-1,y-1);
    else if(mark=='e'||mark=='t') {dot(m-1,y-1);dot(m+1,y-2);}
    else if(mark=='u') {dot(m-1,y-1);dot(m+1,y-1);}
    else if(mark=='c') {dot(m,y+h);dot(m-1,y+h+1);}
    else if(mark=='i') {dot(x,y+h-1);dot(x+1,y+h-2);}
}
}
