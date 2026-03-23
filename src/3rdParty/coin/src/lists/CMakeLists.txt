# source files
set(COIN_LISTS_FILES
	SbList.cpp
	SbPList.cpp
	SbIntList.cpp
	SbVec3fList.cpp
	SbStringList.cpp
	SoActionMethodList.cpp
	SoAuditorList.cpp
	SoBaseList.cpp
	SoCallbackList.cpp
	SoDetailList.cpp
	SoEnabledElementsList.cpp
	SoEngineList.cpp
	SoEngineOutputList.cpp
	SoFieldList.cpp
	SoNodeList.cpp
	SoPathList.cpp
	SoPickedPointList.cpp
	SoTypeList.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_LISTS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} lists/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(lists OBJECT ${COIN_LISTS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_LISTS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/lists/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
