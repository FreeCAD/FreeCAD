
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Robot_Scripts
    Init.py
    KukaExporter.py
    RobotExample.py
    RobotExampleTrajectoryOutOfShapes.py
)

if(BUILD_GUI)
    list (APPEND Robot_Scripts
        InitGui.py
        MovieTool.py
    )
endif(BUILD_GUI)

if (EXISTS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/Lib/Kuka)
SET(Robot_Resources
    Lib/Kuka/kr500_1.wrl
    Lib/Kuka/kr500_1.csv
    Lib/Kuka/kr_500_2.pdf
    Lib/Kuka/kr210.WRL
    Lib/Kuka/kr_210_2.csv
    Lib/Kuka/kr_210_2.pdf
    Lib/Kuka/kr16.wrl
    Lib/Kuka/kr_16.csv
    Lib/Kuka/kr_16.pdf
    Lib/Kuka/kr125_3.wrl
    Lib/Kuka/kr_125.csv
    Lib/Kuka/kr125_2.pdf
)
endif ()

add_custom_target(RobotScripts ALL
    SOURCES ${Robot_Scripts} ${Robot_Resources}
)

fc_target_copy_resource(RobotScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Robot
    ${Robot_Scripts})

fc_target_copy_resource(RobotScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Robot
    ${Robot_Resources})

INSTALL(
    FILES
        ${Robot_Scripts}
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
