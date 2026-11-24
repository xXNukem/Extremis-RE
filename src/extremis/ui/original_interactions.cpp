#include <string_view>
#include <cstddef>
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "extremis/ui/original_interactions.h"
#include "extremis/ui/localization.h"
#include "extremis/ui/utf8_glyph.h"
#include "vortex/core/graphics/color.h"
#include "extremis/world/original_travel.h"
#include "extremis/ui/interface_font.h"
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
// INVENT_T.SPR is the original inventory's shaded 5x9 alphabet. Compose on
// copies of INVENT_A.SPR, in palette indices, before the normal draw path.
void inventoryLine(spr_format::Image& target, const std::vector<spr_format::Image>& alphabet,
                   std::string_view value, int left, int top, int available, bool orange=false)
{
    struct Character { localization::Glyph glyph; int width; };
    std::vector<Character> characters;
    int width=0;
    for(std::size_t i=0;i<value.size();) {
        const auto g=localization::glyph(value,i); i+=g.bytes;
        const auto id=letter(g.base);
        const int w=id?alphabet[*id].width:3;
        characters.push_back({g,w}); width+=w+1;
    }
    if(width) --width;
    const float scale=width>available?float(available)/width:1.f;
    int advance=0;
    const auto pixel=[&](int x,int y,std::uint8_t index) {
        if(x>=0 && x<target.width && y>=0 && y<target.height)
            target.indices[y*target.width+x]=index;
    };
    for(const auto& character:characters) {
        const auto id=letter(character.glyph.base);
        const int origin=left+int(advance*scale);
        if(id) {
            const auto& source=alphabet[*id];
            for(int y=0;y<source.height;++y) for(int x=0;x<source.width;++x) {
                const auto shade=source.indices[y*source.width+x];
                if(shade==254) continue;
                const std::uint8_t color=orange?std::uint8_t(shade<=217?67:shade==218?68:shade==219?69:shade==220?70:shade==221?71:72):shade;
                pixel(origin+int(x*scale),top+y,color);
            }
            const int middle=origin+int(source.width*scale/2);
            const auto mark=character.glyph.mark;
            if(mark) {
                const std::uint8_t ink=orange?67:216;
                if(mark=='a') pixel(middle+1,top-1,ink);
                else if(mark=='g') pixel(middle-1,top-1,ink);
                else if(mark=='u') {pixel(middle-1,top-1,ink);pixel(middle+1,top-1,ink);}
                else if(mark=='c') {pixel(middle,top+source.height,ink);pixel(middle-1,top+source.height+1,ink);}
                else {pixel(middle-1,top-1,ink);pixel(middle+1,top-2,ink);}
            }
        }
        advance+=character.width+1;
    }
}
void localizedInventory(std::vector<spr_format::Image>& bank,
                        const std::vector<spr_format::Image>& alphabet,
                        localization::Language language)
{
    // Synthetic test resources intentionally contain one-pixel placeholders.
    if(bank[6].width!=71 || bank[6].height!=25 ||
       alphabet[0].width!=5 || alphabet[0].height!=9) return;
    const bool spanish=language==localization::Language::Spanish;
    // Base text occupies x=70..109 in the 184x180 sprite. The connector ends
    // at x=69 and the separate section panel begins at screen x=180.
    constexpr int bands[][2]={{2,22},{29,39},{48,68},{74,94},{98,118},{122,133}};
    for(const auto& band:bands) for(int y=band[0];y<band[1];++y)
        std::fill_n(bank[0].indices.begin()+y*bank[0].width+70,40,254);
    const auto base=[&](int y,std::string_view first,std::string_view second={}) {
        inventoryLine(bank[0],alphabet,first,70,y,40);
        if(!second.empty()) inventoryLine(bank[0],alphabet,second,70,y+9,40);
    };
    base(4,spanish?"Batería":"Batterie",spanish?"Cargador":"Chargeur");
    base(31,spanish?"Oxígeno":"Oxygène");
    base(50,spanish?"Bolsillo:":"Poche:",spanish?"Inyectores":"Injecteurs");
    base(76,spanish?"Bolsillo:":"Poche:",spanish?"Tarjetas":"Cartes");
    base(100,spanish?"Bolsillo:":"Poche:",spanish?"Bomba":"Bombe");
    base(124,spanish?"Salir":"Quitter");

    // The selected labels are their own opaque black-backed sprites. Their
    // original widths are too small for translations; 40 px stays within the
    // left pane, ending before the section panel at screen x=180.
    constexpr int heights[]={18,9,18,18,18};
    for(int n=0;n<5;++n) {
        auto& sprite=bank[1+n]; sprite.width=40; sprite.height=heights[n];
        sprite.indices.assign(std::size_t(sprite.width)*sprite.height,254);
    }
    const auto selected=[&](int n,std::string_view first,std::string_view second={}) {
        inventoryLine(bank[n],alphabet,first,0,0,40,true);
        if(!second.empty()) inventoryLine(bank[n],alphabet,second,0,9,40,true);
    };
    selected(1,spanish?"Batería":"Batterie",spanish?"Cargador":"Chargeur");
    selected(2,spanish?"Oxígeno":"Oxygène");
    selected(3,spanish?"Bolsillo:":"Poche:",spanish?"Inyectores":"Injecteurs");
    selected(4,spanish?"Bolsillo:":"Poche:",spanish?"Tarjetas":"Cartes");
    selected(5,spanish?"Bolsillo:":"Poche:",spanish?"Bomba":"Bombe");

    constexpr int titleTop[]={8,3,3,5,9};
    const std::string_view titles[5][2]={
        {spanish?"Batería SE":"Batterie SE",""},
        {spanish?"Reserva":"Réserve",spanish?"Oxígeno":"Oxygène"},
        {spanish?"Inyector":"Injecteur",spanish?"Tónico":"Tonique"},
        {spanish?"Tarjeta":"Carte",spanish?"Electrónica":"Électronique"},
        {spanish?"Bomba DAM10":"Bombe DAM10",""}};
    for(int n=0;n<5;++n) {
        auto& sprite=bank[6+n];
        // The title interior is uniformly palette 223 under the original
        // glyphs. Restore only that interior; the four border edges remain.
        for(int y=2;y<22;++y)
            std::fill_n(sprite.indices.begin()+y*sprite.width+2,67,223);
        const auto centered=[&](std::string_view line,int y) {
            int width=0;
            for(std::size_t i=0;i<line.size();) {
                const auto g=localization::glyph(line,i); i+=g.bytes;
                const auto id=letter(g.base);
                width+=(id?alphabet[*id].width:3)+1;
            }
            if(width) --width;
            inventoryLine(sprite,alphabet,line,2+(67-std::min(width,61))/2,y,61);
        };
        centered(titles[n][0],titleTop[n]);
        if(!titles[n][1].empty()) centered(titles[n][1],titleTop[n]+10);
    }
    bank[26].width=40; bank[26].indices.assign(40*9,254); bank[26].height=9;
    inventoryLine(bank[26],alphabet,spanish?"Salir":"Quitter",0,0,40,true);
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
        colors_[i]=vortex::packOpaqueRgb(c.r, c.g, c.b);
    }
    for(unsigned i=0;i<2;++i) {
        localizedInventory_[i]=resources_.inventory;
        localizedInventory(localizedInventory_[i],resources_.letters,
                           i?localization::Language::French:localization::Language::Spanish);
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
    const auto localized=localization::translate(text);
    interface_font::draw(pixels,tiny?resources_.tinyFont:resources_.smallFont,
                         colors_[tiny?24:102],localized,x,y);
}
void Renderer::menuText(std::vector<std::uint32_t>& pixels, std::string_view text, int y, bool selected) const
{
    const auto localized=localization::translate(text); text=localized;
    int width=0;
    for (std::size_t i=0;i<text.size();) { const auto decoded=localization::glyph(text,i); i+=decoded.bytes; const auto id=letter(decoded.base); width+=id ? resources_.letters[*id].width+1 : 4; }
    const float factor=localization::current()!=localization::Language::English && width>176 ? 176.f/width : 1.f;
    int x=72+int((176-width*factor)/2);
    // EXE 0x1ce21 centers within 176 pixels; 0x1d0a2 suppresses blinking text.
    if (selected) return;
    for (std::size_t i=0;i<text.size();) {
        const auto decoded=localization::glyph(text,i); i+=decoded.bytes;
        const auto id=letter(decoded.base);
        if (id) {
            const auto& s=resources_.letters[*id];
            for (int row=0;row<s.height;++row) for (int col=0;col<s.width;++col) {
                const int px=x+int(col*factor);
                if (px>=72 && px<248 && y+row>=146 && y+row<187)
                    pixels[(y+row)*320+px]=colors_[s.indices[(decoded.mark=='i'?s.height-1-row:row)*s.width+(decoded.mark=='i'?s.width-1-col:col)]];
            }
            if(decoded.mark && decoded.mark!='i') localization::markPixels(pixels,x,y,s.width,s.height,colors_[102],decoded.mark);
            x+=int((s.width+1)*factor);
        } else x+=int(4*factor);
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
    const std::string prompt="TYPE IN CODE ZONE "+s.zoneName+"    ";
    const bool localized=localization::current()!=localization::Language::English;
    smallText(pixels,localized?std::string_view(prompt).substr(0,prompt.size()-4):std::string_view(prompt),77,36);
    const int codeX=localized?77:77+4*int(prompt.size());
    const int codeY=localized?46:36;
    smallText(pixels,s.entered,codeX,codeY);
    for (std::size_t i=s.entered.size();i<6;++i)
        smallText(pixels,".",codeX+4*int(i),codeY);
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
    image(pixels,bank[weapon?kind+6:kind<3?kind+7:kind==6?11:10],75,32);
    if(!weapon && localization::current()!=localization::Language::English) {
        for(int row=34;row<56;++row)
            std::fill_n(pixels.begin()+row*320+77,67,colors_[220]);
        constexpr const char* ids[]={"modal.supply_battery","modal.supply_oxygen","modal.supply_energy",
            "modal.supply_card","modal.supply_card","modal.supply_card","modal.supply_bomb"};
        const auto label=localization::byId(ids[kind],localization::current());
        const auto line=label.find('\n');
        smallText(pixels,label.substr(0,line),79,line==std::string_view::npos?41:37,true);
        if(line!=std::string_view::npos) smallText(pixels,label.substr(line+1),79,44,true);
    }
    if (!weapon && kind>=3 && kind<=5) {
        image(pixels,bank[27],145,32);
        image(pixels,bank[28+kind-3],214,36);
    } else if (!weapon) image(pixels,bank[26],145,32);
    const unsigned number=(!weapon && kind<2)?amount/(kind==0?9:18):amount;
    for (unsigned i=0,div=100;i<3;++i,div/=10)
        image(pixels,bank[16+(number/div)%10],(weapon?220:221)+6*int(i),36);
    if(localization::current()!=localization::Language::English) {
        for(int row=32;row<47;++row)
            std::fill_n(pixels.begin()+row*320+145,106,colors_[223]);
        const bool card=!weapon && kind>=3 && kind<=5;
        const auto label=localization::byId(weapon?"modal.ammo":card?"modal.number":"modal.condition",
                                            localization::current());
        interface_font::draw(pixels,resources_.tinyFont,colors_[102],
                             std::string(label)+": "+(card?std::string(1,kind==3?'E':kind==4?'C':'N'):"")+
                             std::to_string(number)+(weapon||card?"":"%"),172,36);
    }
    if (!weapon && kind==0 && s.replacing) {
        // Battery charge is in PIT units in inventory and whole seconds in the cabinet.
        const auto option=[&](unsigned index, const char* label, unsigned percent) {
            if (s.selection==index && !s.blinkVisible) return;
            const bool localized=localization::current()!=localization::Language::English;
            const auto text=std::string(label)+std::to_string(percent)+"%";
            if(localized) interface_font::draw(pixels,resources_.tinyFont,colors_[102],
                localization::translate(text),77,83+14*int(index));
            else smallText(pixels,text,77,80+11*int(index));
        };
        if(localization::current()!=localization::Language::English) {
            const auto label=localization::byId("panel.battery_owned",localization::current());
            const auto split=localization::current()==localization::Language::French?std::size_t(9):std::size_t(12);
            interface_font::draw(pixels,resources_.tinyFont,colors_[102],label.substr(0,split),77,68);
            interface_font::draw(pixels,resources_.tinyFont,colors_[102],label.substr(split+1),77,75);
        } else smallText(pixels,"ALREADY HAVE A BATTERY",77,68);
        option(0,"TAKE BATTERY ",number);
        option(1,"KEEP BATTERY ",unsigned(s.batteryCharge)/1440);
        return;
    }
    image(pixels,bank[13],75,60);
    if(localization::current()==localization::Language::English) {
        // Original question and choice sprites retain their DOS presentation.
        image(pixels,bank[s.selection==0?15:14],s.selection==0?117:90,96);
    } else {
        for(int row=64;row<109;++row)
            std::fill_n(pixels.begin()+row*320+77,60,colors_[220]);
        const auto language=localization::current();
        const auto question=localization::byId("modal.take_question",language);
        const auto split=language==localization::Language::Spanish?std::size_t(7):std::size_t(11);
        smallText(pixels,question.substr(0,split),77,72,true);
        smallText(pixels,question.substr(split+1),77,80,true);
        const auto choice=[&](std::string_view id,int x,unsigned index) {
            if(s.selection==index && !s.blinkVisible) return;
            interface_font::draw(pixels,resources_.tinyFont,colors_[s.selection==index?102:24],
                                 localization::byId(id,language),x,97);
        };
        choice("modal.yes",89,0); choice("modal.no",117,1);
    }
    if (!s.notice.empty()) smallText(pixels,s.notice,77,110);
}
void Renderer::cards(std::vector<std::uint32_t>& pixels, const State& s) const
{
    const auto language=localization::current();
    const auto& bank=language==localization::Language::English?resources_.inventory:
        localizedInventory_[language==localization::Language::French?1:0];
    image(pixels,bank[0],68,10,false);
    unsigned pocket=0;
    for (int card:s.cards) if (card>=0) {
        const unsigned family=unsigned(card)>>12;
        if (family<3) image(pixels,bank[19+family],203,50+31*int(pocket));
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
    const auto language=localization::current();
    const auto& bank=language==localization::Language::English?resources_.inventory:
        localizedInventory_[language==localization::Language::French?1:0];
    image(pixels,bank[0],68,10,false);
    const unsigned section=std::min(s.selection,5u);
    constexpr int rows[]{14,41,60,86,110,134}; // EXE file 0x1aafa.
    if (section<5) image(pixels,bank[6+section],180,11);
    if (s.blinkVisible) image(pixels,bank[section==5?26:1+section],139,rows[section]);
    for (int row=146;row<187;++row)
        std::fill_n(pixels.begin()+row*320+72,176,colors_[254]);
    if (s.inventoryUse) {
        if (section==0) {
            using Mode=player::BatteryMode;
            const char* first=s.batteryMode==Mode::Off ? "Turn on radar" :
                              s.batteryMode==Mode::Radar ? "Turn off radar" : "Turn off optics";
            const char* second=s.batteryMode==Mode::Optics ? "Turn on radar" : "Turn on optics";
            menuText(pixels,"Battery level "+std::to_string(s.batteryCharge/1440),146,false);
            menuText(pixels,first,157,s.useSelection==0 && !s.blinkVisible);
            menuText(pixels,second,168,s.useSelection==1 && !s.blinkVisible);
            menuText(pixels,"No change",179,s.useSelection==2 && !s.blinkVisible);
            return;
        }
        if (section==2 && s.injectors[0]>0 && s.injectors[1]>0) {
            menuText(pixels,"Use injector-1",146,s.useSelection==0 && !s.blinkVisible);
            menuText(pixels,"Use injector-2",157,s.useSelection==1 && !s.blinkVisible);
            menuText(pixels,"Exit",168,s.useSelection==2 && !s.blinkVisible);
            return;
        }
        const char* action=section==4?"Use bomb":section==1?"Use oxygen bottle":"Use injector";
        menuText(pixels,action,146,
                 s.useSelection==0 && !s.blinkVisible);
        menuText(pixels,"Do not use",157,s.useSelection==1 && !s.blinkVisible);
        return;
    }
    switch(section) {
    case 0:
        image(pixels,bank[s.batteryCharge<0?11:16],199,66);
        menuText(pixels,s.batteryCharge<0 ? "No battery" : s.batteryCharge==0 ? "Empty battery" :
                 s.batteryMode==player::BatteryMode::Radar ? "Battery utilized for the radar" :
                 s.batteryMode==player::BatteryMode::Optics ? "Battery utilized for optics system" :
                 "Battery available",146,false);
        if (s.batteryCharge>0)
            menuText(pixels,"Battery level "+std::to_string(s.batteryCharge/1440),157,false);
        break;
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
    case 4: image(pixels,bank[s.mine<0?15:22],198,60);
        menuText(pixels,s.mine<0?"No mine":"Mine",146,false); break;
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
