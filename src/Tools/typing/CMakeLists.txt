# SPDX-License-Identifier: LGPL-2.1-or-later

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FREECAD_PYTHON_STUBS_DIR "${CMAKE_BINARY_DIR}/python-stubs")

add_custom_target(FreeCADPythonStubs ALL
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_stubs.py"
        --root "${CMAKE_SOURCE_DIR}"
        --out-dir "${FREECAD_PYTHON_STUBS_DIR}"
    COMMENT "Generating FreeCAD Python type stubs"
    VERBATIM
)
