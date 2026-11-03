#include "enemy_navigation.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace enemies {
namespace {
using collision::Point;
struct Choice { unsigned link=0; double cost=std::numeric_limits<double>::infinity(); };
double distance(Point a, Point b) { return std::max(std::abs(a.x-b.x),std::abs(a.y-b.y)); }
// EXE 0xf723..0xf838: visit each path, retain the first least Chebyshev-cost route.
Choice sectorRoute(const game_lab::Level& level, unsigned from, unsigned target,
                   Point start, std::vector<bool>& visited)
{
    Choice best; visited[from]=true;
    const auto& links=level.sectors[from].routes;
    for (unsigned i=0;i<links.size();++i) {
        const auto& link=links[i];
        if (visited[link.sector]) continue;
        const Point p{double(link.x),double(link.y)};
        const auto tail=link.sector==target ? 0 : sectorRoute(level,link.sector,target,p,visited).cost;
        const auto cost=link.sector==target ? 0 : distance(start,p)+tail;
        if (cost<best.cost) best={i,cost};
    }
    visited[from]=false; return best;
}
Choice polygonRoute(const game_lab::Sector& sector, unsigned from, unsigned target,
                    Point start, std::vector<bool>& visited)
{
    Choice best; visited[from]=true;
    const auto& links=sector.polygons[from].connections;
    for (unsigned i=0;i<links.size();++i) {
        const auto& link=links[i];
        if (visited[link.polygon]) continue;
        const Point p{double(link.x)+sector.gx,double(link.y)+sector.gy};
        const auto tail=link.polygon==target ? 0 : polygonRoute(sector,link.polygon,target,p,visited).cost;
        const auto cost=link.polygon==target ? 0 : distance(start,p)+tail;
        if (cost<best.cost) best={i,cost};
    }
    visited[from]=false; return best;
}
}
std::optional<Point> waypoint(const game_lab::Level& level, collision::Location from,
                             Point start, collision::Location target, Point goal)
{
    // Native routing changes the navigation polygon when within 50 of its
    // waypoint, before the physical center has necessarily crossed the portal.
    for (std::size_t hop=0;from.sector!=target.sector && hop<level.sectors.size();++hop) {
        const auto& sector=level.sectors.at(from.sector);
        std::vector<bool> visited(level.sectors.size());
        const auto choice=sectorRoute(level,from.sector,target.sector,start,visited);
        if (!std::isfinite(choice.cost)) return {};
        const auto& link=sector.routes[choice.link];
        const Point portal{double(link.x),double(link.y)};
        if (distance(start,portal)<50) {
            const auto next=collision::locate(level,portal,link.sector);
            if (!next || next->sector!=link.sector) return {};
            from=*next;
            continue;
        }
        goal=portal;
        const auto local=collision::locate(level,goal,from.sector);
        if (!local || local->sector!=from.sector) return goal;
        target=*local;
        break;
    }
    if (from.sector!=target.sector) return {};
    const auto& sector=level.sectors.at(from.sector);
    for (std::size_t hop=0;from.polygon!=target.polygon && hop<sector.polygons.size();++hop) {
        std::vector<bool> visited(sector.polygons.size());
        const auto choice=polygonRoute(sector,from.polygon,target.polygon,start,visited);
        if (!std::isfinite(choice.cost)) return {};
        const auto& link=sector.polygons[from.polygon].connections[choice.link];
        const Point portal{double(link.x)+sector.gx,double(link.y)+sector.gy};
        if (distance(start,portal)>=50) return portal;
        from.polygon=link.polygon;
    }
    return goal;
}
}
