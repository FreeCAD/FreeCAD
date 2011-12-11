
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Start
)
