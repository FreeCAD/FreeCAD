cmake_minimum_required(VERSION 3.16)

project(OndselSolverLibrary VERSION 1.0.1 DESCRIPTION "Assembly Constraints and Multibody Dynamics code")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(testapp OndselSolver.cpp)
target_link_libraries(testapp OndselSolver)
target_include_directories(testapp PUBLIC "../OndselSolver")
