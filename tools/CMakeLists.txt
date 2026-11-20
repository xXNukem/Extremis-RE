set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

add_executable(elevator_preview elevator_preview.cpp)
target_link_libraries(elevator_preview PRIVATE original_elevator)

add_executable(inspect_wall_materials EXCLUDE_FROM_ALL inspect_wall_materials.cpp)
target_link_libraries(inspect_wall_materials PRIVATE game_lab)

add_executable(audio_probe EXCLUDE_FROM_ALL audio_probe.cpp)
target_link_libraries(audio_probe PRIVATE sdl_audio)

# Diagnostic only; production rendering and its public interfaces stay unchanged.
add_executable(render_benchmark EXCLUDE_FROM_ALL render_benchmark.cpp ${PROJECT_SOURCE_DIR}/src/app/diagnostics/player_debug.cpp)
target_include_directories(render_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/scene
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/materials)
target_compile_features(render_benchmark PRIVATE cxx_std_17)
set_target_properties(render_benchmark PROPERTIES CXX_EXTENSIONS NO)
target_link_libraries(render_benchmark PRIVATE extremis_render_diagnostics Vortex::CPU wall_material doors sprites original_hud SDL3::SDL3)

add_executable(raycast_benchmark raycast_benchmark.cpp)
target_include_directories(raycast_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/scene)
target_compile_features(raycast_benchmark PRIVATE cxx_std_17)
set_target_properties(raycast_benchmark PROPERTIES CXX_EXTENSIONS NO)
target_link_libraries(raycast_benchmark PRIVATE extremis_render_diagnostics Vortex::CPU wall_material doors sprites original_hud SDL3::SDL3)

# Bounded resource validation; no gameplay sessions.
add_executable(check_level_banks EXCLUDE_FROM_ALL check_level_banks.cpp)
target_link_libraries(check_level_banks PRIVATE enemies)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(shuttle_preview shuttle_preview.cpp)
    target_link_libraries(shuttle_preview PRIVATE original_shuttle)
endif()

add_executable(level_render_benchmark EXCLUDE_FROM_ALL level_render_benchmark.cpp)
target_include_directories(level_render_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/presentation
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/scene)
target_compile_definitions(level_render_benchmark PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
target_link_libraries(level_render_benchmark PRIVATE extremis_render enemies)
