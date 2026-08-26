# SPDX-License-Identifier: LGPL-2.1-or-later

set(CadXGuiApp_LIBS
    CadXApp
    FreeCADGui
)

if(BUILD_ASSEMBLY)
    list(APPEND CadXGuiApp_LIBS Assembly)
endif()

set(CadXGuiApp_SRCS
    ActiveAssemblyResolver.cpp
    ActiveAssemblyResolver.h
    AppCadXGui.cpp
    AppCadXGuiPy.cpp
    AssemblyViewCapture.cpp
    AssemblyViewCapture.h
    MainThreadGateway.h
)

add_library(CadXGuiApp SHARED ${CadXGuiApp_SRCS})
target_include_directories(CadXGuiApp
    PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver
)
target_link_libraries(CadXGuiApp ${CadXGuiApp_LIBS})

if(BUILD_ASSEMBLY)
    target_compile_definitions(CadXGuiApp PRIVATE CADX_HAVE_ASSEMBLY=1)
endif()

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(CadXGuiApp)
endif()

SET_BIN_DIR(CadXGuiApp CadXGuiApp /Mod/CadX)
SET_PYTHON_PREFIX_SUFFIX(CadXGuiApp)
INSTALL(TARGETS CadXGuiApp DESTINATION ${CMAKE_INSTALL_LIBDIR})
