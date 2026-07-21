macro(SetupSwig)

#-------------------------------- Swig ----------------------------------

# Force CMake to re-search for SWIG when CMAKE_PREFIX_PATH changes.
unset(SWIG_EXECUTABLE CACHE)
unset(SWIG_DIR CACHE)
unset(SWIG_VERSION CACHE)
unset(SWIG_FOUND CACHE)

if(BUILD_SKETCHER)
    find_package(SWIG QUIET)

    if(NOT SWIG_FOUND)
        message(FATAL_ERROR
                "-----------------------------------------------------\n"
                "SWIG not found, swig & pivy required for sketcher WB.\n"
                "-----------------------------------------------------\n")
        return()
    endif()

    message(STATUS "swig binary version building bundled pivy: ${SWIG_VERSION}")
endif()

endmacro()
