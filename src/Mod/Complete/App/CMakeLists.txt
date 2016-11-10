if(WIN32)
endif(WIN32)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(Complete_LIBS
    FreeCADApp
)

SET(Complete_SRCS
    AppComplete.cpp
    PreCompiled.cpp
    PreCompiled.h
    CompleteConfiguration.h
)

add_library(Complete SHARED ${Complete_SRCS})
target_link_libraries(Complete ${Complete_LIBS})


fc_target_copy_resource(Complete 
    ${CMAKE_SOURCE_DIR}/src/Mod/Complete
    ${CMAKE_BINARY_DIR}/Mod/Complete
    Init.py)

SET_BIN_DIR(Complete Complete /Mod/Complete)
SET_PYTHON_PREFIX_SUFFIX(Complete)

INSTALL(TARGETS Complete DESTINATION ${CMAKE_INSTALL_LIBDIR})
