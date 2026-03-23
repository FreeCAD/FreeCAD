# source files
set(COIN_BASE_FILES
	dict.cpp
	hash.cpp
	heap.cpp
	list.cpp
	memalloc.cpp
	rbptree.cpp
	time.cpp
	string.cpp
	dynarray.cpp
	namemap.cpp
	SbBSPTree.cpp
	SbByteBuffer.cpp
	SbBox2s.cpp
	SbBox2i32.cpp
	SbBox2f.cpp
	SbBox2d.cpp
	SbBox3s.cpp
	SbBox3i32.cpp
	SbBox3f.cpp
	SbBox3d.cpp
	SbClip.cpp
	SbColor.cpp
	SbColor4f.cpp
	SbCylinder.cpp
	SbDict.cpp
	SbDPLine.cpp
	SbDPMatrix.cpp
	SbDPPlane.cpp
	SbDPRotation.cpp
	SbHeap.cpp
	SbImage.cpp
	SbLine.cpp
	SbMatrix.cpp
	SbName.cpp
	SbOctTree.cpp
	SbPlane.cpp
	SbRotation.cpp
	SbSphere.cpp
	SbString.cpp
	SbTesselator.cpp
	SbGLUTessellator.cpp
	SbTime.cpp
	SbVec2b.cpp
	SbVec2ub.cpp
	SbVec2s.cpp
	SbVec2us.cpp
	SbVec2i32.cpp
	SbVec2ui32.cpp
	SbVec2f.cpp
	SbVec2d.cpp
	SbVec3b.cpp
	SbVec3ub.cpp
	SbVec3s.cpp
	SbVec3us.cpp
	SbVec3i32.cpp
	SbVec3ui32.cpp
	SbVec3f.cpp
	SbVec3d.cpp
	SbVec4b.cpp
	SbVec4ub.cpp
	SbVec4s.cpp
	SbVec4us.cpp
	SbVec4i32.cpp
	SbVec4ui32.cpp
	SbVec4f.cpp
	SbVec4d.cpp
	SbViewVolume.cpp
	SbDPViewVolume.cpp
	SbViewportRegion.cpp
	SbXfBox3f.cpp
	SbXfBox3d.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_BASE_INTERNAL_FILES
	coinString.h
	dict.h
	dict.cpp
	dictp.h
	dynarray.h
	dynarray.cpp
	hashp.h
	heapp.h
	namemap.h
	namemap.cpp
	SbGLUTessellator.h
	SbGLUTessellator.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_BASE_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} base/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(base OBJECT ${COIN_BASE_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_BASE_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/base/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_BASE_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/base/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
