#include "extremis/render/first_person/presentation/view_scale.h"
#include "extremis/gameplay/player/continuation.h"
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
constexpr std::array<std::string_view,3> languageLabels{{"ENGLISH","ESPAÑOL","FRANÇAIS"}};
constexpr std::array<UserLanguage,3> languageValues{{UserLanguage::English,UserLanguage::Spanish,UserLanguage::French}};

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
    settings_.render.preferences.aspectChoice=std::min(aspect,unsigned(aspectValues.size()-1));
    resolution_=std::min(resolution,unsigned(videoModes_.size()));
}

void PortMenuState::setAvailableVideoModes(std::vector<PortVideoMode> modes)
{
    videoModes_=std::move(modes);
    if(resolution_>videoModes_.size()) resolution_=automaticResolution;
}

void PortMenuState::restoreResolution()
{
    resolution_ = automaticResolution;
    const auto& mode = settings_.render.mode;
    for (unsigned i = 0; i < videoModes_.size(); ++i) {
        if (mode.nativeMax ? videoModes_[i].nativeMax :
            videoModes_[i].dosPreset && videoModes_[i].height / 200 == mode.renderScale) {
            resolution_ = i + 1;
            break;
        }
    }
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
    const auto size=selectedRenderMode().requestedSize;
    return {size.width,size.height};
}

VideoRenderMode PortMenuState::selectedRenderMode() const
{
    const auto* mode=selectedVideoMode();
    const bool nativeMax=mode && mode->nativeMax;
    const float density=mode && mode->density>0.f ? mode->density : 1.f;
    const int width=mode ? int(std::lround(mode->width*density)) : 320;
    const int height=mode ? int(std::lround(mode->height*density)) : 200;
    VideoRenderMode output{nativeMax ? 1 : std::max(1,height/200),
                           nativeMax,nativeMax ? width : 0,nativeMax ? height : 0,{width,height}};
    if (appliedWidth_>0 && appliedHeight_>0 && (!mode || nativeMax || mode->dosPreset)) {
        output.requestedSize=extremis::composedViewSize(appliedWidth_,appliedHeight_,
            {settings_.render.preferences,output});
    } else if (mode && mode->dosPreset) {
        const double aspect = extremis::selectedAspect(settings_.render.preferences.aspectChoice, monitorWidth_, monitorHeight_);
        output.requestedSize.width=extremis::dosViewWidth(aspect)*output.renderScale;
    }
    return output;
}

unsigned PortMenuState::rowCount() const
{
    switch(page_) {
    case PortMenuPage::Main: return inGame_?5:4;
    case PortMenuPage::Options: return 5;
    case PortMenuPage::Video: return 7;
    case PortMenuPage::Audio: case PortMenuPage::Controls: return 2;
    case PortMenuPage::Language: return 4;
    case PortMenuPage::Password: return 2;
    }
    return 4;
}

const char* PortMenuState::pageTitle() const
{
    switch(page_) {
    case PortMenuPage::Options: return localization::byEnglish("OPTIONS",settings_.language).data();
    case PortMenuPage::Video: return localization::byEnglish("VIDEO",settings_.language).data();
    case PortMenuPage::Audio: return localization::byEnglish("AUDIO",settings_.language).data();
    case PortMenuPage::Controls: return localization::byEnglish("CONTROLS",settings_.language).data();
    case PortMenuPage::Language: return localization::byEnglish("LANGUAGE",settings_.language).data();
    case PortMenuPage::Password: return localization::byEnglish("CONTINUATION CODE",settings_.language).data();
    case PortMenuPage::Main: break;
    }
    return "";
}

std::string PortMenuState::label(unsigned row) const
{
    if (row>=rowCount()) return "";
    switch(page_) {
    case PortMenuPage::Main: return std::string(localization::byEnglish(inGame_?inGameLabels[row]:mainLabels[row],settings_.language));
    case PortMenuPage::Options: return std::string(localization::byEnglish(optionsLabels[row],settings_.language));
    case PortMenuPage::Video: {
        const auto& video = settings_.render.preferences;
        switch(row) {
        case 0: return std::string(localization::byId("options.aspect_ratio",settings_.language))+" : "+std::string(aspectValues[video.aspectChoice])+
                       (video.aspectChoice==3?" ("+std::string(detectedAspect(monitorWidth_,monitorHeight_))+")":"");
        case 1: {
            const auto [w,h]=videoDimensions();
            return std::string(localization::byId("options.resolution",settings_.language))+" : "+std::to_string(w)+" X "+std::to_string(h)+
                (resolution_==0 ? " (ORIGINAL)" :
                 (selectedVideoMode() && selectedVideoMode()->nativeMax ? " (MAX)":""));
        }
        case 2: return std::string(localization::byId("options.fisheye",settings_.language))+" : "+(video.fisheyeEnabled?std::string(localization::byId("options.on",settings_.language)):std::string(localization::byId("options.off",settings_.language)));
        case 3: return std::string(localization::byId("options.floor_ceiling",settings_.language))+" : "+(video.floorAndCeiling?std::string(localization::byId("options.on",settings_.language)):std::string(localization::byId("options.off",settings_.language)));
        case 4: return std::string(localization::byId("options.colored_lights",settings_.language))+" : "+(video.coloredLights?std::string(localization::byId("options.on",settings_.language)):std::string(localization::byId("options.off",settings_.language)));
        case 5:
            if (!vsyncInterval_) return "VSYNC : "+std::string(localization::byId("options.unavailable",settings_.language));
            return "VSYNC : "+std::string(localization::byId(*vsyncInterval_ == SDL_RENDERER_VSYNC_DISABLED?"options.off":"options.on",settings_.language));
        default: return std::string(localization::byId("options.back",settings_.language));
        }
    }
    case PortMenuPage::Audio:
        return row==0?std::string(localization::byId("options.master_volume",settings_.language))+" : "+std::to_string(settings_.masterVolume):std::string(localization::byId("options.back",settings_.language));
    case PortMenuPage::Controls:
        return row==0?std::string(localization::byId("options.control_scheme",settings_.language))+" : "+(settings_.classicControls?std::string(localization::byId("options.classic",settings_.language)):std::string(localization::byId("options.modern",settings_.language))):std::string(localization::byId("options.back",settings_.language));
    case PortMenuPage::Password:
        if (row==0) return password_ + std::string(8-password_.size(), '.');
        if (passwordSubmitted_) return checkpoint_ ? std::string(localization::byId("password.accepted",settings_.language)) : std::string(localization::byId("password.refused",settings_.language));
        return std::string(localization::byId("password.enter_confirm",settings_.language));
    case PortMenuPage::Language:
        return row==3?std::string(localization::byId("options.back",settings_.language)):std::string(languageLabels[row])+(languageValues[row]==settings_.language?" - "+std::string(localization::byId("options.selected",settings_.language)) : "");
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
    auto& video = settings_.render.preferences;
    switch(page_) {
    case PortMenuPage::Video:
        switch(selection_) {
        case 0: video.aspectChoice=unsigned((int(video.aspectChoice)+delta+int(aspectValues.size()))%int(aspectValues.size())); break;
        case 1: {
            const int choices=int(videoModes_.size())+1;
            resolution_=unsigned((int(resolution_)+delta+choices)%choices);
            break;
        }
        case 2: video.fisheyeEnabled=!video.fisheyeEnabled; break;
        case 3: video.floorAndCeiling=!video.floorAndCeiling; break;
        case 4: video.coloredLights=!video.coloredLights; break;
        case 5:
            refreshVSync();
            if (vsyncInterval_) {
                const int requested = *vsyncInterval_ == SDL_RENDERER_VSYNC_DISABLED
                    ? 1 : SDL_RENDERER_VSYNC_DISABLED;
                const bool applied = SDL_SetRenderVSync(renderer_, requested);
                if (!applied) SDL_Log("VIDEO: VSync switch failed: %s", SDL_GetError());
                refreshVSync();
                vsyncChangeFailed_ = !applied || vsyncInterval_ != requested;
                if (!vsyncChangeFailed_) settings_.vsync = requested != SDL_RENDERER_VSYNC_DISABLED;
            }
            break;
        }
        break;
    case PortMenuPage::Audio:
        if(selection_==0) settings_.masterVolume=unsigned(std::clamp(int(settings_.masterVolume)+delta*10,0,100));
        break;
    case PortMenuPage::Controls:
        if(selection_==0) settings_.classicControls=!settings_.classicControls;
        break;
    case PortMenuPage::Language:
        if (selection_<3) settings_.language=languageValues[selection_];
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
        if (selection_==(inGame_?2u:1u)) {
            password_.clear();
            passwordSubmitted_=false;
            checkpoint_=nullptr;
            page_=PortMenuPage::Password;
            selection_=0;
        }
        break;
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
    case PortMenuPage::Password: break;
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
    if (page_==PortMenuPage::Password) return false;
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0 || unsigned(hit)==selection_) return false;
    selection_=unsigned(hit);
    return true;
}

PortMenuChoice PortMenuState::pointerClick(float x,float y)
{
    if (page_==PortMenuPage::Password) return PortMenuChoice::None;
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0) return PortMenuChoice::None;
    selection_=unsigned(hit);
    return activate();
}

PortMenuChoice PortMenuState::key(SDL_Scancode pressed,bool repeated)
{
    if (repeated) return PortMenuChoice::None;
    if (page_==PortMenuPage::Password) {
        if (!passwordSubmitted_) {
            if (pressed==SDL_SCANCODE_BACKSPACE && !password_.empty()) password_.pop_back();
            if ((pressed==SDL_SCANCODE_RETURN || pressed==SDL_SCANCODE_KP_ENTER) && password_.size()==8) {
                checkpoint_=continuation::find(password_);
                passwordSubmitted_=true;
            }
        }
        return PortMenuChoice::None;
    }
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

void PortMenuState::typePassword(std::string_view text)
{
    if (page_!=PortMenuPage::Password || passwordSubmitted_) return;
    for (char c:text) {
        if (password_.size()==8) break;
        if (const char normalized=continuation::normalize(c)) password_+=normalized;
    }
}

const continuation::Checkpoint* PortMenuState::finishPasswordResponse()
{
    if (!passwordSubmitted_) return nullptr;
    passwordSubmitted_=false;
    page_=PortMenuPage::Main;
    selection_=0;
    return std::exchange(checkpoint_,nullptr);
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
    return settings_.render.preferences.aspectChoice;
}

unsigned PortMenuState::resolutionChoice() const
{
    return resolution_;
}

bool PortMenuState::fisheyeEnabled() const
{
    return settings_.render.preferences.fisheyeEnabled;
}

void PortMenuState::refreshVSync()
{
    int interval = 0;
    if (renderer_ && SDL_GetRenderVSync(renderer_, &interval)) vsyncInterval_ = interval;
    else vsyncInterval_.reset();
}

PortMenuState::PortMenuState(bool inGame, UserSettings& settings, SDL_Renderer* renderer)
    : inGame_(inGame), settings_(settings), renderer_(renderer)
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
