# source files
set(COIN_SHADERS_FILES
	SoFragmentShader.cpp
	SoGeometryShader.cpp
	SoGLARBShaderParameter.cpp
	SoGLARBShaderObject.cpp
	SoGLARBShaderProgram.cpp
	SoGLCgShaderObject.cpp
	SoGLCgShaderParameter.cpp
	SoGLCgShaderProgram.cpp
	SoGLSLShaderParameter.cpp
	SoGLSLShaderObject.cpp
	SoGLSLShaderProgram.cpp
	SoGLShaderObject.cpp
	SoGLShaderParameter.cpp
	SoGLShaderProgram.cpp
	SoGLShaderProgramElement.cpp
	SoShaderObject.cpp
	SoShaderParameter.cpp
	SoShaderProgram.cpp
	SoShader.cpp
	SoVertexShader.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_SHADERS_INTERNAL_FILES
	SoGLARBShaderParameter.h
	SoGLARBShaderParameter.cpp
	SoGLARBShaderObject.h
	SoGLARBShaderObject.cpp
	SoGLARBShaderProgram.h
	SoGLARBShaderProgram.cpp
	SoGLCgShaderObject.h
	SoGLCgShaderObject.cpp
	SoGLCgShaderParameter.h
	SoGLCgShaderParameter.cpp
	SoGLCgShaderProgram.h
	SoGLCgShaderProgram.cpp
	SoGLSLShaderParameter.h
	SoGLSLShaderParameter.cpp
	SoGLSLShaderObject.h
	SoGLSLShaderObject.cpp
	SoGLSLShaderProgram.h
	SoGLSLShaderProgram.cpp
	SoGLShaderObject.h
	SoGLShaderObject.cpp
	SoGLShaderParameter.h
	SoGLShaderParameter.cpp
	SoGLShaderProgram.h
	SoGLShaderProgram.cpp
	SoGLShaderProgramElement.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_SHADERS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} shaders/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(shaders OBJECT ${COIN_SHADERS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_SHADERS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/shaders/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_SHADERS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/shaders/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
