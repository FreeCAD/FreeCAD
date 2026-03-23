# source files
set(COIN_FOREIGNFILES_FILES
	SoForeignFileKit.cpp
	SoSTLFileKit.cpp
	steel-wrapper.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_FOREIGNFILES_INTERNAL_FILES
	steel.cpp
	steel.h
	steel-wrapper.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_FOREIGNFILES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} foreignfiles/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(foreignfiles OBJECT ${COIN_FOREIGNFILES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_FOREIGNFILES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/foreignfiles/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_FOREIGNFILES_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/foreignfiles/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
