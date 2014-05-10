
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
 #   ${ODE_INCLUDE_DIRS}
)

set(Assembly_LIBS
#    ${ODE_LIBRARIES}
    FreeCADApp
)

SET(Assembly_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Assembly SHARED ${Assembly_SRCS})
target_link_libraries(Assembly ${Assembly_LIBS})


fc_target_copy_resource(Assembly 
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly
    ${CMAKE_BINARY_DIR}/Mod/Assembly
    Init.py)

SET_BIN_DIR(Assembly Assembly /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(Assembly)

INSTALL(TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
