#pragma once

#include "app/startup/port_menu_types.h"
#include "app/startup/user_settings.h"

#include <SDL3/SDL_scancode.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct SDL_Renderer;

class PortMenuState {
public:
    // Resolution 0 is native DOS. Other entries are scaled 3D presets and MAX.
    static constexpr unsigned automaticResolution=0;

    explicit PortMenuState(bool inGame, UserSettings& settings, SDL_Renderer* renderer = nullptr);
    unsigned selection() const;
    PortMenuPage page() const;
    UserLanguage language() const { return settings_.language; }
    unsigned rowCount() const;
    std::string label(unsigned row) const;
    const char* pageTitle() const;
    PortMenuChoice key(SDL_Scancode key, bool repeated = false);
    void typePassword(std::string_view text);
    bool passwordSubmitted() const { return passwordSubmitted_; }
    const continuation::Checkpoint* finishPasswordResponse();
    bool pointerMove(float x, float y);
    PortMenuChoice pointerClick(float x, float y);
    void setDisplaySize(int physicalWidth,int physicalHeight);
    void setVideoChoices(unsigned aspect,unsigned resolution);
    void setAvailableVideoModes(std::vector<PortVideoMode> modes);
    void restoreResolution();
    const PortVideoMode* selectedVideoMode() const;
    VideoRenderMode selectedRenderMode() const;
    unsigned availableModeCount() const;
    void setAppliedVideoSize(int physicalWidth,int physicalHeight);
    void setVideoModeAvailable(bool available);
    bool videoModeAvailable() const;
    bool vsyncChangeFailed() const { return vsyncChangeFailed_; }
    unsigned aspectChoice() const;
    unsigned resolutionChoice() const;
    bool fisheyeEnabled() const;
    std::pair<int,int> videoDimensions() const;
private:
    std::string password_;
    bool passwordSubmitted_ = false;
    const continuation::Checkpoint* checkpoint_ = nullptr;
    bool inGame_;
    UserSettings& settings_;
    SDL_Renderer* renderer_;
    std::optional<int> vsyncInterval_;
    bool vsyncChangeFailed_ = false;
    void refreshVSync();
    PortMenuPage page_ = PortMenuPage::Main;
    unsigned selection_ = 0;
    unsigned resolution_ = 0;
    std::vector<PortVideoMode> videoModes_;
    int monitorWidth_ = 1920, monitorHeight_ = 1080;
    int appliedWidth_ = 0, appliedHeight_ = 0;
    bool videoModeAvailable_ = true;
    static int hitTest(float x, float y, unsigned count, bool submenu);
    PortMenuChoice activate();
    void back();
    void changeValue(int delta);
};

