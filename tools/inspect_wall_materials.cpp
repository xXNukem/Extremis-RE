#include "game_lab.h"

#include <iomanip>
#include <iostream>
#include <cmath>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: inspect_wall_materials path/to/GAME.LAB\n";
        return 1;
    }
    try {
        const auto data = game_lab::loadLevel1(argv[1]);
        std::cout << "sector,wall,vertex_a,vertex_b,type,low12,low8,extra,ax,ay,bx,by,length\n";
        for (std::size_t i = 0; i < data.level.sectors.size(); ++i) {
            const auto& sector = data.level.sectors[i];
            for (std::size_t j = 0; j < sector.walls.size(); ++j) {
                const auto& wall = sector.walls[j];
                const auto& a = sector.vertices[wall.vertexA];
                const auto& b = sector.vertices[wall.vertexB];
                std::cout << i << ',' << j << ',' << wall.vertexA << ',' << wall.vertexB
                          << ",0x" << std::hex << std::setw(4) << std::setfill('0')
                          << wall.type << std::dec << std::setfill(' ') << ','
                          << (wall.type & 0x0fff) << ',' << (wall.type & 0xff) << ','
                          << wall.extra << ',' << (std::uint32_t(sector.gx) + a.x)
                          << ',' << (std::uint32_t(sector.gy) + a.y)
                          << ',' << (std::uint32_t(sector.gx) + b.x)
                          << ',' << (std::uint32_t(sector.gy) + b.y)
                          << ',' << std::hypot(double(b.x) - a.x, double(b.y) - a.y) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
