
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(Assembly_LIBS
    Part
    FreeCADApp
	OndselSolver
)

generate_from_xml(AssemblyObjectPy)
generate_from_xml(JointGroupPy)

SET(Python_SRCS
    AssemblyObjectPy.xml
    AssemblyObjectPyImp.cpp
    JointGroupPy.xml
    JointGroupPyImp.cpp
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
    JointGroup.cpp
    JointGroup.h
    ${Module_SRCS}
    ${Python_SRCS}
)

add_library(Assembly SHARED ${Assembly_SRCS})
target_link_libraries(Assembly ${Assembly_LIBS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Assembly_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Assembly PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Assembly AssemblyApp /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(Assembly)

INSTALL(TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
