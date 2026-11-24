add_library(game_lab STATIC
    game_lab.cpp)
target_include_directories(game_lab PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(game_lab PUBLIC cxx_std_17)
set_target_properties(game_lab PROPERTIES CXX_EXTENSIONS NO)

add_library(texture_bank STATIC texture_bank.cpp spr_format.cpp)
target_include_directories(texture_bank PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(texture_bank PUBLIC cxx_std_17)
set_target_properties(texture_bank PROPERTIES CXX_EXTENSIONS NO)

add_library(anm_format STATIC anm_format.cpp)
target_include_directories(anm_format PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(anm_format PUBLIC cxx_std_17)
set_target_properties(anm_format PROPERTIES CXX_EXTENSIONS NO)


add_library(vga_palette STATIC vga_palette.cpp)
target_include_directories(vga_palette PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(vga_palette PUBLIC cxx_std_17)
set_target_properties(vga_palette PROPERTIES CXX_EXTENSIONS NO)
target_link_libraries(texture_bank PRIVATE vga_palette)
target_link_libraries(anm_format PRIVATE vga_palette)
