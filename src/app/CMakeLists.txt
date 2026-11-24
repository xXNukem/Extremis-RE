set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

add_library(session_interfaces STATIC
    session/item_session.cpp
    session/item_animation.cpp
    session/map_session.cpp
    session/item_input.cpp)
target_compile_definitions(session_interfaces PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
target_link_libraries(session_interfaces PUBLIC survival_session original_hud door_control SDL3::SDL3)
target_compile_features(session_interfaces PUBLIC cxx_std_17)
set_target_properties(session_interfaces PROPERTIES CXX_EXTENSIONS NO)

add_library(victory_session STATIC session/victory_session.cpp)
target_include_directories(victory_session PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(victory_session PUBLIC cxx_std_17)
set_target_properties(victory_session PROPERTIES CXX_EXTENSIONS NO)

add_library(shuttle_session STATIC
    session/shuttle_session.cpp session/shuttle_prompt.cpp)
target_link_libraries(shuttle_session PUBLIC session_interfaces original_shuttle)

set(startup_credits_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/startup/startup_credits.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/startup/credits_audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/startup/credits_view.cpp)

set(startup_credits_sources "${startup_credits_sources}" PARENT_SCOPE)

add_library(port_menu STATIC startup/port_menu.cpp startup/port_menu_state.cpp startup/port_menu_view.cpp
    startup/user_settings.cpp ${PROJECT_SOURCE_DIR}/src/extremis/ui/original_font.cpp)
target_link_libraries(port_menu PUBLIC original_hud player_inventory SDL3::SDL3 PRIVATE audio_data extremis_render)
target_compile_features(port_menu PUBLIC cxx_std_17)
set_target_properties(port_menu PROPERTIES CXX_EXTENSIONS NO)

add_executable(InExtremisPort main.cpp
    startup/launch_options.cpp startup/original_data.cpp startup/initial_level.cpp startup/startup_sequence.cpp
    session/app_session.cpp session/destination_level.cpp session/session_cinematics.cpp
    session/session_frame.cpp session/session_travel.cpp session/session_bomb.cpp
    session/session_events.cpp session/session_death.cpp
    diagnostics/map_view.cpp
    ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/materials/texture_preview.cpp ${PROJECT_SOURCE_DIR}/src/extremis/render/first_person/materials/plane_appearance.cpp ${startup_credits_sources}
    ${PROJECT_SOURCE_DIR}/src/extremis/cinematics/original_intro_player.cpp)
target_sources(InExtremisPort PRIVATE
    $<$<CONFIG:Debug>:diagnostics/debug_menu.cpp>
    $<$<CONFIG:Debug>:diagnostics/gameplay_fps.cpp>)
target_compile_definitions(InExtremisPort PRIVATE $<$<CONFIG:Debug>:EXTREMIS_DEBUG_MENU=1>)
set_target_properties(InExtremisPort PROPERTIES
    OUTPUT_NAME "EXTREMIS-RE"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
if(APPLE AND EXTREMIS_MACOS_BUNDLE)
    target_sources(InExtremisPort PRIVATE "${PROJECT_SOURCE_DIR}/assets/branding/extremis-re.icns")
    set_source_files_properties("${PROJECT_SOURCE_DIR}/assets/branding/extremis-re.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(InExtremisPort PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.extremisre.game"
        MACOSX_BUNDLE_BUNDLE_NAME "Extremis RE"
        MACOSX_BUNDLE_ICON_FILE "extremis-re.icns")
elseif(WIN32)
    set_target_properties(InExtremisPort PROPERTIES WIN32_EXECUTABLE TRUE)
    target_include_directories(InExtremisPort PRIVATE "${PROJECT_SOURCE_DIR}/assets/branding")
    target_sources(InExtremisPort PRIVATE "${PROJECT_SOURCE_DIR}/assets/branding/extremis-re.rc")
endif()
target_link_libraries(InExtremisPort PRIVATE
    shuttle_session level22_communication victory_session
    original_ending_player original_elevator original_intro
    port_menu extremis_render weapon_fire enemies session_interfaces player_control wall_animation
    wall_material sprites Vortex::CPU SDL3::SDL3 sdl_input sdl_audio)
