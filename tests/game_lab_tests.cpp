#include "game_lab.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Bytes = std::vector<std::uint8_t>;

void put16(Bytes& bytes, std::size_t at, std::uint16_t value)
{
    bytes.at(at) = value & 0xff;
    bytes.at(at + 1) = value >> 8;
}

void put32(Bytes& bytes, std::size_t at, std::uint32_t value)
{
    put16(bytes, at, value & 0xffff);
    put16(bytes, at + 2, value >> 16);
}

void check(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Bytes fixture()
{
    // Entirely synthetic: one sector with one neighbor, vertex, wall and entity.
    Bytes bytes(126 + 100 + 29 * 20, 0);
    put16(bytes, 0, 30);
    put32(bytes, 2, 126);
    for (unsigned i = 1; i <= 30; ++i) {
        put32(bytes, 2 + i * 4, 126 + 100 + (i - 1) * 20);
    }
    put16(bytes, 126, 0x1234);
    put16(bytes, 126 + 0x12, 1);
    put16(bytes, 146, 26);
    put16(bytes, 148, 65000);
    put16(bytes, 150, 64000);
    put16(bytes, 152, 1); // neighbor count; neighbor 0
    put16(bytes, 156, 1); // vertex count
    put16(bytes, 158, 1000);
    put16(bytes, 160, 2000);
    put16(bytes, 162, 1); // wall count; both indices 0
    put16(bytes, 168, 0x4321); // wall type
    put16(bytes, 172, 1); // entity count; vertex 0
    put16(bytes, 176, 0x4007);
    put16(bytes, 178, 256);
    put16(bytes, 180, 7);
    put16(bytes, 182, 2); // Table B: two polygons, offsets relative to 182.
    put16(bytes, 184, 6);
    put16(bytes, 186, 20);
    for (const auto start : {188u, 202u}) {
        put16(bytes, start, 1); // One vertex: index 0.
        put16(bytes, start + 4, 0xabcd); // Unknown field.
        put16(bytes, start + 6, 1); // One connection.
        put16(bytes, start + 8, start == 188 ? 1 : 0);
        put16(bytes, start + 10, 1234);
        put16(bytes, start + 12, 5678);
    }
    return bytes;
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 1;
    }
    const std::filesystem::path path = argv[1];
    try {
        const auto write = [&](const Bytes& bytes) {
            std::ofstream file(path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            check(bool(file), "Cannot write synthetic fixture");
        };
        const auto valid = fixture();
        write(valid);
        const auto data = game_lab::loadLevel1(path);
        const auto& sector = data.level.sectors.at(0);
        check(data.header.levelCount == 30 && data.level.palBank == 0x1234,
              "Header / little-endian decoding failed");
        check(sector.neighbors.size() == 1 && sector.walls.at(0).type == 0x4321,
              "Neighbor / wall decoding failed");
        check(sector.entitiesA.at(0).type == 0x4007 &&
              sector.entitiesA.at(0).param1 == 256 && sector.entitiesA.at(0).param2 == 7,
              "Entity decoding failed");
        check(std::uint32_t(sector.gx) + sector.vertices.at(0).x == 66000 &&
              std::uint32_t(sector.gy) + sector.vertices.at(0).y == 66000,
              "Global coordinates overflowed");
        check(sector.polygons.size() == 2 &&
              sector.polygons.at(0).vertices.at(0) == 0 &&
              sector.polygons.at(0).extra == 0xabcd &&
              sector.polygons.at(0).connections.at(0).polygon == 1 &&
              sector.polygons.at(0).connections.at(0).x == 1234 &&
              sector.polygons.at(0).connections.at(0).y == 5678 &&
              sector.polygons.at(1).connections.at(0).polygon == 0,
              "Polygon / connection decoding failed");

        unsigned rejected = 0;
        const auto reject = [&](const Bytes& bytes) {
            write(bytes);
            try {
                game_lab::loadLevel1(path);
            } catch (const std::runtime_error&) {
                ++rejected;
                return;
            }
            throw std::runtime_error("Malformed fixture was accepted");
        };
        // Every truncation inside the parsed header/sector must fail.
        for (std::size_t length = 0; length < 182; ++length) {
            reject(Bytes(valid.begin(), valid.begin() + length));
        }
        for (const auto at : {0u, 146u, 154u, 164u, 166u, 174u}) {
            auto bytes = valid;
            put16(bytes, at, 0xffff);
            reject(bytes);
        }
        for (const auto at : {2u, 6u, 122u}) {
            auto bytes = valid;
            put32(bytes, at, 0xffffffff);
            reject(bytes);
        }
        auto bytes = valid;
        put32(bytes, 6, 181); // Sector records cross the level boundary.
        reject(bytes);
        bytes = valid;
        put32(bytes, 6, 126); // Repeated level offsets.
        reject(bytes);
        bytes = valid;
        put16(bytes, 152, 0xffff); // Count exceeds sector bounds.
        reject(bytes);
        for (const auto at : {182u, 184u, 188u, 190u, 194u, 196u}) {
            bytes = valid;
            put16(bytes, at, 0xffff);
            reject(bytes);
        }
        bytes = valid;
        put16(bytes, 186, 6); // Duplicate polygon offsets.
        reject(bytes);
        bytes = valid;
        put16(bytes, 184, 2); // Offset points inside table B header.
        reject(bytes);
        bytes = valid;
        put16(bytes, 186, 18); // First record overlaps the next polygon.
        reject(bytes);
        // Keep the file/header valid while truncating level 1's polygon records.
        for (unsigned end = 182; end < 216; ++end) {
            bytes = valid;
            put32(bytes, 6, end);
            reject(bytes);
        }
        // Unsorted, distinct offsets are valid; indices follow table order.
        bytes = valid;
        put16(bytes, 184, 20);
        put16(bytes, 186, 6);
        write(bytes);
        check(game_lab::loadLevel1(path).level.sectors.at(0).polygons.at(0)
                  .connections.at(0).polygon == 0,
              "Unsorted polygon offset table failed");
        bytes = valid;
        put16(bytes, 182, 0); // A sector may have an empty table B.
        write(bytes);
        check(game_lab::loadLevel1(path).level.sectors.at(0).polygons.empty(),
              "Empty polygon table failed");
        std::filesystem::remove(path);
        std::cout << "Synthetic decoding passed; rejected " << rejected
                  << " malformed inputs.\n";
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
