#include "original_font.h"
#include "spr_format.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace original_font {
namespace {
int spriteIndex(unsigned char c)
{
    // Original DOS glyph order; lowercase is displayed as original uppercase.
    if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return 42 + c - 'A';
    if (c >= '0' && c <= '9') return 68 + c - '0';
    switch (c) {
    case '?': return 78;
    case '!': return 79;
    case '\'': return 80;
    case '.': return 81;
    case ',': return 82;
    case ':': return 83;
    case '-': return 84;
    default: return -1;
    }
}
} // namespace

Font::Font(SDL_Renderer* renderer, const std::filesystem::path& casqueSpr)
{
    if (!renderer) throw std::invalid_argument("Original font requires an SDL renderer");
    const auto sprites = spr_format::load(casqueSpr);
    if (sprites.size() != 86) throw std::runtime_error("Invalid CASQUE.SPR font bank");
    try {
        for (std::size_t i = 0; i < glyphs_.size(); ++i) {
            const auto& sprite = sprites[42+i];
            if (sprite.height != 10 || sprite.width < 1 || sprite.width > 32 ||
                sprite.indices.size() != std::size_t(sprite.width)*sprite.height)
                throw std::runtime_error("Invalid original font glyph");

            SDL_Surface* surface = SDL_CreateSurface(sprite.width,sprite.height,SDL_PIXELFORMAT_RGBA32);
            if (!surface) throw std::runtime_error(std::string("Font surface: ")+SDL_GetError());
            const auto* format = SDL_GetPixelFormatDetails(surface->format);
            if (!format) {
                SDL_DestroySurface(surface);
                throw std::runtime_error("Cannot map font pixel format");
            }
            for (int y=0;y<sprite.height;++y) {
                auto* row = reinterpret_cast<std::uint32_t*>(
                    static_cast<std::uint8_t*>(surface->pixels)+y*surface->pitch);
                for (int x=0;x<sprite.width;++x) {
                    const auto index = sprite.indices[std::size_t(y)*sprite.width+x];
                    // Keep both original pixel layers: the darker ink is
                    // translucent and the brighter strokes remain fully lit.
                    const Uint8 alpha = index==0 ? 0 : index==104 ? 165 : 255;
                    row[x]=SDL_MapRGBA(format,nullptr,255,255,255,alpha);
                }
            }
            glyphs_[i].texture=SDL_CreateTextureFromSurface(renderer,surface);
            glyphs_[i].width=sprite.width;
            SDL_DestroySurface(surface);
            if (!glyphs_[i].texture ||
                !SDL_SetTextureBlendMode(glyphs_[i].texture,SDL_BLENDMODE_BLEND) ||
                !SDL_SetTextureScaleMode(glyphs_[i].texture,SDL_SCALEMODE_NEAREST))
                throw std::runtime_error(std::string("Font texture: ")+SDL_GetError());
        }
    } catch (...) {
        for (auto& glyph:glyphs_) {
            if (glyph.texture) SDL_DestroyTexture(glyph.texture);
            glyph.texture=nullptr;
        }
        throw;
    }
}

Font::~Font()
{
    for (auto& glyph:glyphs_)
        if (glyph.texture) SDL_DestroyTexture(glyph.texture);
}

float Font::measure(std::string_view text) const
{
    float width=0.f, longest=0.f;
    for (unsigned char c : text) {
        if (c=='\r' || c=='\n') {
            longest=std::max(longest,width);
            width=0.f;
        } else if (c==' ') {
            width+=9.f;
        } else {
            const int index=spriteIndex(c);
            width+=index<0 ? 9.f : float(glyphs_[std::size_t(index-42)].width+1);
        }
    }
    return std::max(longest,width);
}

bool Font::draw(SDL_Renderer* renderer, std::string_view text, float x, float y,
                float scale, SDL_Color tint, float opacity) const
{
    if (scale<=0.f || !renderer) return false;
    const float originX=x;
    const Uint8 alpha=Uint8(std::clamp(opacity,0.f,1.f)*255.f);
    for (unsigned char c : text) {
        if (c=='\r' || c=='\n') { x=originX; y+=11.f*scale; continue; }
        if (c==' ') { x+=9.f*scale; continue; }
        const int index=spriteIndex(c);
        if (index<0) { x+=9.f*scale; continue; }
        const auto& glyph=glyphs_[std::size_t(index-42)];
        if (!SDL_SetTextureColorMod(glyph.texture,tint.r,tint.g,tint.b) ||
            !SDL_SetTextureAlphaMod(glyph.texture,alpha)) return false;
        const SDL_FRect destination{x,y,glyph.width*scale,10.f*scale};
        if (!SDL_RenderTexture(renderer,glyph.texture,nullptr,&destination)) return false;
        x+=(glyph.width+1)*scale;
    }
    return true;
}

bool Font::drawCentered(SDL_Renderer* renderer, std::string_view text, float centerX,
                        float y, float scale, SDL_Color tint, float opacity) const
{
    return draw(renderer,text,centerX-measure(text)*scale*.5f,y,scale,tint,opacity);
}
} // namespace original_font
