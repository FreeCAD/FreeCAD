include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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
    add_definitions(-D_USE_MATH_DEFINES)
endif(WIN32)


# Add test executables here
add_executable(test_run)
target_include_directories(
	test_run PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../OndselSolver
)
target_sources(test_run
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)
target_link_libraries(test_run
    gtest_main
    gmock_main
    ${Google_Tests_LIBS}
    OndselSolver
)

include(GoogleTest)
# discovers tests by asking the compiled test executable to enumerate its tests
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

gtest_discover_tests(test_run)
