#undef NDEBUG
#include <string_view>
#include <cstddef>
#include <filesystem>
#include "app/startup/port_menu.h"
#include <fstream>
#include "app/startup/port_menu_state.h"
#include "../src/app/startup/port_menu_layout.h"
#include "../src/app/startup/port_menu_view.h"
#include "extremis/ui/original_font.h"
#include "extremis/ui/utf8_glyph.h"
#include <SDL3/SDL.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <array>
#include <string>
#include <tuple>
#include <vector>

static void choose(PortMenuState& s,unsigned index)
{
    while(s.selection()!=index) s.key(SDL_SCANCODE_DOWN);
    assert(s.key(SDL_SCANCODE_RETURN)==PortMenuChoice::None);
}

static void queueKey(SDL_Window* window, SDL_Scancode key)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.scancode = key;
    assert(SDL_PushEvent(&event));
}

static void gameplayTranslations(SDL_Renderer* renderer, SDL_Surface* surface, const original_font::Font* font)
{
    using namespace app::menu_detail;
    struct Labels { UserLanguage language; const char *page, *option, *on, *off, *back; };
    for (const auto& text : std::array<Labels,3>{{
        {UserLanguage::English,"GAMEPLAY","IGNORE LOCKER CODES","ON","OFF","BACK"},
        {UserLanguage::Spanish,"JUGABILIDAD","IGNORAR CÓDIGOS DE TAQUILLAS","ACTIVADO","DESACTIVADO","VOLVER"},
        {UserLanguage::French,"JOUABILITÉ","IGNORER LES CODES DES CASIERS","ACTIVÉ","DÉSACTIVÉ","RETOUR"}
    }}) {
        for (bool paused : {false,true}) {
            UserSettings settings;
            settings.language=text.language;
            PortMenuState menu(paused,settings);
            choose(menu,paused?3:2);
            assert(menu.label(4)==text.page);
            choose(menu,4);
            assert(menu.page()==PortMenuPage::Gameplay && menu.pageTitle()==std::string_view(text.page));
            assert(menu.rowCount()==2 && menu.label(1)==text.back);
            const auto top=rowTop(0,menu.rowCount(),true);
            for (bool enabled : {false,true}) {
                assert(settings.ignoreLockerCodes==enabled);
                assert(menu.label(0)==std::string(text.option)+" : "+(enabled?text.on:text.off));
                menu.key(SDL_SCANCODE_DOWN); // Leave the option free of the selection effect.
                assert(renderPortMenu(renderer,font,menu,0.,1.,0,0));
                int first=surface->w, last=-1;
                unsigned topStrokeCount=0;
                for (int y=int(top)-5; y<int(top+rowHeight)+5; ++y) {
                    unsigned strokes=0;
                    bool previousInk=false;
                    for (int x=0; x<surface->w; ++x) {
                        Uint8 r,g,b,a;
                        assert(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a));
                        if (r>=100 && !previousInk) ++strokes;
                        previousInk=r>=100;
                        if (r<100) continue;
                        assert(y>=top && y<top+rowHeight);
                        first=std::min(first,x);
                        last=std::max(last,x);
                    }
                    if (!topStrokeCount) topStrokeCount=strokes;
                }
                assert(first>submenuLeft+13 && last<submenuRight-3 && last-first>150);
                assert(std::abs(first+last-surface->w)<10);
                // Acute marks sit above the capital letters on the first lit scanline.
                if (text.language==UserLanguage::Spanish) assert(topStrokeCount==1); // CÓDIGOS
                if (text.language==UserLanguage::French) assert(topStrokeCount==(enabled?1u:2u)); // ACTIVÉ / DÉSACTIVÉ
                menu.pointerClick(320,top+rowHeight/2);
            }
            assert(!settings.ignoreLockerCodes);
            choose(menu,1);
            assert(menu.page()==PortMenuPage::Options);
            menu.key(SDL_SCANCODE_ESCAPE);
            assert(menu.page()==PortMenuPage::Main);
        }
    }
}

static void queueClick(SDL_Window* window, float y)
{
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 320;
    event.button.y = y;
    assert(SDL_PushEvent(&event));
}

static void persistence(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    const auto path = directory / "settings.ini";
    const auto cfg = directory / "EXTREMIS.CFG";
    const auto lab = directory / "GAME.LAB";
    constexpr auto labContents = "Read-only game data fixture";
    std::filesystem::remove(path);
    std::ofstream(cfg,std::ios::binary) << std::string(13,'\0');
    std::ofstream(lab,std::ios::binary) << labContents;
    const auto defaults = loadUserSettings(path);
    assert(!defaults.ignoreLockerCodes);
    assert(defaults.masterVolume == 80 && !defaults.classicControls && !defaults.vsync);
    assert(defaults.render.preferences.aspectChoice == 3 && !defaults.render.preferences.fisheyeEnabled &&
           defaults.render.preferences.floorAndCeiling && defaults.render.preferences.coloredLights);
    assert(defaults.render.mode.renderScale == 1 && !defaults.render.mode.nativeMax);
    assert(defaults.language == UserLanguage::English);
    assert(!loadUserSettings(path,cfg).render.preferences.floorAndCeiling);

    {
        std::ofstream file(path);
        file << "garbage\naspect=-1\nresolution=99999\nfisheye=not-a-number\nfloor_and_ceiling=2\n"
                "colored_lights=-9\nvsync=0x1\nmaster_volume=500\nclassic_controls=9\nlanguage=3\n"
                "master_volume=50\n";
    }
    auto settings = loadUserSettings(path);
    assert(!settings.ignoreLockerCodes); // Existing INI predating the preference.
    assert(settings.masterVolume == 50 && settings.audioGain() == .5f);
    assert(settings.render.preferences.aspectChoice == 3 && !settings.render.preferences.fisheyeEnabled &&
           settings.render.preferences.floorAndCeiling && settings.render.preferences.coloredLights);
    assert(settings.render.mode.renderScale == 1 && !settings.render.mode.nativeMax &&
           !settings.vsync && !settings.classicControls && settings.language == UserLanguage::English);
    std::ofstream(path) << "master_volume=-1\naspect=4xyz\nfisheye=1suffix\n";
    assert(loadUserSettings(path).masterVolume == defaults.masterVolume);

    SDL_Window* window = SDL_CreateWindow("Options regression",640,400,0);
    assert(window);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,"software");
    assert(renderer);
    settings.render.mode.renderScale = maxDosResolutionScale;
    initializePortVideo(window,renderer,settings);
    assert(settings.render.mode.renderScale == 1 && !settings.render.mode.nativeMax);
    assert(settings.render.mode.requestedSize.height == 200);

    // Use the same blocking menu that the application opens from gameplay.
    SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
    queueClick(window,273); // OPTIONS from pause
    queueClick(window,110); // VIDEO
    queueClick(window,98); // AUTO -> 4:3
    queueClick(window,137); // 1x -> 2x
    queueClick(window,174); // Fisheye on
    queueClick(window,212); // Planes off
    queueClick(window,251); // Lamps off
    queueClick(window,290); // VSync on
    queueKey(window,SDL_SCANCODE_ESCAPE);
    queueClick(window,156); // AUDIO
    for (int i=0; i<5; ++i) queueKey(window,SDL_SCANCODE_LEFT);
    queueKey(window,SDL_SCANCODE_ESCAPE);
    queueClick(window,202); // CONTROLS
    queueKey(window,SDL_SCANCODE_RETURN);
    queueKey(window,SDL_SCANCODE_ESCAPE);
    queueClick(window,294); // GAMEPLAY
    queueKey(window,SDL_SCANCODE_RIGHT);
    for (int i=0; i<3; ++i) queueKey(window,SDL_SCANCODE_ESCAPE);
    std::vector<float> liveGains;
    assert(showPortMenu(window,renderer,{},true,settings,path,
        [&](float gain) { liveGains.push_back(gain); }).choice == PortMenuChoice::Resume);
    assert(settings.ignoreLockerCodes);
    assert(liveGains.size()==5 && liveGains.front()==.4f && liveGains.back()==0.f);

    SDL_DestroyRenderer(renderer);
    settings = loadUserSettings(path,cfg);
    assert(settings.ignoreLockerCodes);
    assert(settings.render.preferences.aspectChoice == 0 && settings.render.preferences.fisheyeEnabled &&
           !settings.render.preferences.floorAndCeiling && !settings.render.preferences.coloredLights);
    assert(settings.render.mode.renderScale == 2 && !settings.render.mode.nativeMax);
    assert(settings.masterVolume == 0 && settings.audioGain() == 0.f && settings.classicControls &&
           settings.vsync && settings.language == UserLanguage::English);
    renderer = SDL_CreateRenderer(window,"software");
    assert(renderer);
    initializePortVideo(window,renderer,settings);
    int interval = 0;
    assert(SDL_GetRenderVSync(renderer,&interval) && interval == 1);
    assert(settings.render.mode.requestedSize.width == 640 && settings.render.mode.requestedSize.height == 400);

    // Main menu still accepts the keyboard and pointer in Classic.
    queueClick(window,176);
    assert(showPortMenu(window,renderer,{},false,settings,path).choice == PortMenuChoice::NewGame);
    queueKey(window,SDL_SCANCODE_DOWN);
    queueKey(window,SDL_SCANCODE_RETURN);
    SDL_Event text{};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.windowID = SDL_GetWindowID(window);
    text.text.text = "ATILAHUN";
    assert(SDL_PushEvent(&text));
    queueKey(window,SDL_SCANCODE_RETURN);
    const auto continuation = showPortMenu(window,renderer,{},false,settings,path);
    assert(continuation.choice == PortMenuChoice::Continue && continuation.checkpoint);
    assert(settings.classicControls && settings.render.mode.renderScale == 2 && settings.masterVolume == 0);
    assert(SDL_GetRenderVSync(renderer,&interval) && interval == 1);

    queueClick(window,273); // OPTIONS from pause
    queueClick(window,294); // GAMEPLAY
    queueKey(window,SDL_SCANCODE_RETURN);
    for (int i=0; i<3; ++i) queueKey(window,SDL_SCANCODE_ESCAPE);
    assert(showPortMenu(window,renderer,{},true,settings,path).choice==PortMenuChoice::Resume);
    assert(!settings.ignoreLockerCodes && !loadUserSettings(path).ignoreLockerCodes);

    settings.render.mode.nativeMax = true;
    settings.render.preferences.aspectChoice = 2;
    settings.masterVolume = 100;
    assert(saveUserSettings(path,settings));
    auto max = loadUserSettings(path);
    assert(max.render.mode.nativeMax && max.masterVolume == 100 && max.audioGain() == 1.f);
    PortMenuState volumeState(false,max,renderer);
    choose(volumeState,2); // OPTIONS
    choose(volumeState,1); // AUDIO
    volumeState.key(SDL_SCANCODE_LEFT);
    assert(max.masterVolume==90);
    volumeState.key(SDL_SCANCODE_RIGHT);
    assert(max.masterVolume==100);
    volumeState.key(SDL_SCANCODE_RIGHT);
    assert(max.masterVolume==100);
    volumeState.key(SDL_SCANCODE_DOWN);
    volumeState.key(SDL_SCANCODE_UP);
    assert(volumeState.selection()==0);
    volumeState.pointerClick(320,178);
    assert(max.masterVolume==100);
    initializePortVideo(window,renderer,max);
    assert(max.render.mode.nativeWidth > 0 && max.render.mode.nativeHeight > 0);
    assert(max.render.mode.requestedSize.width <= max.render.mode.nativeWidth &&
           max.render.mode.requestedSize.height >= 200);
    for (auto language : {UserLanguage::Spanish,UserLanguage::French,UserLanguage::English}) {
        max.language=language;assert(saveUserSettings(path,max));
        assert(loadUserSettings(path).language==language);
    }
    std::ifstream resources(lab,std::ios::binary);
    const std::string bytes{std::istreambuf_iterator<char>(resources),{}};
    assert(bytes == labContents && !loadUserSettings({},cfg).render.preferences.floorAndCeiling);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

int main(int argc,char** argv)
{
    for (const auto& entry:localization::entries) {
        assert(!entry.id.empty() && !entry.en.empty() && !entry.es.empty() && !entry.fr.empty());
        assert(localization::byId(entry.id,UserLanguage::Spanish)==entry.es);
        assert(localization::byId(entry.id,UserLanguage::French)==entry.fr);
        for(std::string_view value:{entry.es,entry.fr})
            for(std::size_t i=0;i<value.size();) {
                const auto g=localization::glyph(value,i);
                assert(g.bytes>0 && i+g.bytes<=value.size());
                assert(g.base!='?' || value[i]=='?' || g.mark=='i');
                i+=g.bytes;
            }
    }
    for(const auto& entry:localization::cinematicEntries)
        for(std::string_view value:{entry.es,entry.fr})
            for(std::size_t i=0;i<value.size();) {
                const auto g=localization::glyph(value,i);
                assert(g.bytes>0 && i+g.bytes<=value.size());
                assert(g.base!='?' || value[i]=='?' || g.mark=='i');
                i+=g.bytes;
            }
    assert(localization::translate("Card C001",UserLanguage::Spanish)=="Tarjeta C001");
    assert(localization::glyph("¿",0).mark=='i');
    assert(localization::glyph("Ñ",0).base=='N');
    assert(localization::glyph("Ç",0).base=='C');
    for (std::string_view character : {"Á","É","Í","Ó","Ú","À","È","Ñ","Ç","¿","¡","’"})
        assert(localization::glyph(character,0).bytes==character.size());
    for (const auto& [character,base,mark] : std::array<std::tuple<std::string_view,char,char>,38>{{
        {"á",'a','a'},{"é",'e','a'},{"í",'i','a'},{"ó",'o','a'},{"ú",'u','a'},
        {"Á",'A','a'},{"É",'E','a'},{"Í",'I','a'},{"Ó",'O','a'},{"Ú",'U','a'},
        {"ñ",'n','t'},{"Ñ",'N','t'},{"ü",'u','u'},{"Ü",'U','u'},
        {"ç",'c','c'},{"Ç",'C','c'},{"à",'a','g'},{"è",'e','g'},
        {"ê",'e','e'},{"â",'a','e'},{"ô",'o','e'},{"û",'u','e'},
        {"À",'A','g'},{"È",'E','g'},{"Ê",'E','e'},{"Â",'A','e'},
        {"Ô",'O','e'},{"Û",'U','e'},{"¿",'?','i'},{"¡",'!','i'},
        {"ö",'o','u'},{"Ö",'O','u'},{"ï",'i','u'},{"Ï",'I','u'},
        {"ë",'e','u'},{"Ë",'E','u'},{"ù",'u','g'},{"Ù",'U','g'}
    }}) {
        const auto g=localization::glyph(character,0);
        if(!(g.bytes==character.size() && g.base==base && g.mark==mark))
            SDL_Log("Glyph mismatch: %.*s got %c/%c expected %c/%c",int(character.size()),character.data(),g.base,g.mark,base,mark);
        assert(g.bytes==character.size() && g.base==base && g.mark==mark);
    }
    assert(localization::translate("ATTENTION:\r\n5M 00S UNTIL EXPLOSION!",UserLanguage::Spanish)==
           "ATENCIÓN:\r\n¡EXPLOSIÓN EN 5M 00S!");
    assert(argc == 2 || argc == 3);
    UserSettings startSettings;
    PortMenuState start(false,startSettings);
    start.setAvailableVideoModes({{853,480,1.f},{1920,1080,1.f}});
    assert(start.page()==PortMenuPage::Main && start.rowCount()==4);
    assert(start.label(1)=="LOAD GAME");
    assert(start.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(start.pointerMove(320.f,217.f));
    assert(start.selection()==1);
    assert(!start.pointerMove(320.f,217.f));
    assert(start.pointerClick(320.f,258.f)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Options && start.selection()==0);
    assert(start.rowCount()==6 && std::string(start.pageTitle())=="OPTIONS");
    // The submenu is wider and higher than the main-menu selector.
    assert(start.pointerMove(96.f,110.f)==false); // Already on VIDEO.
    assert(start.pointerMove(96.f,156.f));
    assert(start.selection()==1);
    assert(start.pointerMove(320.f,110.f));
    assert(start.label(0)=="VIDEO" && start.label(1)=="AUDIO");
    assert(start.label(2)=="CONTROLS" && start.label(3)=="LANGUAGE");
    assert(start.label(4)=="GAMEPLAY" && start.label(5)=="BACK");

    // Seven VIDEO rows: the effect is independent of resolution and aspect.
    assert(start.pointerClick(320.f,110.f)==PortMenuChoice::None);
    assert(start.page()==PortMenuPage::Video && start.rowCount()==7);
    assert(start.label(0)=="ASPECT RATIO : AUTO (16:9)");
    assert(start.videoDimensions()==std::make_pair(320,200));
    assert(start.label(1)=="RESOLUTION : 320 X 200 (ORIGINAL)");
    assert(start.label(2)=="FISHEYE EFFECT : OFF");
    assert(!start.fisheyeEnabled());
    assert(start.label(3)=="FLOOR AND CEILING : ON");
    assert(start.label(4)=="COLORED LIGHTS : ON");
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
    assert(start.pointerClick(320.f,220.f)==PortMenuChoice::None);
    assert(start.label(3)=="FLOOR AND CEILING : OFF");
    assert(start.pointerClick(320.f,255.f)==PortMenuChoice::None);
    assert(start.label(4)=="COLORED LIGHTS : OFF");
    assert(start.label(5)=="VSYNC : UNAVAILABLE");
    assert(start.pointerClick(320.f,298.f)==PortMenuChoice::None);
    assert(start.selection()==5 && start.label(5)=="VSYNC : UNAVAILABLE");
    assert(start.pointerClick(320.f,338.f)==PortMenuChoice::None);
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
    assert(start.label(0)=="ENGLISH - SELECTED");
    start.key(SDL_SCANCODE_DOWN); start.key(SDL_SCANCODE_RETURN);
    assert(startSettings.language==UserLanguage::Spanish);
    assert(start.label(1)=="ESPAÑOL - SELECCIONADO");
    start.key(SDL_SCANCODE_DOWN); start.key(SDL_SCANCODE_RETURN);
    assert(startSettings.language==UserLanguage::French);
    assert(start.label(2)=="FRANÇAIS - SÉLECTIONNÉ");
    start.key(SDL_SCANCODE_UP);start.key(SDL_SCANCODE_UP);start.key(SDL_SCANCODE_RETURN);
    assert(startSettings.language==UserLanguage::English);
    start.key(SDL_SCANCODE_ESCAPE);
    assert(start.page()==PortMenuPage::Options);
    choose(start,5);
    assert(start.page()==PortMenuPage::Main);
    assert(start.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(start.pointerClick(320.f,298.f)==PortMenuChoice::Exit);

    // Preview data includes Retina and synthetic internal-resolution modes.
    UserSettings videoSettings;
    PortMenuState video(false,videoSettings);
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

    UserSettings unavailableSettings;
    PortMenuState unavailable(false,unavailableSettings);
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

    UserSettings noModesSettings;
    PortMenuState noModes(false,noModesSettings);
    noModes.setDisplaySize(2940,1912);
    noModes.setAvailableVideoModes({});
    noModes.setVideoChoices(3,99);
    assert(noModes.videoDimensions()==std::make_pair(320,200));
    assert(noModes.label(0)=="NEW GAME"); // no phantom resolution choices

    UserSettings transitionSettings;
    PortMenuState pause(true,transitionSettings);
    choose(pause,3); // Options from pause.
    choose(pause,3); // Language.
    pause.key(SDL_SCANCODE_DOWN);pause.key(SDL_SCANCODE_RETURN); // Español.
    assert(transitionSettings.language==UserLanguage::Spanish);
    pause.key(SDL_SCANCODE_ESCAPE);pause.key(SDL_SCANCODE_ESCAPE);
    assert(pause.label(0)=="VOLVER AL JUEGO");
    pause.key(SDL_SCANCODE_DOWN);
    assert(pause.key(SDL_SCANCODE_RETURN)==PortMenuChoice::NewGame);
    assert(transitionSettings.language==UserLanguage::Spanish);
    PortMenuState reopenedPause(true,transitionSettings);
    assert(reopenedPause.label(2)=="CARGAR PARTIDA");

    UserSettings gameSettings;
    PortMenuState game(true,gameSettings);
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

    auto* surface=SDL_CreateSurface(640,400,SDL_PIXELFORMAT_RGBA8888);
    assert(surface);
    auto* renderer=SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    assert(SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND));
    gameplayTranslations(renderer,surface,nullptr);
    if (argc==3) {
        const original_font::Font font(renderer,std::filesystem::u8path(argv[2]));
        gameplayTranslations(renderer,surface,&font);
    }
    const auto openVideo=[](PortMenuState& menu) {
        choose(menu,3);
        choose(menu,0);
        assert(menu.page()==PortMenuPage::Video);
    };
    UserSettings unsupportedSettings;
    PortMenuState unsupported(true,unsupportedSettings,renderer);
    openVideo(unsupported);
    assert(unsupported.pointerClick(320.f,294.f)==PortMenuChoice::None);
    assert(unsupported.label(5)=="VSYNC : OFF" && unsupported.vsyncChangeFailed());
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);

    assert(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER,"dummy",SDL_HINT_OVERRIDE));
    assert(SDL_Init(SDL_INIT_VIDEO));
    auto* window=SDL_CreateWindow("Video test",640,400,0);
    assert(window);
    assert(SDL_SetWindowFullscreen(window,true));
    renderer=SDL_CreateRenderer(window,"software");
    assert(renderer);
    assert(SDL_SetRenderVSync(renderer,0));
    int outputWidth=0, outputHeight=0;
    assert(SDL_GetRenderOutputSize(renderer,&outputWidth,&outputHeight));
    UserSettings syncSettings;
    PortMenuState sync(true,syncSettings,renderer);
    openVideo(sync);
    int interval=-99;
    assert(SDL_GetRenderVSync(renderer,&interval));
    assert(sync.label(5)==(interval?"VSYNC : ON":"VSYNC : OFF"));
    assert(sync.pointerMove(320.f,294.f) && sync.selection()==5);
    assert(!sync.pointerMove(320.f,313.f)); // Gap before BACK.
    sync.key(SDL_SCANCODE_RIGHT);
    assert(SDL_GetRenderVSync(renderer,&interval) && interval==1);
    assert(sync.label(5)=="VSYNC : ON" && syncSettings.vsync);
    assert(sync.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(sync.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::None);
    assert(sync.key(SDL_SCANCODE_ESCAPE)==PortMenuChoice::Resume);
    PortMenuState reopened(true,syncSettings,renderer);
    openVideo(reopened);
    assert(reopened.label(5)=="VSYNC : ON");
    assert(reopened.pointerClick(320.f,294.f)==PortMenuChoice::None);
    assert(SDL_GetRenderVSync(renderer,&interval) && interval==0);
    assert(reopened.label(5)=="VSYNC : OFF");
    assert(reopened.label(2)=="FISHEYE EFFECT : OFF" && !syncSettings.vsync);
    assert(reopened.label(3)=="FLOOR AND CEILING : ON");
    assert(reopened.videoDimensions()==std::make_pair(320,200));
    int width=0, height=0;
    assert(SDL_GetRenderOutputSize(renderer,&width,&height));
    assert((SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0);
    assert(width==outputWidth && height==outputHeight);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
    persistence(argv[1]);
    SDL_Quit();
}
