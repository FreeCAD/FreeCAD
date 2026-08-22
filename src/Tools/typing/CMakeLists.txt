# SPDX-License-Identifier: LGPL-2.1-or-later

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FREECAD_PYTHON_STUBS_DIR
    "${CMAKE_BINARY_DIR}/python-stubs"
)
add_custom_target(FreeCADPythonStubs
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
        "-DOUTPUT_DIR=${FREECAD_PYTHON_STUBS_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/GeneratePythonStubs.cmake"
    COMMENT "Generating FreeCAD Python type stubs"
    VERBATIM
)
