# source files
set(COIN_MISC_FILES
	AudioTools.cpp
	CoinStaticObjectInDLL.cpp
	SoAudioDevice.cpp
	SoBase.cpp
	SoBaseP.cpp
	SoChildList.cpp
	SoCompactPathList.cpp
	SoConfigSettings.cpp
	SoContextHandler.cpp
	SoDB.cpp
	SoDebug.cpp
	SoFullPath.cpp
	SoGenerate.cpp
	SoGlyph.cpp
	SoInteraction.cpp
	SoJavaScriptEngine.cpp
	SoLightPath.cpp
	SoLockManager.cpp
	SoNormalGenerator.cpp
	SoNotRec.cpp
	SoNotification.cpp
	SoPath.cpp
	SoPick.cpp
	SoPickedPoint.cpp
	SoPrimitiveVertex.cpp
	SoProto.cpp
	SoProtoInstance.cpp
	SoSceneManager.cpp
	SoSceneManagerP.cpp
	SoShaderGenerator.cpp
	SoState.cpp
	SoTempPath.cpp
	SoType.cpp
	CoinResources.cpp
	SoDBP.cpp
	SoEventManager.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_MISC_INTERNAL_FILES
	AudioTools.h
	AudioTools.cpp
	CoinStaticObjectInDLL.h
	CoinStaticObjectInDLL.cpp
	SbHash.h
	SoBaseP.h
	SoBaseP.cpp
	SoCompactPathList.h
	SoCompactPathList.cpp
	SoConfigSettings.h
	SoConfigSettings.cpp
	SoDBP.h
	SoDBP.cpp
	SoGenerate.h
	SoGenerate.cpp
	SoPick.h
	SoPick.cpp
	SoSceneManagerP.h
	SoSceneManagerP.cpp
	SoShaderGenerator.h
	SoShaderGenerator.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_MISC_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} misc/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(misc OBJECT ${COIN_MISC_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_MISC_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/misc/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_MISC_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/misc/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
