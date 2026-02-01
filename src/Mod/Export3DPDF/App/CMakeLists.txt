include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/3rdParty/libPRC/src/asymptote
    ${CMAKE_SOURCE_DIR}/src/3rdParty/libharu/include
    ${CMAKE_BINARY_DIR}/src/3rdParty/libharu/include
)

set(Export3DPDF_LIBS
    FreeCADApp
)

# Add libPRC library (part of FreeCAD 3rdParty)
set(Export3DPDF_LIBS ${Export3DPDF_LIBS}
    asymptote
    libPRC
)

# Add libharu library (part of FreeCAD 3rdParty) 
set(Export3DPDF_LIBS ${Export3DPDF_LIBS}
    hpdf
)

SET(Export3DPDF_SRCS
    AppExport3DPDF.cpp
    AppExport3DPDFPy.cpp
    Export3DPDFCore.cpp
    Export3DPDFCore.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Export3DPDF SHARED ${Export3DPDF_SRCS})
target_link_libraries(Export3DPDF ${Export3DPDF_LIBS})

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Export3DPDF)
endif()

SET_BIN_DIR(Export3DPDF Export3DPDF /Mod/Export3DPDF)
SET_PYTHON_PREFIX_SUFFIX(Export3DPDF)

INSTALL(TARGETS Export3DPDF DESTINATION ${CMAKE_INSTALL_LIBDIR}) 