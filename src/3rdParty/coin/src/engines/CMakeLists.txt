# source files
set(COIN_ENGINES_FILES
	SoBoolOperation.cpp
	SoCalculator.cpp
	SoComposeMatrix.cpp
	SoComposeRotation.cpp
	SoComposeRotationFromTo.cpp
	SoComposeVec2f.cpp
	SoComposeVec3f.cpp
	SoComposeVec4f.cpp
	SoComputeBoundingBox.cpp
	SoConcatenate.cpp
	SoConvertAll.cpp
	SoCounter.cpp
	SoDecomposeMatrix.cpp
	SoDecomposeRotation.cpp
	SoDecomposeVec2f.cpp
	SoDecomposeVec3f.cpp
	SoDecomposeVec4f.cpp
	SoElapsedTime.cpp
	SoEngine.cpp
	SoEngineOutput.cpp
	SoFieldConverter.cpp
	SoGate.cpp
	SoInterpolate.cpp
	SoInterpolateFloat.cpp
	SoInterpolateRotation.cpp
	SoInterpolateVec2f.cpp
	SoInterpolateVec3f.cpp
	SoInterpolateVec4f.cpp
	SoNodeEngine.cpp
	SoOnOff.cpp
	SoOneShot.cpp
	SoOutputData.cpp
	SoSelectOne.cpp
	SoTimeCounter.cpp
	SoTransformVec3f.cpp
	SoTriggerAny.cpp
	SoTexture2Convert.cpp
	SoHeightMapToNormalMap.cpp
	evaluator.c
	evaluator_tab.c
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_ENGINES_INTERNAL_FILES
	SoConvertAll.h
	SoConvertAll.cpp
	evaluator.h
	evaluator.c
	evaluator_tab.c
	SoSubEngineP.h
	SoSubNodeEngineP.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_ENGINES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} engines/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(engines OBJECT ${COIN_ENGINES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_ENGINES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/engines/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_ENGINES_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/engines/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
