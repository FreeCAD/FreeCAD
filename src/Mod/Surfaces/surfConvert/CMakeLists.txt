SET(Surfaces_Convert
	__init__.py
	Preview.py
	TaskPanel.py
	TaskPanel.ui
)

SOURCE_GROUP("" FILES ${Surfaces_Convert})

ADD_CUSTOM_TARGET(SurfacesConvert ALL
    SOURCES ${Surfaces_Convert}
)

fc_copy_sources("Mod/Surfaces/surfConvert" "Surfaces/surfConvert" ${Surfaces_Convert})

INSTALL(
    FILES
		${Surfaces_Convert}
    DESTINATION
		Mod/Surfaces/surfConvert
)

