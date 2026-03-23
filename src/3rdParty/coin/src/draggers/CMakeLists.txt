# source files
set(COIN_DRAGGERS_FILES
	SoDragger.cpp
	SoCenterballDragger.cpp
	SoDirectionalLightDragger.cpp
	SoDragPointDragger.cpp
	SoHandleBoxDragger.cpp
	SoJackDragger.cpp
	SoPointLightDragger.cpp
	SoRotateCylindricalDragger.cpp
	SoRotateDiscDragger.cpp
	SoRotateSphericalDragger.cpp
	SoScale1Dragger.cpp
	SoScale2Dragger.cpp
	SoScale2UniformDragger.cpp
	SoScaleUniformDragger.cpp
	SoSpotLightDragger.cpp
	SoTabBoxDragger.cpp
	SoTabPlaneDragger.cpp
	SoTrackballDragger.cpp
	SoTransformBoxDragger.cpp
	SoTransformerDragger.cpp
	SoTranslate1Dragger.cpp
	SoTranslate2Dragger.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_DRAGGERS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} draggers/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(draggers OBJECT ${COIN_DRAGGERS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_DRAGGERS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/draggers/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
