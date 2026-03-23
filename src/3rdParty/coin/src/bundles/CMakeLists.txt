# source files
set(COIN_BUNDLES_FILES
	SoBundle.cpp
	SoMaterialBundle.cpp
	SoNormalBundle.cpp
	SoVertexAttributeBundle.cpp
	SoTextureCoordinateBundle.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_BUNDLES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} bundles/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(bundles OBJECT ${COIN_BUNDLES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_BUNDLES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/bundles/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
