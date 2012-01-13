SET(ShipMain_SRCS
	InitGui.py
	ShipGui.py
    Instance.py
)
SOURCE_GROUP("" FILES ${ShipMain_SRCS})

SET(ShipIcons_SRCS
	Icons/AreaCurveIco.png
	Icons/AreaCurveIco.xcf
	Icons/AreaCurveIco.xpm
	Icons/DataIco.png
	Icons/DataIco.xcf
	Icons/DataIco.xpm
	Icons/DiscretizeIco.png
	Icons/DiscretizeIco.xcf
	Icons/DiscretizeIco.xpm
	Icons/HydrostaticsIco.png
	Icons/HydrostaticsIco.xcf
	Icons/HydrostaticsIco.xpm
	Icons/Ico.png
	Icons/Ico.xcf
	Icons/Ico.xpm
	Icons/OutlineDrawIco.png
	Icons/OutlineDrawIco.xcf
	Icons/OutlineDrawIco.xpm
	Icons/ReparametrizeIco.png
	Icons/ReparametrizeIco.xcf
	Icons/ReparametrizeIco.xpm
	Icons/Ship.xcf
	Icons/Ship.xpm
)
SOURCE_GROUP("shipicons" FILES ${ShipIcons_SRCS})

SET(ShipCreateShip_SRCS
	shipCreateShip/__init__.py
	shipCreateShip/Preview.py
	shipCreateShip/TaskPanel.py
	shipCreateShip/TaskPanel.ui
)
SOURCE_GROUP("shipcreateship" FILES ${ShipCreateShip_SRCS})

SET(ShipOutlineDraw_SRCS
	shipOutlineDraw/__init__.py
	shipOutlineDraw/Plot.py
	shipOutlineDraw/Preview.py
	shipOutlineDraw/TaskPanel.py
	shipOutlineDraw/TaskPanel.ui
)
SOURCE_GROUP("shipoutlinedraw" FILES ${ShipOutlineDraw_SRCS})

SET(ShipAreasCurve_SRCS
	shipAreasCurve/__init__.py
	shipAreasCurve/Preview.py
	shipAreasCurve/TaskPanel.py
	shipAreasCurve/TaskPanel.ui
)
SOURCE_GROUP("shipareascurve" FILES ${ShipAreasCurve_SRCS})

SET(ShipUtils_SRCS
	shipUtils/__init__.py
	shipUtils/Math.py
	shipUtils/Paths.py
	shipUtils/Translator.py
)
SOURCE_GROUP("shiputils" FILES ${ShipUtils_SRCS})

SET(all_files ${ShipMain_SRCS} ${ShipIcons_SRCS} ${ShipCreateShip_SRCS} ${ShipOutlineDraw_SRCS} ${ShipAreasCurve_SRCS} ${ShipUtils_SRCS})

ADD_CUSTOM_TARGET(Ship ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Ship" "Ship" ${all_files})

INSTALL(
    FILES
        ${ShipIcons_SRCS}
    DESTINATION
        Mod/Ship/Icons
)
INSTALL(
    FILES
        ${ShipCreateShip_SRCS}
    DESTINATION
        Mod/Ship/shipCreateShip
)
INSTALL(
    FILES
        ${ShipOutlineDraw_SRCS}
    DESTINATION
        Mod/Ship/shipOutlineDraw
)
INSTALL(
    FILES
        ${ShipAreasCurve_SRCS}
    DESTINATION
        Mod/Ship/shipAreasCurve
)
INSTALL(
    FILES
        ${ShipUtils_SRCS}
    DESTINATION
        Mod/Ship/shipUtils
)
INSTALL(
    FILES
        ${ShipMain_SRCS}
    DESTINATION
        Mod/Ship
)

