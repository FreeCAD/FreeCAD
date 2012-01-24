# Change data dir from default ($(prefix)/share) to actual dir
datadir = $(prefix)/Mod/Surfaces

data_DATA = InitGui.py \
	SurfGui.py

nobase_data_DATA = \
	Icons/BorderIco.png \
	Icons/BorderIco.xcf \
	Icons/BorderIco.xpm \
	Icons/ConvertIco.png \
	Icons/ConvertIco.xcf \
	Icons/ConvertTo4SidesIco.xpm \
	Icons/Ico.png \
	Icons/Ico.xcf \
	Icons/Ico.xpm \
	Icons/IsoCurveIco.png \
	Icons/IsoCurveIco.xcf \
	Icons/IsoCurveIco.xpm \
	Icons/SliceIco.png \
	Icons/SliceIco.xcf \
	Icons/SliceIco.xpm \
	surfBorder/__init__.py \
	surfConvert/__init__.py \
	surfConvert/Preview.py \
	surfConvert/TaskPanel.py \
	surfConvert/TaskPanel.ui \
	surfISOCurve/__init__.py \
	surfISOCurve/PointTracker.py \
	surfISOCurve/Preview.py \
	surfISOCurve/TaskPanel.py \
	surfISOCurve/TaskPanel.ui \
	surfSlice/__init__.py \
	surfSlice/PointTracker.py \
	surfSlice/Preview.py \
	surfSlice/TaskPanel.py \
	surfSlice/TaskPanel.ui \
	surfUtils/Geometry.py \
	surfUtils/__init__.py \
	surfUtils/Math.py \
	surfUtils/Paths.py \
	surfUtils/Translator.py

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		$(nobase_data_DATA) \
		CMakeLists.txt \
		README \
		surfaces.dox

