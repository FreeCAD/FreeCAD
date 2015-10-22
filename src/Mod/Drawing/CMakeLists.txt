
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        DrawingAlgos.py
        DrawingExample.py
        DrawingTests.py
        DrawingPatterns.py
    DESTINATION
        Mod/Drawing
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Drawing
    FILES_MATCHING 
        PATTERN "*.svg*"
        PATTERN "*.dxf*"
)
