SET(ShipMain_SRCS
	InitGui.py
	ShipGui.py
    Instance.py
    TankInstance.py
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
	Icons/LoadIco.png
	Icons/LoadIco.xcf
	Icons/LoadIco.xpm
	Icons/OutlineDrawIco.png
	Icons/OutlineDrawIco.xcf
	Icons/OutlineDrawIco.xpm
	Icons/ReparametrizeIco.png
	Icons/ReparametrizeIco.xcf
	Icons/ReparametrizeIco.xpm
	Icons/Ship.xcf
	Icons/Ship.xpm
	Icons/Weight.png
	Icons/Weight.xcf
	Icons/Weight.xpm
	Icons/Tank.png
	Icons/Tank.xcf
	Icons/Tank.xpm
)
SOURCE_GROUP("shipicons" FILES ${ShipIcons_SRCS})

SET(ShipExamples_SRCS
	Examples/s60.fcstd
	Examples/s60_katamaran.fcstd
	Examples/wigley.fcstd
	Examples/wigley_katamaran.fcstd
)
SOURCE_GROUP("shipexamples" FILES ${ShipExamples_SRCS})

SET(ShipLoadExample_SRCS
	shipLoadExample/__init__.py
	shipLoadExample/TaskPanel.py
	shipLoadExample/TaskPanel.ui
)
SOURCE_GROUP("shiploadexample" FILES ${ShipLoadExample_SRCS})

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
	shipAreasCurve/Plot.py
	shipAreasCurve/Preview.py
	shipAreasCurve/TaskPanel.py
	shipAreasCurve/TaskPanel.ui
)
SOURCE_GROUP("shipareascurve" FILES ${ShipAreasCurve_SRCS})

SET(ShipHydrostatics_SRCS
	shipHydrostatics/__init__.py
	shipHydrostatics/Plot.py
	shipHydrostatics/TaskPanel.py
	shipHydrostatics/TaskPanel.ui
	shipHydrostatics/Tools.py
)
SOURCE_GROUP("shiphydrostatics" FILES ${ShipHydrostatics_SRCS})

SET(ShipUtils_SRCS
	shipUtils/__init__.py
	shipUtils/Math.py
	shipUtils/Paths.py
	shipUtils/Translator.py
)
SOURCE_GROUP("shiputils" FILES ${ShipUtils_SRCS})

SET(ShipWeights_SRCS
	tankWeights/__init__.py
	tankWeights/Preview.py
	tankWeights/TaskPanel.py
	tankWeights/TaskPanel.ui
)
SOURCE_GROUP("shipweights" FILES ${ShipWeights_SRCS})

SET(ShipCreateTank_SRCS
	tankCreateTank/__init__.py
	tankCreateTank/TaskPanel.py
	tankCreateTank/TaskPanel.ui
)
SOURCE_GROUP("shipcreatetank" FILES ${ShipCreateTank_SRCS})

SET(ShipGZ_SRCS
	tankGZ/__init__.py
	tankGZ/Plot.py
	tankGZ/TaskPanel.py
	tankGZ/TaskPanel.ui
)
SOURCE_GROUP("shipcreatetank" FILES ${ShipCreateTank_SRCS})

SET(all_files ${ShipMain_SRCS} ${ShipIcons_SRCS} ${ShipExamples_SRCS} ${ShipLoadExample_SRCS} ${ShipCreateShip_SRCS} ${ShipOutlineDraw_SRCS} ${ShipAreasCurve_SRCS} ${ShipHydrostatics_SRCS} ${ShipUtils_SRCS} ${ShipWeights_SRCS} ${ShipCreateTank_SRCS} ${ShipGZ_SRCS})

ADD_CUSTOM_TARGET(Ship ALL
    SOURCES ${all_files}
)

fc_copy_sources(Mod/Ship "${CMAKE_BINARY_DIR}/Mod/Ship" ${all_files})

INSTALL(
    FILES
        ${ShipIcons_SRCS}
    DESTINATION
        Mod/Ship/Icons
)
INSTALL(
    FILES
        ${ShipExamples_SRCS}
    DESTINATION
        Mod/Ship/Examples
)
INSTALL(
    FILES
        ${ShipLoadExample_SRCS}
    DESTINATION
        Mod/Ship/shipLoadExample
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
        ${ShipHydrostatics_SRCS}
    DESTINATION
        Mod/Ship/shipHydrostatics
)
INSTALL(
    FILES
        ${ShipUtils_SRCS}
    DESTINATION
        Mod/Ship/shipUtils
)
INSTALL(
    FILES
        ${ShipWeights_SRCS}
    DESTINATION
        Mod/Ship/tankWeights
)
INSTALL(
    FILES
        ${ShipCreateTank_SRCS}
    DESTINATION
        Mod/Ship/tankCreateTank
)
INSTALL(
    FILES
        ${ShipGZ_SRCS}
    DESTINATION
        Mod/Ship/tankGZ
)
INSTALL(
    FILES
        ${ShipMain_SRCS}
    DESTINATION
        Mod/Ship
)


