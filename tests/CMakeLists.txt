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

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    list(APPEND Google_Tests_LIBS
        ${PYTHON_LIBRARIES}
    )
endif()


add_executable(Tests_run)
add_subdirectory(lib)
add_subdirectory(src)
target_include_directories(Tests_run PUBLIC ${Python3_INCLUDE_DIRS} ${XercesC_INCLUDE_DIRS})
target_link_libraries(Tests_run gtest_main gmock_main ${Google_Tests_LIBS} FreeCADApp)

add_executable(Sketcher_tests_run)
add_subdirectory(src/Mod/Sketcher)
target_include_directories(Sketcher_tests_run PUBLIC ${EIGEN3_INCLUDE_DIR} ${OCC_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})
target_link_libraries(Sketcher_tests_run gtest_main ${Google_Tests_LIBS} Sketcher)

include(GoogleTest)
# discovers tests by asking the compiled test executable to enumerate its tests
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(Tests_run) 
gtest_discover_tests(Sketcher_tests_run) 