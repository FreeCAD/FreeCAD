# source files
set(COIN_GLELEMENTS_FILES
	SoGLCacheContextElement.cpp
	SoGLClipPlaneElement.cpp
	SoGLColorIndexElement.cpp
	SoGLCoordinateElement.cpp
	SoGLDepthBufferElement.cpp
	SoGLDrawStyleElement.cpp
	SoGLEnvironmentElement.cpp
	SoGLLazyElement.cpp
	SoGLLightIdElement.cpp
	SoGLLinePatternElement.cpp
	SoGLLineWidthElement.cpp
	SoGLModelMatrixElement.cpp
	SoGLMultiTextureCoordinateElement.cpp
	SoGLMultiTextureEnabledElement.cpp
	SoGLMultiTextureMatrixElement.cpp
	SoGLMultiTextureImageElement.cpp
	SoGLNormalElement.cpp
	SoGLPointSizeElement.cpp
	SoGLPolygonOffsetElement.cpp
	SoGLProjectionMatrixElement.cpp
	SoGLRenderPassElement.cpp
	SoGLShapeHintsElement.cpp
	SoGLUpdateAreaElement.cpp
	SoGLVBOElement.cpp
	SoGLViewingMatrixElement.cpp
	SoGLViewportRegionElement.cpp
	SoGLDisplayList.cpp
	SoGLVertexAttributeElement.cpp
	SoResetMatrixElement.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_GLELEMENTS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} elements/GL/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(glelements OBJECT ${COIN_GLELEMENTS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_GLELEMENTS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/elements/GL/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
set(COIN_DOCUMENTATION_FILES
	${COIN_DOCUMENTATION_FILES}
	${PROJECT_SOURCE_DIR}/src/elements/GL/SoResetMatrixElement.h
	CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
)
