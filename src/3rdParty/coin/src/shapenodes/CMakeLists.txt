# source files
set(COIN_SHAPENODES_FILES
	SoAsciiText.cpp
	SoCone.cpp
	SoCube.cpp
	SoCylinder.cpp
	SoFaceSet.cpp
	SoImage.cpp
	SoIndexedFaceSet.cpp
	SoIndexedLineSet.cpp
	SoIndexedMarkerSet.cpp
	SoIndexedNurbsCurve.cpp
	SoIndexedNurbsSurface.cpp
	SoIndexedPointSet.cpp
	SoIndexedShape.cpp
	SoIndexedTriangleStripSet.cpp
	SoLineSet.cpp
	SoMarkerSet.cpp
	SoNonIndexedShape.cpp
	SoNurbsCurve.cpp
	SoNurbsSurface.cpp
	SoPointSet.cpp
	SoQuadMesh.cpp
	SoShape.cpp
	SoSphere.cpp
	SoText2.cpp
	SoText3.cpp
	SoTriangleStripSet.cpp
	SoVertexShape.cpp
	soshape_bigtexture.cpp
	soshape_bumprender.cpp
	soshape_primdata.cpp
	soshape_trianglesort.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_SHAPENODES_INTERNAL_FILES
	SoNurbsP.h
	soshape_bigtexture.h
	soshape_bigtexture.cpp
	soshape_bumprender.h
	soshape_bumprender.cpp
	soshape_primdata.h
	soshape_primdata.cpp
	soshape_trianglesort.h
	soshape_trianglesort.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_SHAPENODES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} shapenodes/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(shapenodes OBJECT ${COIN_SHAPENODES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_SHAPENODES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/shapenodes/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_SHAPENODES_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/shapenodes/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
