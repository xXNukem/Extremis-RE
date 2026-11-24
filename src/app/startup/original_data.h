#pragma once

#include <filesystem>
#include <string>

namespace app {
// Validates an original installation without changing its files.
bool validateOriginalData(const std::filesystem::path& lab, std::string& error);
// Searches only the executable directory (and, for .app, its containing directory).
std::filesystem::path findOriginalData(const std::filesystem::path& executableDirectory);
}
