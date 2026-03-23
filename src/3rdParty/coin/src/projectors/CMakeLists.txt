# source files
set(COIN_PROJECTORS_FILES
	SbCylinderPlaneProjector.cpp
	SbCylinderProjector.cpp
	SbCylinderSectionProjector.cpp
	SbCylinderSheetProjector.cpp
	SbLineProjector.cpp
	SbPlaneProjector.cpp
	SbProjector.cpp
	SbSpherePlaneProjector.cpp
	SbSphereProjector.cpp
	SbSphereSectionProjector.cpp
	SbSphereSheetProjector.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_PROJECTORS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} projectors/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(projectors OBJECT ${COIN_PROJECTORS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_PROJECTORS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/projectors/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
