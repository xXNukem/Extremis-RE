#include "extremis/gameplay/player/continuation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>

namespace continuation {
namespace {
struct Entry {
    Checkpoint checkpoint;
    std::uint64_t arrivalSectors;
};

constexpr std::uint64_t sectors(std::initializer_list<unsigned> ids)
{
    std::uint64_t mask=0;
    for (const auto sector:ids) mask |= std::uint64_t{1} << sector;
    return mask;
}

// Raw game units and zero-based zones/sectors. See password-continuation.md.
constexpr std::array<Entry,16> entries{{
    {{"ATILAHUN", 2, {0,300,280,540}, 0, {{0x0001, 0x1002, -1}, {100, -1}, -1, -1, -1, 1, 400}, 900, 216000}, sectors({0,22})},
    {{"ZENDOR56", 4, {0,300,280,540}, 2, {{0x0002, 0x1002, -1}, {100, 100}, 100, -1, 72000, 2, 400}, 850, 288000}, sectors({0})},
    {{"STRADIUS", 5, {10,300,280,540}, 3, {{0x0003, 0x1003, -1}, {100, 100}, -1, -1, 57600, 2, 500}, 700, 216000}, sectors({0,10,21})},
    {{"TERRAC12", 7, {0,300,280,540}, 2, {{0x0003, 0x1003, -1}, {100, 100}, -1, -1, 115200, 3, 480}, 500, 115200}, sectors({0})},
    {{"CARTOON0", 9, {0,280,300,360}, 0, {{0x0004, 0x1003, -1}, {100, 100}, -1, 288000, 50400, 3, 350}, 900, 72000}, sectors({0,1})},
    {{"GOREXZ73", 10, {0,320,300,0}, 2, {{0x0004, 0x1003, -1}, {100, 100}, -1, -1, 14400, 3, 380}, 750, 288000}, sectors({0})},
    {{"ALTEA117", 11, {0,320,300,0}, 1, {{0x0005, 0x1003, -1}, {100, -1}, -1, -1, 144000, 3, 500}, 1000, 172800}, sectors({0,1})},
    {{"RAMDAC38", 14, {0,320,300,0}, 2, {{0x0006, 0x1004, -1}, {100, 100}, -1, -1, 28800, 2, 500}, 500, 288000}, sectors({0,1,2})},
    {{"EPSILON4", 17, {0,300,280,540}, 3, {{0x0007, 0x1006, -1}, {100, 100}, -1, -1, 100800, 4, 460}, 500, 57600}, sectors({0,1})},
    {{"RUBISCOP", 20, {0,280,300,360}, 5, {{0x0008, 0x1006, -1}, {100, 100}, -1, -1, 144000, 5, 500}, 750, 288000}, sectors({0,1})},
    {{"NEAREND1", 21, {0,280,300,360}, 7, {{0x0008, 0x1008, -1}, {100, -1}, -1, -1, 144000, 4, 500}, 1000, 288000}, sectors({0})},
    {{"CENTRAL7", 22, {1,320,300,0}, 8, {{0x100b, -1, -1}, {100, -1}, -1, -1, 144000, 4, 400}, 1000, 288000}, sectors({0,1,2})},
    {{"ARKETYPE", 25, {0,300,320,180}, 7, {{0x0009, 0x100d, -1}, {-1, -1}, -1, -1, 72000, 4, 300}, 800, 144000}, sectors({0,1})},
    {{"PRACTIX4", 26, {0,280,300,360}, 5, {{0x0009, 0x100d, -1}, {100, -1}, -1, -1, 14400, 4, 150}, 1000, 288000}, sectors({0})},
    {{"ANTARES5", 28, {1,300,320,180}, 5, {{0x0009, 0x100d, -1}, {-1, -1}, -1, -1, 144000, 3, 500}, 1000, 144000}, sectors({0,1})},
    {{"CLEENEND", 29, {0,300,320,180}, 6, {{0x0009, 0x100d, -1}, {100, -1}, -1, -1, 28800, 4, 400}, 500, 172800}, sectors({0})},
}};
}

char normalize(char character)
{
    if (character >= 'a' && character <= 'z') return char(character - 'a' + 'A');
    if ((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'))
        return character;
    return 0;
}

const Checkpoint* find(std::string_view code)
{
    if (code.size() != 8) return nullptr;
    const auto hit = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
        return std::equal(code.begin(), code.end(), entry.checkpoint.code.begin(),
            [](char a, char b) { return normalize(a) == b; });
    });
    return hit == entries.end() ? nullptr : &hit->checkpoint;
}

void Disclosures::consume(const Checkpoint& checkpoint)
{
    consumed_.set(checkpoint.zone);
}

const Checkpoint* Disclosures::arrive(unsigned zone, unsigned sector)
{
    if (zone >= consumed_.size() || consumed_.test(zone) || sector >= 64) return nullptr;
    for (const auto& entry : entries) {
        if (entry.checkpoint.zone == zone && (entry.arrivalSectors & (std::uint64_t{1} << sector))) {
            consume(entry.checkpoint);
            return &entry.checkpoint;
        }
    }
    return nullptr;
}

} // namespace continuation
