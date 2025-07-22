
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Points_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Points_Scripts InitGui.py)
endif(BUILD_GUI)

# ************************************************************************************************
# ****** Python non Gui packages and modules *****************************************************
# ************************************************************************************************

SET(PointsCommands_SRCS
    pointscommands/__init__.py
    pointscommands/commands.py
)

SET(PointsAllScripts
    ${PointsCommands_SRCS}
)

ADD_CUSTOM_TARGET(PointsScripts ALL
    SOURCES ${PointsAllScripts}
)
fc_copy_sources(PointsScripts "${CMAKE_BINARY_DIR}/Mod/Points" ${PointsAllScripts})

# install directories for Python packages
INSTALL(FILES ${Points_Scripts} DESTINATION Mod/Points)
INSTALL(FILES ${PointsCommands_SRCS} DESTINATION Mod/Points/pointscommands)
