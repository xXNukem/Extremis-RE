#include "game_lab.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace game_lab {
namespace {

class Reader {
public:
    Reader(const std::vector<std::uint8_t>& bytes, std::size_t begin,
           std::size_t end, std::string context)
        : bytes_(bytes), position_(begin), end_(end), context_(std::move(context))
    {
        if (begin > end || end > bytes.size()) {
            throw std::runtime_error(context_ + ": invalid bounds");
        }
    }

    void require(std::size_t count) const
    {
        if (count > end_ - position_) {
            throw std::runtime_error(context_ + ": truncated data at byte " +
                                     std::to_string(position_));
        }
    }

    std::uint16_t u16()
    {
        require(2);
        const auto value = static_cast<std::uint16_t>(
            bytes_[position_] | (std::uint16_t(bytes_[position_ + 1]) << 8));
        position_ += 2;
        return value;
    }

    std::uint32_t u32()
    {
        const std::uint32_t low = u16();
        return low | (std::uint32_t(u16()) << 16);
    }

    std::size_t position() const { return position_; }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_, end_;
    std::string context_;
};

} // namespace

Level1 loadLevel1(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open GAME.LAB: " + path.string());
    }
    const auto size = file.tellg();
    if (size < 0 || static_cast<std::uintmax_t>(size) >
                        std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("Invalid GAME.LAB file size");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()))) {
        throw std::runtime_error("Cannot read GAME.LAB");
    }

    Level1 result{};
    Reader header(bytes, 0, bytes.size(), "Header");
    result.header.levelCount = header.u16();
    if (result.header.levelCount != 30) {
        throw std::runtime_error("GAME.LAB must contain 30 levels");
    }
    const std::size_t offsetCount = std::size_t(result.header.levelCount) + 1;
    header.require(offsetCount * 4);
    const std::size_t headerEnd = 2 + offsetCount * 4;
    for (std::size_t i = 0; i < offsetCount; ++i) {
        const auto offset = header.u32();
        if (offset < headerEnd || offset > bytes.size() ||
            (i > 0 && offset <= result.header.offsets.back())) {
            throw std::runtime_error("Invalid level offset at index " + std::to_string(i));
        }
        result.header.offsets.push_back(offset);
    }

    const std::size_t begin = result.header.offsets[0];
    const std::size_t end = result.header.offsets[1];
    Reader banks(bytes, begin, end, "Level 1 banks");
    auto& level = result.level;
    level.palBank = banks.u16();
    level.dalleBank = banks.u16();
    level.objetBank = banks.u16();
    level.monsBank = banks.u16();
    if (end - begin < 0x14) {
        throw std::runtime_error("Truncated level 1 header");
    }
    Reader table(bytes, begin + 0x12, end, "Level 1 sector table");
    const auto sectorCount = table.u16();
    table.require(std::size_t(sectorCount) * 6);
    const std::size_t tableEnd = 0x14 + std::size_t(sectorCount) * 6;
    level.sectors.resize(sectorCount);
    std::vector<std::size_t> sectorOffsets;
    for (auto& sector : level.sectors) {
        sector.localOffset = table.u16();
        sector.gx = table.u16();
        sector.gy = table.u16();
        if (sector.localOffset < tableEnd || sector.localOffset >= end - begin) {
            throw std::runtime_error("Invalid sector offset");
        }
        sectorOffsets.push_back(sector.localOffset);
    }
    std::sort(sectorOffsets.begin(), sectorOffsets.end());
    if (std::adjacent_find(sectorOffsets.begin(), sectorOffsets.end()) != sectorOffsets.end()) {
        throw std::runtime_error("Duplicate sector offsets");
    }

    for (std::size_t i = 0; i < level.sectors.size(); ++i) {
        auto& sector = level.sectors[i];
        const auto next = std::upper_bound(sectorOffsets.begin(), sectorOffsets.end(),
                                           sector.localOffset);
        const auto sectorEnd = next == sectorOffsets.end() ? end : begin + *next;
        const auto context = "Level 1 sector " + std::to_string(i);
        Reader reader(bytes, begin + sector.localOffset, sectorEnd, context);
        const auto neighborCount = reader.u16();
        reader.require(std::size_t(neighborCount) * 2);
        for (unsigned n = 0; n < neighborCount; ++n) {
            const auto neighbor = reader.u16();
            if (neighbor >= sectorCount) {
                throw std::runtime_error(context + ": invalid neighbor index");
            }
            sector.neighbors.push_back(neighbor);
        }
        const auto vertexCount = reader.u16();
        reader.require(std::size_t(vertexCount) * 4);
        for (unsigned n = 0; n < vertexCount; ++n) {
            sector.vertices.push_back({reader.u16(), reader.u16()});
        }
        const auto wallCount = reader.u16();
        reader.require(std::size_t(wallCount) * 8);
        for (unsigned n = 0; n < wallCount; ++n) {
            Wall wall{reader.u16(), reader.u16(), reader.u16(), reader.u16()};
            if (wall.vertexA >= vertexCount || wall.vertexB >= vertexCount) {
                throw std::runtime_error(context + ": invalid wall vertex index");
            }
            sector.walls.push_back(wall);
        }
        const auto entityCount = reader.u16();
        reader.require(std::size_t(entityCount) * 8);
        for (unsigned n = 0; n < entityCount; ++n) {
            EntityA entity{reader.u16(), reader.u16(), reader.u16(), reader.u16()};
            if (entity.vertex >= vertexCount) {
                throw std::runtime_error(context + ": invalid entity vertex index");
            }
            sector.entitiesA.push_back(entity);
        }
        const auto polygonBase = reader.position();
        const auto polygonCount = reader.u16();
        reader.require(std::size_t(polygonCount) * 2);
        const auto polygonTableSize = 2 + std::size_t(polygonCount) * 2;
        std::vector<std::size_t> offsets;
        for (unsigned n = 0; n < polygonCount; ++n) {
            const auto offset = reader.u16();
            if (offset < polygonTableSize || offset >= sectorEnd - polygonBase) {
                throw std::runtime_error(context + ": invalid polygon offset at index " +
                                         std::to_string(n));
            }
            offsets.push_back(offset);
        }
        auto sortedOffsets = offsets;
        std::sort(sortedOffsets.begin(), sortedOffsets.end());
        if (std::adjacent_find(sortedOffsets.begin(), sortedOffsets.end()) !=
            sortedOffsets.end()) {
            throw std::runtime_error(context + ": duplicate polygon offsets");
        }
        for (std::size_t n = 0; n < offsets.size(); ++n) {
            const auto nextPolygon = std::upper_bound(sortedOffsets.begin(),
                                                      sortedOffsets.end(), offsets[n]);
            const auto polygonEnd = nextPolygon == sortedOffsets.end()
                ? sectorEnd : polygonBase + *nextPolygon;
            const auto polygonContext = context + " polygon " + std::to_string(n);
            Reader polygonReader(bytes, polygonBase + offsets[n], polygonEnd,
                                 polygonContext);
            Polygon polygon{};
            const auto count = polygonReader.u16();
            polygonReader.require(std::size_t(count) * 2 + 4);
            for (unsigned v = 0; v < count; ++v) {
                const auto index = polygonReader.u16();
                if (index >= vertexCount) {
                    throw std::runtime_error(polygonContext + ": invalid vertex index");
                }
                polygon.vertices.push_back(index);
            }
            polygon.extra = polygonReader.u16();
            const auto connectionCount = polygonReader.u16();
            polygonReader.require(std::size_t(connectionCount) * 6);
            for (unsigned c = 0; c < connectionCount; ++c) {
                PolygonConnection connection{polygonReader.u16(), polygonReader.u16(),
                                             polygonReader.u16()};
                if (connection.polygon >= polygonCount) {
                    throw std::runtime_error(polygonContext + ": invalid destination polygon");
                }
                polygon.connections.push_back(connection);
            }
            sector.polygons.push_back(std::move(polygon));
        }
    }
    return result;
}

} // namespace game_lab
