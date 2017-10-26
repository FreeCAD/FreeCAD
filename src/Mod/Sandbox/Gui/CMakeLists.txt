if(MSVC)
    add_definitions(-DHAVE_PART -DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_PART -DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/Gui
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

if(BUILD_QT5)
    include_directories(
        ${Qt5Core_INCLUDE_DIRS}
    )
else()
    include_directories(
        ${QT_INCLUDE_DIR}
    )
endif()

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

if(BUILD_QT5)
    qt5_add_resources(Resource_SRCS Resources/Sandbox.qrc)
else()
    qt4_add_resources(Resource_SRCS Resources/Sandbox.qrc)
endif()
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
    TaskPanelView.cpp
    TaskPanelView.h
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
