
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Inspection_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Inspection_Scripts InitGui.py)
endif(BUILD_GUI)

INSTALL(
    FILES
        ${Inspection_Scripts}
    DESTINATION
        Mod/Inspection
)
