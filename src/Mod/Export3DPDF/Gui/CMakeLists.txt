include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(Export3DPDFGui_LIBS
    FreeCADGui
    Export3DPDF
    Part
    PartGui
    Mesh
)

SET(Export3DPDFGui_SRCS
    AppExport3DPDFGui.cpp
    AppExport3DPDFGuiPy.cpp
    Command.cpp
    Command.h
    Export3DPDFGui.cpp
    Export3DPDFGui.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Export3DPDFGui SHARED ${Export3DPDFGui_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Export3DPDFGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)
target_link_libraries(Export3DPDFGui ${Export3DPDFGui_LIBS})

# Ensure FreeCADGui is built before Export3DPDFGui
add_dependencies(Export3DPDFGui FreeCADGui)

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Export3DPDFGui)
endif()

SET_BIN_DIR(Export3DPDFGui Export3DPDFGui /Mod/Export3DPDF)
SET_PYTHON_PREFIX_SUFFIX(Export3DPDFGui)

INSTALL(TARGETS Export3DPDFGui DESTINATION ${CMAKE_INSTALL_LIBDIR}) 