SET(SurfMain_SRCS
	InitGui.py
	SurfGui.py
)
SOURCE_GROUP("" FILES ${SurfMain_SRCS})

SET(SurfIcons_SRCS
	Icons/BorderIco.png
	Icons/BorderIco.xpm
	Icons/ConvertIco.png
	Icons/ConvertTo4SidesIco.xpm
	Icons/Ico.xcf
	Icons/IsoCurveIco.png
	Icons/IsoCurveIco.xpm
	Icons/SliceIco.xcf
	Icons/BorderIco.xcf
	Icons/ConvertIco.xcf
	Icons/Ico.png
	Icons/Ico.xpm
	Icons/IsoCurveIco.xcf
	Icons/SliceIco.png
	Icons/SliceIco.xpm
)
SOURCE_GROUP("surficons" FILES ${SurfIcons_SRCS})

SET(SurfBorder_SRCS
	surfBorder/__init__.py
)
SOURCE_GROUP("surfborder" FILES ${SurfBorder_SRCS})

SET(SurfConvert_SRCS
	surfConvert/__init__.py
	surfConvert/Preview.py
	surfConvert/TaskPanel.py
	surfConvert/TaskPanel.ui
)
SOURCE_GROUP("surfconvert" FILES ${SurfConvert_SRCS})

SET(SurfISOCurve_SRCS
	surfISOCurve/__init__.py
	surfISOCurve/PointTracker.py
	surfISOCurve/TaskPanel.py
	surfISOCurve/Preview.py
	surfISOCurve/TaskPanel.ui
)
SOURCE_GROUP("surfISOcurve" FILES ${SurfISOCurve_SRCS})

SET(SurfSlice_SRCS
	surfSlice/__init__.py
	surfSlice/PointTracker.py
	surfSlice/TaskPanel.py
	surfSlice/Preview.py
	surfSlice/TaskPanel.ui
)
SOURCE_GROUP("surfslice" FILES ${SurfSlice_SRCS})

SET(SurfUtils_SRCS
	surfUtils/__init__.py
	surfUtils/Geometry.py
	surfUtils/Math.py
	surfUtils/Paths.py
	surfUtils/Translator.py
)
SOURCE_GROUP("surfutils" FILES ${SurfUtils_SRCS})

SET(all_files ${SurfMain_SRCS} ${SurfIcons_SRCS} ${SurfBorder_SRCS} ${SurfConvert_SRCS} ${SurfISOCurve_SRCS} ${SurfSlice_SRCS} ${SurfUtils_SRCS})

ADD_CUSTOM_TARGET(Surfaces ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Surfaces" "Surfaces" ${all_files})

INSTALL(
    FILES
        ${SurfIcons_SRCS}
    DESTINATION
        Mod/Surfaces/Icons
)
INSTALL(
    FILES
        ${SurfBorder_SRCS}
    DESTINATION
        Mod/Surfaces/surfBorder
)
INSTALL(
    FILES
        ${SurfConvert_SRCS}
    DESTINATION
        Mod/Surfaces/surfConvert
)
INSTALL(
    FILES
        ${SurfISOCurve_SRCS}
    DESTINATION
        Mod/Surfaces/surfISOCurve
)
INSTALL(
    FILES
        ${SurfSlice_SRCS}
    DESTINATION
        Mod/Surfaces/surfSlice
)
INSTALL(
    FILES
        ${SurfUtils_SRCS}
    DESTINATION
        Mod/Surfaces/surfUtils
)
INSTALL(
    FILES
        ${SurfMain_SRCS}
    DESTINATION
        Mod/Surfaces
)

