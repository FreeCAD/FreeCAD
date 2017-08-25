EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c 
"from distutils.sysconfig import get_python_lib; print(get_python_lib())"
OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
SET(PYTHON_MAIN_DIR ${python_libs})

SET(FREECAD_SRC
    __init__.py
)

ADD_CUSTOM_TARGET(freecad ALL
    SOURCES ${FREECAD_SRC})

configure_file(__init__.py ${CMAKE_BINARY_DIR}/freecad/__init__.py @ONLY)

INSTALL(
    FILES
        ${FREECAD_SRC}
    DESTINATION
        ${PYTHON_MAIN_DIR}/freecad
)