# SPDX-License-Identifier: LGPL-2.1-or-later

set(CadXApp_LIBS
    FreeCADApp
    ${QtCore_LIBRARIES}
)

if(BUILD_ASSEMBLY)
    list(APPEND CadXApp_LIBS Assembly)
endif()

if(BUILD_PART_DESIGN)
    list(APPEND CadXApp_LIBS PartDesign)
endif()

set(CadXApp_SRCS
    AppCadX.cpp
    AppCadXPy.cpp
    AssemblyCapture.cpp
    AssemblyCapture.h
    AssemblyDocumentObserver.cpp
    AssemblyDocumentObserver.h
    AssemblyGraphBuilder.cpp
    AssemblyGraphBuilder.h
    AssemblyObjectAdapter.cpp
    AssemblyObjectAdapter.h
    CadXService.cpp
    CadXService.h
    GraphQuery.cpp
    GraphQuery.h
    GraphJsonCodec.cpp
    GraphJsonCodec.h
    GraphAudit.cpp
    GraphAudit.h
    GraphRevision.cpp
    GraphRevision.h
    GraphSnapshot.cpp
    GraphSnapshot.h
    GraphStore.cpp
    GraphStore.h
    GraphTypes.cpp
    GraphTypes.h
    MainThreadGateway.cpp
    MainThreadGateway.h
    NativeAssemblyOperations.cpp
    NativeAssemblyOperations.h
    NativeAssemblyConstraintOperations.cpp
    NativeAssemblyConstraintOperations.h
    NativeMutationSupport.cpp
    NativeMutationSupport.h
    ToolDefinition.cpp
    ToolDefinition.h
    ToolRegistry.cpp
    ToolRegistry.h
    ToolResult.cpp
    ToolResult.h
)

if(BUILD_PART_DESIGN)
    list(APPEND CadXApp_SRCS
        NativePrimitiveOperations.cpp
        NativePrimitiveOperations.h
    )
endif()

add_library(CadXApp SHARED ${CadXApp_SRCS})
target_include_directories(CadXApp
    PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)
if(BUILD_ASSEMBLY AND NOT FREECAD_USE_EXTERNAL_ONDSELSOLVER)
    # AssemblyObject.h includes OndselSolver headers.  Assembly's source-tree
    # include_directories() is directory-scoped and does not propagate to the
    # sibling CadX target, so make that dependency explicit here.
    target_include_directories(CadXApp PRIVATE
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver
    )
endif()
target_link_libraries(CadXApp ${CadXApp_LIBS})

if(BUILD_ASSEMBLY)
    target_compile_definitions(CadXApp PRIVATE CADX_HAVE_ASSEMBLY=1)
endif()

if(BUILD_PART_DESIGN)
    target_compile_definitions(CadXApp PRIVATE CADX_HAVE_PART_DESIGN=1)
endif()

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(CadXApp)
endif()

SET_BIN_DIR(CadXApp CadXApp /Mod/CadX)
SET_PYTHON_PREFIX_SUFFIX(CadXApp)
INSTALL(TARGETS CadXApp DESTINATION ${CMAKE_INSTALL_LIBDIR})
