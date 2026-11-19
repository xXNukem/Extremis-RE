#include "port_menu.h"
#include <cassert>
#include <string>

static void choose(PortMenuState& s,unsigned index)
{
    while(s.selection()!=index) s.key(SDL_SCANCODE_DOWN);
    assert(s.key(SDL_SCANCODE_RETURN)==PortMenuChoice::None);
}

int main()
{
    PortMenuState start(false);
    start.setAvailableVideoModes({{853,480,1.f},{1920,1080,1.f}});
    assert(start.page()==PortMenuPage::Main && start.rowCount()==4);
    assert(start.label(1)=="LOAD GAME");
    assert(start.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(start.pointerMove(320.f,217.f));
    assert(start.selection()==1);
    assert(!start.pointerMove(320.f,217.f));
    assert(start.pointerClick(320.f,217.f)==PortMenuChoice::None);
    assert(start.pointerClick(320.f,258.f)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Options && start.selection()==0);
    assert(start.rowCount()==5 && std::string(start.pageTitle())=="OPTIONS");
    // The submenu is wider and higher than the main-menu selector.
    assert(start.pointerMove(96.f,131.f)==false); // Already on VIDEO.
    assert(start.pointerMove(96.f,178.f));
    assert(start.selection()==1);
    assert(start.pointerMove(320.f,131.f));
    assert(start.label(0)=="VIDEO" && start.label(1)=="AUDIO");
    assert(start.label(2)=="CONTROLS" && start.label(3)=="LANGUAGE");
    assert(start.label(4)=="BACK");

    // Six VIDEO rows: the effect is independent of resolution and aspect.
    assert(start.pointerClick(320.f,131.f)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Video && start.rowCount()==6);
    assert(start.label(0)=="ASPECT RATIO : AUTO (16:9)");
    assert(start.videoDimensions()==std::make_pair(320,200));
    assert(start.label(1)=="RESOLUTION : 320 X 200 (ORIGINAL)");
    assert(start.label(2)=="FISHEYE EFFECT : OFF");
    assert(!start.fisheyeEnabled());
    assert(start.label(3)=="FLOOR AND CEILING : ON");
    assert(start.label(4)=="COLORED LIGHTS : OFF");
    start.key(SDL_SCANCODE_RIGHT);
    assert(start.label(0)=="ASPECT RATIO : 4:3");
    start.key(SDL_SCANCODE_LEFT);
    assert(start.label(0)=="ASPECT RATIO : AUTO (16:9)");
    assert(start.pointerClick(320.f,154.f)==PortMenuChoice::None);
    assert(start.label(1)=="RESOLUTION : 853 X 480");
    assert(start.pointerClick(320.f,197.f)==PortMenuChoice::None);
    assert(start.label(2)=="FISHEYE EFFECT : ON");
    assert(start.fisheyeEnabled());
    start.key(SDL_SCANCODE_LEFT);
    assert(start.label(2)=="FISHEYE EFFECT : OFF");
    start.key(SDL_SCANCODE_RETURN);
    assert(start.label(2)=="FISHEYE EFFECT : ON");
    assert(start.pointerClick(320.f,240.f)==PortMenuChoice::None);
    assert(start.label(3)=="FLOOR AND CEILING : OFF");
    assert(start.pointerClick(320.f,283.f)==PortMenuChoice::None);
    assert(start.label(4)=="COLORED LIGHTS : ON");
    assert(start.pointerClick(320.f,326.f)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Options);

    choose(start,1);
    assert(start.page()==PortMenuPage::Audio && start.rowCount()==2);
    assert(start.label(0)=="MASTER VOLUME : 80");
    start.key(SDL_SCANCODE_LEFT);
    assert(start.label(0)=="MASTER VOLUME : 70");
    for(int i=0;i<15;++i) start.key(SDL_SCANCODE_LEFT);
    assert(start.label(0)=="MASTER VOLUME : 0");
    start.key(SDL_SCANCODE_RETURN);
    assert(start.label(0)=="MASTER VOLUME : 10");
    assert(start.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Options);

    choose(start,2);
    assert(start.page()==PortMenuPage::Controls && start.rowCount()==2);
    assert(start.label(0)=="CONTROL SCHEME : MODERN");
    start.key(SDL_SCANCODE_E);
    assert(start.label(0)=="CONTROL SCHEME : CLASSIC");
    assert(start.key(SDL_SCANCODE_BACKSPACE)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Options);

    choose(start,3);
    assert(start.page()==PortMenuPage::Language && start.rowCount()==4);
    assert(start.label(1)=="ENGLISH - SELECTED");
    start.key(SDL_SCANCODE_E);
    assert(start.label(0)=="FRENCH - SELECTED");
    assert(start.label(1)=="ENGLISH");
    start.key(SDL_SCANCODE_DOWN);start.key(SDL_SCANCODE_DOWN);
    assert(start.label(2)=="SPANISH");
    start.key(SDL_SCANCODE_RETURN);
    assert(start.label(2)=="SPANISH - SELECTED");
    start.key(SDL_SCANCODE_ESCAPE);
    assert(start.page()==PortMenuPage::Options);
    choose(start,4);
    assert(start.page()==PortMenuPage::Main);
    assert(start.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(start.pointerClick(320.f,298.f)==PortMenuChoice::Exit);

    // Preview data includes Retina and synthetic internal-resolution modes.
    PortMenuState video(false);
    video.setDisplaySize(3840,2160);
    video.setAvailableVideoModes({{1920,1080,1.f},{1470,956,2.f}});
    assert(video.videoDimensions()==std::make_pair(320,200));
    while (video.page()!=PortMenuPage::Video) {
        video.key(SDL_SCANCODE_DOWN);video.key(SDL_SCANCODE_DOWN);
        video.key(SDL_SCANCODE_RETURN); // Main -> Options
        video.key(SDL_SCANCODE_RETURN); // Options -> Video
    }
    assert(video.label(0)=="ASPECT RATIO : AUTO (16:9)");
    video.key(SDL_SCANCODE_RIGHT); // manual 4:3
    assert(video.videoDimensions()==std::make_pair(320,200));
    video.key(SDL_SCANCODE_RIGHT); // manual 16:9
    assert(video.videoDimensions()==std::make_pair(320,200));
    video.key(SDL_SCANCODE_RIGHT); // 21:9 fits within desktop
    assert(video.videoDimensions()==std::make_pair(320,200));
    video.setVideoChoices(3,2); // Real Retina mode: 1470x956 at density 2
    assert(video.videoDimensions()==std::make_pair(2940,1912));
    assert(video.label(1)=="RESOLUTION : 2940 X 1912");
    video.setDisplaySize(3024,1964); // Retina displays may not be 16:9.
    video.setVideoChoices(3,0);
    assert(video.videoDimensions()==std::make_pair(320,200));
    assert(video.label(0)=="ASPECT RATIO : AUTO (CUSTOM)");

    // Native maximum is an internal rendering mode, not exclusive fullscreen.
    video.setAvailableVideoModes({{640,400,1.f,false},{3840,2160,1.f,true}});
    video.setVideoChoices(3,2);
    assert(video.selectedVideoMode() && video.selectedVideoMode()->nativeMax);
    assert(video.label(1)=="RESOLUTION : 3840 X 2160 (MAX)");
    video.setVideoChoices(3,0);
    assert(video.videoDimensions()==std::make_pair(320,200));

    PortMenuState unavailable(false);
    unavailable.setAvailableVideoModes({{1920,1080,1.f},{3360,1440,1.f}});
    unavailable.setDisplaySize(2560,1600);
    unavailable.setVideoChoices(1,1); // Real SDL 1920x1080 mode
    unavailable.setAppliedVideoSize(2560,1600); // Desktop fallback
    unavailable.setVideoModeAvailable(false);
    assert(!unavailable.videoModeAvailable());
    unavailable.key(SDL_SCANCODE_DOWN);unavailable.key(SDL_SCANCODE_DOWN);
    unavailable.key(SDL_SCANCODE_RETURN); // OPTIONS
    unavailable.key(SDL_SCANCODE_RETURN); // VIDEO
    assert(unavailable.label(0)=="ASPECT RATIO : 16:9");
    assert(unavailable.label(1)=="RESOLUTION : 1920 X 1080");
    unavailable.key(SDL_SCANCODE_RIGHT); // 21:9
    assert(unavailable.label(0)=="ASPECT RATIO : 21:9");
    unavailable.key(SDL_SCANCODE_DOWN);
    unavailable.key(SDL_SCANCODE_RIGHT); // 1440p
    assert(unavailable.label(1)=="RESOLUTION : 3360 X 1440");
    unavailable.key(SDL_SCANCODE_LEFT); // 1080p
    assert(unavailable.label(1)=="RESOLUTION : 1920 X 1080");

    PortMenuState noModes(false);
    noModes.setDisplaySize(2940,1912);
    noModes.setAvailableVideoModes({});
    noModes.setVideoChoices(3,99);
    assert(noModes.videoDimensions()==std::make_pair(320,200));
    assert(noModes.label(0)=="NEW GAME"); // no phantom resolution choices

    PortMenuState game(true);
    assert(game.rowCount()==5);
    assert(game.label(0)=="BACK TO GAME");
    assert(game.label(1)=="NEW GAME");
    assert(game.label(4)=="EXIT");
    assert(game.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::Resume);
    assert(game.key(SDL_SCANCODE_RETURN)==PortMenuChoice::Resume);
    assert(game.pointerClick(320.f,158.f)==PortMenuChoice::Resume);
    game.key(SDL_SCANCODE_DOWN);
    assert(game.key(SDL_SCANCODE_RETURN)==PortMenuChoice::NewGame);
    game.key(SDL_SCANCODE_DOWN);game.key(SDL_SCANCODE_DOWN);
    game.key(SDL_SCANCODE_RETURN);
    assert(game.page()==PortMenuPage::Options);
    assert(game.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(game.page()==PortMenuPage::Main);
    assert(game.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::Resume);
    assert(!game.pointerMove(600.f,300.f));
    assert(game.key(SDL_SCANCODE_DOWN,true)==PortMenuChoice::None);
}
