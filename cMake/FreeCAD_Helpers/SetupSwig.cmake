macro(SetupSwig)

#-------------------------------- Swig ----------------------------------

# force cmake to re-search for SWIG when CMAKE_PREFIX_PATH changes
unset(SWIG_EXECUTABLE CACHE)
unset(SWIG_DIR CACHE)
unset(SWIG_VERSION CACHE)
unset(SWIG_FOUND CACHE)

find_package(SWIG)

if(NOT SWIG_FOUND)
    message("-----------------------------------------------------\n"
        "SWIG not found, will not build SWIG binding for pivy.\n"
        "-----------------------------------------------------\n")
endif()

# check SWIG version compatibility with pivy
message(STATUS "Checking SWIG version compatibility with pivy...")

# determine which SWIG runtime version FreeCAD is using
if(SWIG_VERSION VERSION_GREATER_EQUAL "4.4.0")
    set(FREECAD_SWIG_RUNTIME "4.4+")
elseif(SWIG_VERSION VERSION_GREATER_EQUAL "4.0.0")
    set(FREECAD_SWIG_RUNTIME "4.0-4.3")
else()
    set(FREECAD_SWIG_RUNTIME "PRE_4.0")
endif()

# Check which SWIG runtime version pivy was built with
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
    "import sys

try:
    import pivy.coin
except ImportError as e:
    print('ERROR_IMPORT')
    sys.exit(1)

# Check which SWIG runtime module pivy loaded
runtime_modules = [m for m in sys.modules if m.startswith('swig_runtime_data')]

if not runtime_modules:
    print('UNKNOWN')
    sys.exit(0)

runtime_name = runtime_modules[0]
version = runtime_name.replace('swig_runtime_data', '')

# Map runtime version to SWIG version range
if version == '5':
    print('4.4+')
elif version == '4':
    print('4.0-4.3')
else:
    print('PRE_4.0')
"
    OUTPUT_VARIABLE PIVY_SWIG_RUNTIME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE PIVY_SWIG_CHECK_ERROR
    RESULT_VARIABLE PIVY_SWIG_CHECK_RESULT
    TIMEOUT 10
)

# Handle errors
if(PIVY_SWIG_CHECK_RESULT)
    if(PIVY_SWIG_RUNTIME STREQUAL "ERROR_IMPORT")
        message(WARNING
            "Could not import pivy to check SWIG compatibility.\n"
            "Error: ${PIVY_SWIG_CHECK_ERROR}\n"
            "Proceeding without SWIG version check."
        )
    else()
        message(WARNING
            "Failed to check pivy SWIG version.\n"
            "Proceeding without SWIG version check."
        )
    endif()
    return()
endif()

# Check for compatibility
if(PIVY_SWIG_RUNTIME STREQUAL "UNKNOWN")
    message(WARNING
        "Could not determine SWIG runtime version used by pivy.\n"
        "Proceeding without SWIG version check."
    )
elseif(NOT PIVY_SWIG_RUNTIME STREQUAL FREECAD_SWIG_RUNTIME)
    message(FATAL_ERROR
        "--------------------------------------------------------\n"
        "SWIG / PIVY VERSION MISMATCH DETECTED!\n"
        "--------------------------------------------------------\n"
        "FreeCAD is being built with SWIG: ${SWIG_VERSION} (${FREECAD_SWIG_RUNTIME})\n"
        "But Pivy was built with SWIG: ${PIVY_SWIG_RUNTIME}\n"
        "This will cause runtime warnings/errors: 'No SWIG wrapped library loaded'\n"
        "Solution: make sure SWIG binary: ${SWIG_VERSION} is the same version as ${PIVY_SWIG_RUNTIME}\n"
        "--------------------------------------------------------\n"
    )
else()
    message(STATUS "SWIG compatibility check: PASSED")
    message(STATUS "  FreeCAD: SWIG ${SWIG_VERSION} (${FREECAD_SWIG_RUNTIME})")
    message(STATUS "  Pivy:    ${PIVY_SWIG_RUNTIME}")
endif()

endmacro()
