
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

set(_TEMPLATE__LIBS
    FreeCADApp
)

SET(_TEMPLATE__SRCS
    App_TEMPLATE_.cpp
    App_TEMPLATE_Py.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(_TEMPLATE_ SHARED ${_TEMPLATE__SRCS})
target_link_libraries(_TEMPLATE_ ${_TEMPLATE__LIBS})


fc_target_copy_resource(_TEMPLATE_ 
    ${CMAKE_SOURCE_DIR}/src/Tools/_TEMPLATE_
    ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_
    Init.py)

SET_BIN_DIR(_TEMPLATE_ _TEMPLATE_ /Mod/_TEMPLATE_)
SET_PYTHON_PREFIX_SUFFIX(_TEMPLATE_)

install(TARGETS _TEMPLATE_ DESTINATION ${CMAKE_INSTALL_LIBDIR})
