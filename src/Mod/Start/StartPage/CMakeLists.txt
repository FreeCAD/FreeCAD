SET(StartPage_SRCS
    StartPage.py
    __init__.py
)

SET(StartPage_DATA 
    ArchDesign.py
    DefaultWorkbench.py
    LoadDrawingExample.py
    LoadPartDesignExample.py
    LoadRobotExample.py
    LoadSchenkel.py
    LoadMRU0.py
    LoadMRU1.py
    LoadMRU2.py
    LoadArchExample.py
    Mesh.py
    PartDesign.py
    Background.jpg
    FreeCAD.png
    PartDesign.png
    ArchDesign.png
    Mesh.png
    Complete.png
    PartDesignExample.png
    ArchExample.png
    web.png
    blank.png
    complete.jpg
    Ship.py
    Ship.png
    ShipExample.png
)

INSTALL(FILES ${StartPage_SRCS}
    DESTINATION Mod/Start/StartPage
)

INSTALL(FILES ${StartPage_DATA}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
)

