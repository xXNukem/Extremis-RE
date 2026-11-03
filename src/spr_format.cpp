#include "spr_format.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace spr_format {
namespace {
std::uint32_t integer(const std::vector<std::uint8_t>& bytes, std::size_t p, unsigned size)
{
    if (p > bytes.size() || size > bytes.size() - p)
        throw std::runtime_error("Truncated SPR integer");
    std::uint32_t result = 0;
    for (unsigned i = 0; i < size; ++i) result |= std::uint32_t(bytes[p + i]) << (8 * i);
    return result;
}

Image decodeImage(const std::vector<std::uint8_t>& bytes, std::size_t start,
                  std::size_t end)
{
    if (start > end || end > bytes.size() || end - start < 8)
        throw std::runtime_error("Invalid SPR record bounds");
    const auto w = integer(bytes, start, 2), h = integer(bytes, start + 2, 2);
    if (w == 0 || h == 0 || std::uint64_t(w) * h != end - start - 8)
        throw std::runtime_error("Invalid SPR dimensions");
    return {int(w), int(h), int(integer(bytes, start + 4, 2)),
            int(integer(bytes, start + 6, 2)),
            {bytes.begin() + start + 8, bytes.begin() + end}};
}
} // namespace

std::vector<Image> decode(const std::vector<std::uint8_t>& bytes)
{
    const auto count = integer(bytes, 2, 2);
    const std::size_t tableEnd = 4 + count * 4;
    if (count == 0 || tableEnd > bytes.size()) throw std::runtime_error("Invalid SPR count");
    const auto offset = [&](std::size_t i) {
        return std::uint64_t(4) + integer(bytes, 4 + i * 4, 4);
    };
    if (offset(0) != tableEnd) throw std::runtime_error("Invalid first SPR offset");
    std::vector<Image> images;
    images.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto start = offset(i), end = i + 1 < count ? offset(i + 1) : bytes.size();
        if (start < tableEnd || start > bytes.size() || end > bytes.size())
            throw std::runtime_error("SPR offset outside file");
        images.push_back(decodeImage(bytes, start, end));
    }
    return images;
}

std::vector<Image> load(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() < 4 || file.tellg() > 64 * 1024 * 1024)
        throw std::runtime_error("Cannot read SPR or invalid size: " + path.string());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
        throw std::runtime_error("Cannot read " + path.string());
    return decode(bytes);
}
} // namespace spr_format
