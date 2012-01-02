SET(Surfaces_ISOCurve
	__init__.py
	PointTracker.py
	TaskPanel.py
	Preview.py
	TaskPanel.ui
)

SOURCE_GROUP("" FILES ${Surfaces_ISOCurve})

ADD_CUSTOM_TARGET(SurfacesISOCurve ALL
    SOURCES ${Surfaces_ISOCurve}
)

fc_copy_sources("Mod/Surfaces/surfISOCurve" "Surfaces/surfISOCurve" ${Surfaces_ISOCurve})

INSTALL(
    FILES
		${Surfaces_ISOCurve}
    DESTINATION
		Mod/Surfaces/surfISOCurve
)

INSTALL(
    FILES
    DESTINATION
        Mod/Surfaces/surfISOCurve
)

