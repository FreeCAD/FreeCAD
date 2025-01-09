
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(Assembly_LIBS
    Part
    PartDesign
	Spreadsheet
    FreeCADApp
	OndselSolver
)

generate_from_xml(AssemblyObjectPy)
generate_from_xml(AssemblyLinkPy)
generate_from_xml(BomObjectPy)
generate_from_xml(BomGroupPy)
generate_from_xml(JointGroupPy)
generate_from_xml(ViewGroupPy)
generate_from_xml(SimulationGroupPy)

SET(Python_SRCS
    AssemblyObjectPy.xml
    AssemblyObjectPyImp.cpp
    AssemblyLinkPy.xml
    AssemblyLinkPyImp.cpp
    BomObjectPy.xml
    BomObjectPyImp.cpp
    BomGroupPy.xml
    BomGroupPyImp.cpp
    JointGroupPy.xml
    JointGroupPyImp.cpp
    ViewGroupPy.xml
    ViewGroupPyImp.cpp
    SimulationGroupPy.xml
    SimulationGroupPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Module_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Assembly_SRCS
    AssemblyObject.cpp
    AssemblyObject.h
    AssemblyLink.cpp
    AssemblyLink.h
    AssemblyUtils.cpp
    AssemblyUtils.h
    BomObject.cpp
    BomObject.h
    BomGroup.cpp
    BomGroup.h
    JointGroup.cpp
    JointGroup.h
    ViewGroup.cpp
    ViewGroup.h
    SimulationGroup.cpp
    SimulationGroup.h
    ${Module_SRCS}
    ${Python_SRCS}
)

add_library(Assembly SHARED ${Assembly_SRCS})
target_link_libraries(Assembly ${Assembly_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Assembly)
endif()

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Assembly_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Assembly PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Assembly AssemblyApp /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(Assembly)

INSTALL(TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
