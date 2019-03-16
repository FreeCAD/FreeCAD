# - Find PySide2 installation
#
# The following variable are set:
#   PYSIDE2_FOUND   - PySide2 was found
#   PYSIDE2_LIBRARY - The PySide2 libraries
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import PySide2;import os;print(os.path.dirname(PySide2.__file__))"
    RESULT_VARIABLE SUCCESS
    OUTPUT_VARIABLE PRINT_OUTPUT
)

set(PYSIDE2_FOUND FALSE)
if(NOT SUCCESS)
    set(PYSIDE2_FOUND TRUE)
    set(PYSIDE2_LIBRARY ${PRINT_OUTPUT} CACHE PATH "The location of the PySide2 library")
endif(NOT SUCCESS)

if(NOT PYSIDE2_FOUND)
    MESSAGE("==================\n"
            "PySide2 not found.\n"
            "==================\n")
endif(NOT PYSIDE2_FOUND)
