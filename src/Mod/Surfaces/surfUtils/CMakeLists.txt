SET(Surfaces_Utils
	__init__.py
	Geometry.py
	Math.py
	Paths.py
	Translator.py
)

SOURCE_GROUP("" FILES ${Surfaces_Utils})

ADD_CUSTOM_TARGET(SurfacesUtils ALL
    SOURCES ${Surfaces_Utils}
)

fc_copy_sources("Mod/Surfaces/surfUtils" "Surfaces/surfUtils" ${Surfaces_Utils})

INSTALL(
    FILES
		${Surfaces_Utils}
    DESTINATION
		Mod/Surfaces/surfUtils
)

INSTALL(
    FILES
    DESTINATION
        Mod/Surfaces/surfUtils
)

