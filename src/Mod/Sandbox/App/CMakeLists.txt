if(WIN32)
    add_definitions(-DFCAppSandbox)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)

if (BUILD_QT5)
    include_directories(
        ${Qt5Core_INCLUDE_DIRS}
    )
else()
    include_directories(
        ${QT_INCLUDE_DIR}
    )
endif()


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

set (Sandbox_Scripts
    ../Init.py
    ../exportDRAWEXE.py
)

add_library(Sandbox SHARED ${Sandbox_SRCS} ${Sandbox_Scripts})
target_link_libraries(Sandbox ${Sandbox_LIBS})


fc_target_copy_resource_flat(Sandbox
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Sandbox
    ${Sandbox_Scripts})

SET_BIN_DIR(Sandbox Sandbox /Mod/Sandbox)
SET_PYTHON_PREFIX_SUFFIX(Sandbox)

INSTALL(TARGETS Sandbox DESTINATION ${CMAKE_INSTALL_LIBDIR})
