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

std::vector<std::uint16_t> readConnectionStates(const std::vector<std::uint8_t>& bytes,
                                              std::size_t begin, std::size_t end)
{
    Reader header(bytes, begin + 0x0a, end, "Connection state offset");
    const auto offset = header.u16();
    if (offset == 0) return {}; // Absent in older synthetic fixtures; infer no doors.
    if (offset < 0x14 || offset > end - begin) {
        throw std::runtime_error("Invalid connection state table offset");
    }
    const auto nextOffset = header.u16(); // Following section at header +0x0c.
    if (nextOffset != 0 && (nextOffset <= offset || nextOffset > end - begin)) {
        throw std::runtime_error("Invalid connection state table boundary");
    }
    Reader table(bytes, begin + offset, nextOffset ? begin + nextOffset : end,
                 "Connection states");
    const auto count = table.u16();
    table.require(std::size_t(count) * 2);
    std::vector<std::uint16_t> states;
    for (unsigned i = 0; i < count; ++i) states.push_back(table.u16());
    return states;
}

} // namespace

Level1 loadLevel1(const std::filesystem::path& path) { return loadLevel(path,0); }

static std::vector<std::uint8_t> readLab(const std::filesystem::path& path)
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

    return bytes;
}

static Level1 decodeLevel(const std::vector<std::uint8_t>& bytes, unsigned zoneIndex, bool banksOnly = false)
{
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

    if (zoneIndex>=result.header.levelCount) throw std::runtime_error("Invalid level index");
    const std::size_t begin = result.header.offsets[zoneIndex];
    const std::size_t end = result.header.offsets[zoneIndex+1];
    Reader banks(bytes, begin, end, "Level 1 banks");
    auto& level = result.level;
    level.palBank = banks.u16();
    level.dalleBank = banks.u16();
    level.objetBank = banks.u16();
    level.monsBank = banks.u16();
    level.zoneIndex = zoneIndex;
    if (banksOnly) return result;
    if (end - begin < 0x14) {
        throw std::runtime_error("Truncated level 1 header");
    }
    // EXTREMIS.EXE new-game template, file 0x1d8a0; loaded at 0x1de72.
    level.zoneIndex=zoneIndex;
    if (zoneIndex==0) level.playerStart = PlayerStart{0,300,320,180};
    level.connectionStates = readConnectionStates(bytes, begin, end);
    Reader codeHeader(bytes,begin+0x0e,end,"Zone access code offset");
    const auto codeOffset=codeHeader.u16();
    if (codeOffset) {
        if (codeOffset<0x14 || codeOffset>end-begin-2)
            throw std::runtime_error("Invalid zone access code offset");
        Reader code(bytes,begin+codeOffset,end,"Zone access code");
        level.accessCode=code.u16();
        const auto count=code.u16();
        code.require(std::size_t(count)*2);
        for (unsigned i=0;i<count;++i) level.codeTargets.push_back(code.u16());
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
        // Original level 3 sector 18 aliases polygon indices 2/3 (LAB 0x5420).
        // EXE 0xf502..0xf50f indexes offsets directly: preserve both indices.
        // upper_bound below still bounds each record by the next distinct offset.
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
    Reader routeHeader(bytes, begin + 8, end, "Sector route offsets");
    const auto routeOffset = routeHeader.u16();
    const auto routeEnd = routeHeader.u16();
    if (routeOffset) {
        if (routeOffset < tableEnd || routeEnd <= routeOffset || routeEnd > end-begin)
            throw std::runtime_error("Invalid sector route table bounds");
        Reader offsets(bytes, begin+routeOffset, begin+routeEnd, "Sector routes");
        offsets.require(level.sectors.size()*2);
        for (auto& sector : level.sectors) {
            const auto offset = offsets.u16();
            if (offset < level.sectors.size()*2 || offset >= routeEnd-routeOffset)
                throw std::runtime_error("Invalid sector route offset");
            Reader links(bytes, begin+routeOffset+offset, begin+routeEnd, "Sector route links");
            const auto count = links.u16(); links.require(std::size_t(count)*6);
            for (unsigned i=0;i<count;++i) {
                SectorRoute route{links.u16(),links.u16(),links.u16()};
                if (route.sector >= level.sectors.size()) throw std::runtime_error("Invalid sector route target");
                sector.routes.push_back(route);
            }
        }
    }
    Reader enemyHeader(bytes, begin + 0x0c, end, "Enemy table offset");
    const auto enemyOffset = enemyHeader.u16();
    const auto enemyEnd = enemyHeader.u16();
    if (enemyOffset) {
        if (enemyOffset < tableEnd || enemyEnd <= enemyOffset || enemyEnd > end - begin)
            throw std::runtime_error("Invalid enemy table bounds");
        Reader enemies(bytes, begin + enemyOffset, begin + enemyEnd, "Enemies");
        const auto count = enemies.u16();
        enemies.require(std::size_t(count) * 28);
        for (unsigned i = 0; i < count; ++i) {
            EnemyRecord record{enemies.u16(), enemies.u16(), enemies.u16(), enemies.u16(),
                enemies.u16(), enemies.u16(), enemies.u16(), enemies.u16(),
                enemies.u16(), enemies.u16(), enemies.u16(), enemies.u16(),
                enemies.u16(), enemies.u16()};
            if (record.sector >= level.sectors.size())
                throw std::runtime_error("Invalid enemy sector");
            level.enemies.push_back(record);
        }
    }
    return result;
}

Level1 loadLevel(const std::filesystem::path& path, unsigned zoneIndex)
{
    return decodeLevel(readLab(path), zoneIndex);
}

std::vector<Level> loadBankTable(const std::filesystem::path& path)
{
    const auto bytes = readLab(path);
    auto first = decodeLevel(bytes, 0, true);
    std::vector<Level> levels;
    levels.push_back(std::move(first.level));
    for (unsigned i = 1; i < first.header.levelCount; ++i)
        levels.push_back(decodeLevel(bytes, i, true).level);
    return levels;
}

} // namespace game_lab
