cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add_library(some_target main.cpp)
add_executable(ondsel-solver MbDCode/MbDCode.cpp)
add_subdirectory(MbDCode)
