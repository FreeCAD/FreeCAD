add_subdirectory(Icons)
add_subdirectory(surfBorder)
add_subdirectory(surfConvert)
add_subdirectory(surfISOCurve)
add_subdirectory(surfSlice)
add_subdirectory(surfUtils)


SET(Surfaces_SRCS
        InitGui.py
        SurfGui.py
)
SOURCE_GROUP("" FILES ${Surfaces_SRCS})

SET(all_files ${Surfaces_SRCS})

ADD_CUSTOM_TARGET(Surfaces ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Surfaces" "Surfaces" ${all_files})

INSTALL(
    FILES
		${Surfaces_SRCS}
		README
    DESTINATION
		Mod/Surfaces
)

