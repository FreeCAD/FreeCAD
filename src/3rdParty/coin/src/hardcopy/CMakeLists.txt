# source files
set(COIN_HARDCOPY_FILES
	HardCopy.cpp
	PSVectorOutput.cpp
	VectorOutput.cpp
	VectorizeAction.cpp
	VectorizeActionP.cpp
	VectorizePSAction.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_HARDCOPY_INTERNAL_FILES
	VectorizeActionP.h
	VectorizeActionP.cpp
	VectorizeItems.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_HARDCOPY_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} hardcopy/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(hardcopy OBJECT ${COIN_HARDCOPY_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_HARDCOPY_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/hardcopy/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_HARDCOPY_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/hardcopy/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
