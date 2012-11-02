# Change data dir from default ($(prefix)/share) to actual dir
datadir = $(prefix)/Mod/Plot

data_DATA = \
	Plot.py \
	InitGui.py \
	PlotGui.py

nobase_data_DATA = \
	resources/icons/Axes.svg \
	resources/icons/Grid.svg \
	resources/icons/Icon.svg \
	resources/icons/Labels.svg \
	resources/icons/Legend.svg \
	resources/icons/Positions.svg \
	resources/icons/Save.svg \
	resources/icons/Series.svg \
	resources/translations/Plot.ts \
	resources/translations/Plot.qm \
	resources/translations/Plot_es-ES.ts \
	resources/translations/Plot_es-ES.qm \
	plotAxes/__init__.py \
	plotAxes/TaskPanel.py \
	plotAxes/TaskPanel.ui \
	plotLabels/__init__.py \
	plotLabels/TaskPanel.py \
	plotLabels/TaskPanel.ui \
	plotPositions/__init__.py \
	plotPositions/TaskPanel.py \
	plotPositions/TaskPanel.ui \
	plotSave/__init__.py \
	plotSave/TaskPanel.py \
	plotSave/TaskPanel.ui \
	plotSeries/__init__.py \
	plotSeries/TaskPanel.py \
	plotSeries/TaskPanel.ui \
	plotUtils/__init__.py \
	plotUtils/Paths.py \
	plotUtils/Translator.py

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		$(nobase_data_DATA) \
		CMakeLists.txt \
		README \
		plot.dox

