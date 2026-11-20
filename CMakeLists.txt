cmake_minimum_required(VERSION 3.16)

project(InExtremisPort LANGUAGES CXX)

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

# The port keeps its archive output paths for diagnostic consumers.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
add_subdirectory(src/platform/sdl)
add_subdirectory(src/extremis)
add_subdirectory(src/app)

add_subdirectory(tools)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
