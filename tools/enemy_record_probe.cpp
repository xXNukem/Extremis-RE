// Deterministic per-record comparison using the production parser, AI and CPU rasterizer.
#include "extremis/gameplay/enemies/enemy_system.h"
#include "extremis/world/door_control.h"
#include "extremis/world/original_travel.h"
#include "extremis/render/first_person/camera/camera_start.h"
#include "ray_scene.h"
#include "vortex/render/first_person/sprites/sprite_renderer.h"

#include <SDL3/SDL_main.h> // Match the game's UTF-8 argument handling on Windows.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {
namespace cpu = vortex::first_person;

void snapshot(const game_lab::Level& level, const enemies::System& actors,
              const sprites::Scene& objects, const std::vector<doors::Door>& doors,
              const Camera& camera)
{
    constexpr int width = 320, height = 150;
    const double plane = std::tan(Camera::horizontalFov / 2);
    const double focal = width / (2 * plane);
    const auto scene = extremis::render_detail::prepareRayScene(level, camera, &doors);
    const cpu::View view{camera.renderPose(), {width,height,std::cos(camera.angle),
        std::sin(camera.angle),plane,focal,camera.viewHeight()},256,focal,scene};
    const auto& mons = actors.scene();
    cpu::SpriteLayer layers[]{{&objects.bank,&objects.objects},{&mons.bank,&mons.objects}};
    cpu::SceneStyle style;
    style.layers = layers;
    style.layerCount = 2;
    cpu::RenderBuffers buffers;
    std::vector<std::uint32_t> full, without;
    if (!cpu::render(view,style,full,buffers)) throw std::runtime_error("World render failed");
    style.layerCount = 1;
    if (!cpu::render(view,style,without,buffers)) throw std::runtime_error("Background render failed");
    const auto backgroundDepth = buffers.depth; // Walls, doors and OBJET layer.
    style.layerCount = 2;
    const auto& ids = actors.sceneRecordIds();
    if (ids.size()!=mons.objects.size() || actors.records().size()!=level.enemies.size() ||
        !std::is_sorted(ids.begin(),ids.end()))
        throw std::runtime_error("Scene/record mapping mismatch");
    const auto connected = doors::connectedSectors(level,camera.sector.value());
    for (std::size_t i=0; i<actors.records().size(); ++i) {
        const auto& e = actors.records()[i];
        const auto pos = std::lower_bound(ids.begin(),ids.end(),i);
        const bool present = pos!=ids.end() && *pos==i;
        std::string reason;
        unsigned changed = 0;
        if (!present) reason = e.health==0xffff ? "removed-sentinel" : "missing-scene-entry";
        else {
            const auto index = pos-ids.begin();
            const auto& obj = mons.objects[index];
            const auto rect = vortex::sprites::project(obj,mons.bank.images.at(obj.image),
                view.camera,{width,height,focal,camera.viewHeight()});
            if (!rect) reason = "projection-rejected";
            else if (rect->left>=width || rect->top>=height ||
                     rect->left+rect->width<=0 || rect->top+rect->height<=0)
                reason = "outside-view";
            else {
                auto others = mons.objects;
                others.erase(others.begin()+index);
                layers[1].objects = &others;
                if (!cpu::render(view,style,without,buffers)) throw std::runtime_error("Comparison render failed");
                layers[1].objects = &mons.objects;
                for (std::size_t p=0; p<full.size(); ++p) changed += full[p]!=without[p];
                reason = "visible";
                if (!changed) {
                    // Use depth writes to distinguish actual coverage from identical colors.
                    const auto draws = [&](std::vector<double> depth) {
                        const auto before = depth;
                        vortex::sprites::render(without,depth,mons.bank,{obj},view.camera,
                                                {width,height,focal,camera.viewHeight()});
                        return depth!=before;
                    };
                    if (draws(backgroundDepth)) reason = "enemy-overlap-or-identical-color";
                    else if (draws(std::vector<double>(full.size(),std::numeric_limits<double>::infinity())))
                        reason = "occluded-by-world-or-objet";
                    else reason = "no-opaque-sample";
                }
            }
        }
        std::cout << level.zoneIndex+1 << ',' << i << ',' << actors.simulationTicks() << ','
                  << level.monsBank << ',' << e.sector << ',' << e.x << ',' << e.y << ','
                  << e.health << ',' << e.behavior << ',' << e.state << ',' << e.phase << ','
                  << e.sprite << ',' << e.polygon << ',' << e.elevation << ',' << e.angle << ','
                  << e.reserved << ',' << e.homeX << ',' << e.homeY << ',' << connected.at(e.sector) << ',' << present << ','
                  << reason << ',' << changed << '\n';
        if (present!=(e.health!=0xffff)) throw std::runtime_error("Health/scene membership mismatch");
    }
}
}

int main(int argc, char** argv)
{
    if (argc!=2 && argc!=3 && argc!=7) {
        std::cerr << "enemy_record_probe GAME.LAB [--near-first | LEVEL [SECTOR LOCAL_X LOCAL_Y ANGLE_720]]\n";
        return 2;
    }
    try {
        const auto lab = std::filesystem::u8path(argv[1]);
        const auto data = lab.parent_path()/"DATA";
        const auto levels = game_lab::loadBankTable(lab);
        const bool nearFirst = argc==3 && std::string_view(argv[2])=="--near-first";
        const bool oneLevel = argc>=3 && !nearFirst;
        const auto first = oneLevel ? std::stoul(argv[2])-1 : 0;
        if (first>=levels.size()) throw std::runtime_error("Invalid level");
        std::cout << "level,record,tick,bank,sector,x,y,health,behavior,state,phase,sprite,polygon,elevation,angle,reserved,home_x,home_y,connected,instantiated,result,pixels\n";
        for (std::size_t z=first; z<(oneLevel ? first+1 : levels.size()); ++z) {
            auto level = game_lab::loadLevel(lab,static_cast<unsigned>(z)).level;
            if (argc==7) {
                auto word = [&](int arg, unsigned max) {
                    auto n = std::stoul(argv[arg]);
                    if (n>max) throw std::runtime_error("Camera argument out of range");
                    return static_cast<std::uint16_t>(n);
                };
                level.playerStart = game_lab::PlayerStart{word(3,65535),word(4,65535),word(5,65535),word(6,719)};
            } else if (nearFirst) {
                const auto& e = level.enemies.at(0);
                // Fixed comparison pose. A wall between the camera and the record is valid.
                const int x = e.x ? std::max(0,int(e.x)-150) : 150;
                level.playerStart = game_lab::PlayerStart{e.sector,std::uint16_t(x),e.y,
                    std::uint16_t(e.x ? 0 : 360)};
            } else {
                for (const auto& lift : interaction::original::lifts)
                    for (const auto& r : lift.routes)
                        if (!level.playerStart && r.exists && r.zone==z)
                            level.playerStart = game_lab::PlayerStart{r.sector,lift.x,lift.y,lift.angle};
            }
            const auto camera = initialCamera(level);
            enemies::System actors;
            actors.load(level,data);
            const auto objects = sprites::loadScene(level,data);
            const auto doors = doors::closedDoors(level);
            const auto world = collision::staticWorld(level);
            DoorControl control(world,doors,level);
            player::Vitals vitals;
            snapshot(level,actors,objects,doors,camera);
            actors.update(level,camera,control.movementWorld(),vitals,0,true);
            for (int i=1; i<=120; ++i) {
                actors.update(level,camera,control.movementWorld(),vitals,1./18,true,{},{},doors,&objects);
                if (i==18 || i==120) snapshot(level,actors,objects,doors,camera);
            }
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
