# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# 'Google_Tests_run' is the target name

# 'test1.cpp tests2.cpp' are source files with tests
add_executable(
    Google_Tests_run
    src/test1.cpp
    src/test2.cpp
)

target_link_libraries(Google_Tests_run gtest gtest_main)

# ------------------------------------------------------

enable_testing()

function(SETUP_TESTS)
    foreach(_testname ${ARGN})
        add_executable(${_testname}_Tests_run src/${_testname}.cpp)
        add_test(NAME ${_testname}_Tests_run COMMAND ${_testname}_Tests_run)
        target_link_libraries(${_testname}_Tests_run ${QtTest_LIBRARIES} ${${_testname}_LIBS})
    endforeach()
endfunction()

set(CMAKE_AUTOMOC ON)

# Qt Test
include_directories(
    ${QtGui_INCLUDE_DIRS}
    ${QtTest_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
)

# ------------------------------------------------------

set (InventorBuilder_LIBS
    ${COIN3D_LIBRARIES}
    FreeCADBase
)

SETUP_TESTS(
    InventorBuilder
)
