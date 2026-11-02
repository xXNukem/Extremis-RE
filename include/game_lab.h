#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace game_lab {

struct Vertex {
    std::uint16_t x, y;
};

struct Wall {
    std::uint16_t vertexA, vertexB, type, extra;
};

struct EntityA {
    std::uint16_t vertex, type, param1, param2;
};

struct PolygonConnection {
    std::uint16_t polygon, x, y;
};

struct Polygon {
    std::vector<std::uint16_t> vertices; // Indices into Sector::vertices.
    std::uint16_t extra; // Unknown meaning; preserved without interpretation.
    std::vector<PolygonConnection> connections;
};

struct Sector {
    std::uint16_t localOffset, gx, gy;
    std::vector<std::uint16_t> neighbors;
    std::vector<Vertex> vertices;
    std::vector<Wall> walls;
    std::vector<EntityA> entitiesA;
    std::vector<Polygon> polygons;
};

struct Level {
    std::uint16_t palBank, dalleBank, objetBank, monsBank;
    std::vector<Sector> sectors;
};

struct Header {
    std::uint16_t levelCount;
    std::vector<std::uint32_t> offsets;
};

struct Level1 {
    Header header;
    Level level;
};

// Reads level 1 only. Throws std::runtime_error on I/O or invalid data.
// Includes table B polygons and connections. Coordinates remain local;
// add sector.gx/gy using uint32_t to obtain global coordinates.
Level1 loadLevel1(const std::filesystem::path& path);

} // namespace game_lab
