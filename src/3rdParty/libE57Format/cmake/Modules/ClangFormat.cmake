# SPDX-License-Identifier: MIT
# Copyright 2020 Andy Maloney <asmaloney@gmail.com>

find_program( E57_CLANG_FORMAT_EXE NAMES clang-format )

if ( E57_CLANG_FORMAT_EXE )
    get_target_property( e57_sources ${PROJECT_NAME} SOURCES )

    # Remove some files from the list
    list( FILTER e57_sources EXCLUDE REGEX ".*/E57Export.h" )
    list( FILTER e57_sources EXCLUDE REGEX ".*/extern/.*" )

    add_custom_target( format
        COMMAND clang-format --style=file -i ${e57_sources}
        COMMENT "Running clang-format..."
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()
