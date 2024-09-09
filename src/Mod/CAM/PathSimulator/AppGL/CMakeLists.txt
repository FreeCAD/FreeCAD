include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS})

link_directories (${OCC_LIBRARY_DIR})

set (
    CAMSimulator_LIBS
    Path
    Part
    Mesh
    FreeCADApp
    FreeCADGui
    ${QtOpenGL_LIBRARIES}
    ${OPENGL_gl_LIBRARY})

set (CAMSimulator_SRCS_Python CAMSimPy.xml CAMSimPyImp.cpp)

set (
    CAMSimulator_SRCS_Module
    AppCAMSimulator.cpp
    CAMSim.cpp
    CAMSim.h
    CAMSimPyImp.cpp
    DlgCAMSimulator.cpp
    DlgCAMSimulator.h
    PreCompiled.cpp
    PreCompiled.h)

set (
    CAMSimulator_SRCS_Core
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
    TextureLoader.h)

generate_from_xml (CAMSimPy)

source_group ("Module" FILES ${CAMSimulator_SRCS_Module})
source_group ("Python" FILES ${CAMSimulator_SRCS_Python})
source_group ("Core" FILES ${CAMSimulator_SRCS_Core})

set (CAMSimulator_SRCS_precomp ${CAMSimulator_SRCS_Module} ${CAMSimulator_SRCS_Python})

set (CAMSimulator_SRCS ${CAMSimulator_SRCS_precomp} ${CAMSimulator_SRCS_Core})

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${CAMSimulator_SRCS_precomp})
    add_msvc_precompiled_header (CAMSimulator PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

add_library (CAMSimulator SHARED ${CAMSimulator_SRCS})
target_link_libraries (CAMSimulator ${CAMSimulator_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (CAMSimulator)
endif ()

set_bin_dir (CAMSimulator CAMSimulator /Mod/CAM)
set_python_prefix_suffix (CAMSimulator)

install (TARGETS CAMSimulator DESTINATION ${CMAKE_INSTALL_LIBDIR})
