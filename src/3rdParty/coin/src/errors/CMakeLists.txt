# source files
set(COIN_ERRORS_FILES
	error.cpp
	debugerror.cpp
	SoDebugError.cpp
	SoError.cpp
	SoMemoryError.cpp
	SoReadError.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_ERRORS_INTERNAL_FILES
	error.cpp
	debugerror.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_ERRORS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} errors/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(errors OBJECT ${COIN_ERRORS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_ERRORS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/errors/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_ERRORS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/errors/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
