#pragma once

#include "app/startup/port_menu_types.h"

#include <SDL3/SDL_scancode.h>
#include <string>
#include <utility>
#include <vector>

class PortMenuState {
public:
    // Resolution 0 is native DOS. Other entries are scaled 3D presets and MAX.
    static constexpr unsigned automaticResolution=0;

    explicit PortMenuState(bool inGame);
    unsigned selection() const;
    PortMenuPage page() const;
    unsigned rowCount() const;
    std::string label(unsigned row) const;
    const char* pageTitle() const;
    PortMenuChoice key(SDL_Scancode key, bool repeated = false);
    bool pointerMove(float x, float y);
    PortMenuChoice pointerClick(float x, float y);
    void setDisplaySize(int physicalWidth,int physicalHeight);
    void setVideoChoices(unsigned aspect,unsigned resolution);
    void setAvailableVideoModes(std::vector<PortVideoMode> modes);
    const PortVideoMode* selectedVideoMode() const;
    unsigned availableModeCount() const;
    void setAppliedVideoSize(int physicalWidth,int physicalHeight);
    void setVideoModeAvailable(bool available);
    bool videoModeAvailable() const;
    unsigned aspectChoice() const;
    unsigned resolutionChoice() const;
    bool fisheyeEnabled() const;
    void setFisheyeEnabled(bool enabled);
    std::pair<int,int> videoDimensions() const;
private:
    bool inGame_;
    PortMenuPage page_ = PortMenuPage::Main;
    unsigned selection_ = 0;
    unsigned aspect_ = 3, resolution_ = 0, volume_ = 80, language_ = 1;
    std::vector<PortVideoMode> videoModes_;
    int monitorWidth_ = 1920, monitorHeight_ = 1080;
    int appliedWidth_ = 0, appliedHeight_ = 0;
    bool videoModeAvailable_ = true;
    bool planes_ = true, coloredLights_ = false, modernControls_ = true;
    bool fisheye_ = false;
    static int hitTest(float x, float y, unsigned count, bool submenu);
    PortMenuChoice activate();
    void back();
    void changeValue(int delta);
};

