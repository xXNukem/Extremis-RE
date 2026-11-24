#include <filesystem>
#include "startup_sequence.h"

#include "extremis/cinematics/original_intro_player.h"
#include "app/startup/port_menu.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>

namespace app {
StartupCreditsResult playStartupSequence(SDL_Window* window, SDL_Renderer* renderer,
    const LaunchOptions& options, const std::filesystem::path& dataDirectory,
    bool hasLevel, UserSettings& settings, const std::filesystem::path& settingsPath, int& exitCode,
    const continuation::Checkpoint*& checkpoint)
{
    auto credits=options.communicationDebug?StartupCreditsResult::SkipAll:
                 showStartupCredits(window,renderer,dataDirectory,settings.audioGain());
    if (credits==StartupCreditsResult::Error) {
        SDL_Log("Startup credits failed: %s",SDL_GetError());
        exitCode=1;
    }
    // Diagnostic launchers keep their previous direct-to-game / intro behaviour.
    const bool diagnosticStartup=options.communicationDebug || options.shuttleDebug || options.vitalsDebug || options.destructionDebug;
    if ((credits==StartupCreditsResult::Continue || credits==StartupCreditsResult::SkipAll) &&
        hasLevel && !diagnosticStartup) {
        const auto menu=showPortMenu(window,renderer,dataDirectory,false,settings,settingsPath);
        if (menu.choice==PortMenuChoice::Continue) {
            checkpoint=menu.checkpoint;
            return StartupCreditsResult::Continue;
        }
        const auto choice=menu.choice;
        if (choice==PortMenuChoice::NewGame) credits=StartupCreditsResult::Continue;
        else {
            if (choice==PortMenuChoice::Error) exitCode=1;
            credits=choice==PortMenuChoice::Error ? StartupCreditsResult::Error : StartupCreditsResult::Quit;
        }
    }
    // The DOS opening plays only after NEW GAME (unless running a diagnostic).
    if (credits==StartupCreditsResult::Continue && hasLevel) {
        credits=showOriginalPresentation(window,renderer,dataDirectory);
        if(credits==StartupCreditsResult::Error) exitCode=1;
    }
    if (credits==StartupCreditsResult::Continue && hasLevel) {
        credits=showOriginalIntro(window,renderer,dataDirectory,settings.audioGain());
        if(credits==StartupCreditsResult::Error) exitCode=1;
    }
    return credits;
}
}
