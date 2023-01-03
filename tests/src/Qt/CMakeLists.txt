# 'Google_test' is the subproject name
project(Google_tests)

if(WIN32)
    add_definitions(-DCOIN_DLL)
endif(WIN32)

# 'lib' is the folder with Google Test sources

# 'Google_Tests_run' is the target name
#target_link_libraries(Google_Tests_run gtest gtest_main ${Google_Tests_LIBS})

# ------------------------------------------------------

enable_testing()

function(SETUP_TESTS)
    foreach(_testname ${ARGN})
        add_executable(${_testname}_Tests_run ${_testname}.cpp)
        add_test(NAME ${_testname}_Tests_run COMMAND ${_testname}_Tests_run)
        if(NOT BUILD_DYNAMIC_LINK_PYTHON)
            list(APPEND ${_testname}_LIBS
                ${PYTHON_LIBRARIES}
            )
        endif()
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
