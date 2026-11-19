add_library(player_vitals STATIC player/player_vitals.cpp player/self_destruction.cpp)
target_compile_definitions(player_vitals PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
target_include_directories(player_vitals PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(player_vitals PUBLIC cxx_std_17)
set_target_properties(player_vitals PROPERTIES CXX_EXTENSIONS NO)

add_library(player_inventory STATIC player/player_inventory.cpp items/level_items.cpp)
target_link_libraries(player_inventory PUBLIC player_vitals player_collision)
target_compile_features(player_inventory PUBLIC cxx_std_17)
set_target_properties(player_inventory PROPERTIES CXX_EXTENSIONS NO)

add_library(survival_session STATIC
    survival_session.cpp
    ../ui/game_messages.cpp)
target_compile_definitions(survival_session PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
target_link_libraries(survival_session PUBLIC player_inventory game_audio PRIVATE SDL3::SDL3)
target_compile_features(survival_session PUBLIC cxx_std_17)
set_target_properties(survival_session PROPERTIES CXX_EXTENSIONS NO)

add_library(enemies STATIC
    enemies/enemy_system.cpp
    enemies/enemy_navigation.cpp
    enemies/enemy_mons5.cpp
    enemies/enemy_effects.cpp
    enemies/motion_detector.cpp)
target_link_libraries(enemies PUBLIC sprites player_collision player_vitals doors PRIVATE player_control door_control)
target_compile_features(enemies PUBLIC cxx_std_17)
set_target_properties(enemies PROPERTIES CXX_EXTENSIONS NO)

add_library(weapon_fire STATIC player/weapon_fire.cpp)
target_link_libraries(weapon_fire PUBLIC player_inventory sprites PRIVATE enemies)
target_compile_features(weapon_fire PUBLIC cxx_std_17)
set_target_properties(weapon_fire PROPERTIES CXX_EXTENSIONS NO)

add_library(player_control STATIC player/player_control.cpp player/camera.cpp)
target_include_directories(player_control PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(player_control PUBLIC cxx_std_17)
set_target_properties(player_control PROPERTIES CXX_EXTENSIONS NO)
target_link_libraries(player_control PUBLIC player_collision)
