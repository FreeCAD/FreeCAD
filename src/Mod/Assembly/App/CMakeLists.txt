# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(Assembly_LIBS
    Part
    PartDesign
    Spreadsheet
    FreeCADApp
    OndselSolver
)

generate_from_py(AssemblyObject)
generate_from_py(AssemblyLink)
generate_from_py(BomObject)
generate_from_py(BomGroup)
generate_from_py(JointGroup)
generate_from_py(ViewGroup)
generate_from_py(SimulationGroup)


SET(Python_SRCS
    AssemblyObject.pyi
    AssemblyObjectPyImp.cpp
    AssemblyLink.pyi
    AssemblyLinkPyImp.cpp
    BomObject.pyi
    BomObjectPyImp.cpp
    BomGroup.pyi
    BomGroupPyImp.cpp
    JointGroup.pyi
    JointGroupPyImp.cpp
    ViewGroup.pyi
    ViewGroupPyImp.cpp
    SimulationGroup.pyi
    SimulationGroupPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Module_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
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
target_include_directories(
    Assembly
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(Assembly ${Assembly_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Assembly)
endif()

if(FREECAD_USE_PCH)
    target_precompile_headers(Assembly PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Assembly AssemblyApp /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(Assembly)

INSTALL(TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
