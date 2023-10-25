
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


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
    ${NETGEN_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
)


link_directories(${OCC_LIBRARY_DIR})
link_directories(${SMESH_LIB_PATH})


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


generate_from_xml(FemMeshPy)
generate_from_xml(FemPostPipelinePy)


SET(Python_SRCS
    FemMeshPy.xml
    FemMeshPyImp.cpp
    HypothesisPy.cpp
    HypothesisPy.h
)


if(BUILD_FEM_VTK)
    SET(Python_SRCS
        ${Python_SRCS}
        FemPostPipelinePy.xml
        FemPostPipelinePyImp.cpp
    )
endif(BUILD_FEM_VTK)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


if(BUILD_FEM_VTK)
    SET(FemPost_SRCS
        PropertyPostDataObject.h
        PropertyPostDataObject.cpp
        FemPostObject.h
        FemPostObject.cpp
        FemPostPipeline.h
        FemPostPipeline.cpp
        FemPostFilter.h
        FemPostFilter.cpp
        FemPostFunction.h
        FemPostFunction.cpp
        FemVTKTools.h
        FemVTKTools.cpp
    )
    SOURCE_GROUP("PostObjects" FILES ${FemPost_SRCS})
endif(BUILD_FEM_VTK)


SET(Mod_SRCS
    AppFem.cpp
    AppFemPy.cpp
    FemTools.cpp
    FemTools.h
    PreCompiled.cpp
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
    ${Mod_SRCS}
    ${Python_SRCS}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Fem_CPP_SRCS ${Fem_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Fem PreCompiled.h PreCompiled.cpp Fem_CPP_SRCS)
endif(FREECAD_USE_PCH)

add_library(Fem SHARED ${Fem_SRCS})
target_link_libraries(Fem ${Fem_LIBS} ${VTK_LIBRARIES})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(Fem OpenMP::OpenMP_CXX)
endif()


SET_BIN_DIR(Fem Fem /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(Fem)


INSTALL(TARGETS Fem DESTINATION ${CMAKE_INSTALL_LIBDIR})
