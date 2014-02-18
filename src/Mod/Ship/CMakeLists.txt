SET(ShipMain_SRCS
	InitGui.py
	ShipGui.py
    Instance.py
    SimInstance.py
    TankInstance.py
	Ship_rc.py
)
SOURCE_GROUP("" FILES ${ShipMain_SRCS})

SET(ShipExamples_SRCS
	resources/examples/s60.fcstd
	resources/examples/s60_katamaran.fcstd
	resources/examples/wigley.fcstd
	resources/examples/wigley_katamaran.fcstd
)
SOURCE_GROUP("shipexamples" FILES ${ShipExamples_SRCS})

SET(ShipOpenCL_SRCS
	resources/opencl/matrixGen.cl
	resources/opencl/jacobi.cl
	resources/opencl/minres.cl
	resources/opencl/lsqr.cl
)
SOURCE_GROUP("shipopencl" FILES ${ShipOpenCL_SRCS})

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

SET(ShipUtils_SRCS
	shipUtils/__init__.py
	shipUtils/Math.py
	shipUtils/Paths.py
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
	tankGZ/PlotAux.py
	tankGZ/TaskPanel.py
	tankGZ/TaskPanel.ui
)
SOURCE_GROUP("shipgz" FILES ${ShipGZ_SRCS})

SET(SimCreate_SRCS
	simCreate/__init__.py
	simCreate/TaskPanel.py
	simCreate/TaskPanel.ui
)
SOURCE_GROUP("simcreate" FILES ${SimCreate_SRCS})

SET(SimRun_SRCS
	simRun/__init__.py
	simRun/Simulation.py
	simRun/TaskPanel.py
	simRun/TaskPanel.ui
	simRun/clSim/__init__.py
	simRun/clSim/initialization.py
	simRun/clSim/matrixGen.py
	simRun/clSim/BEMsolver.py
	simRun/clSim/evolution.py
	simRun/clSim/clUtils.py
	simRun/clSim/bem_jacobi_cl.py
	simRun/clSim/bem_minres_cl.py
	simRun/clSim/bem_lsqr_cl.py
	simRun/Sim/__init__.py
	simRun/Sim/initialization.py
	simRun/Sim/matrixGen.py
	simRun/Sim/BEMsolver.py
	simRun/Sim/evolution.py
)
SOURCE_GROUP("simrun" FILES ${SimRun_SRCS})

SET(SimPost_SRCS
	simPost/__init__.py
	simPost/TaskPanel.py
	simPost/TaskPanel.ui
)
SOURCE_GROUP("simpost" FILES ${SimPost_SRCS})

SET(all_files ${ShipMain_SRCS} ${ShipExamples_SRCS} ${ShipOpenCL_SRCS} ${ShipLoadExample_SRCS} ${ShipCreateShip_SRCS} ${ShipOutlineDraw_SRCS} ${ShipAreasCurve_SRCS} ${ShipHydrostatics_SRCS} ${ShipUtils_SRCS} ${ShipWeights_SRCS} ${ShipCreateTank_SRCS} ${ShipGZ_SRCS} ${SimCreate_SRCS} ${SimRun_SRCS} ${SimPost_SRCS})

ADD_CUSTOM_TARGET(Ship ALL
    SOURCES ${all_files}
)

fc_copy_sources(Mod/Ship "${CMAKE_BINARY_DIR}/Mod/Ship" ${all_files})

INSTALL(
    FILES
        ${ShipExamples_SRCS}
    DESTINATION
        Mod/Ship/resources/examples
)
INSTALL(
    FILES
        ${ShipOpenCL_SRCS}
    DESTINATION
        Mod/Ship/resources/opencl
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
        ${SimCreate_SRCS}
    DESTINATION
        Mod/Ship/simCreate
)
INSTALL(
    FILES
        ${SimRun_SRCS}
    DESTINATION
        Mod/Ship/simRun
)
INSTALL(
    FILES
        ${SimPost_SRCS}
    DESTINATION
        Mod/Ship/simPost
)
INSTALL(
    FILES
        ${ShipMain_SRCS}
    DESTINATION
        Mod/Ship
)


