if(WIN32)
    add_definitions(-DFCAppSandbox)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)


set(Sandbox_LIBS
    Mesh
)

SET(Sandbox_SRCS
    AppSandbox.cpp
    DocumentProtector.cpp
    DocumentProtector.h
    DocumentProtectorPy.cpp
    DocumentProtectorPy.h
    DocumentThread.cpp
    DocumentThread.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Sandbox SHARED ${Sandbox_SRCS})
target_link_libraries(Sandbox ${Sandbox_LIBS})


fc_target_copy_resource(Sandbox 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sandbox
    ${CMAKE_BINARY_DIR}/Mod/Sandbox
    Init.py)

SET_BIN_DIR(Sandbox Sandbox /Mod/Sandbox)
SET_PYTHON_PREFIX_SUFFIX(Sandbox)

INSTALL(TARGETS Sandbox DESTINATION ${CMAKE_INSTALL_LIBDIR})
