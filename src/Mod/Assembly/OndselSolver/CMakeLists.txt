cmake_minimum_required(VERSION 3.16)

project(OndselSolver VERSION 1.0.1 DESCRIPTION "Assembly Constraints and Multibody Dynamics code")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ONDSELSOLVER_BUILD_TESTS "Set to ON to build the Google Test-based C++ test suite." OFF)
option(ONDSELSOLVER_BUILD_EXE "Set to ON to build the standalone OndselSolver executable." OFF)

include(GNUInstallDirs)

if( UNIX )
    set( ONDSELSOLVER_BUILD_SHARED ON )
ELSEIF ( APPLE )
    set( ONDSELSOLVER_BUILD_SHARED ON )
ELSE()
    set( ONDSELSOLVER_BUILD_SHARED OFF )
ENDIF ()

add_compile_definitions(TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testapp")
add_subdirectory(OndselSolver)
if(ONDSELSOLVER_BUILD_EXE)
    add_subdirectory(OndselSolverMain)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/OndselSolverMain PROPERTY VS_STARTUP_PROJECT OndselSolverMain)
endif()

if(ONDSELSOLVER_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
    target_compile_definitions(test_run PUBLIC TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testapp")
endif()
