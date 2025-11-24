include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Gui
)

if(BUILD_QT5)
    include_directories(
        SYSTEM
        ${Qt5Core_INCLUDE_DIRS}
    )
else()
    include_directories(
        SYSTEM
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

qt_add_resources(Resource_SRCS Resources/Sandbox.qrc)
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
    PreCompiled.h
    TaskPanelView.cpp
    TaskPanelView.h
    Workbench.cpp
    Workbench.h
)

set (SandboxGui_Scripts
    ../InitGui.py
)

add_library(SandboxGui SHARED ${SandboxGui_SRCS} ${SandboxGui_Scripts})
target_link_libraries(SandboxGui ${SandboxGui_LIBS})

if (MSVC)
    target_compile_definitions(SandboxGui PRIVATE WIN32_LEAN_AND_MEAN)
endif()


if(FREECAD_USE_PCH)
    target_precompile_headers(SandboxGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

fc_target_copy_resource_flat(SandboxGui
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Sandbox
    ${SandboxGui_Scripts})

SET_BIN_DIR(SandboxGui SandboxGui /Mod/Sandbox)
SET_PYTHON_PREFIX_SUFFIX(SandboxGui)

INSTALL(TARGETS SandboxGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
