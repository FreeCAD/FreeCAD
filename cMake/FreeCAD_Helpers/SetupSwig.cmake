macro(SetupSwig)

# -------------------------------- Swig ----------------------------------
find_package(SWIG)
if(SWIG_VERSION VERSION_GREATER_EQUAL "4.4.0")
    # find pivy's _coin.so library path
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import pivy._coin; print(pivy._coin.__file__)"
        OUTPUT_VARIABLE PIVY_LIBRARY_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    message("-------------------------------------------------------")
    message("PIVY_LIBRARY_PATH is ${PIVY_LIBRARY_PATH}")
    message("-------------------------------------------------------")

    if(PIVY_LIBRARY_PATH)
        message(STATUS "Checking SWIG version compatibility with pivy...")

        # NOTE: this should be platform agnostic ie. should work on all platforms including windows
        # Look for QUALIFIED type names like "swig_runtime_data5.SwigPyObject"
        # which only exist in SWIG 4.4.0+
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c
            "import sys, re; data = open(sys.argv[1], 'rb').read(); pattern = rb'swig_runtime_data\\d+\\.Swig'; sys.exit(0 if re.search(pattern, data) else 1)"
            "${PIVY_LIBRARY_PATH}"
            RESULT_VARIABLE PIVY_SWIG_CHECK
            ERROR_QUIET
            TIMEOUT 10
        )
        if(NOT PIVY_SWIG_CHECK EQUAL 0)
            message(FATAL_ERROR
                "swig version mismatch detected!\n"
                "freecad is being built with SWIG ${SWIG_VERSION} (>=4.4.0)\n"
                "but pivy was built with swig < 4.4.0.\n"
                "this will cause runtime warnings / errors.\n"
                "please make sure pivy is built with the same swig ${SWIG_VERSION} that cmake finds in the setup process."
            )
        else()
            message(STATUS "Pivy SWIG version check: OK (built with SWIG >= 4.4.0)")
        endif()
    else()
        message(WARNING "Could not find pivy library to check SWIG version compatibility")
    endif()
endif()

if(NOT SWIG_FOUND)
    message("-----------------------------------------------------\n"
        "SWIG not found, will not build SWIG binding for pivy.\n"
        "-----------------------------------------------------\n")
endif()

endmacro()
