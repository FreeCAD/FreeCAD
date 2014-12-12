# Change data dir from default ($(prefix)/share) to actual dir
datadir = $(prefix)/Mod/Ship

data_DATA = \
	InitGui.py \
	ShipGui.py \
	Instance.py \
	Ship_rc.py \
	TankInstance.py \
	WeightInstance.py

nobase_data_DATA = \
	resources/examples/s60.fcstd \
	resources/examples/s60_katamaran.fcstd \
	resources/examples/wigley.fcstd \
	resources/examples/wigley_katamaran.fcstd \
	shipLoadExample/__init__.py \
	shipLoadExample/TaskPanel.py \
	shipLoadExample/TaskPanel.ui \
	shipCreateShip/__init__.py \
	shipCreateShip/Preview.py \
	shipCreateShip/TaskPanel.py \
	shipCreateShip/TaskPanel.ui \
	shipOutlineDraw/__init__.py \
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
	shipCreateWeight/__init__.py \
	shipCreateWeight/TaskPanel.py \
	shipCreateWeight/TaskPanel.ui \
	shipCreateTank/__init__.py \
	shipCreateTank/TaskPanel.py \
	shipCreateTank/TaskPanel.ui \
	shipCapacityCurve/__init__.py \
	shipCapacityCurve/PlotAux.py \
	shipCapacityCurve/TaskPanel.py \
	shipCapacityCurve/TaskPanel.ui \
	shipUtils/__init__.py \
	shipUtils/Locale.py \
	shipUtils/Math.py \
	shipUtils/Paths.py \
	shipUtils/Units.py

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		$(nobase_data_DATA) \
		CMakeLists.txt \
		README \
		ship.dox

