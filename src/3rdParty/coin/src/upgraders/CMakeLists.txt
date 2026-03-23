# source files
set(COIN_UPGRADERS_FILES
	SoUpgrader.cpp
	SoPackedColorV20.cpp
	SoShapeHintsV10.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_UPGRADERS_INTERNAL_FILES
	SoUpgrader.h
	SoUpgrader.cpp
	SoPackedColorV20.h
	SoPackedColorV20.cpp
	SoShapeHintsV10.h
	SoShapeHintsV10.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_UPGRADERS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} upgraders/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(upgraders OBJECT ${COIN_UPGRADERS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_UPGRADERS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/upgraders/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_UPGRADERS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/upgraders/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
