#pragma once

#include <SDL3/SDL_scancode.h>
#include <filesystem>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

enum class PortMenuChoice { None, Resume, NewGame, Exit, Error };
enum class PortMenuPage { Main, Options, Video, Audio, Controls, Language };

// Menu-only preview: settings do not affect the running game or persist.
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
private:
    bool inGame_;
    PortMenuPage page_ = PortMenuPage::Main;
    unsigned selection_ = 0;
    unsigned aspect_ = 1, resolution_ = 3, volume_ = 80, language_ = 1;
    bool planes_ = true, coloredLights_ = false, modernControls_ = true;
    static int hitTest(float x, float y, unsigned count, bool submenu);
    PortMenuChoice activate();
    void back();
    void changeValue(int delta);
};

// SDL modal: suspends gameplay; all settings here are presentation-only.
PortMenuChoice showPortMenu(SDL_Window* window, SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory, bool inGame);
