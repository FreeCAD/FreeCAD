# SPDX-FileNotice: Part of the FreeCAD project.

# Many warnings caused by vtk
if(CMAKE_COMPILER_IS_CLANGXX)
    add_compile_options(-Wno-pedantic) # needed for vtk headers
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0)
        add_compile_options(-Wno-inconsistent-missing-override)
    endif()
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wno-pedantic) # needed for vtk headers
endif()


if(BUILD_FEM_NETGEN)
    add_definitions(-DFCWithNetgen ${NETGEN_DEFINITIONS})
endif(BUILD_FEM_NETGEN)

set(Fem_LIBS
    Part
    FreeCADApp
)

if (FREECAD_USE_EXTERNAL_SMESH)
   list(APPEND Fem_LIBS ${EXTERNAL_SMESH_LIBS})
else()
   if (BUILD_FEM_NETGEN)
       list(APPEND Fem_LIBS StdMeshers SMESH SMDS SMESHDS NETGENPlugin)
   else ()
       list(APPEND Fem_LIBS StdMeshers SMESH SMDS SMESHDS)
   endif()
endif()


generate_from_py(FemMesh)


SET(Python_SRCS
    FemMesh.pyi
    FemMeshPyImp.cpp
    HypothesisPy.cpp
    HypothesisPy.h
)


if(BUILD_FEM_VTK)
    SET(Python_SRCS
        ${Python_SRCS}
        FemPostObject.pyi
        FemPostObjectPyImp.cpp
        FemPostPipeline.pyi
        FemPostPipelinePyImp.cpp
        FemPostFilter.pyi
        FemPostFilterPyImp.cpp
        FemPostBranchFilter.pyi
        FemPostBranchFilterPyImp.cpp
    )
    generate_from_py(FemPostObject)
    generate_from_py(FemPostPipeline)
    generate_from_py(FemPostFilter)
    generate_from_py(FemPostBranchFilter)

endif(BUILD_FEM_VTK)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


if(BUILD_FEM_VTK)
    SET(FemPost_SRCS
        PropertyPostDataObject.h
        PropertyPostDataObject.cpp
        FemPostObject.h
        FemPostObject.cpp
        FemPostGroupExtension.h
        FemPostGroupExtension.cpp
        FemPostPipeline.h
        FemPostPipeline.cpp
        FemPostBranchFilter.h
        FemPostBranchFilter.cpp
        FemPostFilter.h
        FemPostFilter.cpp
        FemPostFunction.h
        FemPostFunction.cpp
        FemVTKTools.h
        FemVTKTools.cpp
    )
    SOURCE_GROUP("PostObjects" FILES ${FemPost_SRCS})

    SET(FemVTK_SRCS
        VTKExtensions/vtkFemFrameSourceAlgorithm.h
        VTKExtensions/vtkFemFrameSourceAlgorithm.cpp
    )
    SET(VTK_SRCS_0903
        VTKExtensions/vtkCleanUnstructuredGrid.h
        VTKExtensions/vtkCleanUnstructuredGrid.cxx
        VTKExtensions/vtkSMPTools.h
    )
    if (${VTK_MAJOR_VERSION} EQUAL 9 AND ${VTK_MINOR_VERSION} LESS 3)
        list(APPEND FemVTK_SRCS ${VTK_SRCS_0903})
    endif()
    SOURCE_GROUP("VTKExtensions" FILES ${FemVTK_SRCS})
endif(BUILD_FEM_VTK)


SET(Mod_SRCS
    AppFem.cpp
    AppFemPy.cpp
    FemTools.cpp
    FemTools.h
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Mod_SRCS})


SET(FemBase_SRCS
    FemMeshObject.cpp
    FemMeshObject.h
    FemMeshShapeObject.cpp
    FemMeshShapeObject.h
    FemMeshShapeNetgenObject.cpp
    FemMeshShapeNetgenObject.h
    FemAnalysis.cpp
    FemAnalysis.h
    FemMesh.cpp
    FemMesh.h
    FemResultObject.cpp
    FemResultObject.h
    FemSolverObject.cpp
    FemSolverObject.h
    FemConstraint.cpp
    FemConstraint.h
    FemMeshProperty.cpp
    FemMeshProperty.h
    )
SOURCE_GROUP("Base types" FILES ${FemBase_SRCS})


SET(FemSet_SRCS
    FemSetObject.cpp
    FemSetObject.h
    FemSetNodesObject.cpp
    FemSetNodesObject.h
    FemSetElementNodesObject.cpp
    FemSetElementNodesObject.h
    FemSetElementsObject.cpp
    FemSetElementsObject.h
    FemSetFacesObject.cpp
    FemSetFacesObject.h
    FemSetGeometryObject.cpp
    FemSetGeometryObject.h
    )
SOURCE_GROUP("Set objects" FILES ${FemSet_SRCS})


SET(FemConstraints_SRCS
    FemConstraintBearing.h
    FemConstraintBearing.cpp
    FemConstraintFixed.cpp
    FemConstraintFixed.h
    FemConstraintRigidBody.cpp
    FemConstraintRigidBody.h
    FemConstraintForce.cpp
    FemConstraintForce.h
    FemConstraintFluidBoundary.cpp
    FemConstraintFluidBoundary.h
    FemConstraintPressure.cpp
    FemConstraintPressure.h
    FemConstraintSpring.cpp
    FemConstraintSpring.h
    FemConstraintGear.cpp
    FemConstraintGear.h
    FemConstraintPulley.cpp
    FemConstraintPulley.h
    FemConstraintDisplacement.h
    FemConstraintDisplacement.cpp
    FemConstraintTemperature.h
    FemConstraintTemperature.cpp
    FemConstraintHeatflux.h
    FemConstraintHeatflux.cpp
    FemConstraintInitialTemperature.h
    FemConstraintInitialTemperature.cpp
    FemConstraintPlaneRotation.cpp
    FemConstraintPlaneRotation.h
    FemConstraintContact.cpp
    FemConstraintContact.h
    FemConstraintTransform.cpp
    FemConstraintTransform.h
)
SOURCE_GROUP("Constraints" FILES ${FemConstraints_SRCS})


SET(Fem_SRCS
    ${FemBase_SRCS}
    ${FemConstraints_SRCS}
    ${FemPost_SRCS}
    ${FemSet_SRCS}
    ${FemVTK_SRCS}
    ${Mod_SRCS}
    ${Python_SRCS}
)

add_library(Fem SHARED ${Fem_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Fem PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Fem
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Fem
    SYSTEM
    PUBLIC
    ${SMESH_INCLUDE_DIR}
    ${NETGEN_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
)

target_link_directories(Fem PUBLIC ${SMESH_LIB_PATH})
target_link_libraries(Fem ${Fem_LIBS} ${VTK_LIBRARIES})

if (MSVC)
    target_compile_options(Fem PRIVATE /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Fem)
endif()

find_package(OpenMP 4.0)
if(OpenMP_CXX_FOUND)
	target_link_libraries(Fem OpenMP::OpenMP_CXX)
endif()


SET_BIN_DIR(Fem Fem /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(Fem)


INSTALL(TARGETS Fem DESTINATION ${CMAKE_INSTALL_LIBDIR})
