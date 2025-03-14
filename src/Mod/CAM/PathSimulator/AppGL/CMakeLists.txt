include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CAMSimulator_LIBS
    Path
    Part
    Mesh
    FreeCADApp
    FreeCADGui
    ${QtOpenGL_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
)

SET(CAMSimulator_SRCS_Python
    CAMSimPy.xml
    CAMSimPyImp.cpp
)


SET(CAMSimulator_SRCS_Module
    AppCAMSimulator.cpp
    CAMSim.cpp
    CAMSim.h
    CAMSimPyImp.cpp
    DlgCAMSimulator.cpp
    DlgCAMSimulator.h
    PreCompiled.cpp
    PreCompiled.h
)

SET(CAMSimulator_SRCS_Core
    EndMill.cpp
    EndMill.h
    GCodeParser.cpp
    GCodeParser.h
    GlUtils.cpp
    GlUtils.h
    GuiDisplay.cpp
    GuiDisplay.h
    linmath.h
    MillMotion.h
    MillPathLine.cpp
	MillPathLine.h
    MillPathSegment.cpp
    MillPathSegment.h
    MillSimulation.cpp
    MillSimulation.h
    OpenGlWrapper.h
    Shader.cpp
    Shader.h
    SimDisplay.cpp
    SimDisplay.h
    SimShapes.cpp
    SimShapes.h
    StockObject.cpp
    StockObject.h
    SolidObject.cpp
    SolidObject.h
    Texture.cpp
    Texture.h
    TextureLoader.cpp
    TextureLoader.h
)


generate_from_xml(CAMSimPy)


SOURCE_GROUP("Module" FILES ${CAMSimulator_SRCS_Module})
SOURCE_GROUP("Python" FILES ${CAMSimulator_SRCS_Python})
SOURCE_GROUP("Core" FILES ${CAMSimulator_SRCS_Core})

SET(CAMSimulator_SRCS_precomp
    ${CAMSimulator_SRCS_Module}
    ${CAMSimulator_SRCS_Python}
)

SET(CAMSimulator_SRCS
    ${CAMSimulator_SRCS_precomp}
    ${CAMSimulator_SRCS_Core}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${CAMSimulator_SRCS_precomp})
    ADD_MSVC_PRECOMPILED_HEADER(CAMSimulator PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(CAMSimulator SHARED ${CAMSimulator_SRCS})
target_link_libraries(CAMSimulator ${CAMSimulator_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(CAMSimulator)
endif()

SET_BIN_DIR(CAMSimulator CAMSimulator /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(CAMSimulator)

INSTALL(TARGETS CAMSimulator DESTINATION ${CMAKE_INSTALL_LIBDIR})
