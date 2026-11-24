#include "extremis/ui/interface_font.h"
#include "extremis/ui/utf8_glyph.h"
#include "extremis/ui/localization.h"
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace interface_font {
namespace {
std::array<std::uint8_t,5> missingPunctuation(unsigned char c)
{
    switch(c) {
    case '?': return {6,1,2,0,2};
    case '!': return {2,2,2,0,2};
    case '.': return {0,0,0,0,2};
    case ',': return {0,0,0,2,4};
    case ':': return {0,2,0,2,0};
    case '-': return {0,0,7,0,0};
    case '%': return {5,1,2,4,5};
    case '\'': return {2,2,0,0,0};
    case '/': return {1,1,2,4,4};
    default: return {};
    }
}
}
void validate(const std::vector<std::uint8_t>& font, unsigned height)
{
    const auto size=height==8?634u:height==5?278u:0u;
    if (!size || font.size()!=size || font[0]!=3 || font[1]!=height)
        throw std::runtime_error("Unsupported original interface font");
    for (unsigned i=2;i<98;++i)
        if (font[i]!=255 && 98+height*unsigned(font[i])+height>font.size())
            throw std::runtime_error("Invalid original font mapping");
}
std::vector<std::uint8_t> load(const std::filesystem::path& path, unsigned height)
{
    std::ifstream file(path,std::ios::binary);
    std::vector<std::uint8_t> font{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    validate(font,height);
    return font;
}
void draw(std::vector<std::uint32_t>& pixels, const std::vector<std::uint8_t>& font,
          std::uint32_t color, std::string_view text, int x, int y)
{
    if (pixels.size()!=320*200 || font.size()<98) return;
    const int height=font[1];
    unsigned glyphCount=0;for(std::size_t i=0;i<text.size();) {const auto g=localization::glyph(text,i);i+=g.bytes;++glyphCount;}
    const int origin=x;
    const float factor=localization::current()!=localization::Language::English && x>=70 && x<248 && glyphCount*4>248-x
        ? float(248-x)/float(glyphCount*4) : 1.f;
    unsigned column=0;
    for (std::size_t i=0;i<text.size();) {
        const auto decoded=localization::glyph(text,i); i+=decoded.bytes;
        const unsigned char c=decoded.base;
        x=origin+int(column*4*factor);++column;
        if (x>=320) break;
        if (c>=32 && c<128) {
            unsigned glyph=font[2+c-32];
            if(glyph==255 && c>='a' && c<='z') glyph=font[2+c-'a'+'A'-32];
            if (glyph!=255 && 98+height*(glyph+1)<=font.size()) {
                for (int row=0;row<height;++row)
                    for (int col=0;col<3;++col)
                        if (x+int(col*factor)>=0 && y+row>=0 && y+row<200 && x+int(col*factor)<320 &&
                            (font[98+height*glyph+(decoded.mark=='i'?height-1-row:row)]&(128>>(decoded.mark=='i'?2-col:col))))
                            pixels[(y+row)*320+x+int(col*factor)]=color;
            } else if(glyph==255) {
                const auto rows=missingPunctuation(c);
                for(int row=0;row<5;++row) for(int col=0;col<3;++col)
                    if(rows[decoded.mark=='i'?4-row:row]&(4>>(decoded.mark=='i'?2-col:col))) {
                        const int xx=x+int(col*factor),yy=y+row+(height==8?2:0);
                        if(xx>=0 && xx<320 && yy>=0 && yy<200) pixels[yy*320+xx]=color;
                    }
            }
        }
        if(decoded.mark && decoded.mark!='i') localization::markPixels(pixels,x,y,3,height,color,decoded.mark);
    }
}
}
