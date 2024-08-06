include_directories(${Boost_INCLUDE_DIRS} ${OCC_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIR} ${XercesC_INCLUDE_DIRS})

set(_TEMPLATE__LIBS FreeCADApp)

set(_TEMPLATE__SRCS App_TEMPLATE_.cpp PreCompiled.cpp PreCompiled.h)

add_library(_TEMPLATE_ SHARED ${_TEMPLATE__SRCS})
target_link_libraries(_TEMPLATE_ ${_TEMPLATE__LIBS})

set_bin_dir(_TEMPLATE_ _TEMPLATE_ /Mod/_TEMPLATE_)
set_python_prefix_suffix(_TEMPLATE_)

install(TARGETS _TEMPLATE_ DESTINATION ${CMAKE_INSTALL_LIBDIR})
