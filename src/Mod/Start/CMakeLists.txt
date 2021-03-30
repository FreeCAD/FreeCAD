add_subdirectory(App)

set(Start_Scripts
    Init.py
    TestStartGui.py
)

if(BUILD_GUI)
    list(APPEND Start_Scripts InitGui.py)

    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(BUILD_GUI)

set(Start_Tests
    TestStart/__init__.py
    TestStart/TestStartPage.py)

set(Start_Test_Files
    ${Start_Tests}
    ${Start_Scripts})

add_custom_target(StartScripts ALL
    SOURCES ${Start_Scripts}
)

add_custom_target(StartTests ALL
    SOURCES ${Start_Test_Files}
)

fc_target_copy_resource(StartScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Start
    ${Start_Scripts})

fc_copy_sources(StartTests "${CMAKE_BINARY_DIR}/Mod/Start" ${Start_Test_Files})


INSTALL(
    FILES
        ${Start_Scripts}
    DESTINATION
        Mod/Start
)

INSTALL(
    FILES
        ${Start_Tests}
    DESTINATION
        Mod/Start/TestStart
)
