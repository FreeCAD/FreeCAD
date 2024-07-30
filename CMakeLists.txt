cmake_minimum_required(VERSION 3.16)

project(OndselSolver VERSION 1.0.1 DESCRIPTION "Assembly Constraints and Multibody Dynamics code")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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
add_subdirectory(OndselSolverMain)
include(CTest)
enable_testing()
add_subdirectory(tests)
target_compile_definitions(test_run PUBLIC TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testapp")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/OndselSolverMain PROPERTY VS_STARTUP_PROJECT OndselSolverMain)