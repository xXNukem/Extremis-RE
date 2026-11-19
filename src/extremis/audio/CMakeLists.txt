add_library(audio_data STATIC ../resources/audio_data.cpp
    audio_mixer.cpp footstep_cadence.cpp)
target_include_directories(audio_data PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(audio_data PUBLIC Vortex::Audio)
target_compile_features(audio_data PUBLIC cxx_std_17)
set_target_properties(audio_data PROPERTIES CXX_EXTENSIONS NO)

add_library(game_audio STATIC game_audio.cpp)
target_include_directories(game_audio PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(game_audio PRIVATE sdl_audio doors)
target_compile_features(game_audio PUBLIC cxx_std_17)
set_target_properties(game_audio PROPERTIES CXX_EXTENSIONS NO)
