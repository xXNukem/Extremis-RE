add_library(sprites STATIC
    ../resources/sprite_bank.cpp
    first_person/sprites/sprite_scene.cpp
    first_person/sprites/sprite_animation.cpp
    first_person/sprites/decorative_animation.cpp)
target_link_libraries(sprites PUBLIC texture_bank game_lab)
target_compile_features(sprites PUBLIC cxx_std_17)
set_target_properties(sprites PROPERTIES CXX_EXTENSIONS NO)

add_library(wall_animation STATIC first_person/materials/wall_animation.cpp)
target_link_libraries(wall_animation PUBLIC texture_bank)
target_compile_features(wall_animation PUBLIC cxx_std_17)
set_target_properties(wall_animation PROPERTIES CXX_EXTENSIONS NO)

add_library(wall_material STATIC first_person/materials/wall_material.cpp)
target_include_directories(wall_material PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(wall_material PUBLIC cxx_std_17)
set_target_properties(wall_material PROPERTIES CXX_EXTENSIONS NO)

set(extremis_render_sources
    first_person/camera/camera_start.cpp
    first_person/presentation/first_person_view.cpp
    first_person/presentation/first_person_presentation.cpp
    first_person/presentation/view_layout.cpp
    first_person/scene/ray_queries.cpp
    first_person/scene/ray_scene.cpp
    first_person/materials/wall_columns.cpp)
add_library(extremis_render STATIC ${extremis_render_sources})
# Keep the historical Debug profiling distinction between the port and diagnostics.
add_library(extremis_render_diagnostics STATIC ${extremis_render_sources})
target_compile_definitions(extremis_render PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
foreach(render_target IN ITEMS extremis_render extremis_render_diagnostics)
    target_link_libraries(${render_target} PUBLIC original_hud sdl_video Vortex::CPU
        PRIVATE wall_material doors player_control)
    target_compile_features(${render_target} PUBLIC cxx_std_17)
    set_target_properties(${render_target} PROPERTIES CXX_EXTENSIONS NO)
endforeach()
