# source files
set(COIN_COLLISION_FILES
	SbTri3f.cpp
	SoIntersectionDetectionAction.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_COLLISION_INTERNAL_FILES
	SbTri3f.h
	SbTri3f.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_COLLISION_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} collision/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(collision OBJECT ${COIN_COLLISION_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_COLLISION_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/collision/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_COLLISION_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/collision/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
