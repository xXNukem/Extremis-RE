#include "debug_menu.h"
#include "original_travel.h"
#include <algorithm>
#include <utility>
#include <set>
#include <stdexcept>

void DebugMenu::load(const std::filesystem::path& lab) {
    const auto count=game_lab::loadBankTable(lab).size();
    for (unsigned z=0;z<count;++z) levels_.push_back(game_lab::loadLevel(lab,z).level);
    for (unsigned z=0;z<levels_.size();++z)
        for (unsigned i=0;i<interaction::original::lifts.size();++i) {
            const auto& lift=interaction::original::lifts[i];
            for (const auto& r:lift.routes) if (r.exists && r.zone==z) {
                auto level=levels_[z]; level.playerStart=game_lab::PlayerStart{r.sector,lift.x,lift.y,lift.angle};
                const auto& sector=level.sectors.at(r.sector);
                for (const auto& wall:collision::staticWorld(level))
                if (collision::overlaps(wall,{double(sector.gx)+lift.x,double(sector.gy)+lift.y},30))
                    throw std::runtime_error("Invalid original elevator arrival level "+std::to_string(z+1)+" lift "+std::to_string(i));
                entries_[0].push_back({"NIVEL "+std::to_string(z+1)+" ASC "+std::to_string(i)+" S"+std::to_string(r.sector),0,
                    ItemSession::Travel{z,*level.playerStart}});
            }
        }
    std::set<std::uint16_t> weapons,supplies;
    for (const auto& level:levels_) { const items::World world(level); for (const auto& p:world.panels()) {
        if (p.weapon && p.contents!=0xffff && (p.contents&4095)>0) weapons.insert(p.contents);
        if (!p.weapon && !p.cardReader && !p.codePanel && !p.codeReader && !p.elevator && (p.contents>>12)<3) supplies.insert(p.contents);
    }
    }
    for (auto c:weapons) entries_[1].push_back({"ARMA "+std::to_string(c>>12)+" MUNICION "+std::to_string(c&4095),c,{}});
    for (auto c:supplies) entries_[3].push_back({std::string((c>>12)==0?"BATERIA ":(c>>12)==1?"OXIGENO ":"INYECTOR ")+std::to_string(c&4095),c,{}});
    entries_[2].push_back({"ACTIVAR / DESACTIVAR",0,{}});
}
void DebugMenu::refresh(unsigned zone) {
    if (zone_==zone) return;
    zone_=zone; row_=0; replacement_.reset(); entries_[4].clear(); entries_[5].clear();
    if (zone>=levels_.size()) return;
    const auto& level=levels_[zone];
    std::set<std::uint16_t> cards;
    const items::World world(level);
    for (const auto& p:world.panels()) {
        const std::string id="S"+std::to_string(p.sector)+"/P"+std::to_string(p.wall);
        if (!p.weapon && !p.cardReader && !p.codePanel && !p.codeReader && !p.elevator && (p.contents>>12)>=3 && (p.contents>>12)<=5)
            cards.insert(p.contents);
        if (!p.elevator && !p.cardReader && !p.codeReader) {
            std::string code="PENDIENTE DE INVESTIGACION";
            if (zone<interaction::original::codes.size()) {
                const auto letters=interaction::original::codes[zone].letters;
                if (level.accessCode && interaction::codeChecksum(letters)==level.accessCode) code=letters;
            }
            entries_[5].push_back({id+" "+(p.codePanel?"TERMINAL ":"TAQUILLA ")+code,0,{}});
        }
        if (p.codeReader) for (auto index:level.codeTargets) {
            std::string value="PENDIENTE DE INVESTIGACION";
            if (index<interaction::original::codes.size()) {
                const auto& c=interaction::original::codes[index];
                value=std::string(c.zone)+" "+std::string(c.letters);
            }
            entries_[5].push_back({id+" CONSULTA "+value,0,{}});
        }
    }
    for (auto c:cards) entries_[4].push_back({interaction::cardName(c-0x3000),c,{}});
}
bool DebugMenu::event(const SDL_Event& e, player::Inventory& inventory, unsigned zone) {
    refresh(zone);
    cards_=inventory.cards();
    if (e.type==SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.scancode==SDL_SCANCODE_TAB) {
        open_=!open_; replacement_.reset(); return true;
    }
    if (!open_) return false;
    if (e.type!=SDL_EVENT_KEY_DOWN || e.key.repeat) return true;
    const auto key=e.key.scancode;
    if (key==SDL_SCANCODE_ESCAPE) { if(replacement_) replacement_.reset(); else open_=false; return true; }
    if (replacement_) {
        if (key==SDL_SCANCODE_UP) slot_=(slot_+2)%3;
        if (key==SDL_SCANCODE_DOWN) slot_=(slot_+1)%3;
        if (key==SDL_SCANCODE_RETURN) { inventory.replaceCard(*replacement_,slot_); replacement_.reset(); notice_="TARJETA INTERCAMBIADA"; }
        return true;
    }
    if (key==SDL_SCANCODE_LEFT || key==SDL_SCANCODE_RIGHT) { category_=(category_+(key==SDL_SCANCODE_RIGHT?1:5))%6; row_=0; notice_.clear(); }
    auto& list=entries_[category_];
    if(list.empty()) return true;
    if(key==SDL_SCANCODE_UP) row_=(row_+list.size()-1)%list.size();
    if(key==SDL_SCANCODE_DOWN) row_=(row_+1)%list.size();
    if(key==SDL_SCANCODE_RETURN) {
        auto c=list[row_].contents;
        if(category_==0) { travel_=list[row_].travel; notice_="DESTINO SOLICITADO"; }
        if(category_==2) enemies_=!enemies_;
        if(category_==1 || category_==3 || category_==4) {
            const auto result=category_==1?inventory.exchangeWeapon(c):inventory.takeSupply(c);
            notice_=result==player::Transfer::Capacity?"INVENTARIO LLENO":"TRANSFERIDO";
            if(category_==4 && result==player::Transfer::Capacity) { replacement_=c; slot_=0; }
        }
    }
    return true;
}
std::optional<ItemSession::Travel> DebugMenu::takeTravel() { return std::exchange(travel_,{}); }
bool DebugMenu::draw(SDL_Renderer* renderer, unsigned zone) {
    refresh(zone);
    if(!open_) return true;
    int w,h; SDL_GetRenderOutputSize(renderer,&w,&h);
    SDL_SetRenderScale(renderer,float(w)/800,float(h)/450);
    SDL_SetRenderDrawColor(renderer,12,18,26,255); SDL_RenderFillRect(renderer,nullptr);
    SDL_SetRenderDrawColor(renderer,180,210,170,255);
    SDL_RenderDebugTextFormat(renderer,16,16,"DEBUG  NIVEL %u  ENEMIGOS %s",zone+1,enemies_?"SI":"NO");

    // Display the current level's locker access code in the debug header.
    if (zone < levels_.size() && zone < interaction::original::codes.size()) {
        const auto& level = levels_[zone];
        const std::string code(interaction::original::codes[zone].letters);

        if (level.accessCode && interaction::codeChecksum(code) == level.accessCode)
            SDL_RenderDebugTextFormat(renderer,16,28,"CLAVE TAQUILLA: %s",code.c_str());
        else
            SDL_RenderDebugText(renderer,16,28,"CLAVE TAQUILLA: SIN VERIFICAR");
    }
    constexpr const char* names[]={"NIVELES","ARMAS","ENEMIGOS","OBJETOS","TARJETAS","CLAVES"};
    for(unsigned c=0;c<6;++c) SDL_RenderDebugTextFormat(renderer,16+c*130,44,"%c%s",c==category_?'>':' ',names[c]);
    const auto& list=entries_[category_];
    const unsigned start=row_/19*19;
    for(unsigned i=start;i<list.size() && i<start+19;++i)
        SDL_RenderDebugTextFormat(renderer,24,72+(i-start)*16,"%c %s",i==row_?'>':' ',list[i].label.c_str());
    if(list.empty()) SDL_RenderDebugText(renderer,24,72,"SIN ENTRADAS ORIGINALES");
    SDL_RenderDebugText(renderer,16,388,notice_.c_str());
    if(replacement_) SDL_RenderDebugTextFormat(renderer,16,406,"CAMBIAR %u: %s POR %s (ARRIBA/ABAJO, ENTER, ESC)",slot_+1,interaction::cardName(cards_[slot_]).c_str(),interaction::cardName(*replacement_-0x3000).c_str());
    else SDL_RenderDebugText(renderer,16,406,"FLECHAS: CATEGORIA/FILA  ENTER: APLICAR  TAB/ESC: CERRAR");
    SDL_RenderDebugTextFormat(renderer,16,428,"FILA %u/%zu - S/P: SECTOR/PARED (BASE 0)",list.empty()?0:row_+1,list.size());
    return SDL_SetRenderScale(renderer,1,1);
}
