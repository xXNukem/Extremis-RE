#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <optional>

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

struct SectorRoute { std::uint16_t sector, x, y; }; // Global waypoint, header +0x08.

struct Sector {
    std::uint16_t localOffset, gx, gy;
    std::vector<std::uint16_t> neighbors;
    std::vector<Vertex> vertices;
    std::vector<Wall> walls;
    std::vector<EntityA> entitiesA;
    std::vector<Polygon> polygons;
    std::vector<SectorRoute> routes;
};

struct PlayerStart {
    unsigned sector;
    std::uint16_t x, y, angle; // Local coordinates; 720 angle units per turn.
};

struct EnemyRecord {
    // EXE [0x519e]: fourteen words, preserved in their original order.
    std::uint16_t health, behavior, state, phase, sector, x, y, polygon;
    std::uint16_t angle, reserved, sprite, elevation, homeX, homeY;
};

struct Level {
    std::uint16_t palBank, dalleBank, objetBank, monsBank;
    std::vector<Sector> sectors;
    std::vector<EnemyRecord> enemies;
    std::optional<PlayerStart> playerStart;
    std::vector<std::uint16_t> connectionStates; // Raw table at level header +0x0a.
    std::optional<std::uint16_t> accessCode; // Word at header +0x0e target, EXE [0x519a].
    std::vector<std::uint16_t> codeTargets; // Count/list following accessCode for code terminals.
    unsigned zoneIndex = 0; // Zero-based GAME.LAB/MAP.SPR record, not resource-bank number.
};

struct Header {
    std::uint16_t levelCount;
    std::vector<std::uint32_t> offsets;
};

struct Level1 {
    Header header;
    Level level;
};

// Reads a zero-based level. Throws std::runtime_error on I/O or invalid data.
// Includes table B polygons and connections. Coordinates remain local;
// add sector.gx/gy using uint32_t to obtain global coordinates.
Level1 loadLevel(const std::filesystem::path& path, unsigned zoneIndex);
// Read GAME.LAB once; return bank IDs and zoneIndex only (no geometry/entities).
std::vector<Level> loadBankTable(const std::filesystem::path& path);
Level1 loadLevel1(const std::filesystem::path& path);

} // namespace game_lab
