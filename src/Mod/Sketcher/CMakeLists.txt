
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

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
        ProfileLib/RegularPolygon.py
        ProfileLib/__init__.py
    DESTINATION
        Mod/Sketcher/ProfileLib
)
