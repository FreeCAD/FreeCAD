cmake_minimum_required(VERSION 3.20)
project(freecad-launcher LANGUAGES CXX)

# Use MACOS_DEPLOYMENT_TARGET from environment if set, otherwise keep original default
if(DEFINED ENV{MACOS_DEPLOYMENT_TARGET})
    set(CMAKE_OSX_DEPLOYMENT_TARGET "$ENV{MACOS_DEPLOYMENT_TARGET}" CACHE STRING "Minimum OS X deployment version" FORCE)
else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
endif()
set(CMAKE_CXX_STANDARD 17)

add_executable(FreeCAD FreeCAD.cpp)
target_link_libraries(FreeCAD)
