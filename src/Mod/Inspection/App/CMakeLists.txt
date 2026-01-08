# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(Inspection_LIBS
    FreeCADApp
    Mesh
    Points
    Part
)

SET(Inspection_SRCS
    AppInspection.cpp
    InspectionFeature.cpp
    InspectionFeature.h
    PreCompiled.h
)

set(Inspection_Scripts
    ../Init.py
)

add_library(Inspection SHARED ${Inspection_SRCS} ${Inspection_Scripts})

if(FREECAD_USE_PCH)
    target_precompile_headers(Inspection PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Inspection
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Inspection ${Inspection_LIBS})

if (MSVC)
    target_compile_options(Inspection PRIVATE /wd4251)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Inspection)
endif()

fc_target_copy_resource_flat(Inspection
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Inspection
    ${Inspection_Scripts}
)

SET_BIN_DIR(Inspection Inspection /Mod/Inspection)
SET_PYTHON_PREFIX_SUFFIX(Inspection)

INSTALL(TARGETS Inspection DESTINATION ${CMAKE_INSTALL_LIBDIR})
