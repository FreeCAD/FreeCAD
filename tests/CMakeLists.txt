if(MSVC)
    add_compile_options(/wd4251)

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

    # Universal C runtime introduced in VS 2015 (cl version 19)
    if(NOT(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19"))
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

if(WIN32)
    add_definitions(-DCOIN_DLL)
endif(WIN32)

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    list(APPEND Google_Tests_LIBS
        ${PYTHON_LIBRARIES}
    )
endif()

set(CMAKE_AUTOMOC ON)

function(setup_qt_test)
    foreach(_testname ${ARGN})
        add_executable(${_testname}_Tests_run ${_testname}.cpp)
        add_test(NAME ${_testname}_Tests_run COMMAND ${_testname}_Tests_run)

        if(NOT BUILD_DYNAMIC_LINK_PYTHON)
            list(APPEND ${_testname}_LIBS
                ${PYTHON_LIBRARIES}
            )
        endif()

        target_include_directories(${_testname}_Tests_run PUBLIC
            ${Python3_INCLUDE_DIRS}
            ${XercesC_INCLUDE_DIRS}
            ${QtGui_INCLUDE_DIRS}
            ${QtWidgets_INCLUDE_DIRS}
            ${QtTest_INCLUDE_DIRS}
            ${COIN3D_INCLUDE_DIRS})
        target_link_libraries(${_testname}_Tests_run
            FreeCADApp
            FreeCADGui
            ${QtCore_LIBRARIES}
            ${QtWidgets_LIBRARIES}
            ${QtTest_LIBRARIES}
            ${${_testname}_LIBS})
    endforeach()
endfunction()

add_executable(Tests_run)
add_executable(Material_tests_run)
add_executable(Mesh_tests_run)
add_executable(Part_tests_run)
add_executable(Points_tests_run)
add_executable(Sketcher_tests_run)
add_subdirectory(lib)
add_subdirectory(src)
target_include_directories(Tests_run PUBLIC
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)
target_link_libraries(Tests_run
    gtest_main
    gmock_main
    ${Google_Tests_LIBS}
    FreeCADApp
)
