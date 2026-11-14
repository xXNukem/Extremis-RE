#include "enemy_system.h"
#include "enemy_profiles.h"
#include "enemy_navigation.h"
#include "objet_collision.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace enemies {
namespace {
constexpr double pi=3.14159265358979323846;
// Port normalization at the movement reference rate used in EXE 0xe02c.
// DOS animation counters were iteration-based, not an original fixed 18 Hz clock.
constexpr double stepSeconds=1.0/18;
constexpr unsigned waiting[]{4,4,4,4,5,5,5,5,4,4,4,4,5,5,5,5,
                             4,4,4,4,5,5,5,5,5,6,8,8,7,6,5,4};
constexpr unsigned attack[]{5,6,8,8,7,6,5};
constexpr unsigned damage[]{1,1,1,2,5,4};
void frame(game_lab::EnemyRecord& enemy, unsigned image)
{
    enemy.sprite=std::uint16_t(0x8000|(enemy.sprite&0x2000)|image);
}
std::vector<std::uint8_t> read(const std::filesystem::path& path)
{
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file || file.tellg()<0 || file.tellg()>64*1024*1024)
        throw std::runtime_error("Cannot read monster resource: "+path.string());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))
        throw std::runtime_error("Truncated monster resource");
    return bytes;
}

}
void System::load(const game_lab::Level& level, const std::filesystem::path& directory)
{
    if(level.monsBank<1 || level.monsBank>original::profiles.size())
        throw std::runtime_error("Unsupported monster bank ID");
    const auto& profile=original::profiles[level.monsBank-1];
    const std::string name = "MONS" + std::to_string(level.monsBank);

    auto bank = sprites::decode(
        read(directory / (name + ".SPR")),
        wall_textures::loadPalette(
            directory / ("PAL" + std::to_string(level.palBank) + ".PAL"))
    );

    auto masks = read(directory / (name + ".SPT"));

    if (bank.images.empty() ||
        masks.size() != bank.images.size() * 8 ||
        bank.images[0].texture.height == 0)
    {
        throw std::runtime_error("Invalid monster bank: " + name);
    }
    if (bank.images.size() != profile.frames)
        throw std::runtime_error("Invalid native monster action/effect image count");
    std::vector<collision::Point> origins;
    for (const auto& enemy:level.enemies) {
        const auto& sector=level.sectors.at(enemy.sector);
        if ((enemy.sprite&255)>=bank.images.size()) throw std::runtime_error("Invalid monster frame: " + name);
        origins.push_back({double(sector.gx),double(sector.gy)});
    }
    bank_=level.monsBank; terminalDefeated_=false;
    directionalCycle_ = bank_!=1; // Preserve the established MONS1 presentation.
    baseHeight_=profile.height; speed_=profile.speed; baseElevation_=profile.elevation;
    recoil5_.assign(level.enemies.size(),0);
    records_=level.enemies; origins_=std::move(origins); masks_=std::move(masks);
    scene_.bank=std::move(bank); accumulator_=0; reaction_=attackSound_=ticks_=0; wasActive_=false;
    sounds_.clear(); effects_.reset(); rebuildScene();
}
void System::rebuildScene()
{
    scene_.objects.clear();
    for (std::size_t i=0;i<records_.size();++i) {
        const auto& e=records_[i];
        if (e.health==0xffff) continue;
        const auto image=std::size_t(e.sprite&255);
        if (image >= scene_.bank.images.size()) throw std::runtime_error(
            "Invalid monster scene frame: MONS" + std::to_string(bank_) +
            " enemy=" + std::to_string(i) +
            " image=" + std::to_string(image) +
            " total=" + std::to_string(scene_.bank.images.size()) +
            " state=" + std::to_string(e.state) +
            " phase=" + std::to_string(e.phase) +
            " sprite=" + std::to_string(e.sprite));
        const double height=baseHeight_*scene_.bank.images[image].texture.height/scene_.bank.images[0].texture.height;
        scene_.objects.push_back({origins_[i].x+e.x,origins_[i].y+e.y,baseElevation_+double(e.elevation&255),
            height,image,std::uint16_t((e.sprite&0x2000)?0x0200:0),std::uint16_t(i)});
    }
}
void System::update(const game_lab::Level& level, const Camera& camera, const collision::World& walls,
                    player::Vitals& vitals, double seconds, bool active,
                    const std::function<void(double)>& movePlayer,
                    const std::function<void()>& recoilPlayer,
                    const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets)
{
    if (!active || !vitals.alive() || !std::isfinite(seconds) || seconds<0) {
        wasActive_=false; accumulator_=0; return;
    }
    if (!wasActive_) { wasActive_=true; return; } // Discard modal/focus resume interval.
    double remaining=std::min(seconds,0.25);
    while (remaining>1e-12 && vitals.alive()) {
        const double interval=std::min(remaining,stepSeconds-accumulator_);
        // Movement observes reaction_ until the exact tick that expires it.
        if (movePlayer) movePlayer(interval);
        accumulator_+=interval; remaining-=interval;
        if (accumulator_+1e-12>=stepSeconds) {
            accumulator_=0; step(level,camera,walls,vitals,recoilPlayer,dynamicDoors,objets);
        }
    }
    rebuildScene(); effects_.compose(camera,scene_.bank);
}
void System::step(const game_lab::Level& level, const Camera& camera, const collision::World& walls,
                  player::Vitals& vitals, const std::function<void()>& recoilPlayer,
                    const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets)
{
    ++ticks_; effects_.step(camera,walls);
    const auto location=collision::locate(level,{camera.x,camera.y},camera.sector);
    const auto visible=doors::connectedSectors(level,camera.sector.value_or(location ? location->sector : level.sectors.size()));
    for (std::size_t i=0;i<records_.size();++i) {
        auto& e=records_[i];
        if (e.health==0xffff) continue;
        if (e.behavior==2 && e.sector<visible.size() && visible[e.sector]) e.behavior=3;
        if (bank_==5) { advance5(i,level,camera,walls,location,objets); continue; }
        if (e.state==5) {
            frame(e,(e.phase&1)?9:5);
            if (++e.phase>=8) {
                e.health=0xffff;
                if (bank_==9) terminalDefeated_=true; // EXE 0xe25e: ending, no explosion.
                else {
                    sounds_.push_back(17);
                    effects_.begin({origins_[i].x+e.x,origins_[i].y+e.y},
                        baseElevation_+(e.elevation&255),camera,baseHeight_,true,bank_);
                }
            }
        } else if (e.state==4 && e.phase<(bank_==6?3u:1u)) {
            ++e.phase; frame(e,(e.phase&1)?9:5);
        }
        else if (e.state==2) {
            if (++e.phase>=7) {
                e.phase=0; e.state=0; advance(i,level,camera,walls,location,objets);
            }
            else {
                frame(e,bank_==6?original::attack6[e.phase]:attack[e.phase]);
                const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
                const double dx=camera.x-p.x,dy=camera.y-p.y,d=std::hypot(dx,dy);
                if (e.phase==2 && (bank_!=6 || !reaction_) && location && location->sector==e.sector &&
                    (bank_==6 || d<1e-6 || !collision::rayDistance(walls,p,{dx/d,dy/d},d))) {
                    reaction_=8; sounds_.push_back(9+(attackSound_++&1));
                }
            }
        } else {
            if (e.state==4) { e.state=0; e.phase=0; }
            advance(i,level,camera,walls,location,objets);
        }
    }
    // EXE 0x165db..0x166b5: one shared reaction, 2 health units × 8 updates.
    if (reaction_) {
        if (recoilPlayer) {
            rebuildScene(); // Enemy collision boxes must use this tick's positions.
            recoilPlayer();
        }
        vitals.applyDamage(2); --reaction_;
    }
}
void System::advance(std::size_t i, const game_lab::Level& level, const Camera& camera,
                     const collision::World& walls, std::optional<collision::Location> playerLocation,
                     const sprites::Scene* objets)
{
    auto& e=records_[i];
    const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
    if (bank_==6) {
        // EXE 0xe90b..0xe9f0: stationary, same-sector Chebyshev proximity.
        if (playerLocation && playerLocation->sector==e.sector &&
            std::max(std::abs(camera.x-p.x),std::abs(camera.y-p.y))<original::profiles[5].reach) {
            e.state=2; e.phase=1; frame(e,original::attack6[1]);
        } else {
            e.state=6; e.phase=(e.phase+1)&31; frame(e,original::waiting6[e.phase]);
        }
        return;
    }
    auto location=collision::locate(level,p,e.sector);
    if (!location) return;
    e.polygon=std::uint16_t(location->polygon);
    std::optional<collision::Point> destination;
    const bool pursuing=playerLocation &&
        (e.behavior>=3 || (e.behavior==1 && location->sector==playerLocation->sector));
    if (pursuing)
        destination=waypoint(level,*location,p,*playerLocation,{camera.x,camera.y});
    else if (e.behavior>=1) {
        const collision::Point home{origins_[i].x+e.homeX,origins_[i].y+e.homeY};
        const auto homeLocation=collision::locate(level,home,location->sector);
        if (homeLocation) destination=waypoint(level,*location,p,*homeLocation,home);
    }
    if (!destination) { e.state=6; e.phase=(e.phase+1)&31; frame(e,waiting[e.phase]); return; }
    const double dx=destination->x-p.x,dy=destination->y-p.y;
    const bool samePlayerPolygon=playerLocation && location->sector==playerLocation->sector &&
                                 location->polygon==playerLocation->polygon;
    const double distance=std::hypot(camera.x-p.x,camera.y-p.y);
    if (samePlayerPolygon && pursuing && std::abs(camera.x-p.x)<100 && std::abs(camera.y-p.y)<100 &&
        (distance<1e-6 || !collision::rayDistance(walls,p,{(camera.x-p.x)/distance,(camera.y-p.y)/distance},distance))) {
        e.state=2; e.phase=1; e.sprite^=0x2000; frame(e,attack[1]); return;
    }
    if (!pursuing && std::max(std::abs(dx),std::abs(dy))<50) {
        e.state=6; e.phase=(e.phase+1)&31;
        if (!e.phase) e.sprite^=0x2000;
        frame(e,waiting[e.phase]); return;
    }
    move(i,level,*destination,camera,walls,objets);
}
void System::move(std::size_t i, const game_lab::Level& level, collision::Point destination,
                  const Camera& camera, const collision::World& walls, const sprites::Scene* objets)
{
    auto& e=records_[i];
    const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
    const double dx=destination.x-p.x,dy=destination.y-p.y;
    const double target=std::atan2(dy,dx)*360/pi;
    double turn=std::remainder(target-e.angle,720);
    if (std::abs(turn)>1e-6) {
        const double amount=std::max(4.0,std::abs(turn)*.5);
        e.angle=std::uint16_t((int(std::lround(e.angle+std::copysign(amount,turn)))+720)%720);
    }
    if (std::abs(turn)<180) {
        const double angle=e.angle*pi/360;
        auto moved=collision::move(walls,p,{speed_*std::cos(angle),speed_*std::sin(angle)});
        auto next=collision::locate(level,moved,e.sector);
        if (next) {
            moved=collision::constrainObjets(level,next->sector,p,moved,objets,&e.angle);
            next=collision::locate(level,moved,next->sector);
        }
        if (next) {
            const auto& sector=level.sectors[next->sector];
            e.sector=std::uint16_t(next->sector); e.polygon=std::uint16_t(next->polygon);
            origins_[i]={double(sector.gx),double(sector.gy)};
            e.x=std::uint16_t(std::lround(moved.x-sector.gx));
            e.y=std::uint16_t(std::lround(moved.y-sector.gy));
        }
        // EXE 0xe696: MONS3 follows the original direction-dependent cycle.
        // Keep MONS1's established forward-only animation unchanged.
        const bool reverse=directionalCycle_ &&
            std::abs(std::remainder(camera.angle*360/pi-e.angle,720))<180;
        if (e.state!=0 || e.phase>=5) e.phase=0;
        if (reverse) e.phase=e.phase==0?4:e.phase-1;
        else {
            e.phase=(e.phase+1)%5;
            if (!e.phase) e.sprite^=0x2000;
        }
        frame(e,e.phase); e.state=0;
    }
}
std::optional<ShotHit> System::trace(const Camera& view, collision::Point origin,
                                    collision::Point direction, double range) const
{
    const double c=std::cos(view.angle),s=std::sin(view.angle);
    const double denominator=direction.x*c+direction.y*s;
    if (denominator<=1e-9) return {};
    std::optional<ShotHit> best;
    for (const auto& object:scene_.objects) {
        if (object.image >= scene_.bank.images.size() || object.image >= masks_.size()/8)
            throw std::runtime_error("Invalid monster mask index");
        const auto& image=scene_.bank.images[object.image];
        const double distance=((object.x-origin.x)*c+(object.y-origin.y)*s)/denominator;
        if (distance<0 || distance>=range) continue;
        const double scale=object.height/image.texture.height;
        const double side=(origin.x+direction.x*distance-object.x)*s-
                          (origin.y+direction.y*distance-object.y)*c;
        const double anchor=(object.type&0x0200)?image.texture.width-image.anchorX:image.anchorX;
        const double u=anchor+side/scale;
        const double v=image.anchorY+(object.z-view.viewHeight())/scale;
        if (u<0 || v<0 || u>=image.texture.width || v>=image.texture.height) continue;
        const unsigned column=unsigned(u*8/image.texture.width),row=unsigned(v*8/image.texture.height);
        if (masks_[object.image*8+row]&(1u<<column)) continue; // Native bit1 means MISS.
        best=ShotHit{object.param2,distance}; range=distance;
    }
    return best;
}
void System::hit(std::size_t index, unsigned weapon, double angle)
{
    if (index>=records_.size() || weapon>=6 || !std::isfinite(angle)) return;
    auto& e=records_[index];
    if (e.health==0xffff) return;
    sounds_.push_back(16);
    if (e.state==5) return;
    e.angle=std::uint16_t((int(std::lround(std::remainder(angle,2*pi)*360/pi))+360+720)%720);
    // MONS5 0xeaa9: (weapon descriptor +30 - 2)*10. The port retains the
    // impacting weapon; DOS rereads the selected weapon during hurt/death.
    if (bank_==5) recoil5_[index]=damage[weapon]>2 ? (damage[weapon]-2)*10 : 0;
    e.phase=0; e.health=std::uint16_t(std::max(0,int(e.health)-int(damage[weapon])));
    e.state=e.health?4:5; frame(e,5);
    if (e.behavior==2) e.behavior=3;
    rebuildScene();
}
void System::constrainPlayer(const game_lab::Level& level, const Camera& before, Camera& after,
                              const collision::World& walls) const
{
    const auto location=collision::locate(level,{before.x,before.y});
    if (!location) return;
    const auto skin=collision::Settings{}.skin;
    const collision::Point proposed{after.x,after.y};
    // EXE 0x11029..0x11173: same-sector enemy boxes, half-extent100.
    // Keep dying bodies until the original removed sentinel, with numeric skin.
    for (const auto& object:scene_.objects) {
        if (records_[object.param2].sector!=location->sector) continue;
        const double left=object.x-100,right=object.x+100,top=object.y-100,bottom=object.y+100;
        if (after.x<left || after.x>right || after.y<top || after.y>bottom) continue;
        if (before.y>=bottom) after.y=bottom+skin;
        else if (before.y<=top) after.y=top-skin;
        else if (before.x>=right) after.x=right+skin;
        else if (before.x<=left) after.x=left-skin;
    }
    if (after.x==proposed.x && after.y==proposed.y) return;
    const auto corrected=collision::move(walls,{before.x,before.y},{after.x-before.x,after.y-before.y});
    after.x=corrected.x; after.y=corrected.y;
}
bool System::blocksInterface(const game_lab::Level& level, const Camera& camera,
                             const std::vector<doors::Door>& dynamicDoors) const
{
    const auto location=collision::locate(level,{camera.x,camera.y},camera.sector);
    const auto visible=doors::connectedSectors(level,camera.sector.value_or(location ? location->sector : level.sectors.size()));
    for (const auto& e:records_) {
        if (e.health==0xffff || e.sector>=level.sectors.size()) continue;
        if (visible[e.sector]) return true;
    }
    return false;
}
std::vector<unsigned> System::takeSounds()
{
    // EXE0x13103 marks one pending byte per sound ID: same-frame calls coalesce.
    std::vector<unsigned> result;
    for (auto id:sounds_) if (std::find(result.begin(),result.end(),id)==result.end()) result.push_back(id);
    sounds_.clear(); return result;
}
} // namespace enemies
