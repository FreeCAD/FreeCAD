
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        KukaExporter.py
        MovieTool.py
        RobotExample.py
        RobotExampleTrajectoryOutOfShapes.py
    DESTINATION
        Mod/Robot
)

INSTALL(
    DIRECTORY
        Lib
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Robot
    PATTERN "Makefile*" EXCLUDE
    PATTERN "*.pdf" EXCLUDE
)

