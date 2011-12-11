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
)

INSTALL(FILES ${StartPage_SRCS} ${StartPage_DATA}
	DESTINATION Mod/Start/StartPage
)

