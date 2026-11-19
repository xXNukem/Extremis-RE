#include "launch_options.h"

#include <iostream>
#include <string_view>

namespace app {
std::optional<LaunchOptions> parseLaunchOptions(int argc, char* argv[])
{
#ifdef EXTREMIS_DEBUG_MENU
    const bool communicationDebug = argc == 3 && std::string_view(argv[2]) == "--communication-debug";
    const bool shuttleDebug = argc == 3 && std::string_view(argv[2]) == "--shuttle-debug";
    const bool vitalsDebug = argc == 3 && std::string_view(argv[2]) == "--vitals-debug";
    const bool destructionDebug = argc == 3 && std::string_view(argv[2]) == "--self-destruction-debug";
#else
    const bool communicationDebug = false;
    const bool shuttleDebug = false;
    const bool vitalsDebug = false;
    const bool destructionDebug = false;
#endif
    if (argc > 3 || (argc == 3 && !vitalsDebug && !destructionDebug && !shuttleDebug && !communicationDebug)) {
        std::cerr << "Usage: " << argv[0] << " [path/to/GAME.LAB"
#ifdef EXTREMIS_DEBUG_MENU
                  << " [--vitals-debug|--self-destruction-debug|--shuttle-debug|--communication-debug]"
#endif
                  << "]\n";
        return std::nullopt;
    }
    return LaunchOptions{argc >= 2 ? argv[1] : nullptr, communicationDebug,
                         shuttleDebug, vitalsDebug, destructionDebug};
}
}
