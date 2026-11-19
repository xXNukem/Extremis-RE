add_library(doors STATIC door.cpp door_clock.cpp original_travel.cpp)
target_link_libraries(doors PUBLIC game_lab)
target_compile_features(doors PUBLIC cxx_std_17)
set_target_properties(doors PROPERTIES CXX_EXTENSIONS NO)

add_library(player_collision STATIC player_collision.cpp objet_collision.cpp)
target_link_libraries(player_collision PUBLIC game_lab)
target_compile_features(player_collision PUBLIC cxx_std_17)
set_target_properties(player_collision PROPERTIES CXX_EXTENSIONS NO)

add_library(door_control STATIC door_control.cpp)
target_link_libraries(door_control PUBLIC doors player_collision)
target_compile_features(door_control PUBLIC cxx_std_17)
set_target_properties(door_control PROPERTIES CXX_EXTENSIONS NO)
