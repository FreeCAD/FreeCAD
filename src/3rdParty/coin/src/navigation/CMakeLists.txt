# source files
set(COIN_NAVIGATION_FILES
	SoCameraUtils.cpp
	SoScXMLNavigation.cpp
	SoScXMLNavigationTarget.cpp
	SoScXMLPanTarget.cpp
	SoScXMLRotateTarget.cpp
	SoScXMLSeekTarget.cpp
	SoScXMLSpinTarget.cpp
	SoScXMLZoomTarget.cpp
	SoScXMLDollyTarget.cpp
	SoScXMLMiscTarget.cpp
	SoScXMLFlightControlTarget.cpp
	SoScXMLMotionTarget.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_NAVIGATION_INTERNAL_FILES
	SoCameraUtils.h
	SoCameraUtils.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_NAVIGATION_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} navigation/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(navigation OBJECT ${COIN_NAVIGATION_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_NAVIGATION_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/navigation/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_NAVIGATION_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/navigation/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
