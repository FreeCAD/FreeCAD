# - Find Shiboken2 installation
#
# The following variable are set:
#   Shiboken2_FOUND   - Shiboken2 was found
#   SHIBOKEN2_FOUND   - Shiboken2 was found
#   Shiboken2_DIR     - The Shiboken2 libraries dir
#   SHIBOKEN2_LIBRARY - The Shiboken2 libraries


execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import shiboken2;import os;print(os.path.dirname(shiboken2.__file__), end='')"
    RESULT_VARIABLE FAILURE
    OUTPUT_VARIABLE PRINT_OUTPUT
)

set(SHIBOKEN2_FOUND FALSE)
if(NOT FAILURE)
    set(SHIBOKEN2_FOUND TRUE)
    set(SHIBOKEN2_LIBRARY ${PRINT_OUTPUT} CACHE PATH "The location of the Shiboken2 library")
endif(NOT FAILURE)

if(NOT SHIBOKEN2_FOUND)
    MESSAGE("====================\n"
            "shiboken2 not found.\n"
            "====================\n")
endif()

set(Shiboken2_FOUND ${SHIBOKEN2_FOUND})
set(Shiboken2_DIR ${SHIBOKEN2_LIBRARY})
