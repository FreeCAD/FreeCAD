# SPDX-License-Identifier: LGPL-2.1-or-later

foreach(_required_variable IN ITEMS PYTHON_EXECUTABLE SOURCE_ROOT OUTPUT_DIR)
    if(NOT DEFINED ${_required_variable})
        message(FATAL_ERROR "${_required_variable} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND
        "${PYTHON_EXECUTABLE}"
        "${SOURCE_ROOT}/src/Tools/typing/generate_stubs.py"
        --root "${SOURCE_ROOT}"
        --out-dir "${OUTPUT_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)
