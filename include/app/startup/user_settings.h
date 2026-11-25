#pragma once

#include "extremis/render/first_person/presentation/video_render_settings.h"
#include "extremis/ui/localization.h"

#include <filesystem>

struct SDL_Renderer;

using UserLanguage = localization::Language;

struct UserSettings {
    VideoRenderSettings render;
    bool vsync = false;
    unsigned masterVolume = 80;
    bool classicControls = false;
    UserLanguage language = UserLanguage::English;
    bool ignoreLockerCodes = false;

    float audioGain() const { return masterVolume / 100.f; }
};

std::filesystem::path userSettingsPath();
UserSettings loadUserSettings(const std::filesystem::path& path,
                              const std::filesystem::path& originalConfig = {});
bool saveUserSettings(const std::filesystem::path& path, const UserSettings& settings);
bool applyPreferredVSync(SDL_Renderer* renderer, const UserSettings& settings);
