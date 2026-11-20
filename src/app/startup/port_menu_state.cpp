#include "extremis/render/first_person/presentation/view_scale.h"
#include "app/startup/port_menu_state.h"

#include "port_menu_layout.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace {
using app::menu_detail::menuLeft;
using app::menu_detail::menuRight;
using app::menu_detail::rowHeight;
using app::menu_detail::rowTop;
using app::menu_detail::submenuLeft;
using app::menu_detail::submenuRight;

constexpr std::array<std::string_view,4> mainLabels{{"NEW GAME","LOAD GAME","OPTIONS","EXIT"}};
constexpr std::array<std::string_view,5> inGameLabels{{"BACK TO GAME","NEW GAME","LOAD GAME","OPTIONS","EXIT"}};
constexpr std::array<std::string_view,5> optionsLabels{{"VIDEO","AUDIO","CONTROLS","LANGUAGE","BACK"}};
constexpr std::array<std::string_view,4> aspectValues{{"4:3","16:9","21:9","AUTO"}};
constexpr std::array<std::string_view,3> languageLabels{{"FRENCH","ENGLISH","SPANISH"}};

// The reported display ratio is informational. AUTO uses the monitor's
// measured dimensions, rather than assuming that every screen is 16:9.
const char* detectedAspect(int w,int h)
{
    if (w<=0 || h<=0) return "UNKNOWN";
    const double ratio=double(w)/double(h);
    if (std::abs(ratio-4.0/3.0)<.045) return "4:3";
    if (std::abs(ratio-3.0/2.0)<.025) return "3:2";
    if (std::abs(ratio-16.0/10.0)<.045) return "16:10";
    if (std::abs(ratio-16.0/9.0)<.045) return "16:9";
    if (std::abs(ratio-21.0/9.0)<.060) return "21:9";
    if (std::abs(ratio-32.0/9.0)<.080) return "32:9";
    return "CUSTOM";
}

} // namespace

void PortMenuState::setDisplaySize(int width,int height)
{
    if(width>0 && height>0) { monitorWidth_=width; monitorHeight_=height; }
}

void PortMenuState::setVideoChoices(unsigned aspect,unsigned resolution)
{
    aspect_=std::min(aspect,unsigned(aspectValues.size()-1));
    resolution_=std::min(resolution,unsigned(videoModes_.size()));
}

void PortMenuState::setAvailableVideoModes(std::vector<PortVideoMode> modes)
{
    videoModes_=std::move(modes);
    if(resolution_>videoModes_.size()) resolution_=automaticResolution;
}

const PortVideoMode* PortMenuState::selectedVideoMode() const
{
    return resolution_>0 && resolution_<=videoModes_.size()
        ? &videoModes_[resolution_-1] : nullptr;
}

void PortMenuState::setAppliedVideoSize(int physicalWidth,int physicalHeight)
{
    if(physicalWidth>0 && physicalHeight>0) {
        appliedWidth_=physicalWidth;
        appliedHeight_=physicalHeight;
    }
}

std::pair<int,int> PortMenuState::videoDimensions() const
{
    const auto* mode=selectedVideoMode();
    if(!mode) return {320,200};
    if(mode->dosPreset) {
        const double aspect = extremis::selectedAspect(aspect_, monitorWidth_, monitorHeight_);
        const int baseWidth=extremis::dosViewWidth(aspect);
        return {baseWidth*(mode->height/200),mode->height};
    }
    const float density=mode->density>0.f?mode->density:1.f;
    return {int(std::lround(mode->width*density)),
            int(std::lround(mode->height*density))};
}

unsigned PortMenuState::rowCount() const
{
    switch(page_) {
    case PortMenuPage::Main: return inGame_?5:4;
    case PortMenuPage::Options: return 5;
    case PortMenuPage::Video: return 7;
    case PortMenuPage::Audio: case PortMenuPage::Controls: return 2;
    case PortMenuPage::Language: return 4;
    }
    return 4;
}

const char* PortMenuState::pageTitle() const
{
    switch(page_) {
    case PortMenuPage::Options: return "OPTIONS";
    case PortMenuPage::Video: return "VIDEO";
    case PortMenuPage::Audio: return "AUDIO";
    case PortMenuPage::Controls: return "CONTROLS";
    case PortMenuPage::Language: return "LANGUAGE";
    case PortMenuPage::Main: break;
    }
    return "";
}

std::string PortMenuState::label(unsigned row) const
{
    if (row>=rowCount()) return "";
    switch(page_) {
    case PortMenuPage::Main: return std::string(inGame_?inGameLabels[row]:mainLabels[row]);
    case PortMenuPage::Options: return std::string(optionsLabels[row]);
    case PortMenuPage::Video:
        switch(row) {
        case 0: return "ASPECT RATIO : "+std::string(aspectValues[aspect_])+
                       (aspect_==3?" ("+std::string(detectedAspect(monitorWidth_,monitorHeight_))+")":"");
        case 1: {
            const auto [w,h]=videoDimensions();
            return "RESOLUTION : "+std::to_string(w)+" X "+std::to_string(h)+
                (resolution_==0 ? " (ORIGINAL)" :
                 (selectedVideoMode() && selectedVideoMode()->nativeMax ? " (MAX)":""));
        }
        case 2: return std::string("FISHEYE EFFECT : ")+(fisheye_?"ON":"OFF");
        case 3: return std::string("FLOOR AND CEILING : ")+(planes_?"ON":"OFF");
        case 4: return std::string("COLORED LIGHTS : ")+(coloredLights_?"ON":"OFF");
        case 5:
            if (!vsyncInterval_) return "VSYNC : UNAVAILABLE";
            return *vsyncInterval_ == SDL_RENDERER_VSYNC_DISABLED ? "VSYNC : OFF" : "VSYNC : ON";
        default: return "BACK";
        }
    case PortMenuPage::Audio:
        return row==0?"MASTER VOLUME : "+std::to_string(volume_):"BACK";
    case PortMenuPage::Controls:
        return row==0?std::string("CONTROL SCHEME : ")+(modernControls_?"MODERN":"CLASSIC"):"BACK";
    case PortMenuPage::Language:
        return row==3?"BACK":std::string(languageLabels[row])+(row==language_?" - SELECTED":"");
    }
    return "";
}

void PortMenuState::back()
{
    page_=page_==PortMenuPage::Options?PortMenuPage::Main:PortMenuPage::Options;
    selection_=0;
}

void PortMenuState::changeValue(int delta)
{
    switch(page_) {
    case PortMenuPage::Video:
        switch(selection_) {
        case 0: aspect_=unsigned((int(aspect_)+delta+int(aspectValues.size()))%int(aspectValues.size())); break;
        case 1: {
            const int choices=int(videoModes_.size())+1;
            resolution_=unsigned((int(resolution_)+delta+choices)%choices);
            break;
        }
        case 2: fisheye_=!fisheye_; break;
        case 3: planes_=!planes_; break;
        case 4: coloredLights_=!coloredLights_; break;
        case 5:
            refreshVSync();
            if (vsyncInterval_) {
                const int requested = *vsyncInterval_ == SDL_RENDERER_VSYNC_DISABLED
                    ? 1 : SDL_RENDERER_VSYNC_DISABLED;
                const bool applied = SDL_SetRenderVSync(renderer_, requested);
                if (!applied) SDL_Log("VIDEO: VSync switch failed: %s", SDL_GetError());
                refreshVSync();
                vsyncChangeFailed_ = !applied || vsyncInterval_ != requested;
            }
            break;
        }
        break;
    case PortMenuPage::Audio:
        if(selection_==0) volume_=unsigned(std::clamp(int(volume_)+delta*10,0,100));
        break;
    case PortMenuPage::Controls:
        if(selection_==0) modernControls_=!modernControls_;
        break;
    case PortMenuPage::Language:
        if(selection_<3) language_=selection_;
        break;
    default: break;
    }
}

PortMenuChoice PortMenuState::activate()
{
    switch(page_) {
    case PortMenuPage::Main:
        if(inGame_ && selection_==0) return PortMenuChoice::Resume;
        if(selection_==(inGame_?1u:0u)) return PortMenuChoice::NewGame;
        if(selection_==(inGame_?3u:2u)) {page_=PortMenuPage::Options;selection_=0;}
        if(selection_==(inGame_?4u:3u)) return PortMenuChoice::Exit;
        break; // LOAD GAME is visible and selectable, not operational.
    case PortMenuPage::Options:
        if(selection_==4) back();
        else {
            page_=static_cast<PortMenuPage>(int(PortMenuPage::Video)+int(selection_));
            selection_=0;
            if (page_==PortMenuPage::Video) {
                vsyncChangeFailed_=false;
                refreshVSync();
            }
        }
        break;
    case PortMenuPage::Video:
        if(selection_==6) back(); else changeValue(1);
        break;
    case PortMenuPage::Audio: case PortMenuPage::Controls:
        if(selection_==1) back(); else changeValue(1);
        break;
    case PortMenuPage::Language:
        if(selection_==3) back(); else changeValue(1);
        break;
    }
    return PortMenuChoice::None;
}

int PortMenuState::hitTest(float x,float y,unsigned count,bool submenu)
{
    const float left=submenu?submenuLeft:menuLeft;
    const float right=submenu?submenuRight:menuRight;
    if (x<left || x>right) return -1;
    for (unsigned i=0;i<count;++i) {
        const float top=rowTop(i,count,submenu);
        if (y>=top && y<top+rowHeight) return int(i);
    }
    return -1;
}

bool PortMenuState::pointerMove(float x,float y)
{
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0 || unsigned(hit)==selection_) return false;
    selection_=unsigned(hit);
    return true;
}

PortMenuChoice PortMenuState::pointerClick(float x,float y)
{
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0) return PortMenuChoice::None;
    selection_=unsigned(hit);
    return activate();
}

PortMenuChoice PortMenuState::key(SDL_Scancode pressed,bool repeated)
{
    if (repeated) return PortMenuChoice::None;
    if (pressed==SDL_SCANCODE_ESCAPE || pressed==SDL_SCANCODE_BACKSPACE) {
        if (page_!=PortMenuPage::Main) {back(); return PortMenuChoice::None;}
        if(pressed==SDL_SCANCODE_ESCAPE && inGame_) return PortMenuChoice::Resume;
        return PortMenuChoice::None;
    }
    if (pressed==SDL_SCANCODE_UP || pressed==SDL_SCANCODE_W)
        selection_=(selection_+rowCount()-1)%rowCount();
    else if (pressed==SDL_SCANCODE_DOWN || pressed==SDL_SCANCODE_S)
        selection_=(selection_+1)%rowCount();
    else if (pressed==SDL_SCANCODE_LEFT || pressed==SDL_SCANCODE_A)
        changeValue(-1);
    else if (pressed==SDL_SCANCODE_RIGHT || pressed==SDL_SCANCODE_D)
        changeValue(1);
    else if (pressed==SDL_SCANCODE_RETURN || pressed==SDL_SCANCODE_KP_ENTER ||
             pressed==SDL_SCANCODE_SPACE || pressed==SDL_SCANCODE_E)
        return activate();
    return PortMenuChoice::None;
}

unsigned PortMenuState::selection() const
{
    return selection_;
}

PortMenuPage PortMenuState::page() const
{
    return page_;
}

unsigned PortMenuState::availableModeCount() const
{
    return unsigned(videoModes_.size());
}

void PortMenuState::setVideoModeAvailable(bool available)
{
    videoModeAvailable_ = available;
}

bool PortMenuState::videoModeAvailable() const
{
    return videoModeAvailable_;
}

unsigned PortMenuState::aspectChoice() const
{
    return aspect_;
}

unsigned PortMenuState::resolutionChoice() const
{
    return resolution_;
}

bool PortMenuState::fisheyeEnabled() const
{
    return fisheye_;
}

void PortMenuState::setFisheyeEnabled(bool enabled)
{
    fisheye_ = enabled;
}

void PortMenuState::refreshVSync()
{
    int interval = 0;
    if (renderer_ && SDL_GetRenderVSync(renderer_, &interval)) vsyncInterval_ = interval;
    else vsyncInterval_.reset();
}

PortMenuState::PortMenuState(bool inGame, SDL_Renderer* renderer)
    : inGame_(inGame), renderer_(renderer)
{}

namespace app::menu_detail {

float rowTop(unsigned row,unsigned count,bool submenu)
{
    // Seven VIDEO rows fit above the status line; all other pages are unchanged.
    if (submenu && count==7) return 88.f+float(row)*38.f;
    if (submenu)
        return submenuFirstRow+(5.f-float(count))*submenuRowSpacing*.5f+
               float(row)*submenuRowSpacing;
    return firstRow+(4.f-float(count))*rowSpacing*.5f+float(row)*rowSpacing;
}

} // namespace app::menu_detail
