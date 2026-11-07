#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace anm_format {
struct Animation {
    std::array<std::uint32_t,256> palette{};
    std::vector<std::vector<std::uint8_t>> frames;
};
// ANM1: VGA palette, full LZSS frame, then 200 skip/count delta spans.
Animation decode(const std::vector<std::uint8_t>& bytes);
Animation load(const std::filesystem::path& path);
}
