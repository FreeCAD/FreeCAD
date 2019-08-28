# - Find Shiboken2 installation
#
# The following variable are set:
#   Shiboken2_FOUND   - Shiboken2 was found
#   Shiboken2_LIBRARY - The Shiboken2 shared library


execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "
import shiboken2
import os
import re
dirname = os.path.dirname(shiboken2.__file__)
for fname in os.listdir(dirname):
    match = re.match(r'(shiboken2.*).so', fname)
    if match is not None:
        print(match.group(1), end='')
        break
"
    RESULT_VARIABLE FAILURE
    OUTPUT_VARIABLE SHIBOKEN_LIBRARY_NAME
)

if(NOT FAILURE)
    # Get the directory of the python module to use as a HINT for find_library
    execute_process(
        COMMAND 
        ${PYTHON_EXECUTABLE} -c "
import shiboken2
import os
print(os.path.dirname(shiboken2.__file__),end='')
"
        OUTPUT_VARIABLE SHIBOKEN_LIBRARY_HINT
    )
    # Get the version of the shiboken module, which is likely part of the library name
    execute_process(
        COMMAND 
        ${PYTHON_EXECUTABLE} -c "from shiboken2 import _config;print(_config.shiboken_library_soversion, end/'')"
        OUTPUT_VARIABLE SHIBOKEN_LIBRARY_VERSION
    )
    set(_NAMES
        ${SHIBOKEN_LIBRARY_NAME}
        lib${SHIBOKEN_LIBRARY_NAME}
        lib${SHIBOKEN_LIBRARY_NAME}.so
        lib${SHIBOKEN_LIBRARY_NAME}.so.${SHIBOKEN_LIBRARY_VERSION}
        lib${SHIBOKEN_LIBRARY_NAME}.dll
        lib${SHIBOKEN_LIBRARY_NAME}.dll.a
    )
    message("SHIBOKEN_LIBRARY_NAME = ${SHIBOKEN_LIBRARY_NAME}")
    message("SHIBOKEN_LIBRARY_HINT = ${SHIBOKEN_LIBRARY_HINT}")
    find_library(Shiboken2_LIBRARY 
        NAMES "lib${SHIBOKEN_LIBRARY_NAME}.so.5.12"
        HINTS ${SHIBOKEN_LIBRARY_HINT} ENV PATH
        DOC "The Shiboken2 shared library. The shiboken2 python module uses this, and you can too."
    )
    message("Shiboken2_LIBRARY = ${Shiboken2_LIBRARY}")
endif(NOT FAILURE)

# this cmake package handles the expected standard arguments for FindXXX
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Shiboken2 DEFAULT_MSG Shiboken2_LIBRARY)
