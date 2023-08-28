# Locate PyCXX headers and source files

# This module defines
# PYCXX_INCLUDE_DIR
# PYCXX_SOURCE_DIR
# PYCXX_FOUND
# PYCXX_SOURCES
# PYCXX_VERSION
#
# The PYCXX_*_DIR variables can be set to tell this module where
# the files are.


# There's no standard location for PyCXX.
#
# The authors' example is to put it in "~\" [sic].
#
# Ubuntu puts the includes into /usr/include/python2.7/CXX and sources into
# /usr/share/python2.7/CXX.
#
# The Zultron Fedora RPM does the same as Ubuntu.

set(PYCXX_FOUND "YES")

# find the header directory
if(PYCXX_INCLUDE_DIR)
    # headers better be in there
    if(NOT EXISTS "${PYCXX_INCLUDE_DIR}/CXX/Config.hxx")
        if(PyCXX_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR
                "PyCXX: could not find CXX/Config.hxx in PYCXX_INCLUDE_DIR "
            "${PYCXX_INCLUDE_DIR}")
        else(PyCXX_FIND_REQUIRED)
            MESSAGE(WARNING
                "PyCXX: could not find CXX/Config.hxx in PYCXX_INCLUDE_DIR "
            "${PYCXX_INCLUDE_DIR}")
            unset(PYCXX_FOUND)
        endif(PyCXX_FIND_REQUIRED)
    endif(NOT EXISTS "${PYCXX_INCLUDE_DIR}/CXX/Config.hxx")
else(PYCXX_INCLUDE_DIR)
    # check in 'standard' places
    find_path(PYCXX_INCLUDE_DIR CXX/Config.hxx
        ${PYTHON_INCLUDE_DIR}
        "${CMAKE_CURRENT_LIST_DIR}/..")
    if(NOT PYCXX_INCLUDE_DIR)
        if(PyCXX_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR
                "PyCXX not found; please set PYCXX_INCLUDE_DIR to "
                "the location of CXX/Config.hxx")
        else(PyCXX_FIND_REQUIRED)
            MESSAGE(STATUS "PyCXX not found")
            unset(PYCXX_FOUND)
        endif(PyCXX_FIND_REQUIRED)
    endif(NOT PYCXX_INCLUDE_DIR)
endif(PYCXX_INCLUDE_DIR)

# find the sources directory
if(PYCXX_SOURCE_DIR)
    # source directory specified, they'd better be there
    if(NOT EXISTS "${PYCXX_SOURCE_DIR}/cxxextensions.c")
        if(PyCXX_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR
                "PyCXX: cxxextensions.c not found in PYCXX_SOURCE_DIR "
                "${PYCXX_SOURCE_DIR}")
        else(PyCXX_FIND_REQUIRED)
            MESSAGE(WARNING
                "PyCXX: cxxextensions.c not found in PYCXX_SOURCE_DIR "
                "${PYCXX_SOURCE_DIR}")
            unset(PYCXX_FOUND)
        endif(PyCXX_FIND_REQUIRED)
    endif(NOT EXISTS "${PYCXX_SOURCE_DIR}/cxxextensions.c")
else(PYCXX_SOURCE_DIR)
    # check in 'standard' places
    find_path(PYCXX_SOURCE_DIR cxxextensions.c
        "${PYCXX_INCLUDE_DIR}/CXX"
        "${PYCXX_INCLUDE_DIR}/Src"
        "${PYTHON_INCLUDE_DIR}/CXX"
        "${PYTHON_INCLUDE_DIR}/Src"
        "${CMAKE_CURRENT_LIST_DIR}/../Src"
        "${CMAKE_CURRENT_LIST_DIR}/../CXX")
    if(NOT PYCXX_SOURCE_DIR)
        if(PyCXX_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR
                "PyCXX not found; please set PYCXX_SOURCE_DIR to "
                "the location of cxxextensions.c")
        else(PyCXX_FIND_REQUIRED)
            MESSAGE(STATUS "PyCXX not found")
            unset(PYCXX_FOUND)
        endif(PyCXX_FIND_REQUIRED)
    endif(NOT PYCXX_SOURCE_DIR)
endif(PYCXX_SOURCE_DIR)

# Find PyCXX Version
if(PYCXX_INCLUDE_DIR AND PYCXX_SOURCE_DIR)
    file(READ ${PYCXX_INCLUDE_DIR}/CXX/Version.hxx PYCXX_VERSION_H)
    foreach(item IN ITEMS MAJOR MINOR PATCH)
        string(REGEX REPLACE
            ".*#define[ \t]+PYCXX_VERSION_${item}[ \t]+([0-9]+).*"
            "\\1" PYCXX_VERSION_${item}
            "${PYCXX_VERSION_H}"
        )
    endforeach()
    set(PYCXX_VERSION ${PYCXX_VERSION_MAJOR}.${PYCXX_VERSION_MINOR}.${PYCXX_VERSION_PATCH})
endif()

# see what we've got
if(PYCXX_FOUND)
    MESSAGE(STATUS "PyCXX found:")
    MESSAGE(STATUS "  Headers:  ${PYCXX_INCLUDE_DIR}")
    MESSAGE(STATUS "  Sources:  ${PYCXX_SOURCE_DIR}")
    MESSAGE(STATUS "  Version:  ${PYCXX_VERSION}")

    # Build the list of sources for convenience
    set(PYCXX_SOURCES
        ${PYCXX_SOURCE_DIR}/cxxextensions.c
        ${PYCXX_SOURCE_DIR}/cxx_extensions.cxx
        ${PYCXX_SOURCE_DIR}/cxxsupport.cxx
        ${PYCXX_SOURCE_DIR}/IndirectPythonInterface.cxx
    )
    if(NOT ${PYCXX_VERSION} VERSION_LESS 6.3.0)
        list(APPEND PYCXX_SOURCES
            ${PYCXX_SOURCE_DIR}/cxx_exceptions.cxx)
        add_definitions(-DPYCXX_6_2_COMPATIBILITY)
    endif()
else(PYCXX_FOUND)
    MESSAGE(STATUS "PyCXX not found")
endif(PYCXX_FOUND)

