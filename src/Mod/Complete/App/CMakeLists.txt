if(WIN32)
endif(WIN32)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(Complete_LIBS
    FreeCADApp
)

SET(Complete_SRCS
    AppComplete.cpp
    AppCompletePy.cpp
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
if(WIN32)
    set_target_properties(Complete PROPERTIES PREFIX "")
    set_target_properties(Complete PROPERTIES SUFFIX ".pyd")
endif(WIN32)

INSTALL(TARGETS Complete DESTINATION lib)
