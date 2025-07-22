# SPDX-License-Identifier: MIT
# Copyright 2020 Andy Maloney <asmaloney@gmail.com>

# Generate the export header file for E57Format

include( GenerateExportHeader )

set( comment "// NOTE: This is a generated file. Any changes will be overwritten." )

generate_export_header( E57Format
	EXPORT_FILE_NAME E57Export.h
	EXPORT_MACRO_NAME E57_DLL
	CUSTOM_CONTENT_FROM_VARIABLE comment
)

unset( comment )

target_sources( E57Format
	PRIVATE
	    ${CMAKE_CURRENT_BINARY_DIR}/E57Export.h
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
	    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_compile_definitions( E57Format
	PRIVATE
	    $<$<BOOL:E57_BUILD_SHARED>:E57Format_EXPORTS>
)

install(
	FILES
	    ${CMAKE_CURRENT_BINARY_DIR}/E57Export.h
	DESTINATION
	    include/E57Format
)
