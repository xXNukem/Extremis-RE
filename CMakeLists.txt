cmake_minimum_required(VERSION 3.16)

project(InExtremisPort LANGUAGES CXX)

find_package(SDL3 3.2 REQUIRED CONFIG)

add_library(game_lab STATIC src/game_lab.cpp)
target_include_directories(game_lab PUBLIC include)
target_compile_features(game_lab PUBLIC cxx_std_17)
set_target_properties(game_lab PROPERTIES CXX_EXTENSIONS NO)

add_library(texture_bank STATIC src/texture_bank.cpp)
target_include_directories(texture_bank PUBLIC include)
target_compile_features(texture_bank PUBLIC cxx_std_17)
set_target_properties(texture_bank PROPERTIES CXX_EXTENSIONS NO)

add_library(wall_material STATIC src/wall_material.cpp)
target_include_directories(wall_material PUBLIC include)
target_compile_features(wall_material PUBLIC cxx_std_17)
set_target_properties(wall_material PROPERTIES CXX_EXTENSIONS NO)

add_executable(InExtremisPort src/main.cpp src/map_view.cpp src/first_person_view.cpp
    src/texture_preview.cpp)
set_target_properties(InExtremisPort PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_link_libraries(InExtremisPort PRIVATE game_lab texture_bank wall_material SDL3::SDL3)

add_executable(inspect_wall_materials EXCLUDE_FROM_ALL tools/inspect_wall_materials.cpp)
target_link_libraries(inspect_wall_materials PRIVATE game_lab)

include(CTest)
if(BUILD_TESTING)
    add_executable(game_lab_tests tests/game_lab_tests.cpp)
    target_link_libraries(game_lab_tests PRIVATE game_lab)
    add_test(NAME game_lab_reader COMMAND game_lab_tests
        "${CMAKE_CURRENT_BINARY_DIR}/synthetic_game.lab")
    add_executable(first_person_tests tests/first_person_tests.cpp src/first_person_view.cpp)
    target_link_libraries(first_person_tests PRIVATE game_lab wall_material SDL3::SDL3)
    add_test(NAME first_person_renderer COMMAND first_person_tests)
    add_executable(texture_bank_tests tests/texture_bank_tests.cpp src/texture_preview.cpp)
    target_link_libraries(texture_bank_tests PRIVATE texture_bank)
    add_test(NAME texture_bank_reader COMMAND texture_bank_tests
        "${CMAKE_CURRENT_BINARY_DIR}/texture-test-data")
endif()
