# SPDX-FileNotice: Part of the FreeCAD project.

# - Find the lark library
# This module finds if lark is installed, and sets the following variables
# indicating where it is.
#
#  LARK_FOUND            - was lark found
#  LARK_VERSION          - the version of lark found as a string
#  LARK_VERSION_MAJOR    - the major version number of lark
#  LARK_VERSION_MINOR    - the minor version number of lark
#  LARK_VERSION_PATCH    - the patch version number of lark

include(FindPackageHandleStandardArgs)

if(Python3_EXECUTABLE)
    message(STATUS "FindLark: Using Python3_EXECUTABLE = ${Python3_EXECUTABLE}")

    # try to import lark into Python interpreter
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "-c"
        "import lark; print(lark.__version__)"
        RESULT_VARIABLE _LARK_SEARCH_SUCCESS
        OUTPUT_VARIABLE LARK_VERSION
        ERROR_VARIABLE _LARK_ERROR_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(DEBUG "FindLark: Result = ${_LARK_SEARCH_SUCCESS}")
    message(DEBUG "FindLark: Version = ${LARK_VERSION}")
    message(DEBUG "FindLark: Error = ${_LARK_ERROR_VALUE}")

    if(_LARK_SEARCH_SUCCESS MATCHES 0)
        # extract version components
        string(REGEX REPLACE "\\." ";" _LARK_VERSION_LIST ${LARK_VERSION})
        list(LENGTH _LARK_VERSION_LIST _LARK_VERSION_LIST_LEN)
        if(_LARK_VERSION_LIST_LEN GREATER_EQUAL 1)
            list(GET _LARK_VERSION_LIST 0 LARK_VERSION_MAJOR)
        endif()
        if(_LARK_VERSION_LIST_LEN GREATER_EQUAL 2)
            list(GET _LARK_VERSION_LIST 1 LARK_VERSION_MINOR)
        endif()
        if(_LARK_VERSION_LIST_LEN GREATER_EQUAL 3)
            list(GET _LARK_VERSION_LIST 2 LARK_VERSION_PATCH)
        endif()
    else()
        message(STATUS "The BIM workbench requires the lark python package / module to be installed")
    endif()
else()
    message(STATUS "FindLark: Python3_EXECUTABLE not set")
endif()

find_package_handle_standard_args(LARK
    REQUIRED_VARS LARK_VERSION
    VERSION_VAR LARK_VERSION
)
