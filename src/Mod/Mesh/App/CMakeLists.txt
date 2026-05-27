# SPDX-License-Identifier: LGPL-2.1-or-later

add_library(Mesh SHARED)

if(WIN32)
    add_definitions(-DFCAppMesh -DWM4_FOUNDATION_DLL_EXPORT)
endif(WIN32)

target_include_directories(
    Mesh
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Mesh
    SYSTEM
    PUBLIC
    ${KDTREE_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${QtConcurrent_INCLUDE_DIRS}
)

set(Mesh_LIBS
    FreeCADBase
    FreeCADApp
)

list(APPEND Mesh_LIBS
    ${QtConcurrent_LIBRARIES}
)

generate_from_py(Edge)
generate_from_py(Facet)
generate_from_py(MeshFeature)
generate_from_py(MeshPoint)
generate_from_py(Mesh)

SET(Mesh_XML_SRCS
    Edge.pyi
    Facet.pyi
    MeshFeature.pyi
    MeshPoint.pyi
    Mesh.pyi
)
SOURCE_GROUP("XML" FILES ${Mesh_XML_SRCS})

SET(Core_SRCS
    Core/Algorithm.cpp
    Core/Algorithm.h
    Core/Approximation.cpp
    Core/Approximation.h
    Core/Builder.cpp
    Core/Builder.h
    Core/Curvature.cpp
    Core/Curvature.h
    Core/Decimation.cpp
    Core/Decimation.h
    Core/Definitions.cpp
    Core/Definitions.h
    Core/Degeneration.cpp
    Core/Degeneration.h
    Core/Elements.cpp
    Core/Elements.h
    Core/Evaluation.cpp
    Core/Evaluation.h
    Core/Grid.cpp
    Core/Grid.h
    Core/Helpers.h
    Core/Info.cpp
    Core/Info.h
    Core/Iterator.h
    Core/KDTree.cpp
    Core/KDTree.h
    Core/MeshIO.cpp
    Core/MeshIO.h
    Core/MeshKernel.cpp
    Core/MeshKernel.h
    Core/Projection.cpp
    Core/Projection.h
    Core/Segmentation.cpp
    Core/Segmentation.h
    Core/SetOperations.cpp
    Core/SetOperations.h
    Core/Smoothing.cpp
    Core/Smoothing.h
    Core/Tools.cpp
    Core/Tools.h
    Core/TopoAlgorithm.cpp
    Core/TopoAlgorithm.h
    Core/Triangulation.cpp
    Core/Triangulation.h
    Core/Trim.cpp
    Core/Trim.h
    Core/TrimByPlane.cpp
    Core/TrimByPlane.h
    Core/tritritest.h
    Core/Utilities.h
    Core/Visitor.cpp
    Core/Visitor.h
    Core/CylinderFit.cpp
    Core/CylinderFit.h
    Core/SphereFit.cpp
    Core/SphereFit.h
    Core/IO/Reader3MF.cpp
    Core/IO/Reader3MF.h
    Core/IO/ReaderOBJ.cpp
    Core/IO/ReaderOBJ.h
    Core/IO/ReaderPLY.cpp
    Core/IO/ReaderPLY.h
    Core/IO/Writer3MF.cpp
    Core/IO/Writer3MF.h
    Core/IO/WriterInventor.cpp
    Core/IO/WriterInventor.h
    Core/IO/WriterOBJ.cpp
    Core/IO/WriterOBJ.h
)
SOURCE_GROUP("Core" FILES ${Core_SRCS})

SET(Mesh_SRCS
    ${Mesh_XML_SRCS}
    AppMesh.cpp
    AppMeshPy.cpp
    Exporter.cpp
    Exporter.h
    Importer.cpp
    Importer.h
    Edge.cpp
    Edge.h
    EdgePyImp.cpp
    Facet.cpp
    Facet.h
    FacetPyImp.cpp
    FeatureMeshCurvature.cpp
    FeatureMeshCurvature.h
    FeatureMeshDefects.cpp
    FeatureMeshDefects.h
    FeatureMeshExport.cpp
    FeatureMeshExport.h
    FeatureMeshImport.cpp
    FeatureMeshImport.h
    FeatureMeshSegmentByMesh.cpp
    FeatureMeshSegmentByMesh.h
    FeatureMeshSetOperations.cpp
    FeatureMeshSetOperations.h
    FeatureMeshSolid.cpp
    FeatureMeshSolid.h
    FeatureMeshTransform.cpp
    FeatureMeshTransform.h
    FeatureMeshTransformDemolding.cpp
    FeatureMeshTransformDemolding.h
    Mesh.cpp
    Mesh.h
    MeshFeature.cpp
    MeshFeature.h
    MeshFeaturePyImp.cpp
    MeshPointPyImp.cpp
    MeshPoint.h
    MeshProperties.cpp
    MeshProperties.h
    MeshPyImp.cpp
    MeshTexture.cpp
    MeshTexture.h
    PreCompiled.h
    Segment.cpp
    Segment.h
)

# Suppress -Wundefined-var-template
if (MINGW AND CMAKE_COMPILER_IS_CLANGXX)
    unset(_flag_found CACHE)
    check_cxx_compiler_flag("-Wno-undefined-var-template" _flag_found)
    if (_flag_found)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
    endif()
endif()

if(FREECAD_USE_PCH)
    target_precompile_headers(Mesh PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

# Suppress some very long Eigen3 warnings of older versions
if (EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        Core/Approximation.cpp
        Core/CylinderFit.cpp
        Core/SphereFit.cpp
        Core/KDTree.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif ()

target_sources(Mesh PRIVATE ${Core_SRCS} ${Mesh_SRCS})
target_link_libraries(Mesh ${Mesh_LIBS} WildMagic4)

if (MSVC)
    target_compile_options(Mesh PRIVATE /wd4251 /wd4275 /wd4661)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Mesh)
endif()


SET_BIN_DIR(Mesh)
SET_PYTHON_PREFIX_SUFFIX(Mesh)

INSTALL(TARGETS Mesh DESTINATION ${CMAKE_INSTALL_LIBDIR})
