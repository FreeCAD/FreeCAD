EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c 
"from distutils.sysconfig import get_python_lib; print(get_python_lib())"
OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
SET(PYTHON_MAIN_DIR ${python_libs})

SET(EXT_SRCS
    __init__.py
)

ADD_CUSTOM_TARGET(freecad_COPY_SOURCE ALL
    SOURCES ${EXT_SRCS})

fc_copy_sources(freecad_COPY_SOURCE "${CMAKE_BINARY_DIR}/Ext/freecad" ${EXT_SRCS})

INSTALL(
    FILES
        __init__.py
    DESTINATION
        Ext/freecad
)

add_subdirectory(fc_cadquery)
