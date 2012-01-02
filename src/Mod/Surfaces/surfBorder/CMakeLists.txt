SET(Surfaces_Border
	__init__.py
)

SOURCE_GROUP("" FILES ${Surfaces_Border})

ADD_CUSTOM_TARGET(SurfacesBorder ALL
    SOURCES ${Surfaces_Border}
)

fc_copy_sources("Mod/Surfaces/surfBorder" "Surfaces/surfBorder" ${Surfaces_Border})

INSTALL(
    FILES
		${Surfaces_Border}
    DESTINATION
		Mod/Surfaces/surfBorder
)

