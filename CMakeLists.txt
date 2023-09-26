cmake_minimum_required(VERSION 3.10)

project(MbDCodeLib)

add_executable(MbDCode MbDCode/MbDCode.cpp)
add_subdirectory(MbDCode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
