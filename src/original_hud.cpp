#include "original_hud.h"
#include "weapon_fire.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hud {
namespace {
void requireSize(const spr_format::Image& image, int w, int h)
{
    if (image.width != w || image.height != h ||
        image.indices.size() != std::size_t(w) * h)
        throw std::runtime_error("Unsupported original HUD sprite dimensions");
}

void validate(const Resources& resources)
{
    if (!resources.slime.empty()) {
        if (resources.slime.size()!=16) throw std::runtime_error("Expected16 GOUTTE images");
        for (const auto& image:resources.slime) {
            if (image.width<=0 || image.width>320 || image.height<=0 || image.height>150)
                throw std::runtime_error("Invalid GOUTTE dimensions");
            requireSize(image,image.width,image.height);
        }
    }
    if (resources.casque.size() != 86) throw std::runtime_error("Expected 86 CASQUE sprites");
    constexpr std::array<std::array<int, 2>, 26> sizes{{
        {1,150},{320,50},{60,10},{60,10},{46,29},{46,29},{46,29},
        {46,29},{46,29},{46,29},{46,29},{46,29},{46,29},
        {5,5},{3,3},{1,1},{43,10},{43,10},{43,10},{31,41},
        {9,19},{9,19},{11,21},{11,21},{11,15},{13,9}}};
    for (std::size_t i = 0; i < sizes.size(); ++i)
        requireSize(resources.casque[i], sizes[i][0], sizes[i][1]);
    for (std::size_t i = 26; i < 36; ++i) requireSize(resources.casque[i],3,6);
    requireSize(resources.casque[36],18,38);
    requireSize(resources.casque[37],7,10);
    requireSize(resources.casque[38],5,17);
    for (std::size_t i = 36; i < resources.casque.size(); ++i) {
        const auto& sprite = resources.casque[i];
        if (sprite.width <= 0 || sprite.width > 32 || sprite.height <= 0 || sprite.height > 41)
            throw std::runtime_error("Invalid HUD icon/glyph dimensions");
        requireSize(sprite,sprite.width,sprite.height);
        if (i >= 42 && sprite.height != 10) throw std::runtime_error("Invalid HUD font height");
    }
    for (const auto& icon : resources.weaponIcons) requireSize(icon,57,30);
    constexpr int viewSizes[6][2]{{25,28},{33,37},{26,34},{27,50},{46,33},{25,47}};
    constexpr int effects[6][4]{{14,11,21,21},{13,12,21,21},{13,13,27,27},
        {14,21,21,21},{28,18,37,37},{15,25,25,24}};
    for (unsigned i=0;i<6;++i) {
        requireSize(resources.weaponViews[i],viewSizes[i][0],viewSizes[i][1]);
        requireSize(resources.weaponEffects[i][0],effects[i][0],effects[i][1]);
        requireSize(resources.weaponEffects[i][1],effects[i][2],effects[i][3]);
    }
}

std::optional<std::size_t> glyph(unsigned char character)
{
    if (character >= 'A' && character <= 'Z') return 42 + character - 'A';
    if (character >= '0' && character <= '9') return 68 + character - '0';
    switch (character) {
    case '?': return 78;
    case '!': return 79;
    case '\'': return 80;
    case '.': return 81;
    case ',': return 82;
    case ':': return 83;
    case '-': return 84;
    case 255: return 85;
    default: return std::nullopt;
    }
}
} // namespace

Resources load(const std::filesystem::path& directory, unsigned paletteBank)
{
    Resources result;
    result.casque = spr_format::load(directory / "CASQUE.SPR");
    result.slime = spr_format::load(directory / "GOUTTE.SPR");
    result.palette = wall_textures::loadPalette(directory / ("PAL" + std::to_string(paletteBank) + ".PAL"));
    for (unsigned i = 0; i < result.weaponIcons.size(); ++i) {
        auto images = spr_format::load(directory / ("ARME" + std::to_string(i) + ".SPR"));
        if (images.size()!=4) throw std::runtime_error("Expected four original ARME sprites");
        result.weaponIcons[i] = std::move(images.front());
        result.weaponViews[i] = std::move(images[1]);
        result.weaponEffects[i][0]=std::move(images[2]);
        result.weaponEffects[i][1]=std::move(images[3]);
    }
    validate(result);
    return result;
}

Presentation presentation(int outputWidth, int outputHeight)
{
    if (outputWidth <= 0 || outputHeight <= 0) return {};
    const float w = std::min(float(outputWidth), float(outputHeight) * 4 / 3);
    const float h = w * 3 / 4;
    return {(outputWidth-w)/2, (outputHeight-h)/2, w, h};
}

Renderer::Renderer(Resources resources) : resources_(std::move(resources))
{
    validate(resources_);
    for (std::size_t i = 0; i < colors_.size(); ++i) {
        const auto c = resources_.palette[i];
        colors_[i] = (std::uint32_t(c.r)<<24) | (std::uint32_t(c.g)<<16) |
                     (std::uint32_t(c.b)<<8) | 255;
    }
}

void Renderer::image(std::vector<std::uint32_t>& pixels, const spr_format::Image& sprite,
                     int x, int y, bool transparent, int columns) const
{
    const int count = columns < 0 ? sprite.width : std::clamp(columns,0,sprite.width);
    for (int sy = 0; sy < sprite.height; ++sy) {
        if (y+sy < 0 || y+sy >= height) continue;
        for (int sx = 0; sx < count; ++sx) {
            if (x+sx < 0 || x+sx >= width) continue;
            const auto index = sprite.indices[std::size_t(sy)*sprite.width+sx];
            if (!transparent || index) pixels[std::size_t(y+sy)*width+x+sx] = colors_[index];
        }
    }
}

void Renderer::slime(std::vector<std::uint32_t>& pixels, unsigned phase) const
{
    if (phase>=30 || resources_.slime.size()!=16) return;
    // EXE file0x13bb4:43 splats;0x13b94 reads word offsets with stride4.
    constexpr int splats[43][3]{
        {7,77,0},{13,106,1},{22,16,2},{23,64,3},{42,38,4},
        {40,74,5},{38,120,6},{51,97,7},{57,6,0},{62,50,1},
        {72,118,2},{75,17,3},{88,63,4},{90,100,5},{94,25,6},
        {109,1,7},{110,52,0},{116,90,1},{119,25,2},{133,57,3},
        {139,117,4},{139,8,5},{157,40,6},{158,86,7},{174,12,0},
        {181,59,1},{188,107,2},{191,27,3},{204,1,4},{212,64,5},
        {221,124,6},{222,40,7},{225,8,0},{229,86,1},{250,26,2},
        {251,72,3},{265,41,4},{266,117,5},{282,9,6},{283,63,7},
        {287,100,0},{306,23,1},{312,66,2},
    };
    constexpr int drift[]{0,0,0,1,2,2,2,2};
    for (unsigned i=0;i<43;++i) {
        const int local=int(phase/2)-splats[i][2];
        if (local<0 || local>=8) continue;
        const auto& sprite=resources_.slime[local+((43-i)&1)*8];
        for(int y=0;y<sprite.height && y+splats[i][1]<viewHeight;++y)
            for(int x=0;x<sprite.width;++x) {
                const int targetX=splats[i][0]+drift[local]+x;
                const auto index=sprite.indices[y*sprite.width+x];
                if (index && targetX>=0 && targetX<width)
                    pixels[(splats[i][1]+y)*width+targetX]=colors_[index];
            }
    }
}

void Renderer::corners(std::vector<std::uint32_t>& pixels) const
{
    // 0x13cb8..0x13d87: copy row prefixes, mirror to right, bottom table reversed.
    constexpr std::array<int,10> counts{60,48,40,32,24,20,16,12,8,4};
    for (int row = 0; row < 10; ++row) {
        for (int bottom = 0; bottom < 2; ++bottom) {
            const int y = bottom ? 140+row : row;
            const int count = counts[bottom ? 9-row : row];
            const auto& sprite = resources_.casque[bottom ? 3 : 2];
            for (int x = 0; x < count; ++x) {
                const auto color = colors_[sprite.indices[row*60+x]];
                pixels[y*width+x] = color;
                pixels[y*width+width-1-x] = color;
            }
        }
    }
}

void Renderer::bar(std::vector<std::uint32_t>& pixels, int value, int maximum, int y) const
{
    // 0x13fac..0x14014 / 0x140e8: overwrite the LEFT with the empty graphic.
    const int missing = int(std::int64_t(maximum-std::clamp(value,0,maximum))*43/maximum);
    if (missing) image(pixels,resources_.casque[18],68,y,false,missing);
}

void Renderer::weaponView(std::vector<std::uint32_t>& pixels, const State& state) const
{
    if (!state.weapon || *state.weapon>=resources_.weaponViews.size() || state.health<=0 || state.oxygen<=0) return;
    const unsigned id=*state.weapon;
    const auto& d=player::weaponDefinitions[id];
    if (!state.firing) image(pixels,resources_.weaponViews[id],d.gunX,d.gunY,true);
    else {
        // EXE firing composition, including reversed order for weapon 2.
        const unsigned phase=(state.barrelPhase&3)==3?1:(state.barrelPhase&3);
        const auto offset=d.muzzle[d.alternating?phase:0];
        if (!d.alternating) image(pixels,resources_.weaponViews[id],d.gunX,d.gunY+5,true);
        image(pixels,resources_.weaponEffects[id][0],d.gunX+offset[0],d.gunY+5+offset[1],true);
        if (d.alternating) image(pixels,resources_.weaponViews[id],d.gunX,d.gunY+5,true);
    }
    // DOS draws ARME[3] only in the firing branch, centered at (160,58).
    // Stable port reticle follows the current optical axis (160,75).
    const auto& reticle=resources_.weaponEffects[id][1];
    image(pixels,reticle,width/2-reticle.width/2,viewHeight/2-reticle.height/2,true);
}
void Renderer::weapon(std::vector<std::uint32_t>& pixels, const State& state) const
{
    // Clear the unconnected display explicitly; this is an empty preview slot.
    for (int y = 170; y < height; ++y)
        std::fill_n(pixels.begin()+y*width+263,57,colors_[0]);
    if (!state.weapon || *state.weapon >= resources_.weaponIcons.size()) return;
    const unsigned id = *state.weapon;
    image(pixels,resources_.weaponIcons[id],263,170);
    constexpr std::array<std::array<int,2>,6> positions{{
        {265,188},{265,191},{271,191},{287,193},{268,191},{269,192}}};
    int number = std::clamp(state.ammunition,0,999);
    for (int digit = 2; digit >= 0; --digit) {
        image(pixels,resources_.casque[26+number%10],positions[id][0]+digit*4,positions[id][1]);
        number /= 10;
    }
}

int Renderer::message(std::vector<std::uint32_t>& pixels, std::string_view text, int y) const
{
    int x = 8;
    for (unsigned char character : text) {
        if (y+10 > viewHeight) break;
        if (character == '\n') continue; // Original CR controls the newline.
        if (character == '\r') { x = 8; y += 11; continue; }
        if (character == ' ') { x += 9; }
        else if (const auto index = glyph(character)) {
            const auto& sprite = resources_.casque[*index];
            image(pixels,sprite,x,y,true);
            x += sprite.width+1;
        }
        if (x > 312) { x = 8; y += 11; }
    }
    return y+14;
}

bool Renderer::draw(std::vector<std::uint32_t>& pixels, const State& state) const
{
    if (pixels.size() != std::size_t(width)*height) return false;
    weaponView(pixels,state);
    if (state.slimePhase) slime(pixels,*state.slimePhase);
    image(pixels,resources_.casque[1],0,150);
    image(pixels,resources_.casque[17],68,162);
    image(pixels,resources_.casque[16],68,178);
    image(pixels,resources_.casque[19],125,155);
    const auto radar = state.detector.phase ? 5+std::min(*state.detector.phase,7u) : 4;
    image(pixels,resources_.casque[radar],0,171);
    if (state.detector.phase) for(unsigned i=0;i<std::min(state.detector.count,10u);++i) {
        const auto& contact=state.detector.contacts[i];
        const auto& dot=resources_.casque[13+std::min(contact.intensity,2u)];
        if (contact.x < -dot.width || contact.x>45 || contact.y<171-dot.height || contact.y>199) continue;
        for(int y=0;y<dot.height;++y) for(int x=0;x<dot.width;++x) {
            const int px=contact.x+x,py=contact.y+y;
            if (px>=0 && px<=45 && py>=171 && py<=199) {
                const auto index=dot.indices[y*dot.width+x];
                if (index) pixels[py*width+px]=colors_[index];
            }
        }
    }

    // Original inventory renderer 0x141d3..0x14200 clears inclusive rectangle.
    for (int y = 155; y <= 196; ++y)
        std::fill_n(pixels.begin()+y*width+183,59,colors_[0]);
    equipment(pixels,state);
    bar(pixels,state.oxygen,288000,162);
    bar(pixels,state.health,1000,178);
    weapon(pixels,state);
    corners(pixels);
    message(pixels,state.message);
    int messageY = 16;
    for (const auto text : state.messages)
        if (!text.empty()) messageY = message(pixels,text,messageY);
    return true;
}

void Renderer::equipment(std::vector<std::uint32_t>& pixels, const State& state) const
{
    // EXE 0x1420e..0x14305; coordinate tables at file 0x14379 and 0x14391.
    if (state.batteryCharge>=0) image(pixels,resources_.casque[state.detectorEnabled?41:40],222,156,true);
    if (state.oxygenBottle) image(pixels,resources_.casque[36],184,157,true);
    const unsigned cards = std::min(state.cards,3u), injectors = std::min(state.injectors,2u);
    constexpr int cardY[3][3] = {{171,0,0},{161,181,0},{157,171,185}};
    constexpr int injectorY[2][2] = {{168,0},{157,178}};
    for (unsigned i = 0; i < cards; ++i)
        image(pixels,resources_.casque[37],205,cardY[cards-1][i],true);
    for (unsigned i = 0; i < injectors; ++i)
        image(pixels,resources_.casque[38],215,injectorY[injectors-1][i],true);
}
} // namespace hud
