cmake_minimum_required(VERSION 3.14)
project(automate_cpp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(pybind11 CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)

include(FetchContent)
FetchContent_Declare(
  brloader
  GIT_REPOSITORY   https://github.com/deGravity/breploader.git
  GIT_TAG   v0.5
  PATCH_COMMAND
    "${CMAKE_COMMAND}" "-DBRLOADER_SOURCE_DIR=<SOURCE_DIR>" -P
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/apply_brloader_occt_7_8.cmake"
)

FetchContent_MakeAvailable(brloader)

pybind11_add_module(automate_cpp   
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/automate.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/disjointset.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/disjointset.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsh.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsh.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/eclass.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/eclass.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/part.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpp/part.cpp
)

target_link_libraries(automate_cpp PUBLIC breploader Eigen3::Eigen)
