# source files
set(COIN_RENDERING_FILES
	SoGL.cpp
	SoGLBigImage.cpp
	SoGLDriverDatabase.cpp
	SoGLImage.cpp
	SoGLCubeMapImage.cpp
	SoGLNurbs.cpp
	SoRenderManager.cpp
	SoRenderManagerP.cpp
	SoOffscreenRenderer.cpp
	SoOffscreenCGData.cpp
	SoOffscreenGLXData.cpp
	SoOffscreenWGLData.cpp
	SoVBO.cpp
	SoVertexArrayIndexer.cpp
	CoinOffscreenGLCanvas.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_RENDERING_INTERNAL_FILES
	SoGL.h
	SoGL.cpp
	SoGLNurbs.h
	SoGLNurbs.cpp
	SoRenderManagerP.h
	SoRenderManagerP.cpp
	SoOffscreenCGData.h
	SoOffscreenCGData.cpp
	SoOffscreenGLXData.h
	SoOffscreenGLXData.cpp
	SoOffscreenWGLData.h
	SoOffscreenWGLData.cpp
	SoVBO.h
	SoVBO.cpp
	SoVertexArrayIndexer.h
	SoVertexArrayIndexer.cpp
	CoinOffscreenGLCanvas.h
	CoinOffscreenGLCanvas.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_RENDERING_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} rendering/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(rendering OBJECT ${COIN_RENDERING_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_RENDERING_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/rendering/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_RENDERING_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/rendering/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
