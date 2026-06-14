# SPDX-License-Identifier: LGPL-2.1-or-later

set(Sketcher3D_LIBS
    FreeCADApp
    Part
    Sketcher
)

SET(Module_SRCS
    AppSketcher3D.cpp
    AppSketcher3DPy.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Constraint_SRCS
    GeoEnum3D.h
    Constraint3D.cpp
    Constraint3D.h
    PropertyConstraint3DList.cpp
    PropertyConstraint3DList.h
)
SOURCE_GROUP("Constraint" FILES ${Constraint_SRCS})

SET(Solver_SRCS
    GeometryMapper3D.cpp
    GeometryMapper3D.h
    Solver3D.cpp
    Solver3D.h
)
SOURCE_GROUP("Solver" FILES ${Solver_SRCS})

SET(Feature_SRCS
    Sketch3DObject.cpp
    Sketch3DObject.h
)
SOURCE_GROUP("Feature" FILES ${Feature_SRCS})

SET(Sketcher3D_SRCS
    ${Module_SRCS}
    ${Constraint_SRCS}
    ${Solver_SRCS}
    ${Feature_SRCS}
)

add_library(Sketcher3D SHARED ${Sketcher3D_SRCS})
target_include_directories(
    Sketcher3D
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(Sketcher3D ${Sketcher3D_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Sketcher3D)
endif()

if(FREECAD_USE_PCH)
    target_precompile_headers(Sketcher3D PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Sketcher3D Sketcher3D /Mod/Sketcher3D)
SET_PYTHON_PREFIX_SUFFIX(Sketcher3D)

INSTALL(TARGETS Sketcher3D DESTINATION ${CMAKE_INSTALL_LIBDIR})
