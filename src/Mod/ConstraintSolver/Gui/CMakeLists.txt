
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(ConstraintSolverGui_LIBS
    ConstraintSolver
    FreeCADGui
)

if (BUILD_QT5)
    qt5_add_resources(ConstraintSolver_QRC_SRCS Resources/ConstraintSolver.qrc)
else()
    qt4_add_resources(ConstraintSolver_QRC_SRCS Resources/ConstraintSolver.qrc)
endif()

SET(ConstraintSolverGui_SRCS
    ${ConstraintSolver_QRC_SRCS}
    AppConstraintSolverGui.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(ConstraintSolverGui SHARED ${ConstraintSolverGui_SRCS})
target_link_libraries(ConstraintSolverGui ${ConstraintSolverGui_LIBS})

SET_BIN_DIR(ConstraintSolverGui ConstraintSolverGui /Mod/ConstraintSolver)
SET_PYTHON_PREFIX_SUFFIX(ConstraintSolverGui)
    
install(TARGETS ConstraintSolverGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
