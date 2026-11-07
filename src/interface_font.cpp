#include "interface_font.h"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace interface_font {
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
    for (unsigned char c:text) {
        if (x>=320) break;
        if (c>=32 && c<128) {
            const unsigned glyph=font[2+c-32];
            if (glyph!=255 && 98+height*(glyph+1)<=font.size())
                for (int row=0;row<height;++row)
                    for (int col=0;col<3;++col)
                        if (x+col>=0 && y+row>=0 && y+row<200 && x+col<320 &&
                            (font[98+height*glyph+row]&(128>>col)))
                            pixels[(y+row)*320+x+col]=color;
        }
        x+=4;
    }
}
}
