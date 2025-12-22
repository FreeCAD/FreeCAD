# - Find the lark library
# This module finds if lark is installed, and sets the following variables
# indicating where it is.
#
#  LARK_FOUND            - was lark found
#  LARK_VERSION          - the version of lark found as a string
#  LARK_VERSION_MAJOR    - the major version number of lark
#  LARK_VERSION_MINOR    - the minor version number of lark
#  LARK_VERSION_PATCH    - the patch version number of lark

if(Python3_EXECUTABLE)
    # try to import lark into Python interpreter
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "-c"
        "import lark; print(lark.__version__)"
        RESULT_VARIABLE _LARK_SEARCH_SUCCESS
        OUTPUT_VARIABLE _LARK_VERSION
        ERROR_VARIABLE _LARK_ERROR_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_LARK_SEARCH_SUCCESS MATCHES 0)
        set(LARK_FOUND TRUE)
        set(LARK_VERSION ${_LARK_VERSION})

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
        set(LARK_FOUND FALSE)
    endif()
else()
    set(LARK_FOUND FALSE)
endif()
