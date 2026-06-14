# SPDX-License-Identifier: BSL-1.0
# Copyright 2022 Andy Maloney <asmaloney@gmail.com>

project( testE57
    LANGUAGES
        CXX
)

add_executable( testE57 )

target_compile_features( ${PROJECT_NAME}
    PRIVATE
        cxx_std_14
)

set_target_properties( testE57
	PROPERTIES
	    CXX_EXTENSIONS NO
		EXPORT_COMPILE_COMMANDS ON
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_subdirectory( include )
add_subdirectory( src )

# Turn on sanitizers
include( Sanitizers )
enable_all_sanitizers( ${PROJECT_NAME} )

# Set helper variables for readability
set( compiler_is_msvc "$<CXX_COMPILER_ID:MSVC>" )

target_compile_options( testE57
	PUBLIC
	    # add switches to MSVC for UTF-8 handling in our test files
		$<${compiler_is_msvc}:/utf-8>
)

target_compile_definitions( testE57
    PRIVATE
        E57_VALIDATION_LEVEL=${E57_VALIDATION_LEVEL}
)

target_include_directories( testE57
    PRIVATE
        ../src
)

target_link_libraries( testE57
    PRIVATE
        E57Format
)

# ccache
# Turns on ccache if found
if ( CCACHE_PROGRAM )
    message( STATUS "[${PROJECT_NAME}] Using ccache: ${CCACHE_PROGRAM}" )

    set_target_properties( ${PROJECT_NAME}
        PROPERTIES
            CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
            C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
    )
endif()

# Test data
# Find our test data and offer an option to set the path to it
find_path( E57_TEST_DATA_PATH
    NAMES libE57Format-test-data
    PATHS
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/..
        ${PROJECT_SOURCE_DIR}/../..
    NO_DEFAULT_PATH
    DOC "Path to directory containing the libE57Format-test-data repository"
)

if ( E57_TEST_DATA_PATH )
    message( STATUS "[E57 Test] Using test data from: ${E57_TEST_DATA_PATH}/libE57Format-test-data")

    target_compile_definitions( testE57
        PRIVATE
            TEST_DATA_PATH="${E57_TEST_DATA_PATH}/libE57Format-test-data"
    )
else()
    message( WARNING "[E57 Test] Test data not found. Please set E57_TEST_DATA_PATH to the directory containing libE57Format-test-data.")
endif()

# GoogleTest from here: https://github.com/google/googletest

if ( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/googletest/CMakeLists.txt" )
    message( FATAL_ERROR "[E57 Test] The GoogleTest submodule was not downloaded. E57_GIT_SUBMODULE_UPDATE was turned off or failed. Please update submodules and try again." )
endif()

set( BUILD_GMOCK OFF CACHE BOOL "" FORCE )
set( INSTALL_GTEST OFF CACHE BOOL "" FORCE )

if ( MSVC )
    # Prevent overriding the parent project's compiler/linker settings on Windows
    set( gtest_force_shared_crt ON CACHE BOOL "" FORCE )
endif()

add_subdirectory( extern/googletest )

set_target_properties( gtest_main
	PROPERTIES
	    EXPORT_COMPILE_COMMANDS ON
)

enable_all_sanitizers( gtest_main )

unset( BUILD_GMOCK CACHE )
unset( GTEST_HAS_ABSL CACHE )
unset( INSTALL_GTEST CACHE )

target_include_directories( testE57
    PRIVATE
        "${gtest_SOURCE_DIR}/include"
)

target_link_libraries( testE57
    PRIVATE
        gtest_main
)
