# SPDX-License-Identifier: LGPL-2.1-or-later

set(Sketcher3DGui_LIBS
    Sketcher3D
    PartGui
    FreeCADGui
)

SET(Sketcher3DGui_SRCS
    AppSketcher3DGui.cpp
    AutoConstraint3D.h
    Command.cpp
    DlgEditConstraintValue.cpp
    DlgEditConstraintValue.h
    DrawSketchHandler3D.cpp
    DrawSketchHandler3D.h
    DrawSketchHandlerLine3D.cpp
    DrawSketchHandlerLine3D.h
    DrawSketchHandlerPoint3D.cpp
    DrawSketchHandlerPoint3D.h
    DrawSketchHandlerPolyline3D.cpp
    DrawSketchHandlerPolyline3D.h
    PreCompiled.h
    SnapManager3D.cpp
    SnapManager3D.h
    TaskDlgEditSketch3D.cpp
    TaskDlgEditSketch3D.h
    TaskSketcher3DTool.cpp
    TaskSketcher3DTool.h
    Utils.cpp
    Utils.h
    ViewProviderSketch3D.cpp
    ViewProviderSketch3D.h
    WorkbenchManipulator.cpp
    WorkbenchManipulator.h
)

add_library(Sketcher3DGui SHARED ${Sketcher3DGui_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Sketcher3DGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Sketcher3DGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Sketcher3DGui ${Sketcher3DGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Sketcher3DGui)
endif()

SET_BIN_DIR(Sketcher3DGui Sketcher3DGui /Mod/Sketcher3D)
SET_PYTHON_PREFIX_SUFFIX(Sketcher3DGui)

INSTALL(TARGETS Sketcher3DGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
