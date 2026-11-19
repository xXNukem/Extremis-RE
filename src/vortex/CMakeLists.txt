add_library(vortex_cpu STATIC
    render/first_person/scene_renderer.cpp
    render/first_person/raycast/raycast.cpp
    render/first_person/walls/wall_raster.cpp
    render/first_person/postprocess/fisheye_effect.cpp
    render/first_person/planes/plane_renderer.cpp
    render/first_person/planes/plane_material.cpp
    render/first_person/sprites/sprite_renderer.cpp)
add_library(Vortex::CPU ALIAS vortex_cpu)

target_include_directories(vortex_cpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
target_compile_features(vortex_cpu PUBLIC cxx_std_17)
set_target_properties(vortex_cpu PROPERTIES CXX_EXTENSIONS NO)

add_library(vortex_audio STATIC audio/mixer.cpp)
add_library(Vortex::Audio ALIAS vortex_audio)
target_include_directories(vortex_audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
target_compile_features(vortex_audio PUBLIC cxx_std_17)
set_target_properties(vortex_audio PROPERTIES CXX_EXTENSIONS NO)

if(TARGET SDL3::SDL3)
    add_library(vortex_audio_sdl STATIC audio/sdl_player.cpp)
    add_library(Vortex::AudioSDL ALIAS vortex_audio_sdl)
    target_link_libraries(vortex_audio_sdl PUBLIC Vortex::Audio SDL3::SDL3)
    set_target_properties(vortex_audio_sdl PROPERTIES CXX_EXTENSIONS NO)
endif()
