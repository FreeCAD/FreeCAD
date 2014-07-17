
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        SketcherExample.py
        TestSketcherApp.py
        TestSketcherGui.py
        Profiles.py
    DESTINATION
        Mod/Sketcher
)

INSTALL(
    FILES
        ProfileLib/Hexagon.py
        ProfileLib/__init__.py
    DESTINATION
        Mod/Sketcher/ProfileLib
)
