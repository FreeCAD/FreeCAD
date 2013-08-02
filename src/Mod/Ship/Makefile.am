# Change data dir from default ($(prefix)/share) to actual dir
datadir = $(prefix)/Mod/Ship

data_DATA = \
	InitGui.py \
	ShipGui.py \
	Instance.py \
	SimInstance.py \
	TankInstance.py \
	Ship_rc.py

nobase_data_DATA = \
	resources/examples/s60.fcstd \
	resources/examples/s60_katamaran.fcstd \
	resources/examples/wigley.fcstd \
	resources/examples/wigley_katamaran.fcstd \
	resources/icons/Ico.xpm \
	resources/opencl/matrixGen.cl \
	resources/opencl/jacobi.cl \
	resources/opencl/minres.cl \
	resources/opencl/lsqr.cl \
	shipLoadExample/__init__.py \
	shipLoadExample/TaskPanel.py \
	shipLoadExample/TaskPanel.ui \
	shipCreateShip/__init__.py \
	shipCreateShip/Preview.py \
	shipCreateShip/TaskPanel.py \
	shipCreateShip/TaskPanel.ui \
	shipOutlineDraw/__init__.py \
	shipOutlineDraw/Plot.py \
	shipOutlineDraw/Preview.py \
	shipOutlineDraw/TaskPanel.py \
	shipOutlineDraw/TaskPanel.ui \
	shipAreasCurve/__init__.py \
	shipAreasCurve/PlotAux.py \
	shipAreasCurve/Preview.py \
	shipAreasCurve/TaskPanel.py \
	shipAreasCurve/TaskPanel.ui \
	shipHydrostatics/__init__.py \
	shipHydrostatics/PlotAux.py \
	shipHydrostatics/TaskPanel.py \
	shipHydrostatics/TaskPanel.ui \
	shipHydrostatics/Tools.py \
	shipUtils/__init__.py \
	shipUtils/Math.py \
	shipUtils/Paths.py \
	tankWeights/__init__.py \
	tankWeights/Preview.py \
	tankWeights/TaskPanel.py \
	tankWeights/TaskPanel.ui \
	tankCreateTank/__init__.py \
	tankCreateTank/TaskPanel.py \
	tankCreateTank/TaskPanel.ui \
	tankGZ/__init__.py \
	tankGZ/PlotAux.py \
	tankGZ/TaskPanel.py \
	tankGZ/TaskPanel.ui \
	simCreate/__init__.py \
	simCreate/TaskPanel.py \
	simCreate/TaskPanel.ui \
	simRun/__init__.py \
	simRun/Simulation.py \
	simRun/TaskPanel.py \
	simRun/TaskPanel.ui \
	simRun/clSim/__init__.py \
	simRun/clSim/initialization.py \
	simRun/clSim/matrixGen.py \
	simRun/clSim/BEMsolver.py \
	simRun/clSim/evolution.py \
	simRun/clSim/clUtils.py \
	simRun/clSim/bem_jacobi_cl.py \
	simRun/clSim/bem_minres_cl.py \
	simRun/clSim/bem_lsqr_cl.py \
	simRun/Sim/__init__.py \
	simRun/Sim/initialization.py \
	simRun/Sim/matrixGen.py \
	simRun/Sim/BEMsolver.py \
	simRun/Sim/evolution.py \
	simPost/__init__.py \
	simPost/TaskPanel.py \
	simPost/TaskPanel.ui

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		$(nobase_data_DATA) \
		CMakeLists.txt \
		README \
		ship.dox

