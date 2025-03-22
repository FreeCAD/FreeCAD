cmake_minimum_required(VERSION 3.14...3.16)

project(GSLTests LANGUAGES CXX)

set(GSL_CXX_STANDARD "14" CACHE STRING "Use c++ standard")

set(CMAKE_CXX_STANDARD ${GSL_CXX_STANDARD})
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPkgConfig)
include(ExternalProject)

# will make visual studio generated project group files
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CI_TESTING AND GSL_CXX_STANDARD EQUAL 20)
    add_compile_definitions(FORCE_STD_SPAN_TESTS=1)
endif()

if(IOS)
    add_compile_definitions(GTEST_HAS_DEATH_TEST=1 IOS_PROCESS_DELAY_WORKAROUND=1)
endif()

pkg_search_module(GTestMain gtest_main)
if (NOT GTestMain_FOUND)
    # No pre-installed GTest is available, try to download it using Git.
    find_package(Git REQUIRED QUIET)

    configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
    )
    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
    )
    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(GTestMain_LIBRARIES gtest_main)

    add_subdirectory(
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL
    )
endif()

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    find_package(Microsoft.GSL CONFIG REQUIRED)
    enable_testing()

    if (NOT DEFINED Microsoft.GSL_VERSION)
        message(FATAL_ERROR "Microsoft.GSL_VERSION not defined!")
    endif()

    message(STATUS "Microsoft.GSL_VERSION = ${Microsoft.GSL_VERSION}")
endif()

if (MSVC AND (GSL_CXX_STANDARD GREATER_EQUAL 17))
    set(GSL_CPLUSPLUS_OPT -Zc:__cplusplus -permissive-)
endif()

include(CheckCXXCompilerFlag)
# this interface adds compile options to how the tests are run
# please try to keep entries ordered =)
add_library(gsl_tests_config INTERFACE)
if(MSVC) # MSVC or simulating MSVC
    target_compile_options(gsl_tests_config INTERFACE
        ${GSL_CPLUSPLUS_OPT}
        /EHsc
        /W4
        /WX
        $<$<CXX_COMPILER_ID:MSVC>:
          /wd4996  # Use of function or classes marked [[deprecated]]
          /wd26409 # CppCoreCheck - GTest
          /wd26426 # CppCoreCheck - GTest
          /wd26440 # CppCoreCheck - GTest
          /wd26446 # CppCoreCheck - prefer gsl::at()
          /wd26472 # CppCoreCheck - use gsl::narrow(_cast)
          /wd26481 # CppCoreCheck - use span instead of pointer arithmetic
          $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,1920>: # VS2015
            /wd4189 # variable is initialized but not referenced
            $<$<NOT:$<CONFIG:Debug>>: # Release, RelWithDebInfo
              /wd4702 # Unreachable code
            >
          >
        >
        $<$<CXX_COMPILER_ID:Clang>:
          -Weverything
          -Wfloat-equal
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-covered-switch-default # GTest
          -Wno-deprecated-declarations # Allow tests for [[deprecated]] elements
          -Wno-global-constructors # GTest
          -Wno-language-extension-token # GTest gtest-port.h
          -Wno-missing-braces
          -Wno-missing-prototypes
          -Wno-shift-sign-overflow # GTest gtest-port.h
          -Wno-undef # GTest
          -Wno-used-but-marked-unused # GTest EXPECT_DEATH
          -Wno-switch-default # GTest EXPECT_DEATH
          $<$<EQUAL:${GSL_CXX_STANDARD},14>: # no support for [[maybe_unused]]
            -Wno-unused-member-function
            -Wno-unused-variable
            $<$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,15.0.1>:
              -Wno-deprecated # False positive in MSVC Clang 15.0.1 raises a C++17 warning
            >
          >
        >
    )
    check_cxx_compiler_flag("-Wno-reserved-identifier" WARN_RESERVED_ID)
    if (WARN_RESERVED_ID)
      target_compile_options(gsl_tests_config INTERFACE "-Wno-reserved-identifier")
    endif()
else()
    target_compile_options(gsl_tests_config INTERFACE
        -fno-strict-aliasing
        -Wall
        -Wcast-align
        -Wconversion
        -Wctor-dtor-privacy
        -Werror
        -Wextra
        -Wpedantic
        -Wshadow
        -Wsign-conversion
        -Wfloat-equal
        -Wno-deprecated-declarations # Allow tests for [[deprecated]] elements
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
          -Weverything
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-missing-braces
          -Wno-covered-switch-default # GTest
          -Wno-global-constructors # GTest
          -Wno-missing-prototypes
          -Wno-padded
          -Wno-switch-default
          -Wno-unknown-attributes
          -Wno-used-but-marked-unused # GTest EXPECT_DEATH
          -Wno-weak-vtables
          $<$<EQUAL:${GSL_CXX_STANDARD},14>: # no support for [[maybe_unused]]
            -Wno-unused-member-function
            -Wno-unused-variable
          >
        >
        $<$<CXX_COMPILER_ID:Clang>:
          $<$<AND:$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,4.99>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,6>>:
            $<$<EQUAL:${GSL_CXX_STANDARD},17>:-Wno-undefined-func-template>
          >
          $<$<AND:$<EQUAL:${GSL_CXX_STANDARD},20>,$<OR:$<CXX_COMPILER_VERSION:11.0.0>,$<CXX_COMPILER_VERSION:10.0.0>>>:
              -Wno-zero-as-null-pointer-constant  # failing Clang Ubuntu 20.04 tests, seems to be a bug with clang 10.0.0
                                                  # and clang 11.0.0. (operator< is being re-written by the compiler
                                                  # as operator<=> and raising the warning)
          >
        >
        $<$<CXX_COMPILER_ID:AppleClang>:
          $<$<AND:$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,9.1>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,10>>:
            $<$<EQUAL:${GSL_CXX_STANDARD},17>:-Wno-undefined-func-template>
          >
        >
        $<$<CXX_COMPILER_ID:GNU>:
          -Wdouble-promotion # float implicit to double
          -Wlogical-op # suspicious uses of logical operators
          $<$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,6>>:
            -Wduplicated-cond # duplicated if-else conditions
            -Wmisleading-indentation
            -Wnull-dereference
            $<$<EQUAL:${GSL_CXX_STANDARD},14>: # no support for [[maybe_unused]]
              -Wno-unused-variable
            >
          >
          $<$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,7>>:
            -Wduplicated-branches # identical if-else branches
          >
        >
    )
endif(MSVC)
check_cxx_compiler_flag("-Wno-unsafe-buffer-usage" WARN_UNSAFE_BUFFER)
if (WARN_UNSAFE_BUFFER)
  # This test uses very greedy heuristics such as "no pointer arithmetic on raw buffer"
  target_compile_options(gsl_tests_config INTERFACE "-Wno-unsafe-buffer-usage")
endif()

# for tests to find the gtest header
target_include_directories(gsl_tests_config SYSTEM INTERFACE
    googletest/googletest/include
)

add_executable(gsl_tests
    algorithm_tests.cpp
    assertion_tests.cpp
    at_tests.cpp
    byte_tests.cpp
    notnull_tests.cpp
    owner_tests.cpp
    pointers_tests.cpp
    span_compatibility_tests.cpp
    span_ext_tests.cpp
    span_tests.cpp
    strict_notnull_tests.cpp
    
    utils_tests.cpp
)

target_link_libraries(gsl_tests
    Microsoft.GSL::GSL
    gsl_tests_config
    ${GTestMain_LIBRARIES}
)
add_test(gsl_tests gsl_tests)

# No exception tests

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    STRING (REGEX REPLACE "/EHsc" "" ${flag_var} "${${flag_var}}")
endforeach(flag_var)

# this interface adds compile options to how the tests are run
# please try to keep entries ordered =)
add_library(gsl_tests_config_noexcept INTERFACE)
if(MSVC) # MSVC or simulating MSVC
    target_compile_definitions(gsl_tests_config_noexcept INTERFACE
        _HAS_EXCEPTIONS=0 # disable exceptions in the Microsoft STL
    )
    target_compile_options(gsl_tests_config_noexcept INTERFACE
        ${GSL_CPLUSPLUS_OPT}
        /W4
        /WX
        $<$<CXX_COMPILER_ID:MSVC>:
          /wd4577
          /wd4702
          /wd26440 # CppCoreCheck - GTest
          /wd26446 # CppCoreCheck - prefer gsl::at()
        >
        $<$<CXX_COMPILER_ID:Clang>:
          -Weverything
          -Wfloat-equal
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-missing-prototypes
          -Wno-unknown-attributes
          $<$<EQUAL:${GSL_CXX_STANDARD},14>:
            $<$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,15.0.1>: 
              -Wno-deprecated # False positive in MSVC Clang 15.0.1 raises a C++17 warning
            >
          >
        >
    )
    check_cxx_compiler_flag("-Wno-reserved-identifier" WARN_RESERVED_ID)
    if (WARN_RESERVED_ID)
      target_compile_options(gsl_tests_config_noexcept INTERFACE "-Wno-reserved-identifier")
    endif()
else()
    target_compile_options(gsl_tests_config_noexcept INTERFACE
        -fno-exceptions
        -fno-strict-aliasing
        -Wall
        -Wcast-align
        -Wconversion
        -Wctor-dtor-privacy
        -Werror
        -Wextra
        -Wpedantic
        -Wshadow
        -Wsign-conversion
        -Wfloat-equal
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
          -Weverything
          -Wno-c++98-compat
          -Wno-c++98-compat-pedantic
          -Wno-missing-prototypes
          -Wno-unknown-attributes
          -Wno-weak-vtables
        >
        $<$<CXX_COMPILER_ID:GNU>:
          -Wdouble-promotion # float implicit to double
          -Wlogical-op # suspicious uses of logical operators
          -Wuseless-cast # casting to its own type
          $<$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,6>>:
            -Wduplicated-cond # duplicated if-else conditions
            -Wmisleading-indentation
            -Wnull-dereference
          >
          $<$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,7>>:
            -Wduplicated-branches # identical if-else branches
          >
          $<$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,8>>:
            -Wcast-align=strict # increase alignment (i.e. char* to int*)
          >
        >
    )
endif(MSVC)
check_cxx_compiler_flag("-Wno-unsafe-buffer-usage" WARN_UNSAFE_BUFFER)
if (WARN_UNSAFE_BUFFER)
  # This test uses very greedy heuristics such as "no pointer arithmetic on raw buffer"
  target_compile_options(gsl_tests_config_noexcept INTERFACE "-Wno-unsafe-buffer-usage")
endif()

add_executable(gsl_noexcept_tests no_exception_ensure_tests.cpp)
target_link_libraries(gsl_noexcept_tests
    Microsoft.GSL::GSL
    gsl_tests_config_noexcept
)
add_test(gsl_noexcept_tests gsl_noexcept_tests)
