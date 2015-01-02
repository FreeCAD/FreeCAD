PYSIDE_WRAP_RC(Ship_QRC_SRCS resources/Ship.qrc)

SET(ShipMain_SRCS
    InitGui.py
    ShipGui.py
    Instance.py
    TankInstance.py
    WeightInstance.py
)
SOURCE_GROUP("" FILES ${ShipMain_SRCS})

SET(ShipExamples_SRCS
	resources/examples/s60.fcstd
	resources/examples/s60_katamaran.fcstd
	resources/examples/wigley.fcstd
	resources/examples/wigley_katamaran.fcstd
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
	shipOutlineDraw/Preview.py
	shipOutlineDraw/TaskPanel.py
	shipOutlineDraw/TaskPanel.ui
)
SOURCE_GROUP("shipoutlinedraw" FILES ${ShipOutlineDraw_SRCS})

SET(ShipAreasCurve_SRCS
	shipAreasCurve/__init__.py
	shipAreasCurve/PlotAux.py
	shipAreasCurve/Preview.py
	shipAreasCurve/TaskPanel.py
	shipAreasCurve/TaskPanel.ui
)
SOURCE_GROUP("shipareascurve" FILES ${ShipAreasCurve_SRCS})

SET(ShipHydrostatics_SRCS
	shipHydrostatics/__init__.py
	shipHydrostatics/PlotAux.py
	shipHydrostatics/TaskPanel.py
	shipHydrostatics/TaskPanel.ui
	shipHydrostatics/Tools.py
)
SOURCE_GROUP("shiphydrostatics" FILES ${ShipHydrostatics_SRCS})

SET(ShipCreateWeight_SRCS
	shipCreateWeight/__init__.py
	shipCreateWeight/TaskPanel.py
	shipCreateWeight/TaskPanel.ui
)
SOURCE_GROUP("shipcreateweight" FILES ${ShipCreateWeight_SRCS})

SET(ShipCreateTank_SRCS
	shipCreateTank/__init__.py
	shipCreateTank/TaskPanel.py
	shipCreateTank/TaskPanel.ui
)
SOURCE_GROUP("shipcreatetank" FILES ${ShipCreateTank_SRCS})

SET(ShipCapacityCurve_SRCS
	shipCapacityCurve/__init__.py
	shipCapacityCurve/PlotAux.py
	shipCapacityCurve/TaskPanel.py
	shipCapacityCurve/TaskPanel.ui
)
SOURCE_GROUP("shipcapacitycurve" FILES ${ShipCapacityCurve_SRCS})

SET(ShipUtils_SRCS
	shipUtils/__init__.py
	shipUtils/Locale.py
	shipUtils/Math.py
	shipUtils/Paths.py
	shipUtils/Units.py
)
SOURCE_GROUP("shiputils" FILES ${ShipUtils_SRCS})

SET(all_files ${ShipMain_SRCS} ${ShipExamples_SRCS} ${ShipLoadExample_SRCS} ${ShipCreateShip_SRCS} ${ShipOutlineDraw_SRCS} ${ShipAreasCurve_SRCS} ${ShipHydrostatics_SRCS} ${ShipCreateWeight_SRCS} ${ShipCreateTank_SRCS} ${ShipCapacityCurve_SRCS} ${ShipUtils_SRCS})

ADD_CUSTOM_TARGET(Ship ALL
    SOURCES ${all_files} ${Ship_QRC_SRCS}
)

fc_copy_sources(Ship "${CMAKE_BINARY_DIR}/Mod/Ship" ${all_files})

fc_target_copy_resource(Ship
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Ship
    Ship_rc.py)

INSTALL(
    FILES
        ${ShipExamples_SRCS}
    DESTINATION
        Mod/Ship/resources/examples
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
        ${ShipCreateWeight_SRCS}
    DESTINATION
        Mod/Ship/shipCreateWeight
)
INSTALL(
    FILES
        ${ShipCreateTank_SRCS}
    DESTINATION
        Mod/Ship/shipCreateTank
)
INSTALL(
    FILES
        ${ShipCapacityCurve_SRCS}
    DESTINATION
        Mod/Ship/shipCapacityCurve
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
        ${Ship_QRC_SRCS}
    DESTINATION
        Mod/Ship
)


