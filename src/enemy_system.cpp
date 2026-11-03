#include "enemy_system.h"
#include "enemy_navigation.h"
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
bool visible(collision::Point enemy, const Camera& camera, const collision::World& walls)
{
    const double dx=enemy.x-camera.x,dy=enemy.y-camera.y;
    const double forward=dx*std::cos(camera.angle)+dy*std::sin(camera.angle);
    const double side=dx*std::sin(camera.angle)-dy*std::cos(camera.angle);
    if (forward<=0 || std::abs(side)>forward*std::tan(Camera::horizontalFov*.5)+87.75) return false;
    const double distance=std::hypot(dx,dy);
    return distance<1e-6 || !collision::rayDistance(walls,{camera.x,camera.y},{dx/distance,dy/distance},distance);
}
}
void System::load(const game_lab::Level& level, const std::filesystem::path& directory)
{
    if (level.monsBank!=1) throw std::runtime_error("Only MONS1 has been audited");
    auto bank=sprites::decode(read(directory/"MONS1.SPR"),wall_textures::loadPalette(
        directory/("PAL"+std::to_string(level.palBank)+".PAL")));
    auto masks=read(directory/"MONS1.SPT");
    if (bank.images.size()!=20 || masks.size()!=160) throw std::runtime_error("Unexpected MONS1 bank/masks");
    std::vector<collision::Point> origins;
    for (const auto& enemy:level.enemies) {
        const auto& sector=level.sectors.at(enemy.sector);
        if ((enemy.sprite&255)>=bank.images.size()) throw std::runtime_error("Invalid MONS1 frame");
        origins.push_back({double(sector.gx),double(sector.gy)});
    }
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
        const double height=270.0*scene_.bank.images[image].texture.height/scene_.bank.images[0].texture.height;
        scene_.objects.push_back({origins_[i].x+e.x,origins_[i].y+e.y,double(e.elevation&255),
            height,image,std::uint16_t((e.sprite&0x2000)?0x0200:0),std::uint16_t(i)});
    }
}
void System::update(const game_lab::Level& level, const Camera& camera, const collision::World& walls,
                    player::Vitals& vitals, double seconds, bool active)
{
    if (!active || !vitals.alive() || !std::isfinite(seconds) || seconds<0) {
        wasActive_=false; accumulator_=0; return;
    }
    if (!wasActive_) { wasActive_=true; return; } // Discard modal/focus resume interval.
    accumulator_+=std::min(seconds,0.25);
    while (accumulator_+1e-12>=stepSeconds && vitals.alive()) {
        accumulator_-=stepSeconds; step(level,camera,walls,vitals);
    }
    rebuildScene(); effects_.compose(camera,scene_.bank);
}
void System::step(const game_lab::Level& level, const Camera& camera, const collision::World& walls,
                  player::Vitals& vitals)
{
    ++ticks_; effects_.step(camera,walls);
    const auto location=collision::locate(level,{camera.x,camera.y});
    for (std::size_t i=0;i<records_.size();++i) {
        auto& e=records_[i];
        if (e.health==0xffff) continue;
        if (e.state==5) {
            frame(e,(e.phase&1)?9:5);
            if (++e.phase>=8) {
                e.health=0xffff; sounds_.push_back(17);
                effects_.begin({origins_[i].x+e.x,origins_[i].y+e.y},e.elevation&255,camera);
            }
        } else if (e.state==4 && e.phase<1) { ++e.phase; frame(e,9); }
        else if (e.state==2) {
            if (++e.phase>=7) {
                e.phase=0; e.state=0; advance(i,level,camera,walls,location);
            }
            else {
                frame(e,attack[e.phase]);
                const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
                const double dx=camera.x-p.x,dy=camera.y-p.y,d=std::hypot(dx,dy);
                if (e.phase==2 && location && location->sector==e.sector &&
                    (d<1e-6 || !collision::rayDistance(walls,p,{dx/d,dy/d},d))) {
                    reaction_=8; sounds_.push_back(9+(attackSound_++&1));
                }
            }
        } else {
            if (e.state==4) { e.state=0; e.phase=0; }
            advance(i,level,camera,walls,location);
        }
    }
    // EXE 0x165db..0x166b5: one shared reaction, 2 health units × 8 updates.
    if (reaction_) { vitals.applyDamage(2); --reaction_; }
}
void System::advance(std::size_t i, const game_lab::Level& level, const Camera& camera,
                     const collision::World& walls, std::optional<collision::Location> playerLocation)
{
    auto& e=records_[i];
    const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
    if (e.behavior==2 && ((playerLocation && playerLocation->sector==e.sector) || visible(p,camera,walls))) e.behavior=3;
    auto location=collision::locate(level,p,e.sector);
    if (!location) return;
    e.polygon=std::uint16_t(location->polygon);
    std::optional<collision::Point> destination;
    if (e.behavior>=3 && playerLocation)
        destination=waypoint(level,*location,p,*playerLocation,{camera.x,camera.y});
    else if (e.behavior>=1)
        destination=collision::Point{origins_[i].x+e.homeX,origins_[i].y+e.homeY};
    if (!destination) { e.state=6; e.phase=(e.phase+1)&31; frame(e,waiting[e.phase]); return; }
    const double dx=destination->x-p.x,dy=destination->y-p.y;
    const bool samePlayerPolygon=playerLocation && location->sector==playerLocation->sector &&
                                 location->polygon==playerLocation->polygon;
    const double distance=std::hypot(camera.x-p.x,camera.y-p.y);
    if (samePlayerPolygon && e.behavior>=3 && std::abs(camera.x-p.x)<100 && std::abs(camera.y-p.y)<100 &&
        (distance<1e-6 || !collision::rayDistance(walls,p,{(camera.x-p.x)/distance,(camera.y-p.y)/distance},distance))) {
        e.state=2; e.phase=1; e.sprite^=0x2000; frame(e,attack[1]); return;
    }
    if (e.behavior<3 && std::max(std::abs(dx),std::abs(dy))<50) {
        e.state=6; e.phase=(e.phase+1)&31;
        if (!e.phase) e.sprite^=0x2000;
        frame(e,waiting[e.phase]); return;
    }
    move(i,level,*destination,walls);
}
void System::move(std::size_t i, const game_lab::Level& level, collision::Point destination,
                  const collision::World& walls)
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
        const auto moved=collision::move(walls,p,{30*std::cos(angle),30*std::sin(angle)});
        const auto next=collision::locate(level,moved,e.sector);
        if (next) {
            const auto& sector=level.sectors[next->sector];
            e.sector=std::uint16_t(next->sector); e.polygon=std::uint16_t(next->polygon);
            origins_[i]={double(sector.gx),double(sector.gy)};
            e.x=std::uint16_t(std::lround(moved.x-sector.gx));
            e.y=std::uint16_t(std::lround(moved.y-sector.gy));
        }
        e.phase=(e.phase+1)%5;
        if (!e.phase) e.sprite^=0x2000;
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
        const auto& image=scene_.bank.images[object.image];
        const double distance=((object.x-origin.x)*c+(object.y-origin.y)*s)/denominator;
        if (distance<0 || distance>=range) continue;
        const double scale=object.height/image.texture.height;
        const double side=(origin.x+direction.x*distance-object.x)*s-
                          (origin.y+direction.y*distance-object.y)*c;
        const double anchor=(object.type&0x0200)?image.texture.width-image.anchorX:image.anchorX;
        const double u=anchor+side/scale;
        const double v=image.anchorY+(object.z-Camera::eyeHeight)/scale;
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
std::vector<unsigned> System::takeSounds()
{
    // EXE0x13103 marks one pending byte per sound ID: same-frame calls coalesce.
    std::vector<unsigned> result;
    for (auto id:sounds_) if (std::find(result.begin(),result.end(),id)==result.end()) result.push_back(id);
    sounds_.clear(); return result;
}
} // namespace enemies
