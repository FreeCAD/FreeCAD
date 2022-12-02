# 'Google_test' is the subproject name
project(Google_tests)

if(WIN32)
    add_definitions(-DCOIN_DLL)
endif(WIN32)

if(MSVC)
    option(
      gtest_force_shared_crt
      "Use shared (DLL) run-time lib even when Google Test is built as static lib."
      ON)

    set(Google_Tests_LIBS
        oldnames.lib
        debug msvcrtd.lib
        debug msvcprtd.lib
        optimized msvcrt.lib
        optimized msvcprt.lib
    )

    #Universal C runtime introduced in VS 2015 (cl version 19)
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19"))
        list(APPEND Google_Tests_LIBS
            debug vcruntimed.lib
            debug ucrtd.lib
            debug concrtd.lib
            optimized vcruntime.lib
            optimized ucrt.lib
            optimized concrt.lib
        )
    endif()
endif()

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

target_link_libraries(Google_Tests_run gtest gtest_main ${Google_Tests_LIBS})

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
