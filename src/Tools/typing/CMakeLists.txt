# SPDX-License-Identifier: LGPL-2.1-or-later

include(GNUInstallDirs)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FREECAD_PYTHON_STUBS_DIR "${CMAKE_BINARY_DIR}/python-stubs")
set(FREECAD_PYTHON_STUBS_STAMP "${FREECAD_PYTHON_STUBS_DIR}/.generated.stamp")

# The generator discovers bindings throughout the source tree. Keep this list
# aligned with the inputs scanned by stubgen so CMake can avoid regenerating
# the complete stub tree when none of those inputs changed.
file(GLOB_RECURSE FREECAD_PYTHON_STUB_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.hh"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.hxx"
    "${CMAKE_SOURCE_DIR}/src/*.pyi"
    "${CMAKE_SOURCE_DIR}/src/CMakeLists.txt"
)
list(FILTER FREECAD_PYTHON_STUB_INPUTS EXCLUDE REGEX "/src/3rdParty/")
file(GLOB_RECURSE FREECAD_PYTHON_STUB_TOOL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.toml"
)

add_custom_command(
    OUTPUT "${FREECAD_PYTHON_STUBS_STAMP}"
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_stubs.py"
        --root "${CMAKE_SOURCE_DIR}"
        --out-dir "${FREECAD_PYTHON_STUBS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${FREECAD_PYTHON_STUBS_STAMP}"
    DEPENDS
        ${FREECAD_PYTHON_STUB_INPUTS}
        ${FREECAD_PYTHON_STUB_TOOL_SOURCES}
        "${CMAKE_SOURCE_DIR}/src/App/FreeCADInit.py"
        "${CMAKE_SOURCE_DIR}/version.json"
    COMMENT "Generating FreeCAD Python type stubs"
    VERBATIM
)

add_custom_target(FreeCADPythonStubs ALL
    DEPENDS "${FREECAD_PYTHON_STUBS_STAMP}"
)

install(
    DIRECTORY "${FREECAD_PYTHON_STUBS_DIR}/stubs/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/python-stubs"
    COMPONENT FreeCADPythonStubs
)
