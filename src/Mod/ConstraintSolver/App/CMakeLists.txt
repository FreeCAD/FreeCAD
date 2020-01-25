if(MSVC)
    add_definitions(-DFCAppSketcher -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

set(ConstraintSolver_LIBS
    FreeCADApp
)

generate_from_xml(ParameterStorePy)
generate_from_xml(ParameterRefPy)
generate_from_xml(ParameterSubsetPy)
generate_from_xml(ValueSetPy)
generate_from_xml(ConstraintPy)
generate_from_xml(SimpleConstraintPy)
generate_from_xml(ParaObjectPy)
generate_from_xml(ParaGeometryPy)
generate_from_xml(SubSystemPy)
generate_from_xml(SolverBackendPy)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/G2D)
generate_from_xml(G2D/VectorPy)
generate_from_xml(G2D/ParaGeometry2DPy)
generate_from_xml(G2D/ParaVectorPy)
generate_from_xml(G2D/ParaPointPy)
generate_from_xml(G2D/ParaTransformPy)
generate_from_xml(G2D/ParaPlacementPy)
generate_from_xml(G2D/ParaShapePy)
generate_from_xml(G2D/ParaCurvePy)
generate_from_xml(G2D/ParaLinePy)

generate_from_xml(G2D/ConstraintDistancePy)
generate_from_xml(G2D/ConstraintPlacementRulesPy)

SET(ConstraintSolver_G2D
    G2D/Vector.h
    G2D/Vector.cpp
    G2D/VectorPy.xml
    G2D/VectorPyImp.cpp
    G2D/Position.h
    G2D/Position.cpp
    G2D/Placement.h
    G2D/Placement.cpp
    G2D/ParaGeometry2D.h
    G2D/ParaGeometry2D.cpp
    G2D/ParaGeometry2DPy.xml
    G2D/ParaGeometry2DPyImp.cpp
    G2D/ParaVector.h
    G2D/ParaVector.cpp
    G2D/ParaVectorPy.xml
    G2D/ParaVectorPyImp.cpp
    G2D/ParaPoint.h
    G2D/ParaPoint.cpp
    G2D/ParaPointPy.xml
    G2D/ParaPointPyImp.cpp
    G2D/ParaPlacement.h
    G2D/ParaPlacement.cpp
    G2D/ParaPlacementPy.xml
    G2D/ParaPlacementPyImp.cpp
    G2D/ParaTransform.h
    G2D/ParaTransform.cpp
    G2D/ParaTransformPy.xml
    G2D/ParaTransformPyImp.cpp
    G2D/ParaShape.h
    G2D/ParaShape.cpp
    G2D/ParaShapePy.xml
    G2D/ParaShapePyImp.cpp    
    G2D/ParaCurve.h
    G2D/ParaCurve.cpp
    G2D/ParaCurvePy.xml
    G2D/ParaCurvePyImp.cpp
    G2D/ParaLine.h
    G2D/ParaLine.cpp
    G2D/ParaLinePy.xml
    G2D/ParaLinePyImp.cpp
    G2D/ConstraintDistance.h
    G2D/ConstraintDistance.cpp
    G2D/ConstraintDistancePy.xml
    G2D/ConstraintDistancePyImp.cpp
    G2D/ConstraintPlacementRules.h
    G2D/ConstraintPlacementRules.cpp
    G2D/ConstraintPlacementRulesPy.xml
    G2D/ConstraintPlacementRulesPyImp.cpp
)

SET(ConstraintSolver_Utils_SRCS
    Utils.h
    Utils.cpp
    PyUtils.h
    PyUtils.cpp
    DualMath.h
    DualMath.cpp
)

SET(Python_Solver_SRCS
    ParameterStorePy.xml
    ParameterStorePyImp.cpp
    ParameterRefPy.xml
    ParameterRefPyImp.cpp
    ParameterSubsetPy.xml
    ParameterSubsetPyImp.cpp
    ValueSetPy.xml
    ValueSetPyImp.cpp
    ConstraintPy.xml
    ConstraintPyImp.cpp
    SimpleConstraintPy.xml
    SimpleConstraintPyImp.cpp
    ParaObjectPy.xml
    ParaObjectPyImp.cpp
    ParaGeometryPy.xml
    ParaGeometryPyImp.cpp
    SubSystemPy.xml
    SubSystemPyImp.cpp
    SolverBackendPy.xml
    SolverBackendPyImp.cpp
)

SET(ConstraintSolver_Solver_SRCS
    Parameter.h
    Parameter.cpp
    ParameterRef.h
    ParameterRef.cpp
    ParameterStore.h
    ParameterStore.cpp
    ParameterSubset.h
    ParameterSubset.cpp
    ValueSet.h
    ValueSet.cpp
    Constraint.h
    Constraint.cpp
    SimpleConstraint.h
    SimpleConstraint.cpp
    ParaObject.h
    ParaObject.cpp
    ParaGeometry.h
    ParaGeometry.cpp
    SubSystem.h
    SubSystem.cpp
    SolverBackend.h
    SolverBackend.cpp
    DogLeg.h
    DogLeg.cpp
    LM.h
    LM.cpp
    BFGS.h
    BFGS.cpp
    SQP.h
    SQP.cpp
)

SET(ConstraintSolver_Module_SRCS
    AppConstraintSolver.cpp
    PreCompiled.cpp
    PreCompiled.h    
)

SET(ConstraintSolver_SRCS
    ${ConstraintSolver_Utils_SRCS}
    ${ConstraintSolver_Solver_SRCS}
    ${Python_Solver_SRCS}
    ${ConstraintSolver_Module_SRCS}
    ${ConstraintSolver_G2D}
)

add_library(ConstraintSolver SHARED ${ConstraintSolver_SRCS})
target_link_libraries(ConstraintSolver ${ConstraintSolver_LIBS})


SET_BIN_DIR(ConstraintSolver ConstraintSolver /Mod/ConstraintSolver)
SET_PYTHON_PREFIX_SUFFIX(ConstraintSolver)

install(TARGETS ConstraintSolver DESTINATION ${CMAKE_INSTALL_LIBDIR})
