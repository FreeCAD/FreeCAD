
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Start
)
