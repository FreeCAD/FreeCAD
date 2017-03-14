
add_subdirectory(App)
if(BUILD_GUI)
    SET(StartPage_Scripts
        StartPage.py
        TranslationTexts.py
        __init__.py
    )

    SET(StartPage_Resources
        ArchDesign.py
        DefaultWorkbench.py
        LoadDrawingExample.py
        LoadPartDesignExample.py
        LoadRobotExample.py
        LoadSchenkel.py
        LoadMRU0.py
        LoadMRU1.py
        LoadMRU2.py
        LoadMRU3.py
        LoadArchExample.py
        LoadFemExample2D.py
        LoadFemExample3D.py
        Mesh.py
        PartDesign.py
        Ship.py
        StartPage.css
        StartPage.js
        StartPage.html
    )

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
