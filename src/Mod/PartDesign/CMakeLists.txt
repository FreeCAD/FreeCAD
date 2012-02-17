
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        __init__.py
        Init.py
        InitGui.py
        TestPartDesignApp.py
        TestPartDesignGui.py
    DESTINATION
        Mod/PartDesign
)

INSTALL(
    FILES
        Scripts/__init__.py
        Scripts/Gear.py
        Scripts/DistanceBolt.py
        Scripts/Epitrochoid.py
        Scripts/RadialCopy.py
        Scripts/Parallelepiped.py
        Scripts/Spring.py
    DESTINATION
        Mod/PartDesign/Scripts
)

