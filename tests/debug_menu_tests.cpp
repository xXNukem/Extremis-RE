#include "debug_menu.h"
#include "original_travel.h"
#include <iostream>
#include <stdexcept>
#include <set>
struct DebugMenuChecks {
    static void require(bool value) { if(!value) throw std::runtime_error("Debug menu check failed"); }
    static void run(const char* path) {
        DebugMenu menu; menu.load(path);
        require(menu.levels_.size()==30);
        std::set<unsigned> zones,weapons;
        for(const auto& entry:menu.entries_[0]) zones.insert(entry.travel->zone);
        for(const auto& entry:menu.entries_[1]) weapons.insert(entry.contents>>12);
        require(zones.size()==30 && weapons.size()==6);
        player::Inventory inventory;
        auto key=[&](SDL_Scancode code) { SDL_Event e{}; e.type=SDL_EVENT_KEY_DOWN; e.key.scancode=code; menu.event(e,inventory,menu.zone_==999?0:menu.zone_); };
        key(SDL_SCANCODE_TAB); require(menu.open());
        for(unsigned i=0;i<menu.entries_[0].size();++i) {
            menu.row_=i; key(SDL_SCANCODE_RETURN);
            auto travel=menu.takeTravel(); require(travel.has_value());
            const auto& expected=*menu.entries_[0][i].travel;
            require(travel->zone==expected.zone && travel->arrival.sector==expected.arrival.sector);
        }
        unsigned codes=0, pending=0;
        for(unsigned z=0;z<30;++z) {
            menu.refresh(z);
            for(const auto& e:menu.entries_[5]) { ++codes; pending+=e.label.find("PENDIENTE")!=std::string::npos; }
            require(interaction::codeChecksum(interaction::original::codes[z].letters)==menu.levels_[z].accessCode);
        }
        menu.refresh(0); require(!menu.entries_[4].empty());
        for(std::uint16_t c: {0x3001,0x4001,0x5001}) inventory.takeSupply(c);
        const auto before=inventory.cards();
        menu.category_=4; menu.row_=0; key(SDL_SCANCODE_RETURN); require(menu.replacement_.has_value());
        key(SDL_SCANCODE_DOWN); key(SDL_SCANCODE_RETURN);
        require(!menu.replacement_ && inventory.cards()[0]==before[0] && inventory.cards()[2]==before[2]);
        require(inventory.cards()[1]==menu.entries_[4][0].contents-0x3000);
        menu.category_=2; menu.row_=0; key(SDL_SCANCODE_RETURN); require(!menu.enemiesEnabled());
        key(SDL_SCANCODE_RETURN); require(menu.enemiesEnabled());
        require(SDL_Init(SDL_INIT_VIDEO));
        auto* surface=SDL_CreateSurface(960,600,SDL_PIXELFORMAT_RGBA8888);
        auto* renderer=SDL_CreateSoftwareRenderer(surface); require(renderer);
        for(unsigned c=0;c<6;++c) { menu.category_=c; menu.row_=0; require(menu.draw(renderer,0)); }
        SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface); SDL_Quit();
        key(SDL_SCANCODE_TAB); require(!menu.open());
        std::cout<<"30 levels, "<<menu.entries_[0].size()<<" arrivals, 6 weapons, "<<codes<<" code rows, "<<pending<<" pending; replacement and six category rendering OK\n";
    }
};
int main(int argc,char** argv) { try { if(argc!=2) return 2; DebugMenuChecks::run(argv[1]); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
