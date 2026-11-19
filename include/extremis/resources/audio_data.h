#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace audio {
struct Sample {
    std::string name;
    unsigned rate = 5000;
    std::uint16_t loopFlag = 0;
    std::uint32_t loopBegin = 0, loopEnd = 0; // End is exclusive in the original driver.
    std::vector<float> pcm; // Original unsigned PCM8 expanded to signed float mono.
};
struct Effect {
    Sample sample;
    std::uint8_t volume = 64, continuous = 0, priority = 0;
};
using Bank = std::vector<Effect>; // Original zero-based BANK.DSS identifiers.
Sample decodeSample(const std::vector<std::uint8_t>& bytes);
Bank decodeBank(const std::vector<std::uint8_t>& bytes);
Sample loadSample(const std::filesystem::path& path);
Bank loadBank(const std::filesystem::path& path);
} // namespace audio
