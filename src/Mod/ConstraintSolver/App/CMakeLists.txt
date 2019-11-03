
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

set(ConstraintSolver_LIBS
    FreeCADApp
)

SET(ConstraintSolver_SRCS
    AppConstraintSolver.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(ConstraintSolver SHARED ${ConstraintSolver_SRCS})
target_link_libraries(ConstraintSolver ${ConstraintSolver_LIBS})


SET_BIN_DIR(ConstraintSolver ConstraintSolver /Mod/ConstraintSolver)
SET_PYTHON_PREFIX_SUFFIX(ConstraintSolver)

install(TARGETS ConstraintSolver DESTINATION ${CMAKE_INSTALL_LIBDIR})
