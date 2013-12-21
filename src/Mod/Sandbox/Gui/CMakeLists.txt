if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(SandboxGui_LIBS
    Sandbox
    Part
    FreeCADGui
)

qt4_add_resources(Resource_SRCS Resources/Sandbox.qrc)
SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Sandbox.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

SET(SandboxGui_SRCS
    AppSandboxGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(SandboxGui SHARED ${SandboxGui_SRCS})
target_link_libraries(SandboxGui ${SandboxGui_LIBS})


fc_target_copy_resource(SandboxGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sandbox
    ${CMAKE_BINARY_DIR}/Mod/Sandbox
    InitGui.py)

if(MSVC)
    set_target_properties(SandboxGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(SandboxGui PROPERTIES DEBUG_OUTPUT_NAME "SandboxGui_d")
    set_target_properties(SandboxGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(SandboxGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(SandboxGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(SandboxGui PROPERTIES DEBUG_OUTPUT_NAME "SandboxGui_d")
    set_target_properties(SandboxGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(SandboxGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(SandboxGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sandbox)
    set_target_properties(SandboxGui PROPERTIES PREFIX "")
    set_target_properties(SandboxGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS SandboxGui DESTINATION lib)
