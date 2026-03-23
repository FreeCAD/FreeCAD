# source files
set(COIN_CACHES_FILES
	SoBoundingBoxCache.cpp
	SoCache.cpp
	SoConvexDataCache.cpp
	SoGLCacheList.cpp
	SoGLRenderCache.cpp
	SoNormalCache.cpp
	SoTextureCoordinateCache.cpp
	SoPrimitiveVertexCache.cpp
	SoGlyphCache.cpp
	SoShaderProgramCache.cpp
	SoVBOCache.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_CACHES_INTERNAL_FILES
	SoGlyphCache.h
	SoGlyphCache.cpp
	SoShaderProgramCache.h
	SoShaderProgramCache.cpp
	SoVBOCache.h
	SoVBOCache.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_CACHES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} caches/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(caches OBJECT ${COIN_CACHES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_CACHES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/caches/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_CACHES_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/caches/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
