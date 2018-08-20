
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

INSTALL(
    FILES
        ${Points_Scripts}
    DESTINATION
        Mod/Points
)
