
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        DrawingAlgos.py
        DrawingExample.py
        DrawingTests.py
    DESTINATION
        Mod/Drawing
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Drawing
    FILES_MATCHING PATTERN "*.svg*"
)
