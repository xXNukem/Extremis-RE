#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "session/app_session.h"
#include "startup/startup_sequence.h"
#include "app/startup/port_menu.h"
#include "platform/sdl/video/render_state.h"

int main(int argc, char* argv[])
{
    const auto options=app::parseLaunchOptions(argc,argv);
    if (!options) return 1;

    app::AppSession session(options->vitalsDebug);
    if (!session.loadInitialLevel(*options)) return 1;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    const auto labDirectory=options->labPath ? std::filesystem::path(options->labPath).parent_path()
                                             : std::filesystem::path{};
    const auto settingsPath=userSettingsPath();
    auto settings=loadUserSettings(settingsPath,options->labPath ? labDirectory/"EXTREMIS.CFG"
                                                                 : std::filesystem::path{});
    localization::bind(&settings.language);
    SDL_Window* window = SDL_CreateWindow(session.windowTitle().c_str(), 960, 600,
                                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#ifdef SDL_PLATFORM_MACOS
    platform::sdl::configureMetalFullscreenPresentation(renderer);
#endif

    const auto savedMode=settings.render.mode;
    initializePortVideo(window,renderer,settings);
    if (savedMode.nativeMax!=settings.render.mode.nativeMax ||
        savedMode.renderScale!=settings.render.mode.renderScale)
        saveUserSettings(settingsPath,settings);

    int exitCode = 0;
    const auto dataDirectory=options->labPath ? labDirectory/"DATA" : std::filesystem::path{};
    const continuation::Checkpoint* checkpoint=nullptr;
    const auto credits=app::playStartupSequence(window,renderer,*options,dataDirectory,
                                                session.hasLevel(),settings,settingsPath,exitCode,checkpoint);
    if (credits==StartupCreditsResult::Continue || credits==StartupCreditsResult::SkipAll)
        session.run(window,renderer,*options,dataDirectory,settings,settingsPath,exitCode,checkpoint);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exitCode;
}
