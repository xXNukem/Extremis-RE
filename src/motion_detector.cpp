#include "motion_detector.h"
#include "motion_detector_tables.h"
#include "game_lab.h"
#include <algorithm>
#include <cmath>

namespace detector {
namespace {
unsigned angle(int x,int y)
{
    // EXE0x3ee8: table ratio, quadrant transforms, 720 units/turn.
    const bool negativeX=x<0,negativeY=y<0;
    x=std::abs(x); y=std::abs(y);
    const bool swap=x<y;
    if (swap) std::swap(x,y);
    int value=int(arctangent[x?unsigned(y*256/x):0]);
    if (swap) value=180-value;
    if (negativeX) value=360-value;
    if (negativeY) value=-value;
    return unsigned(value<0?value+720:value);
}
}
Reading scan(const game_lab::Level& level, const std::vector<game_lab::EnemyRecord>& enemies,
             const Camera& camera, unsigned phase, bool enabled)
{
    Reading result;
    if (!enabled || !std::isfinite(camera.angle) || !std::isfinite(camera.x) || !std::isfinite(camera.y)) return result;
    result.phase=phase&7;
    constexpr double pi=3.14159265358979323846;
    const int heading=int(std::lround(std::remainder(camera.angle,2*pi)*360/pi));
    const unsigned rotation=unsigned((180-heading+1440)%720);
    const auto c=cosine[rotation],s=cosine[(rotation+540)%720];
    unsigned eligible=0;
    for (const auto& enemy:enemies) {
        if (enemy.health==0xffff || enemy.sector>=level.sectors.size()) continue;
        const auto& sector=level.sectors[enemy.sector];
        const double dx=double(sector.gx)+enemy.x-std::round(camera.x);
        const double dy=double(sector.gy)+enemy.y-std::round(camera.y);
        if (std::abs(dx)>=32768 || std::abs(dy)>=32768) continue;
        // Native Q14 rotation, then arithmetic shift6. Preserve the native X sign.
        const int x=int(std::floor((dy*s-dx*c)/1048576.0));
        const int y=int(std::floor((dy*c+dx*s)/1048576.0));
        if (x< -27 || x>=27 || y< -3 || y>=32) continue;
        const unsigned sectorAngle=angle(-x,y)/60;
        const unsigned fade=unsigned(std::abs(int(*result.phase)-int(sectorAngle)-1));
        if (fade<=2) result.contacts[result.count++]={x+24,198-y,fade};
        if (++eligible==10) break; // Before sweep filtering, original record order.
    }
    return result;
}
}
