if(NOT EXPAT_FOUND)
	add_subdirectory(expat)
endif()

# source files
set(COIN_XML_FILES
	document.cpp
	element.cpp
	attribute.cpp
	entity.cpp
	utils.cpp
	path.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_XML_INTERNAL_FILES
	attributep.h
	documentp.h
	elementp.h
	utils.h
	utils.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_XML_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} xml/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(xml OBJECT ${COIN_XML_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_XML_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/xml/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_XML_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/xml/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
