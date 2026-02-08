# SPDX-FileNotice: Part of the FreeCAD project.

macro(SetupSwig)

#-------------------------------- Swig ----------------------------------

# force cmake to re-search for SWIG when CMAKE_PREFIX_PATH changes
unset(SWIG_EXECUTABLE CACHE)
unset(SWIG_DIR CACHE)
unset(SWIG_VERSION CACHE)
unset(SWIG_FOUND CACHE)

if(BUILD_SKETCHER)
    # SWIG is required for sketcher WB (use QUIET to provide custom error message)
    find_package(SWIG QUIET)

    if(NOT SWIG_FOUND)
        message(FATAL_ERROR
                "-----------------------------------------------------\n"
                "SWIG not found, swig & pivy required for sketcher WB.\n"
                "-----------------------------------------------------\n")
        # do not continue with check if swig not found
        return()
    endif()

    # check swig/pivy runtime compatibility
    message(STATUS "checking SWIG/Pivy runtime compatibility...")

    # get SWIG's runtime version using -external-runtime flag
    execute_process(
        COMMAND ${SWIG_EXECUTABLE} -python -external-runtime "${CMAKE_BINARY_DIR}/swig_runtime_check.h"
        RESULT_VARIABLE SWIG_EXTERNAL_RUNTIME_RESULT
        ERROR_VARIABLE SWIG_EXTERNAL_RUNTIME_ERROR
        ERROR_QUIET
    )
    # NOTE: only print the below output if using `cmake --log-level=DEBUG`
    message(DEBUG "SWIG external-runtime result: ${SWIG_EXTERNAL_RUNTIME_RESULT}")
    message(DEBUG "SWIG external-runtime error: ${SWIG_EXTERNAL_RUNTIME_ERROR}")

    message(STATUS "Looking for: ${CMAKE_BINARY_DIR}/swig_runtime_check.h")
    if(EXISTS "${CMAKE_BINARY_DIR}/swig_runtime_check.h")
        message(STATUS "File exists: YES")
    else()
        message(STATUS "File exists: NO")
    endif()

    if(EXISTS "${CMAKE_BINARY_DIR}/swig_runtime_check.h")
        file(STRINGS "${CMAKE_BINARY_DIR}/swig_runtime_check.h"
            SWIG_RUNTIME_VERSION_LINE
            REGEX "^#define SWIG_RUNTIME_VERSION")

        message(STATUS "SWIG_RUNTIME_VERSION_LINE: ${SWIG_RUNTIME_VERSION_LINE}")

        if(SWIG_RUNTIME_VERSION_LINE)
            # extract the version number (it's in quotes: "5")
            string(REGEX MATCH "\"([0-9]+)\"" _ "${SWIG_RUNTIME_VERSION_LINE}")
            set(SWIG_RUNTIME_VERSION "${CMAKE_MATCH_1}")
            message(STATUS "Extracted SWIG_RUNTIME_VERSION: ${SWIG_RUNTIME_VERSION}")
        endif()

        file(REMOVE "${CMAKE_BINARY_DIR}/swig_runtime_check.h")
    else()
        message(STATUS "swig_runtime_check.h not found!")
    endif()

    # extract pivy's SWIG runtime version from the compiled module
    # NOTE: python code can not be indented
    set(PYTHON_CHECK_PIVY_RUNTIME [=[
import sys
import os
import re

try:
    import pivy
    pivy_dir = os.path.dirname(pivy.__file__)
    print(f'DEBUG:pivy_dir={pivy_dir}', file=sys.stderr)
    print(f'DEBUG:files={os.listdir(pivy_dir)}', file=sys.stderr)

    pivy_path = None

    # Look for _coin module with any extension (.so on Unix, .pyd on Windows)
    for f in os.listdir(pivy_dir):
        print(f'DEBUG:checking={f}', file=sys.stderr)
        if f.startswith('_coin') and (f.endswith('.so') or f.endswith('.pyd')):
            pivy_path = os.path.join(pivy_dir, f)
            break

    print(f'DEBUG:pivy_path={pivy_path}', file=sys.stderr)

    if pivy_path and os.path.exists(pivy_path):
        with open(pivy_path, 'rb') as f:
            content = f.read().decode('latin-1', errors='ignore')

        print(f'DEBUG:content_len={len(content)}', file=sys.stderr)

        # Use regex to find swig_runtime_data followed by a number
        match = re.search(r'swig_runtime_data(\d+)', content)
        print(f'DEBUG:match={match}', file=sys.stderr)
        if match:
            print(match.group(1))

except ImportError as e:
    print(f'DEBUG:import_error={e}', file=sys.stderr)
    print('ERROR_IMPORT')
except Exception as e:
    print(f'DEBUG:exception={e}', file=sys.stderr)
]=])

execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "${PYTHON_CHECK_PIVY_RUNTIME}"
    OUTPUT_VARIABLE PIVY_RUNTIME_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE PIVY_DEBUG_OUTPUT
    TIMEOUT 10
)

message(DEBUG "Pivy debug output: ${PIVY_DEBUG_OUTPUT}")

    # Handle errors and compare versions
    if(PIVY_RUNTIME_VERSION STREQUAL "ERROR_IMPORT")
        message(WARNING
            "Could not import pivy to check SWIG compatibility.\n"
            "Proceeding without SWIG version check."
        )
    elseif(SWIG_RUNTIME_VERSION AND PIVY_RUNTIME_VERSION)
        if(NOT SWIG_RUNTIME_VERSION STREQUAL PIVY_RUNTIME_VERSION)
            message(FATAL_ERROR
" --------------------------------------------------------
 
 SWIG / PIVY RUNTIME VERSION MISMATCH DETECTED!
 
 SWIG runtime API version: ${SWIG_RUNTIME_VERSION}
 Pivy runtime API version: ${PIVY_RUNTIME_VERSION}
  
 These must match for compatibility.
 This will cause runtime errors: 'No SWIG wrapped library loaded'
  
 swig v4.4.x is not compatible with pivy built with swig <=4.3.x
  
 FIX: Install a SWIG version that uses runtime ${PIVY_RUNTIME_VERSION}
 or rebuild Pivy with your current SWIG ${SWIG_VERSION}.
 
--------------------------------------------------------"
            )
        else()
            message(STATUS "SWIG/Pivy runtime compatibility: PASSED")
            message(STATUS "swig runtime API version: ${SWIG_RUNTIME_VERSION}")
            message(STATUS "pivy runtime API version: ${PIVY_RUNTIME_VERSION}")
            message(STATUS "swig binary version building freecad: ${SWIG_VERSION}")
        endif()
    else()
        if(NOT SWIG_RUNTIME_VERSION)
            message(WARNING "Could not determine SWIG runtime version")
        endif()
        if(NOT PIVY_RUNTIME_VERSION)
            message(WARNING "Could not determine Pivy runtime version")
        endif()
        message(WARNING "Proceeding without SWIG/Pivy compatibility check.")
    endif()
endif()

endmacro()
