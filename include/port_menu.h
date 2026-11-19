#pragma once

#include <SDL3/SDL_scancode.h>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

enum class PortMenuChoice { None, Resume, NewGame, Exit, Error };
enum class PortMenuPage { Main, Options, Video, Audio, Controls, Language };

// VIDEO chooses internal software 3D resolution on the active display.
// SDL display modes contribute the physical MAX; the DOS HUD remains unchanged.
struct PortVideoMode { int width=0, height=0; float density=1.f; bool nativeMax=false; bool dosPreset=false; };

// Runtime selection, shared with the renderer. The default is AUTO (3).
// We only keep this UI choice here; SDL display modes remain independent.
inline unsigned portVideoAspectChoice = 3;
// VORTEX_VIDEO_RESOLUTION_V2: requested physical mode, 0 = current desktop.
inline int portVideoRequestedPixelWidth = 0;
inline int portVideoRequestedPixelHeight = 0;
// VORTEX_VIDEO_RESOLUTION_V3: independent quality settings for CPU 3D.
inline int portVideoRenderScale = 1;  // 1 = unscaled original DOS
inline bool portVideoNativeMax = false;
inline int portVideoNativeWidth = 0;
inline int portVideoNativeHeight = 0;
// OFF by default. It affects only the 3D world, never the DOS HUD.
inline bool portVideoFisheyeEnabled = false;


class PortMenuState {
public:
    explicit PortMenuState(bool inGame) : inGame_(inGame) {}
    unsigned selection() const { return selection_; }
    PortMenuPage page() const { return page_; }
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
    unsigned availableModeCount() const { return unsigned(videoModes_.size()); }
    void setAppliedVideoSize(int physicalWidth,int physicalHeight);
    void setVideoModeAvailable(bool available) { videoModeAvailable_ = available; }
    bool videoModeAvailable() const { return videoModeAvailable_; }
    unsigned aspectChoice() const { return aspect_; }
    unsigned resolutionChoice() const { return resolution_; }
    bool fisheyeEnabled() const { return fisheye_; }
    void setFisheyeEnabled(bool enabled) { fisheye_ = enabled; }
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

// SDL modal: suspends gameplay; VIDEO changes the real output window.
// Other settings remain menu-only previews.
PortMenuChoice showPortMenu(SDL_Window* window, SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory, bool inGame);
