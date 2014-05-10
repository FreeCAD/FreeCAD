include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(Start_LIBS
    FreeCADApp
)

SET(Start_SRCS
    AppStart.cpp
    AppStartPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    StartConfiguration.h
)



add_library(Start SHARED ${Start_SRCS})
target_link_libraries(Start ${Start_LIBS})


fc_target_copy_resource(Start 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start
    ${CMAKE_BINARY_DIR}/Mod/Start
    Init.py)

SET_BIN_DIR(Start Start /Mod/Start)
SET_PYTHON_PREFIX_SUFFIX(Start)

INSTALL(TARGETS Start DESTINATION ${CMAKE_INSTALL_LIBDIR})
