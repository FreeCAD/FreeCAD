# SPDX-License-Identifier: LGPL-2.1-or-later

include(GNUInstallDirs)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FREECAD_PYTHON_STUBS_DIR
    "${CMAKE_BINARY_DIR}/python-stubs"
)
set(FREECAD_PYTHON_STUBS_INSTALL_DESTINATION
    "${CMAKE_INSTALL_DATADIR}/python-stubs"
)
set(FREECAD_PYTHON_STUBS_CHECK_INSTALL_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/install-check"
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

add_custom_target(FreeCADPythonStubsCheck
    COMMAND
        "${CMAKE_COMMAND}"
        --install "${CMAKE_BINARY_DIR}"
        --component FreeCADPythonStubs
        --prefix "${FREECAD_PYTHON_STUBS_CHECK_INSTALL_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DFREECAD_PYTHON_STUBS_LAYOUT_ROOT=${FREECAD_PYTHON_STUBS_CHECK_INSTALL_DIR}/${FREECAD_PYTHON_STUBS_INSTALL_DESTINATION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckStubLayout.cmake"
    COMMENT "Checking FreeCAD Python stub layout"
    VERBATIM
)

install(
    CODE
        "execute_process(
            COMMAND
                \"${CMAKE_COMMAND}\"
                \"-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}\"
                \"-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}\"
                \"-DOUTPUT_DIR=${FREECAD_PYTHON_STUBS_DIR}\"
                -P \"${CMAKE_CURRENT_SOURCE_DIR}/GeneratePythonStubs.cmake\"
            COMMAND_ERROR_IS_FATAL ANY
        )"
    COMPONENT FreeCADPythonStubs
)

install(
    DIRECTORY "${FREECAD_PYTHON_STUBS_DIR}/stubs/"
    DESTINATION "${FREECAD_PYTHON_STUBS_INSTALL_DESTINATION}"
    COMPONENT FreeCADPythonStubs
)
