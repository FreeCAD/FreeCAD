# source files
set(COIN_DETAILS_FILES
	SoDetail.cpp
	SoConeDetail.cpp
	SoCubeDetail.cpp
	SoCylinderDetail.cpp
	SoFaceDetail.cpp
	SoLineDetail.cpp
	SoNodeKitDetail.cpp
	SoPointDetail.cpp
	SoTextDetail.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_DETAILS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} details/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(details OBJECT ${COIN_DETAILS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_DETAILS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/details/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
