# - Find Shiboken2 installation
#
# The following variable are set:
#   SHIBOKEN2_FOUND   - Shiboken2 was found
#   SHIBOKEN2_LIBRARY - The Shiboken2 libraries


execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import shiboken2;import os;print(os.path.dirname(shiboken2.__file__))"
    RESULT_VARIABLE SUCCESS
    OUTPUT_VARIABLE PRINT_OUTPUT
)

set(SHIBOKEN2_FOUND FALSE)
if(NOT SUCCESS)
    set(SHIBOKEN2_FOUND TRUE)
    set(SHIBOKEN2_LIBRARY ${PRINT_OUTPUT} CACHE PATH "The location of the Shiboken2 library")
endif(NOT SUCCESS)

if(NOT SHIBOKEN2_FOUND)
    MESSAGE("====================\n"
            "shiboken2 not found.\n"
            "====================\n")
endif()
