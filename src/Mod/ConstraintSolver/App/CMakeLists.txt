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

SET(ConstraintSolver_Utils_SRCS
    Utils.h
    Utils.cpp
)

SET(Python_Solver_SRCS
    ParameterStorePy.xml
    ParameterStorePyImp.cpp
    ParameterRefPy.xml
    ParameterRefPyImp.cpp
)

SET(ConstraintSolver_Solver_SRCS
    Parameter.h
    Parameter.cpp
    ParameterRef.h
    ParameterRef.cpp
    ParameterStore.h
    ParameterStore.cpp
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
)

add_library(ConstraintSolver SHARED ${ConstraintSolver_SRCS})
target_link_libraries(ConstraintSolver ${ConstraintSolver_LIBS})


SET_BIN_DIR(ConstraintSolver ConstraintSolver /Mod/ConstraintSolver)
SET_PYTHON_PREFIX_SUFFIX(ConstraintSolver)

install(TARGETS ConstraintSolver DESTINATION ${CMAKE_INSTALL_LIBDIR})
