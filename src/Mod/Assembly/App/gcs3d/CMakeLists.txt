project(solver)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(XercesC REQUIRED)
find_package(Eigen3)

include_directories(${EIGEN3_INCLUDE_DIR})

#add_subdirectory(NewSolver)

set(solver_SRC 	main.cpp
		Constraints.cpp
		GCS.cpp
		InputParser.cpp
		qp_eq.cpp
		SubSystem.cpp)

add_executable(solver ${solver_SRC})

target_link_libraries(solver ${XERCESC_LIBRARIES})


