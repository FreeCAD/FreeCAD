# - Find the matplotlib libraries
# This module finds IF matplotlib is installed, and sets the following variables
# indicating where it is.
#
#  MATPLOTLIB_FOUND            - was matplotlib found
#  MATPLOTLIB_VERSION          - the version of matplotlib found as a string
#  MATPLOTLIB_VERSION_MAJOR    - the major version number of matplotlib
#  MATPLOTLIB_VERSION_MINOR    - the minor version number of matplotlib
#  MATPLOTLIB_VERSION_PATCH    - the patch version number of matplotlib
#  MATPLOTLIB_PATH_DIRS        - path to the matplotlib include files

IF(Python3_Interpreter_FOUND)
    # Try to import matplotlib into Python interpreter. Python
    # interpreter was found previously as required package, so
    # don't take care about this.
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
        "import matplotlib as m; print(m.__version__); print(m.__path__[0]);"
        RESULT_VARIABLE _MATPLOTLIB_SEARCH_SUCCESS
        OUTPUT_VARIABLE _MATPLOTLIB_VALUES
        ERROR_VARIABLE _MATPLOTLIB_ERROR_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    IF(_MATPLOTLIB_SEARCH_SUCCESS MATCHES 0)
        set(MATPLOTLIB_FOUND TRUE)

        # Convert the process output into a list
        string(REGEX REPLACE ";" "\\\\;" _MATPLOTLIB_VALUES ${_MATPLOTLIB_VALUES})
        string(REGEX REPLACE "\n" ";" _MATPLOTLIB_VALUES ${_MATPLOTLIB_VALUES})
        list(GET _MATPLOTLIB_VALUES 0 MATPLOTLIB_VERSION)
        list(GET _MATPLOTLIB_VALUES 1 MATPLOTLIB_PATH_DIRS)

        # Make sure all directory separators are '/'
        string(REGEX REPLACE "\\\\" "/" MATPLOTLIB_PATH_DIRS ${MATPLOTLIB_PATH_DIRS})

        # Get the major and minor version numbers
        string(REGEX REPLACE "\\." ";" _MATPLOTLIB_VERSION_LIST ${MATPLOTLIB_VERSION})
        list(GET _MATPLOTLIB_VERSION_LIST 0 MATPLOTLIB_VERSION_MAJOR)
        list(GET _MATPLOTLIB_VERSION_LIST 1 MATPLOTLIB_VERSION_MINOR)
        list(GET _MATPLOTLIB_VERSION_LIST 2 MATPLOTLIB_VERSION_PATCH)
    ELSE()
        set(MATPLOTLIB_FOUND FALSE)
    ENDIF()
ELSE()
    set(MATPLOTLIB_FOUND FALSE)
ENDIF()
