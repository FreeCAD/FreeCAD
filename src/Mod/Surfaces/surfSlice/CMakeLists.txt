SET(Surfaces_Slice
	__init__.py
	PointTracker.py
	TaskPanel.py
	Preview.py
	TaskPanel.ui
)

SOURCE_GROUP("" FILES ${Surfaces_Slice})

ADD_CUSTOM_TARGET(SurfacesSlice ALL
    SOURCES ${Surfaces_Slice}
)

fc_copy_sources("Mod/Surfaces/surfSlice" "Surfaces/surfSlice" ${Surfaces_Slice})

INSTALL(
    FILES
		${Surfaces_Slice}
    DESTINATION
		Mod/Surfaces/surfSlice
)

INSTALL(
    FILES
    DESTINATION
        Mod/Surfaces/surfSlice
)

