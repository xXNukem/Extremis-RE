#include <string_view>
#include <cstddef>
#include "app/startup/port_menu_state.h"
#include "extremis/gameplay/player/continuation.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::array<std::string_view,16> passwords{{
    "ATILAHUN","ZENDOR56","STRADIUS","TERRAC12","CARTOON0","GOREXZ73","ALTEA117","RAMDAC38",
    "EPSILON4","RUBISCOP","NEAREND1","CENTRAL7","ARKETYPE","PRACTIX4","ANTARES5","CLEENEND"
}};
constexpr std::array<unsigned,16> levels{{3,5,6,8,10,11,12,15,18,21,22,23,26,27,29,30}};

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void openLoadGame(PortMenuState& menu)
{
    while (menu.selection() != 1) menu.key(SDL_SCANCODE_DOWN);
    menu.key(SDL_SCANCODE_RETURN);
    check(menu.page()==PortMenuPage::Password && menu.label(0)=="........","Load Game did not open an empty editor");
}

void editor()
{
    UserSettings settings;
    PortMenuState menu(false,settings);
    openLoadGame(menu);
    menu.typePassword("ca@rt");
    menu.key(SDL_SCANCODE_RETURN);
    check(!menu.passwordSubmitted() && menu.label(0)=="CART....","Incomplete Enter or normalization");
    menu.key(SDL_SCANCODE_BACKSPACE);
    check(menu.label(0)=="CAR.....","Backspace did not remove the last character");
    menu.typePassword("toon0ignored");
    check(menu.label(0)=="CARTOON0","Editor did not cap input at eight characters");
    menu.key(SDL_SCANCODE_RETURN);
    check(menu.passwordSubmitted() && menu.label(1)=="CODE ACCEPTED","Valid code not accepted");
    check(menu.finishPasswordResponse()==continuation::find("CARTOON0"),"Menu did not return the selected checkpoint");
    openLoadGame(menu);
    menu.typePassword("wrong000");
    menu.key(SDL_SCANCODE_RETURN);
    check(menu.passwordSubmitted() && menu.label(1)=="CODE REFUSED","Incorrect code not refused");
    check(!menu.finishPasswordResponse() && menu.page()==PortMenuPage::Main,"Refusal did not return to main menu");
    check(menu.key(SDL_SCANCODE_RETURN)==PortMenuChoice::NewGame,"New Game inaccessible after refusal");
    PortMenuState pause(true,settings);
    pause.key(SDL_SCANCODE_DOWN);
    pause.key(SDL_SCANCODE_DOWN);
    pause.key(SDL_SCANCODE_RETURN);
    pause.typePassword("ZENDOR56");
    pause.key(SDL_SCANCODE_RETURN);
    check(pause.finishPasswordResponse()==continuation::find("ZENDOR56"),"Pause-menu continuation not accepted");
    check(pause.key(SDL_SCANCODE_RETURN)==PortMenuChoice::Resume,"Resume should remain independent");
    for (const auto password:passwords) {
        openLoadGame(menu);
        menu.typePassword(password);
        menu.key(SDL_SCANCODE_RETURN);
        check(menu.passwordSubmitted() && menu.finishPasswordResponse()==continuation::find(password),
              "One of the sixteen stock codes failed in the menu");
    }
}

void checkpoints()
{
    unsigned count=0;
    for (unsigned i=0;i<passwords.size();++i) {
        const auto* checkpoint=continuation::find(passwords[i]);
        check(checkpoint && checkpoint->zone+1==levels[i],"Missing or misordered checkpoint");
        player::Inventory inventory;
        std::uint16_t battery=900;
        inventory.takeSupply(battery);
        inventory.setBatteryMode(player::BatteryMode::Optics);
        inventory.startWith(checkpoint->inventory);
        player::Vitals vitals;
        vitals.applyDamage(1000);
        vitals.startWith(checkpoint->health,checkpoint->oxygen);
        const auto& loadout=checkpoint->inventory;
        check(vitals.alive() && vitals.health()==checkpoint->health &&
              vitals.oxygen()==checkpoint->oxygen && inventory.cards()==loadout.cards &&
              inventory.injectors()==loadout.injectors && inventory.batteryCharge()==loadout.battery &&
              inventory.oxygenReserve()==loadout.oxygenReserve && inventory.mine()==loadout.mine &&
              inventory.weapon()==loadout.weapon && inventory.ammunition()==loadout.ammunition &&
              inventory.batteryMode()==player::BatteryMode::Off,"Checkpoint values were not applied literally");
        for (unsigned sector=0;sector<64;++sector) {
            continuation::Disclosures freshProcess;
            if (const auto* shown=freshProcess.arrive(checkpoint->zone,sector)) {
                ++count;
                check(shown==checkpoint && !freshProcess.arrive(checkpoint->zone,sector),"Disclosure repeated");
                check(continuation::find(passwords[i])==checkpoint,"Disclosure invalidated reusable code");
            }
        }
    }
    check(count==29,"Stock campaign must have 29 disclosure destinations");
    continuation::Disclosures process;
    const auto* stradius=continuation::find("stradius");
    check(!process.arrive(5,1) && process.arrive(5,0)==stradius &&
          !process.arrive(5,10),"Unused arrival should select by zone and suppress other sectors");
    const auto* central=continuation::find("CENTRAL7");
    check(process.arrive(22,1)==central,"Successful shuttle destination did not disclose CENTRAL7");
    continuation::Disclosures continued;
    continued.consume(*central);
    check(!continued.arrive(22,1) && !continued.arrive(22,2) &&
          continuation::find("CENTRAL7")==central,"Accepted code did not consume its disclosure only");
    check(!continuation::find("ATILA") && !continuation::find("ATILA!UN") &&
          !continuation::find("ATILAHUN0"),"Invalid lookup accepted");
}

void verifyOriginal(const char* executable, const char* lab)
{
    std::ifstream stream(executable,std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
    const auto word=[&](std::size_t p) { return int(bytes.at(p)) | int(bytes.at(p+1))<<8; };
    const auto dword=[&](std::size_t p) {
        const std::uint32_t value=std::uint32_t(word(p)) | std::uint32_t(word(p+2))<<16;
        return value==0xffffffffu ? -1 : int(value);
    };
    const auto slot=[](int value){return value==0xffff ? -1 : value;};
    check(word(0x1d57e)==16,"Original code count differs");
    for (unsigned i=0;i<passwords.size();++i) {
        const auto* cp=continuation::find(passwords[i]);
        constexpr std::string_view mask="BLUE1993";
        for (unsigned j=0;j<8;++j)
            check((bytes.at(0x1d580+8*i+j)^mask[j])==cp->code[j],"Original password differs");
        const std::size_t p=0x1d600+42*i;
        const auto& s=cp->start;
        const auto& l=cp->inventory;
        check(word(p)==s.x && word(p+2)==s.y && word(p+4)==s.angle &&
              word(p+6)==s.sector && word(p+8)==int(cp->zone) && word(p+10)==int(cp->floor) &&
              word(0x1d55e + 2*i)==int(cp->zone),"Original start or disclosure zone differs");
        for (unsigned j=0;j<l.cards.size();++j)
            check(slot(word(p+12+2*j))==l.cards[j],"Original card differs");
        for (unsigned j=0;j<l.injectors.size();++j)
            check(slot(word(p+18+2*j))==l.injectors[j],"Original injector differs");
        check(slot(word(p+22))==l.mine && word(p+24)==l.ammunition &&
              word(p+26)==int(l.weapon) && word(p+28)==cp->health &&
              dword(p+30)==l.battery && dword(p+34)==l.oxygenReserve && dword(p+38)==cp->oxygen,
              "Original equipment or vitals differ");
        const auto level=game_lab::loadLevel(lab,cp->zone);
        check(s.sector<level.level.sectors.size(),"Original destination sector missing");
        std::bitset<64> destinations;
        if (cp->zone==22) destinations.set(1); // Successful shuttle handoff.
        for (unsigned lift=0;lift<25;++lift) {
            const auto descriptor=0xdacf + 54*lift;
            unsigned destinationsOnLift=0;
            for (unsigned row=0;row<4;++row) destinationsOnLift+=bytes.at(descriptor+4*row)!=0;
            if (destinationsOnLift<2) continue;
            for (unsigned row=0;row<4;++row) {
                const auto route=descriptor+4*row;
                if (bytes.at(route) && bytes.at(route+2)==cp->zone)
                    destinations.set(bytes.at(route+1));
            }
        }
        for (unsigned sector=0;sector<destinations.size();++sector) {
            continuation::Disclosures history;
            check(bool(history.arrive(cp->zone,sector))==destinations.test(sector),
                  "Original disclosure sector differs");
        }
    }
}
}

int main(int argc, char** argv)
{
    try {
        editor();
        checkpoints();
        if (argc==3) verifyOriginal(argv[1],argv[2]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
