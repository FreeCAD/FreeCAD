include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

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
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(ImportGui SHARED ${ImportGui_SRCS})
target_link_libraries(ImportGui ${ImportGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(ImportGui)
endif()


SET_BIN_DIR(ImportGui ImportGui /Mod/Import)
SET_PYTHON_PREFIX_SUFFIX(ImportGui)

INSTALL(TARGETS ImportGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
