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

if(MSVC)
    set_target_properties(Sandbox PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sandbox PROPERTIES DEBUG_OUTPUT_NAME "Sandbox_d")
    set_target_properties(Sandbox PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(Sandbox PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Sandbox PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sandbox PROPERTIES DEBUG_OUTPUT_NAME "Sandbox_d")
    set_target_properties(Sandbox PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(Sandbox PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Sandbox PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(Sandbox PROPERTIES PREFIX "")
    set_target_properties(Sandbox PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Sandbox DESTINATION lib)
