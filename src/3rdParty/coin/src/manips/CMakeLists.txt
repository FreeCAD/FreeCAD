# source files
set(COIN_MANIPS_FILES
	SoClipPlaneManip.cpp
	SoDirectionalLightManip.cpp
	SoPointLightManip.cpp
	SoSpotLightManip.cpp
	SoTransformManip.cpp
	SoCenterballManip.cpp
	SoHandleBoxManip.cpp
	SoJackManip.cpp
	SoTabBoxManip.cpp
	SoTrackballManip.cpp
	SoTransformBoxManip.cpp
	SoTransformerManip.cpp
	commoncode.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_MANIPS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} manips/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(manips OBJECT ${COIN_MANIPS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_MANIPS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/manips/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
