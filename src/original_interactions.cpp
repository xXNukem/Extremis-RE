#include "original_interactions.h"
#include "original_travel.h"
#include "interface_font.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace interaction {
namespace {
void requireImage(const std::vector<spr_format::Image>& bank, std::size_t index, int w, int h)
{
    if (index >= bank.size() || bank[index].width != w || bank[index].height != h)
        throw std::runtime_error("Unsupported original interaction graphics");
}
void validate(const Resources& r)
{
    if(r.pickupMovie.frames.size()!=5 || std::any_of(r.pickupMovie.frames.begin(),r.pickupMovie.frames.end(),
        [](const auto& frame){return frame.size()!=64000;})) throw std::runtime_error("Invalid pickup ANM");
    if (r.keyboard.size()!=10 || r.supplies.size()!=31 || r.weapons.size()!=26 ||
        r.inventory.size()!=27 || r.letters.size()!=76 || r.reader.size()!=5 || r.plan.size()!=5 || r.codes.size()!=10 || r.elevator.size()!=10)
        throw std::runtime_error("Invalid original interaction bank count");
    requireImage(r.keyboard,0,196,100); requireImage(r.supplies,12,196,100);
    requireImage(r.weapons,12,196,100); requireImage(r.inventory,0,184,180);
    requireImage(r.reader,0,196,100); requireImage(r.plan,0,196,100);
    requireImage(r.codes,0,196,100); requireImage(r.elevator,0,196,100);
    for (const auto* bank : {&r.keyboard,&r.supplies,&r.weapons,&r.inventory,&r.letters,&r.reader,&r.plan,&r.codes,&r.elevator})
        for (const auto& s : *bank)
            if (s.width<=0 || s.height<=0 || s.width>320 || s.height>200 || s.anchorX<0 || s.anchorX>s.width ||
                s.anchorY<0 || s.anchorY>s.height ||
                s.indices.size()!=std::size_t(s.width)*s.height)
                throw std::runtime_error("Invalid original interaction sprite pixels");
    for (const auto* bank:{&r.supplyDoor,&r.weaponDoor,&r.planDoor}) {
        if (bank->size()!=1) throw std::runtime_error("Invalid terminal door bank");
        requireImage(*bank,0,196,128);
    }
    if (r.supplyOpening.size()!=12 || r.weaponOpening.size()!=11)
        throw std::runtime_error("Invalid cabinet opening bank");
    requireImage(r.supplyOpening,0,196,100); requireImage(r.weaponOpening,0,196,100);
    for (const auto* bank:{&r.supplyDoor,&r.weaponDoor,&r.planDoor,&r.supplyOpening,&r.weaponOpening})
        for (const auto& s:*bank)
            if (s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
                s.anchorX<0 || s.anchorY<0 || s.anchorX>s.width || s.anchorY>s.height ||
                s.indices.size()!=std::size_t(s.width)*s.height)
                throw std::runtime_error("Invalid terminal animation pixels");
    interface_font::validate(r.smallFont,8);
    interface_font::validate(r.tinyFont,5);
}
std::optional<unsigned> letter(unsigned char c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='Z') return 10+c-'A';
    if (c>='a' && c<='z') return 36+c-'a';
    switch(c) { case ':': return 62; case '-': return 63; case '?': return 64;
        case '!': return 65; case '\'': return 66; case ',': return 67; case '.': return 68; }
    return {};
}
}
Resources load(const std::filesystem::path& directory)
{
    Resources r;
    r.pickupMovie=anm_format::load(directory/"A_ARME.ANM");
    r.supplyDoor=spr_format::load(directory/"CH_OBJ2.SPR");
    r.supplyOpening=spr_format::load(directory/"CH_OBJ3.SPR");
    r.weaponDoor=spr_format::load(directory/"CHARME2.SPR");
    r.weaponOpening=spr_format::load(directory/"CHARME3.SPR");
    r.planDoor=spr_format::load(directory/"B_PLAN2.SPR");
    r.keyboard=spr_format::load(directory/"CHARME.SPR");
    r.supplies=spr_format::load(directory/"CH_OBJ4A.SPR");
    r.weapons=spr_format::load(directory/"CHARME4A.SPR");
    r.inventory=spr_format::load(directory/"INVENT_A.SPR");
    r.letters=spr_format::load(directory/"INVENT_T.SPR");
    r.reader=spr_format::load(directory/"B_PORTE.SPR");
    r.plan=spr_format::load(directory/"B_PLAN3.SPR");
    r.elevator=spr_format::load(directory/"CH_LEVEL.SPR");
    std::ifstream tiny(directory/"TYPO_3X5.BIN",std::ios::binary);
    r.tinyFont={std::istreambuf_iterator<char>(tiny),std::istreambuf_iterator<char>()};
    r.codes=spr_format::load(directory/"B_CODE.SPR");
    r.palette=wall_textures::loadPalette(directory/"PAL_DI.PAL");
    std::ifstream file(directory/"TYPO_3X8.BIN",std::ios::binary);
    r.smallFont={std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    validate(r);
    return r;
}
Renderer::Renderer(Resources resources): resources_(std::move(resources))
{
    validate(resources_);
    for (std::size_t i=0;i<256;++i) {
        const auto c=resources_.palette[i];
        colors_[i]=(std::uint32_t(c.r)<<24)|(std::uint32_t(c.g)<<16)|(std::uint32_t(c.b)<<8)|255;
    }
}
void Renderer::image(std::vector<std::uint32_t>& pixels, const spr_format::Image& s,
                     int x, int y, bool transparent, int clipTop, int clipBottom,
                     int clipLeft, int clipRight) const
{
    // EXE 0x233f/0x2346 subtracts the per-image pivot before clipping.
    x-=s.anchorX; y-=s.anchorY;
    for (int row=std::max(0,clipTop-y);row<s.height && y+row<clipBottom;++row)
        for (int col=std::max(0,clipLeft-x);col<s.width && x+col<clipRight;++col) {
            const auto index=s.indices[row*s.width+col];
            if (!transparent || index) pixels[(y+row)*320+x+col]=colors_[index];
        }
}
void Renderer::smallText(std::vector<std::uint32_t>& pixels, std::string_view text, int x, int y, bool tiny) const
{
    interface_font::draw(pixels,tiny?resources_.tinyFont:resources_.smallFont,
                         colors_[tiny?24:102],text,x,y);
}
void Renderer::menuText(std::vector<std::uint32_t>& pixels, std::string_view text, int y, bool selected) const
{
    int width=0;
    for (unsigned char c : text) { const auto id=letter(c); width+=id ? resources_.letters[*id].width+1 : 4; }
    int x=72+(176-width)/2;
    // EXE 0x1ce21 centers within 176 pixels; 0x1d0a2 suppresses blinking text.
    if (selected) return;
    for (unsigned char c : text) {
        const auto id=letter(c);
        if (id) {
            const auto& s=resources_.letters[*id];
            for (int row=0;row<s.height;++row) for (int col=0;col<s.width;++col)
                if (x+col>=72 && x+col<248 && y+row>=146 && y+row<187)
                    pixels[(y+row)*320+x+col]=colors_[s.indices[row*s.width+col]];
            x+=s.width+1;
        } else x+=4;
    }
}
void Renderer::code(std::vector<std::uint32_t>& pixels, const State& s) const
{
    image(pixels,resources_.keyboard[0],62,25,false);
    // Selected key positions, EXE 0x1967c..0x196af. Rightmost keys are erase/enter.
    const unsigned key=std::min(s.selection,13u);
    if (s.pressedKey && *s.pressedKey<14 && s.fingerPhase==2) {
        const auto pressed=*s.pressedKey;
        image(pixels,resources_.keyboard[1],88+15*int(pressed%7),pressed<7?58:75);
    }
    // Recovered resting glove/finger pose, EXE 0x196b4..0x19707.
    const int handX=70+15*int(key%7), handY=70+17*int(key/7)-(s.fingerPhase==2?8:s.fingerPhase?4:0)
        +(s.phase==Phase::HandOut?4*int(s.frame):s.phase==Phase::HandIn?64-4*int(s.frame):0);
    image(pixels,resources_.keyboard[6],handX,handY,true,25,125,62,258);
    image(pixels,resources_.keyboard[s.fingerPhase==2?9:s.fingerPhase?8:7],handX+42,handY+9,true,25,125,62,258);
    if (!s.notice.empty()) { smallText(pixels,s.notice,77,36); return; }
    constexpr std::string_view prompt="TYPE IN CODE ZONE C200    ";
    smallText(pixels,prompt,77,36);
    smallText(pixels,s.entered,77+4*int(prompt.size()),36);
    for (std::size_t i=s.entered.size();i<6;++i)
        smallText(pixels,".",77+4*int(prompt.size()+i),36);
}
void Renderer::supply(std::vector<std::uint32_t>& pixels, const State& s, bool weapon) const
{
    const auto& bank=weapon?resources_.weapons:resources_.supplies;
    image(pixels,bank[12],62,25,false);
    const unsigned kind=s.contents>>12, amount=s.contents&0xfff;
    // Supply descriptor table at file 0x1835e, eight signed words per class.
    constexpr int positions[7][2]={{174,54},{146,61},{188,54},{181,75},{181,75},{181,75},{174,47}};
    if (!weapon && kind<7) image(pixels,bank[kind],positions[kind][0],positions[kind][1]);
    if (weapon && kind<6) {
        // Weapon final placement uses its own descriptor at file 0x7fe2.
        constexpr int pos[6][2]={{160,54},{139,61},{146,54},{139,47},{139,54},{146,54}};
        image(pixels,bank[kind],pos[kind][0],pos[kind][1]);
    }
    if (weapon && kind>=6) return;
    if (!weapon && kind>=7) return;
    image(pixels,bank[weapon?kind+6:kind<3?kind+7:10],75,32);
    if (!weapon && kind>=3 && kind<=5) {
        image(pixels,bank[27],145,32);
        image(pixels,bank[28+kind-3],214,36);
    } else if (!weapon) image(pixels,bank[26],145,32);
    const unsigned number=(!weapon && kind<2)?amount/(kind==0?9:18):amount;
    for (unsigned i=0,div=100;i<3;++i,div/=10)
        image(pixels,bank[16+(number/div)%10],(weapon?220:221)+6*int(i),36);
    image(pixels,bank[13],75,60);
    // The question sprite already contains both bright labels. These overlays
    // contain palette 220 (dark), so dim the option that is NOT selected.
    image(pixels,bank[s.selection==0?15:14],s.selection==0?117:90,96);
    if (!s.notice.empty()) smallText(pixels,s.notice,77,110);
}
void Renderer::cards(std::vector<std::uint32_t>& pixels, const State& s) const
{
    image(pixels,resources_.inventory[0],68,10,false);
    unsigned pocket=0;
    for (int card:s.cards) if (card>=0) {
        const unsigned family=unsigned(card)>>12;
        if (family<3) image(pixels,resources_.inventory[19+family],203,50+31*int(pocket));
        ++pocket;
    }
    for (int row=0;row<41;++row)
        std::fill_n(pixels.begin()+(146+row)*320+72,176,colors_[254]);
    unsigned count=0;
    for (int card:s.cards) if (card>=0) ++count;
    if (!count) menuText(pixels,s.elevator?"No elevator card":"No security card",146,false);
    unsigned entry=0;
    for (int card:s.cards) if (card>=0) {
        const std::string_view prefix=s.replacing?"Exchange with card ":"Use card ";
        const auto name=cardName(card);
        std::array<char,32> label{};
        std::copy(prefix.begin(),prefix.end(),label.begin());
        std::copy(name.begin(),name.end(),label.begin()+prefix.size());
        menuText(pixels,std::string_view(label.data(),prefix.size()+name.size()),
                 146+11*int(entry),s.selection==entry && !s.blinkVisible);
        ++entry;
    }
    menuText(pixels,"Exit",146+11*int(count?count:1),s.selection==count && !s.blinkVisible);
    // Rejection owns the menu region until navigation or confirmation.
    if (!s.notice.empty()) {
        for (int row=0;row<41;++row)
            std::fill_n(pixels.begin()+(146+row)*320+72,176,colors_[254]);
        menuText(pixels,s.notice,146,false);
        if (!count) menuText(pixels,"Exit",157,s.selection==0 && !s.blinkVisible);
    }
}
void Renderer::inventory(std::vector<std::uint32_t>& pixels, const State& s) const
{
    const auto& bank=resources_.inventory;
    image(pixels,bank[0],68,10,false);
    const unsigned section=std::min(s.selection,5u);
    constexpr int rows[]{14,41,60,86,110,134}; // EXE file 0x1aafa.
    if (section<5) image(pixels,bank[6+section],180,11);
    if (s.blinkVisible) image(pixels,bank[section==5?26:1+section],139,rows[section]);
    for (int row=146;row<187;++row)
        std::fill_n(pixels.begin()+row*320+72,176,colors_[254]);
    if (s.inventoryUse) {
        if (section==0) {
            menuText(pixels,s.detectorEnabled?"Battery utilized for the radar":"Battery is not usable",146,false);
            menuText(pixels,s.detectorEnabled?"Eteindre le radar":"Radar is running",157,s.useSelection==0 && !s.blinkVisible);
            menuText(pixels,"Exit",168,s.useSelection==1 && !s.blinkVisible);
            return;
        }
        if (section==2 && s.injectors[0]>0 && s.injectors[1]>0) {
            menuText(pixels,"Use injector-1",146,s.useSelection==0 && !s.blinkVisible);
            menuText(pixels,"Use injector-2",157,s.useSelection==1 && !s.blinkVisible);
            menuText(pixels,"Exit",168,s.useSelection==2 && !s.blinkVisible);
            return;
        }
        menuText(pixels,section==1?"Use oxygen bottle":"Use injector",146,
                 s.useSelection==0 && !s.blinkVisible);
        menuText(pixels,"Do not use",157,s.useSelection==1 && !s.blinkVisible);
        return;
    }
    switch(section) {
    case 0:
        image(pixels,bank[s.batteryCharge<0?11:16],199,66);
        menuText(pixels,s.batteryCharge<0?"No battery":s.batteryCharge==0?"Batterie vide":
                 s.detectorEnabled?"Battery utilized for the radar":"Battery is not usable",146,false); break;
    case 1:
        image(pixels,bank[s.oxygenReserve<0?12:17],197,40);
        menuText(pixels,s.oxygenReserve<0?"No oxygen bottle":s.oxygenReserve==0?"Empty oxygen bottle":"Use oxygen bottle",146,false);
        break;
    case 2:
        for (unsigned i=0;i<2;++i) image(pixels,bank[s.injectors[i]<0?13:18],i?224:195,i?85:44);
        menuText(pixels,s.injectors[0]<0 && s.injectors[1]<0?"No injector":
                 (s.injectors[0]>0 || s.injectors[1]>0)?"Use injector":"Empty injector",146,false);
        break;
    case 3: {
        unsigned row=0;
        for (int card:s.cards) if (card>=0) {
            const auto family=unsigned(card)>>12;
            if (family<3) image(pixels,bank[19+family],203,50+31*int(row));
            menuText(pixels,"Card "+cardName(card),146+11*int(row++),false);
        }
        if (!row) menuText(pixels,"No cards",146,false);
        break;
    }
    case 4: image(pixels,bank[15],198,60); menuText(pixels,"No mine",146,false); break;
    case 5: menuText(pixels,"Exit",146,false); break;
    }
}
bool Renderer::draw(std::vector<std::uint32_t>& pixels, const State& state) const
{
    if (pixels.size()!=320*200) return false;
    if (sequence(pixels,state)) return true;
    switch(state.screen) {
    case Screen::Elevator: {
        const auto& bank=resources_.elevator;
        image(pixels,bank[0],62,25,false);
        for (unsigned row=0;row<4;++row) {
            const auto* route=original::route(state.elevatorIndex,row);
            if (!route || !route->exists) continue;
            const int y=82-16*int(row);
            smallText(pixels,route->label.substr(0,4),200,y,true);
            smallText(pixels,route->label.substr(4,4),200,y+6,true);
            if (original::admitted(*route,state.useSelection,state.elevatorZone,state.elevatorSector))
                image(pixels,bank[route->zone==state.elevatorZone?3:1],142,y-3);
        }
        if(state.phase!=Phase::Opening) {
            const int percent=state.phase==Phase::CardInsert?5*int(std::min(state.frame,20u)):100;
            image(pixels,bank[5],62+16*percent/100,percent==100?41:125-85*percent/100,true,54,125,62,258);
        }
        if(state.phase==Phase::Opening || state.phase==Phase::CardInsert) break;
        const int handY=state.phase==Phase::HandIn?123-2*int(state.frame):87-16*int(std::min(state.selection,3u));
        image(pixels,bank[6],136,handY+4,true,25,125,62,258);
        image(pixels,bank[7],178,handY+13,true,25,125,62,258);
        if (!state.notice.empty()) smallText(pixels,state.notice,77,110);
        break;
    }
    case Screen::Inventory: inventory(pixels,state); break;
    case Screen::Code: code(pixels,state); break;
    case Screen::Supply: supply(pixels,state,false); break;
    case Screen::Weapon: supply(pixels,state,true); break;
    case Screen::Cards: cards(pixels,state); break;
    case Screen::Reader: reader(pixels,state); break;
    case Screen::Codes:
        image(pixels,resources_.codes[0],62,25,false);
        image(pixels,resources_.codes[1],85,52,true,65,125);
        {
            const int handY=state.phase==Phase::HandIn?168-5*int(std::min(state.frame,20u)):state.selection?86:68;
            image(pixels,resources_.codes[5],134,handY,true,25,125,62,258);
            image(pixels,resources_.codes[6],173,handY+8,true,25,125,62,258);
        }
        smallText(pixels,state.notice,76,37); break;
    case Screen::Plan: shutters(pixels,state); break;
    }
    return true;
}
std::optional<std::uint8_t> codeChecksum(std::string_view code)
{
    if (code.size()!=6) return {};
    unsigned sum=0;
    for (unsigned i=0;i<6;++i) {
        if (code[i]<'A' || code[i]>'L') return {};
        sum+=unsigned(code[i]-'A')<<i;
    }
    return std::uint8_t(sum);
}
std::string cardName(int packed)
{
    if (packed<0) return {};
    const unsigned kind=unsigned(packed)>>12, value=unsigned(packed)&0xfff;
    std::string result(1,kind==0?'E':kind==1?'C':'N');
    result+=char('0'+value/100%10); result+=char('0'+value/10%10); result+=char('0'+value%10);
    return result;
}
}
