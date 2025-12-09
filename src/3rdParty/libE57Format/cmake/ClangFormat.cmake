# SPDX-License-Identifier: BSL-1.0
# Copyright 2020 Andy Maloney <asmaloney@gmail.com>

find_program( E57_CLANG_FORMAT_PROGRAM NAMES clang-format )

if ( E57_CLANG_FORMAT_PROGRAM )
    message( STATUS "[${PROJECT_NAME}] Using clang-format: ${E57_CLANG_FORMAT_PROGRAM}" )

    get_target_property( e57_sources ${PROJECT_NAME} SOURCES )

    # Remove some files from the list
    list( FILTER e57_sources EXCLUDE REGEX ".*/E57Export.h" )
    list( FILTER e57_sources EXCLUDE REGEX ".*/extern/.*" )

    # Get list of test files. We cannot use get_target_property here
    # since we will not have a target if E57_BUILD_TEST is off.
    file( GLOB e57_test_sources
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/test/include/*.h
        ${PROJECT_SOURCE_DIR}/test/src/*.cpp
    )

    list( APPEND e57_sources ${e57_test_sources} )

    add_custom_target( e57-clang-format
        COMMAND ${E57_CLANG_FORMAT_PROGRAM} --style=file -i ${e57_sources}
        COMMENT "Running clang-format..."
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()
