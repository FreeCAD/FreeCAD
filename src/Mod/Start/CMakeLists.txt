add_subdirectory(App)

set(Start_Scripts
    Init.py
)

if(BUILD_GUI)
    list(APPEND Start_Scripts InitGui.py)

    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(BUILD_GUI)

add_custom_target(StartScripts ALL
    SOURCES ${Start_Scripts}
)

fc_target_copy_resource(StartScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Start
    ${Start_Scripts})

INSTALL(
    FILES
        ${Start_Scripts}
    DESTINATION
        Mod/Start
)
