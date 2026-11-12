#include "objet_collision.h"
#include <array>

namespace collision {
namespace {
constexpr std::array<unsigned,10> counts{22,21,29,30,14,31,27,23,27,19};
// Original uint16 tables, file offsets 0x5d4f, 0x5da5, 0x5e09, 0x5e7f,
// 0x5ed7, 0x5f31, 0x5fa5, 0x6009, 0x606d, 0x60c9.
constexpr std::array<std::array<unsigned,31>,10> extents{{
    {{200,200,200,200,80,80,80,0,0,80,80,80,0,0,0,80,80,80,80,80,80,150}},
    {{200,200,0,0,100,80,80,80,100,100,100,100,80,0,80,150,0,80,80,100,0}},
    {{200,200,150,0,0,0,0,80,80,100,100,80,0,150,80,80,80,80,80,80,100,100,0,0,0,0,0,0,0}},
    {{200,200,80,80,100,0,80,80,0,100,80,0,0,0,0,0,0,80,80,80,100,100,0,0,0,0,80,80,80,80}},
    {{200,200,100,100,100,80,80,80,150,80,100,80,100,100}},
    {{200,200,0,0,0,0,0,80,80,80,80,0,0,0,0,0,0,0,0,0,0,0,0,100,150,100,80,80,80,100,100}},
    {{200,200,0,0,0,150,80,80,80,0,0,100,80,80,100,80,0,150,100,80,80,100,0,0,0,80,80}},
    {{200,200,100,80,80,80,80,80,0,100,80,0,0,0,0,0,0,80,100,100,80,100,80}},
    {{80,80,80,80,80,100,100,100,80,100,100,120,100,100,100,100,100,80,80,100,100,100,100,100,100,100,100}},
    {{120,100,100,80,100,80,100,80,100,100,100,120,100,100,80,120,100,80,120}}
}};
}

unsigned objetExtent(unsigned bank, std::size_t image)
{
    return bank >= 1 && bank <= counts.size() && image < counts[bank-1]
        ? extents[bank-1][image] : 0;
}

Point constrainObjets(const game_lab::Level& level, unsigned sectorIndex,
                      Point before, Point candidate, const sprites::Scene* scene,
                      std::uint16_t* enemyAngle)
{
    if (sectorIndex >= level.sectors.size()) return candidate;
    const auto& sector = level.sectors[sectorIndex];
    for (std::size_t i = 0; i < sector.entitiesA.size(); ++i) {
        const auto& entity = sector.entitiesA[i];
        if ((entity.type & 0x2000) || !(entity.type & 0x1000) ||
            entity.vertex >= sector.vertices.size()) continue;
        std::size_t image = entity.type & 0xff;
        if (scene && scene->objetBank == level.objetBank)
            for (const auto& object : scene->objects)
                if (object.sourceSector == sectorIndex && object.sourceEntity == i) {
                    image = object.image;
                    break;
                }
        const double r = objetExtent(level.objetBank,image);
        if (r == 0) continue;
        const auto& vertex = sector.vertices[entity.vertex];
        const double x = double(sector.gx)+vertex.x, y = double(sector.gy)+vertex.y;
        if (candidate.x < x-r || candidate.x >= x+r ||
            candidate.y < y-r || candidate.y >= y+r) continue;
        int normal;
        if (before.y >= y+r) { candidate.y = y+r; normal = 540; }
        else if (before.y <= y-r) { candidate.y = y-r; normal = 180; }
        else if (before.x >= x+r) { candidate.x = x+r; normal = 360; }
        else if (before.x <= x-r) { candidate.x = x-r; normal = 0; }
        else continue;
        if (enemyAngle) {
            // DOS 0x10ff7 / 0x3e9e: normalize each angle to [-360,360]
            // before subtracting; preserve the sign at the antipodal tie.
            const auto signedAngle = [](int angle) {
                while (angle > 360) angle -= 720;
                while (angle < -360) angle += 720;
                return angle;
            };
            const int delta = signedAngle(signedAngle(*enemyAngle)-signedAngle(normal));
            *enemyAngle = std::uint16_t((int(*enemyAngle)+(delta < 0 ? -80 : 80)+720)%720);
        }
    }
    return candidate;
}
}
