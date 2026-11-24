cmake_minimum_required(VERSION 3.16)

project(InExtremisPort LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

option(EXTREMIS_MACOS_BUNDLE "Build the Finder-launchable macOS distribution" OFF)

option(VORTEX_CPU_ONLY "Build Vortex CPU without the SDL port" OFF)
if(NOT VORTEX_CPU_ONLY)
    find_package(SDL3 3.2 REQUIRED CONFIG)
endif()
add_subdirectory(external/Vortex-Engine/src/vortex)

# Public Vortex headers used throughout Extremis RE.
include_directories("${PROJECT_SOURCE_DIR}/external/Vortex-Engine/include")

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(external/Vortex-Engine/tests/vortex)
endif()

if(VORTEX_CPU_ONLY)
    return()
endif()

# Keep UTF-8 literals in the port's localization sources intact under MSVC.
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

# The port keeps its archive output paths for diagnostic consumers.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
add_subdirectory(src/platform/sdl)
add_subdirectory(src/extremis)
add_subdirectory(src/app)

add_subdirectory(tools)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS InExtremisPort RUNTIME DESTINATION . BUNDLE DESTINATION .)
if(APPLE AND EXTREMIS_MACOS_BUNDLE)
    install(CODE "include(BundleUtilities)\nget_filename_component(_extremis_bundle \"\${CMAKE_INSTALL_PREFIX}/EXTREMIS-RE.app\" ABSOLUTE)\nfixup_bundle(\"\${_extremis_bundle}\" \"\" \"$<TARGET_FILE_DIR:SDL3::SDL3>\")\nexecute_process(COMMAND codesign --force --deep --sign - \"\${_extremis_bundle}\" RESULT_VARIABLE _sign_result)\nif(NOT _sign_result EQUAL 0)\n  message(FATAL_ERROR \"Could not sign the macOS app bundle\")\nendif()")
elseif(WIN32)
    install(FILES $<TARGET_FILE:SDL3::SDL3> DESTINATION .)
endif()
install(FILES docs/distribution.md DESTINATION . RENAME README.txt)
set(CPACK_GENERATOR ZIP)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
if(APPLE)
    set(CPACK_PACKAGE_FILE_NAME "Extremis-RE-macOS")
elseif(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "Extremis-RE-Windows")
endif()
include(CPack)
