
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

set(PathSimulator_LIBS
    FreeCADApp
)

SET(PathSimulator_SRCS
    AppPathSimulator.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(PathSimulator SHARED ${PathSimulator_SRCS})
target_link_libraries(PathSimulator ${PathSimulator_LIBS})


fc_target_copy_resource(PathSimulator 
    ${CMAKE_SOURCE_DIR}/src/Mod/PathSimulator
    ${CMAKE_BINARY_DIR}/Mod/PathSimulator
    Init.py)

SET_BIN_DIR(PathSimulator PathSimulator /Mod/PathSimulator)
SET_PYTHON_PREFIX_SUFFIX(PathSimulator)

install(TARGETS PathSimulator DESTINATION ${CMAKE_INSTALL_LIBDIR})
