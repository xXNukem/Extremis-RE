#include <system_error>
#include <filesystem>
#include "app/startup/user_settings.h"

#include <SDL3/SDL.h>

#include <charconv>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

std::filesystem::path userSettingsPath()
{
    const std::unique_ptr<char, decltype(&SDL_free)> directory(
        SDL_GetPrefPath("Extremis RE", "Extremis RE"), SDL_free);
    if (!directory) {
        SDL_Log("Configuration directory unavailable: %s", SDL_GetError());
        return {};
    }
    return std::filesystem::u8path(directory.get()) / "settings.ini";
}

UserSettings loadUserSettings(const std::filesystem::path& path,
                              const std::filesystem::path& originalConfig)
{
    UserSettings settings;
    auto& video = settings.render.preferences;
    auto& mode = settings.render.mode;
    // Preserve the port's first-run floor/ceiling default from the DOS resources.
    if (!originalConfig.empty()) {
        std::ifstream original(originalConfig, std::ios::binary | std::ios::ate);
        if (original && original.tellg() == 13) {
            char value = 0;
            original.seekg(12);
            if (original.get(value)) video.floorAndCeiling = value != 0;
        }
    }
    if (path.empty()) return settings;

    std::ifstream input(path, std::ios::binary);
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string_view key(line.data(), separator);
        const auto* begin = line.data() + separator + 1;
        const auto* end = line.data() + line.size();
        unsigned value = 0;
        const auto parsed = std::from_chars(begin, end, value);
        if (parsed.ec != std::errc{} || parsed.ptr != end) continue;

        if (key == "aspect" && value <= 3) video.aspectChoice = value;
        else if (key == "resolution" && value <= maxDosResolutionScale) {
            mode.nativeMax = value == 0;
            mode.renderScale = mode.nativeMax ? 1 : static_cast<int>(value);
        }
        else if (key == "fisheye" && value <= 1) video.fisheyeEnabled = value != 0;
        else if (key == "floor_and_ceiling" && value <= 1) video.floorAndCeiling = value != 0;
        else if (key == "colored_lights" && value <= 1) video.coloredLights = value != 0;
        else if (key == "vsync" && value <= 1) settings.vsync = value != 0;
        else if (key == "master_volume" && value <= 100) settings.masterVolume = value;
        else if (key == "classic_controls" && value <= 1) settings.classicControls = value != 0;
        else if (key == "language" && value <= unsigned(UserLanguage::Spanish))
            settings.language = static_cast<UserLanguage>(value);
    }
    return settings;
}

bool saveUserSettings(const std::filesystem::path& path, const UserSettings& settings)
{
    if (path.empty()) return false;
    auto temporary = path;
#ifdef _WIN32
    const auto process = _getpid();
#else
    const auto process = getpid();
#endif
    temporary += ".tmp." + std::to_string(process);
    const auto& video = settings.render.preferences;
    const auto& mode = settings.render.mode;
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    output << "aspect=" << video.aspectChoice << '\n'
           << "resolution=" << (mode.nativeMax ? 0 : mode.renderScale) << '\n'
           << "fisheye=" << video.fisheyeEnabled << '\n'
           << "floor_and_ceiling=" << video.floorAndCeiling << '\n'
           << "colored_lights=" << video.coloredLights << '\n'
           << "vsync=" << settings.vsync << '\n'
           << "master_volume=" << settings.masterVolume << '\n'
           << "classic_controls=" << settings.classicControls << '\n'
           << "language=" << unsigned(settings.language) << '\n';
    output.close();
    std::error_code error;
    if (output) {
        std::filesystem::rename(temporary, path, error);
        if (!error) return true;
    }
    SDL_Log("Could not save user settings: %s", error ? error.message().c_str() : "write failed");
    std::filesystem::remove(temporary, error);
    return false;
}

bool applyPreferredVSync(SDL_Renderer* renderer, const UserSettings& settings)
{
    if (SDL_SetRenderVSync(renderer, settings.vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        return true;
    // Some renderers already run unsynced and don't support setting it explicitly.
    int interval = 0;
    if (!settings.vsync && SDL_GetRenderVSync(renderer, &interval) &&
        interval == SDL_RENDERER_VSYNC_DISABLED) return true;
    SDL_Log("VIDEO: could not apply the VSync preference: %s", SDL_GetError());
    return false;
}
