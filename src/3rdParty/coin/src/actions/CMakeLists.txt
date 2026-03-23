# source files
set(COIN_ACTIONS_FILES
	SoAction.cpp
	SoActionP.cpp
	SoBoxHighlightRenderAction.cpp
	SoCallbackAction.cpp
	SoGLRenderAction.cpp
	SoGetBoundingBoxAction.cpp
	SoGetMatrixAction.cpp
	SoGetPrimitiveCountAction.cpp
	SoHandleEventAction.cpp
	SoLineHighlightRenderAction.cpp
	SoPickAction.cpp
	SoRayPickAction.cpp
	SoReorganizeAction.cpp
	SoSearchAction.cpp
	SoSimplifyAction.cpp
	SoToVRMLAction.cpp
	SoToVRML2Action.cpp
	SoWriteAction.cpp
	SoAudioRenderAction.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_ACTIONS_INTERNAL_FILES
	SoActionP.h
	SoActionP.cpp
	SoSubActionP.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_ACTIONS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} actions/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(actions OBJECT ${COIN_ACTIONS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_ACTIONS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/actions/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_ACTIONS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/actions/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
