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

set(SandboxGui_MOC_HDRS
    GLGraphicsView.h
)

fc_wrap_cpp(SandboxGui_MOC_SRCS ${SandboxGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SandboxGui_MOC_SRCS})

qt4_add_resources(Resource_SRCS Resources/Sandbox.qrc)
SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Sandbox.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

SET(SandboxGui_SRCS
    AppSandboxGui.cpp
    Command.cpp
    GLGraphicsView.cpp
    GLGraphicsView.h
    Overlay.cpp
    Overlay.h
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

SET_BIN_DIR(SandboxGui SandboxGui /Mod/Sandbox)
SET_PYTHON_PREFIX_SUFFIX(SandboxGui)

INSTALL(TARGETS SandboxGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
