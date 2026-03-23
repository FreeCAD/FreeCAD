# source files
set(COIN_FIELDS_FILES
	SoField.cpp
	SoFieldContainer.cpp
	SoFieldData.cpp
	SoMFBitMask.cpp
	SoMFBool.cpp
	SoMFColor.cpp
	SoMFColorRGBA.cpp
	SoMFDouble.cpp
	SoMFEngine.cpp
	SoMFEnum.cpp
	SoMFFloat.cpp
	SoMFInt32.cpp
	SoMFMatrix.cpp
	SoMFName.cpp
	SoMFNode.cpp
	SoMFPath.cpp
	SoMFPlane.cpp
	SoMFRotation.cpp
	SoMFShort.cpp
	SoMFString.cpp
	SoMFTime.cpp
	SoMFUInt32.cpp
	SoMFUShort.cpp
	SoMFVec2b.cpp
	SoMFVec2s.cpp
	SoMFVec2i32.cpp
	SoMFVec2f.cpp
	SoMFVec2d.cpp
	SoMFVec3b.cpp
	SoMFVec3s.cpp
	SoMFVec3i32.cpp
	SoMFVec3f.cpp
	SoMFVec3d.cpp
	SoMFVec4b.cpp
	SoMFVec4ub.cpp
	SoMFVec4s.cpp
	SoMFVec4us.cpp
	SoMFVec4i32.cpp
	SoMFVec4ui32.cpp
	SoMFVec4f.cpp
	SoMFVec4d.cpp
	SoMField.cpp
	SoSFBitMask.cpp
	SoSFBool.cpp
	SoSFBox2s.cpp
	SoSFBox2i32.cpp
	SoSFBox2f.cpp
	SoSFBox2d.cpp
	SoSFBox3s.cpp
	SoSFBox3i32.cpp
	SoSFBox3f.cpp
	SoSFBox3d.cpp
	SoSFColor.cpp
	SoSFColorRGBA.cpp
	SoSFDouble.cpp
	SoSFEngine.cpp
	SoSFEnum.cpp
	SoSFFloat.cpp
	SoSFImage.cpp
	SoSFImage3.cpp
	SoSFInt32.cpp
	SoSFMatrix.cpp
	SoSFName.cpp
	SoSFNode.cpp
	SoSFPath.cpp
	SoSFPlane.cpp
	SoSFRotation.cpp
	SoSFShort.cpp
	SoSFString.cpp
	SoSFTime.cpp
	SoSFTrigger.cpp
	SoSFUInt32.cpp
	SoSFUShort.cpp
	SoSFVec2b.cpp
	SoSFVec2s.cpp
	SoSFVec2i32.cpp
	SoSFVec2f.cpp
	SoSFVec2d.cpp
	SoSFVec3b.cpp
	SoSFVec3s.cpp
	SoSFVec3i32.cpp
	SoSFVec3f.cpp
	SoSFVec3d.cpp
	SoSFVec4b.cpp
	SoSFVec4ub.cpp
	SoSFVec4s.cpp
	SoSFVec4us.cpp
	SoSFVec4i32.cpp
	SoSFVec4ui32.cpp
	SoSFVec4f.cpp
	SoSFVec4d.cpp
	SoSField.cpp
	SoGlobalField.cpp
	shared.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_FIELDS_INTERNAL_FILES
	SoGlobalField.h
	SoGlobalField.cpp
	shared.h
	shared.cpp
	SoSubFieldP.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_FIELDS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} fields/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(fields OBJECT ${COIN_FIELDS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_FIELDS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/fields/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_FIELDS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/fields/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
