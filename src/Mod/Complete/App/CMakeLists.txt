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

if(MSVC)
    set_target_properties(Complete PROPERTIES SUFFIX ".pyd")
    set_target_properties(Complete PROPERTIES DEBUG_OUTPUT_NAME "Complete_d")
    set_target_properties(Complete PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(Complete PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Complete PROPERTIES SUFFIX ".pyd")
    set_target_properties(Complete PROPERTIES DEBUG_OUTPUT_NAME "Complete_d")
    set_target_properties(Complete PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(Complete PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Complete PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(Complete PROPERTIES PREFIX "")
    set_target_properties(Complete PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Complete DESTINATION lib)
