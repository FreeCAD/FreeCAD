if ( EXISTS "${CMAKE_SOURCE_DIR}/tests/lib/googletest" )
    include_directories( ${CMAKE_SOURCE_DIR}/tests/lib/googletest/include/ )
    include_directories( ${CMAKE_SOURCE_DIR}/tests/lib/googlemock/include/ )
else()
    find_package(GTest)
    if( GTest_FOUND )
        message( STATUS "Found Google Test: version ${GTest_VERSION}" )
    else()
        message( SEND_ERROR "The Google Test submodule is not available. Please run git submodule update --init" )
    endif()
endif()

if(MSVC)
    add_compile_options(/wd4251)

    option(
        gtest_force_shared_crt
        "Use shared (DLL) run-time lib even when Google Test is built as static lib."
        ON)
    option(gtest_disable_pthreads "Disable uses of pthreads in gtest." ON)

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
    add_definitions(-DCOIN_DLL -D_USE_MATH_DEFINES)
endif(WIN32)

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    list(APPEND Google_Tests_LIBS
        ${PYTHON_LIBRARIES}
    )
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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

# Add test executables here

set(TestExecutables
    Tests_run
)

if(BUILD_ASSEMBLY)
  list (APPEND TestExecutables Assembly_tests_run)
endif(BUILD_ASSEMBLY)
if(BUILD_MATERIAL)
  list (APPEND TestExecutables Material_tests_run)
endif(BUILD_MATERIAL)
if(BUILD_MEASURE)
  list (APPEND TestExecutables Measure_tests_run)
endif(BUILD_MEASURE)
if(BUILD_MESH)
  list (APPEND TestExecutables Mesh_tests_run)
endif(BUILD_MESH)
if(BUILD_MESH_PART)
  list (APPEND TestExecutables MeshPart_tests_run)
endif(BUILD_MESH_PART)
if(BUILD_PART)
  list (APPEND TestExecutables Part_tests_run)
endif(BUILD_PART)
if(BUILD_PART_DESIGN)
    list (APPEND TestExecutables PartDesign_tests_run)
endif(BUILD_PART_DESIGN)
if(BUILD_POINTS)
  list (APPEND TestExecutables Points_tests_run)
endif(BUILD_POINTS)
if(BUILD_SKETCHER)
  list (APPEND TestExecutables Sketcher_tests_run)
endif(BUILD_SKETCHER)
if(BUILD_SPREADSHEET)
  list (APPEND TestExecutables Spreadsheet_tests_run)
endif()

# -------------------------

foreach (exe ${TestExecutables})
    add_executable(${exe})
endforeach()

if ( EXISTS "${CMAKE_SOURCE_DIR}/tests/lib/googletest" )
    add_subdirectory(lib)
endif()
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

include(GoogleTest)
# discovers tests by asking the compiled test executable to enumerate its tests
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

foreach (exe ${TestExecutables})
    gtest_discover_tests(${exe})
endforeach()
