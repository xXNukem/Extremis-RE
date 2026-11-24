#include "extremis/ui/ambient_effects.h"
#include "extremis/resources/game_lab.h"
#include "extremis/ui/frame_composition.h"
#include "extremis/ui/game_messages.h"
#include "extremis/audio/game_audio.h"
#include "extremis/gameplay/player/survival_session.h"

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
}

int main(int argc, char** argv)
{
    try {
        game_lab::Level level{};
        level.objetBank = 1;
        level.sectors.resize(1);
        auto& sector = level.sectors.front();
        sector.gx = 1000;
        sector.vertices = {{100, 300}, {100, 500}};
        sector.entitiesA = {{0, 0x4002, 256, 0}, {1, 0x4003, 256, 0}};
        auto lamp = extremis::lampTint(level, 0, 1100, 440);
        check(lamp.percent == 10 && lamp.dac[0] == 63, "First qualifying lamp must win");
        lamp = extremis::lampTint(level, 0, 1100, 460);
        check(lamp.percent == 60 && lamp.dac[1] == 63, "160-unit cutoff or green lamp failed");
        check(extremis::lampTint(level, 0, 1259, 300).percent == 1, "159-unit boundary");
        check(!extremis::lampTint(level, 0, 1260, 300).percent, "Outside lamp box");
        sector.entitiesA[0].type |= 0x1000;
        check(extremis::lampTint(level, 0, 1100, 440).dac[1] == 63, "Physical entity selected");
        check(!extremis::lampTint(level, 1, 1100, 440).percent, "Invalid sector selected");

        extremis::PowerSurge surge;
        check(!surge.update(9600) && !surge.tint().percent, "Surge started at zero");
        check(surge.update(1), "Surge failed to start");
        check(surge.tint().percent >= 30 && surge.tint().percent <= 93, "Surge intensity range");
        const auto mixed = extremis::ambientTint(lamp, surge.tint());
        check(mixed.dac[1] == 0 && mixed.percent == surge.tint().percent, "Surge lost priority");
        const auto lightsOff = extremis::lampTint(level, 0, 1100, 460, false);
        check(!lightsOff.percent && extremis::ambientTint(lightsOff,surge.tint(),true).optics &&
              extremis::ambientTint(lightsOff,surge.tint()).percent == mixed.percent,
              "Colored Lights OFF changed optics or a power surge");
        check(surge.update(639, true) && !surge.tint().percent, "Overridden surge lost its cue");
        check(surge.update(1), "Last active surge update lost");
        check(!surge.update(0) && !surge.tint().percent, "Rearm left the old tint");
        check(extremis::ambientTint(lamp, surge.tint()).dac[1] == 63, "Lamp failed to restore");

        messages::Queue queue;
        queue.post(messages::Id::PowerSurge);
        queue.post(messages::Id::PowerSurge);
        check(queue.visible()[0] == "ATTENTION\r\n POWER IS SURGING!!!" &&
              queue.visible()[1].empty(), "Warning text or duplicate suppression");

        std::vector<std::uint32_t> world(48000, 0);
        std::vector<std::uint32_t> hud(64000, 0x000000ff), wide;
        hud.front() = 0xffffffff;
        const auto composed = extremis::composeFrame(world,hud,wide,320,150,true,false,nullptr);
        std::vector<std::uint32_t> output(hud.size());
        extremis::tintFrame(composed.pixels,output.data(),output.size(),{80,{63,0,0}});
        check(output.front() == 0xfb3131ff && output.back() == 0xca0000ff, "World/HUD tint or DAC arithmetic");
        check(hud.front() == 0xffffffff && hud.back() == 0x000000ff, "Base framebuffer changed");
        extremis::tintFrame(composed.pixels,output.data(),output.size(),{});
        check(output == hud, "Untinted framebuffer not restored");

        const auto optics=extremis::ambientTint(lamp,{},true);
        check(optics.optics && !optics.percent, "Optics did not bypass the lamp");
        hud.back() = 0x704240ff;
        const auto optComposed=extremis::composeFrame(world,hud,wide,320,150,true,false,nullptr);
        extremis::tintFrame(optComposed.pixels,output.data(),output.size(),optics);
        check(output.front()==0x00ff00ff && output.back()==0x008200ff,
              "Optics did not cover both world and HUD");
        for (unsigned green=0;green<=63;++green) {
            const unsigned rgb=(green*255+31)/63;
            const std::uint32_t source=0xf000c055 | (rgb<<16);
            std::uint32_t destination{};
            extremis::tintFrame(&source,&destination,1,optics);
            const auto expected=(std::min(63u,green*2)*255+31)/63;
            check(destination==((expected<<16)|0x55), "Wrong six-bit optical mapping, clipping or alpha");
        }
        const auto darkOptics=extremis::ambientTint(lamp,{93,{0,0,0}},true);
        const std::uint32_t saturated=0x82b0abff;
        std::uint32_t destination{};
        extremis::tintFrame(&saturated,&destination,1,darkOptics);
        check(destination==0x001000ff,"Surge did not darken the converted palette");
        SurvivalSession session;
        GameAudio audio;
        std::uint16_t battery=1;
        session.inventory().takeSupply(battery);
        session.inventory().setBatteryMode(player::BatteryMode::Optics);
        session.update(10,false,audio,false,false,160);
        check(session.inventory().batteryCharge()==160,"A modal or inactive frame drained the battery");
        session.update(.005,true,audio,false,false,159);
        check(session.inventory().batteryCharge()==1 && session.hudState().batteryInUse &&
              session.hudState().messages[0]==messages::text(messages::Id::BatteryLow),
              "Active optics did not drain on the game clock and post its warning");
        session.update(.005,true,audio,false,false,1);
        const auto expired=session.hudState();
        check(expired.batteryCharge==0 && !expired.batteryInUse &&
              expired.messages[1]==messages::text(messages::Id::OpticsExpired),"Battery exhaustion did not update the HUD");
        const auto baseAfterCutoff=extremis::ambientTint(lamp,{93,{0,0,0}},
            session.inventory().batteryMode()==player::BatteryMode::Optics);
        extremis::tintFrame(&saturated,&destination,1,baseAfterCutoff);
        check(destination==0x080c08ff,"The normal palette did not return under a surge on the cutoff frame");
        extremis::tintFrame(&saturated,&destination,1,extremis::ambientTint({}, {}, false));
        check(destination==saturated,"Optical or surge conversion accumulated across frames");

        if (argc > 1) {
            unsigned red = 0, green = 0, sectors = 0;
            for (unsigned zone = 0; zone < 30; ++zone) {
                const auto original = game_lab::loadLevel(argv[1],zone).level;
                if (original.objetBank != 1) continue;
                for (unsigned index = 0; index < original.sectors.size(); ++index) {
                    const auto& originalSector = original.sectors[index];
                    bool hasLamp = false;
                    for (const auto& entity : originalSector.entitiesA) {
                        const auto image = entity.type & 255u;
                        if ((entity.type & 0x3000) || (image != 2 && image != 3)) continue;
                        const auto vertex = originalSector.vertices.at(entity.vertex);
                        const auto tint = extremis::lampTint(original,index,
                            originalSector.gx + vertex.x,originalSector.gy + vertex.y);
                        check(tint.percent == 80 && tint.dac[image-2] == 63,
                              "Original lamp center or color mismatch");
                        check(!extremis::lampTint(original,index,originalSector.gx + vertex.x,
                                                  originalSector.gy + vertex.y,false).percent,
                              "Colored Lights OFF did not suppress an original lamp");
                        if (image == 2) ++red;
                        else ++green;
                        hasLamp = true;
                    }
                    sectors += hasLamp;
                }
            }
            check(red == 15 && green == 15 && sectors == 17, "Original lamp catalog mismatch");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
