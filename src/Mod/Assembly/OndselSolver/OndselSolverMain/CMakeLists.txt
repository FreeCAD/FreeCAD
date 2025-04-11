set(ONDSELSOLVER_MAIN_SRC
	OndselSolver.cpp
)
set(ONDSELSOLVER_MAIN_HEADERS
	OndselSolver.h
)

add_executable(OndselSolverMain ${ONDSELSOLVER_MAIN_SRC} ${ONDSELSOLVER_MAIN_HEADERS})
target_link_libraries(OndselSolverMain OndselSolver)
