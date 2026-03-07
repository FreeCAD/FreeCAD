# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(PathSimulator_LIBS
    Path
    Part
    Mesh
    FreeCADApp
)

SET(Python_SRCS
    PathSim.pyi
    PathSimPyImp.cpp
)


SET(PathSimulator_SRCS
    AppPathSimulator.cpp
    PathSim.cpp
    PathSim.h
    VolSim.cpp
    VolSim.h
    PreCompiled.h
    ${Python_SRCS}
)

generate_from_py(PathSim)

SOURCE_GROUP("Python" FILES ${Python_SRCS})

add_library(PathSimulator SHARED ${PathSimulator_SRCS})
target_include_directories(
    PathSimulator
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(PathSimulator ${PathSimulator_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(PathSimulator)
endif()

SET_BIN_DIR(PathSimulator PathSimulator /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(PathSimulator)

install(TARGETS PathSimulator DESTINATION ${CMAKE_INSTALL_LIBDIR})
