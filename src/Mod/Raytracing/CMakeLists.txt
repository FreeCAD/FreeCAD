
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        RaytracingExample.py
    DESTINATION
        Mod/Raytracing
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Raytracing
    FILES_MATCHING PATTERN "*.pov*" PATTERN "*.lxs*"
)

