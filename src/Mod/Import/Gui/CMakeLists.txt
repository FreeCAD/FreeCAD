include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS})

link_directories(${OCC_LIBRARY_DIR})

set(ImportGui_LIBS FreeCADGui PartGui Import ${OCC_OCAF_LIBRARIES} ${OCC_OCAF_DEBUG_LIBRARIES})

set(ImportGui_SRCS
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
    Workbench.h)

add_library(ImportGui SHARED ${ImportGui_SRCS})
target_link_libraries(ImportGui ${ImportGui_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(ImportGui)
endif()

set_bin_dir(ImportGui ImportGui /Mod/Import)
set_python_prefix_suffix(ImportGui)

install(TARGETS ImportGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
