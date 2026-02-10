# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(ImportGui_LIBS
    FreeCADGui
    PartGui
    Import
    ${OCC_OCAF_LIBRARIES}
    ${OCC_OCAF_DEBUG_LIBRARIES}
)

SET(ImportGui_SRCS
    AppImportGui.cpp
    AppImportGuiPy.cpp
    Command.cpp
    ExportOCAFGui.cpp
    ExportOCAFGui.h
    dxf/ImpExpDxfGui.cpp
    dxf/ImpExpDxfGui.h
    ImportOCAFGui.cpp
    ImportOCAFGui.h
    OCAFBrowser.cpp
    OCAFBrowser.h
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(ImportGui SHARED ${ImportGui_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(ImportGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    ImportGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(ImportGui ${ImportGui_LIBS})

if (MSVC)
    target_compile_options(ImportGui PRIVATE /wd4251)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(ImportGui)
endif()


SET_BIN_DIR(ImportGui ImportGui /Mod/Import)
SET_PYTHON_PREFIX_SUFFIX(ImportGui)

INSTALL(TARGETS ImportGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
