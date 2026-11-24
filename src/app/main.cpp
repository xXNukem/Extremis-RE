#include <filesystem>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "session/app_session.h"
#include "startup/startup_sequence.h"
#include "startup/original_data.h"
#include "app/startup/port_menu.h"
#include "platform/sdl/video/render_state.h"

int main(int argc, char* argv[])
{
    auto options=app::parseLaunchOptions(argc,argv);
    if (!options) return 1;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
    const auto* base=SDL_GetBasePath();
    const auto executableDirectory=base ? std::filesystem::u8path(base) : std::filesystem::path{};
    std::string detectedPath;
    if (!options->labPath && !executableDirectory.empty()) {
        SDL_Log("Original data search base: %s",executableDirectory.u8string().c_str());
        const auto found=app::findOriginalData(executableDirectory);
        if (!found.empty()) {
            detectedPath=found.u8string();
            options->labPath=detectedPath.c_str();
        }
    }
    std::string dataError;
    if (!options->labPath || !app::validateOriginalData(std::filesystem::u8path(options->labPath),dataError)) {
        const std::string message=options->labPath
            ? "The original In Extremis installation at:\n" + std::string(options->labPath) +
              "\nis invalid:\n" + dataError
            : "Extremis RE requires the original In Extremis files.\n"
              "Place GAME.LAB and EXTREMIS.EXE beside the executable or in a subfolder, "
              "with their original DATA folder beside GAME.LAB.\n"
              "For a macOS .app, the same layout is accepted beside the .app.";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Extremis RE: original data required",
                                 message.c_str(),nullptr);
        SDL_Log("%s",message.c_str());
        SDL_Quit();
        return 1;
    }

    app::AppSession session(options->vitalsDebug);
    if (!session.loadInitialLevel(*options)) {
        SDL_Quit();
        return 1;
    }

    const auto labDirectory=options->labPath ? std::filesystem::u8path(options->labPath).parent_path()
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
