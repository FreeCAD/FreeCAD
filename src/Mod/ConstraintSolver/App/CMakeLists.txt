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

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/G2D)
generate_from_xml(G2D/ParaPointPy)

SET(ConstraintSolver_G2D
    G2D/Vector.h
    G2D/Vector.cpp
    G2D/ParaPoint.h
    G2D/ParaPoint.cpp
    G2D/ParaPointPy.xml
    G2D/ParaPointPyImp.cpp
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
