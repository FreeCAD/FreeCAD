SET(PlotMain_SRCS
	Plot.py
	InitGui.py
	PlotGui.py
	Plot_rc.py
)
SOURCE_GROUP("" FILES ${PlotMain_SRCS})

SET(PlotAxes_SRCS
	plotAxes/__init__.py
	plotAxes/TaskPanel.py
	plotAxes/TaskPanel.ui
)
SOURCE_GROUP("plotaxes" FILES ${PlotAxes_SRCS})

SET(PlotLabels_SRCS
	plotLabels/__init__.py
	plotLabels/TaskPanel.py
	plotLabels/TaskPanel.ui
)
SOURCE_GROUP("plotlabels" FILES ${PlotLabels_SRCS})

SET(PlotPositions_SRCS
	plotPositions/__init__.py
	plotPositions/TaskPanel.py
	plotPositions/TaskPanel.ui
)
SOURCE_GROUP("plotpositions" FILES ${PlotPositions_SRCS})

SET(PlotSave_SRCS
	plotSave/__init__.py
	plotSave/TaskPanel.py
	plotSave/TaskPanel.ui
)
SOURCE_GROUP("plotsave" FILES ${PlotSave_SRCS})

SET(PlotSeries_SRCS
	plotSeries/__init__.py
	plotSeries/TaskPanel.py
	plotSeries/TaskPanel.ui
)
SOURCE_GROUP("plotseries" FILES ${PlotSeries_SRCS})

SET(PlotUtils_SRCS
	plotUtils/__init__.py
	plotUtils/Paths.py
)
SOURCE_GROUP("plotutils" FILES ${PlotUtils_SRCS})

SET(all_files ${PlotMain_SRCS} ${PlotAxes_SRCS} ${PlotLabels_SRCS} ${PlotPositions_SRCS} ${PlotSave_SRCS} ${PlotSeries_SRCS} ${PlotUtils_SRCS})

ADD_CUSTOM_TARGET(Plot ALL
    SOURCES ${all_files}
)

fc_copy_sources(Mod/Plot "${CMAKE_BINARY_DIR}/Mod/Plot" ${all_files})

INSTALL(
    FILES
        ${PlotAxes_SRCS}
    DESTINATION
        Mod/Plot/plotAxes
)
INSTALL(
    FILES
        ${PlotLabels_SRCS}
    DESTINATION
        Mod/Plot/plotLabels
)
INSTALL(
    FILES
        ${PlotPositions_SRCS}
    DESTINATION
        Mod/Plot/plotPositions
)
INSTALL(
    FILES
        ${PlotSave_SRCS}
    DESTINATION
        Mod/Plot/plotSave
)
INSTALL(
    FILES
        ${PlotSeries_SRCS}
    DESTINATION
        Mod/Plot/plotSeries
)
INSTALL(
    FILES
        ${PlotUtils_SRCS}
    DESTINATION
        Mod/Plot/plotUtils
)
INSTALL(
    FILES
        ${PlotMain_SRCS}
    DESTINATION
        Mod/Plot
)


